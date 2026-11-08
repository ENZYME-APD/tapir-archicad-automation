#include "ScriptManager.hpp"
#include "ResourceIds.hpp"
#include "AddOnVersion.hpp"
#include "Config.hpp"
#include "VersionChecker.hpp"
#include "ScriptRequirements.hpp"
#include "Sha256.hpp"
#include "TapirPalette.hpp"
#include "HTTP/Client/ClientConnection.hpp"
#include "HTTP/Client/Request.hpp"
#include "HTTP/Client/Response.hpp"
#include "IBinaryChannelUtilities.hpp"
#include "ObjectStateJSONConversion.hpp"
#include "FileSystem.hpp"
#include "File.hpp"

#include <cstdlib>
#include <string>
#include <utility>

const GS::Guid          ScriptManager::paletteGuid ("{8C1F4E27-5B3A-4D96-A0E2-7F64B19C3D58}");
GS::Ref<ScriptManager>  ScriptManager::instance;

// The catalogue tools/generate_catalogue.py writes, read from the site-data branch on raw.githubusercontent.com,
// never through GitHub's API. Documents/Tapir/config.json's marketplaceCatalogueUrl overrides it for testing.
static const char* const DefaultCatalogueUrl = "https://raw.githubusercontent.com/ENZYME-APD/tapir-archicad-automation/site-data/catalogue.json";
static const char* const ScriptsPageUrl = "https://archicad-api.com/scripts/";
static const char* const ShareScriptPageUrl = "https://archicad-api.com/scripts/share/";

// A marketplace script may import the helpers of the built-in scripts (marketplace/README.md, "The helpers
// utilities and aclib"): installing one that does puts copies of both folders, from main, next to it. These are
// their files; a file added to either folder on main must be added here too.
static const char* const HelpersBaseUrl = "https://raw.githubusercontent.com/ENZYME-APD/tapir-archicad-automation/main/builtin-scripts/";
static const char* const HelperFiles[][2] = {
    { "aclib", "__init__.py" },
    { "utilities", "__init__.py" },
    { "utilities", "archicad.py" },
    { "utilities", "export.py" },
    { "utilities", "navigator_utils.py" },
    { "utilities", "ui.py" }
};

// The catalogue is fetched on the main thread when the window opens and on Refresh, so a server that accepts the
// connection but never answers must not keep Archicad waiting for long.
static constexpr int CatalogueRequestTimeoutMs = 15 * 1000;

// The category keys of marketplace/listing.schema.json, in the order of the Categories list. Their display names
// are ID_SCRIPT_MANAGER_STRINGS_CATEGORY_FIRST and the entries after it, in the same order.
static const char* const CategoryKeys[] = {
    "documentation-layouts",
    "element-data-properties",
    "modeling-geometry",
    "zones-areas",
    "import-export",
    "quality-assurance",
    "quantities-scheduling",
    "libraries-objects",
    "navigator-views",
    "classification-attributes",
    "batch-automation",
    "utilities"
};
static constexpr short CategoryCount = static_cast<short> (sizeof (CategoryKeys) / sizeof (CategoryKeys[0]));
static constexpr short AllCategoriesItem = 1;   // the Categories list's first item; the categories follow it

// The columns of the lists
static constexpr short CategoryNameTab = 1;
static constexpr short CategoryCountTab = 2;
static constexpr short ScriptNameTab = 1;
static constexpr short ScriptSummaryTab = 2;
static constexpr short ScriptVersionTab = 3;
static constexpr short ScriptTabCount = 3;

// The texts of the user interface, from the localized resources (RINT/AddOn.grc, see LOCALIZATION.md)
static GS::UniString ManagerText (Int32 index)
{
    return RSGetIndString (ID_SCRIPT_MANAGER_STRINGS, index, ACAPI_GetOwnResModule ());
}

static GS::UniString PaletteText (Int32 index)
{
    return RSGetIndString (ID_PALETTE_STRINGS, index, ACAPI_GetOwnResModule ());
}

static GS::UniString UpdateText (Int32 index)
{
    return RSGetIndString (ID_AUTOUPDATE_STRINGS, index, ACAPI_GetOwnResModule ());
}

static GS::UniString ToLowerCased (const GS::UniString& text)
{
#ifdef ServerMainVers_3000
    return text.GetLowerCased ();
#else
    return text.ToLowerCase ();
#endif
}

static void OpenWebpage (const GS::UniString& webpage)
{
    const GS::UniString command = GS::UniString::Printf (
        "%s %T",
#ifdef WINDOWS
        "start",
#else
        "open",
#endif
        webpage.ToPrintf ());
    system (command.ToCStr ().Get ());
}

static IO::Location GetCatalogueCacheLocation ()
{
    IO::Location location = MarketplaceInstalls::GetMarketplaceFolder ();
    location.AppendToLocal (IO::Name ("catalogue.json"));
    return location;
}

// Well-formed UTF-8 as in Table 3-7 of the Unicode Standard. Text that is not must never reach GS::UniString's
// UTF-8 conversion, whose assert can take Archicad down (see SafeUTF8String in TapirPalette.cpp); a catalogue
// address in the config may point at anything.
static bool IsValidUTF8 (const std::vector<char>& bytes)
{
    size_t i = 0;
    while (i < bytes.size ()) {
        const unsigned char c = static_cast<unsigned char> (bytes[i]);
        size_t length = 0;
        unsigned char secondMin = 0x80;
        unsigned char secondMax = 0xBF;
        if (c < 0x80) {
            length = 1;
        } else if (c >= 0xC2 && c <= 0xDF) {
            length = 2;
        } else if (c >= 0xE0 && c <= 0xEF) {
            length = 3;
            if (c == 0xE0) {
                secondMin = 0xA0;
            } else if (c == 0xED) {
                secondMax = 0x9F;
            }
        } else if (c >= 0xF0 && c <= 0xF4) {
            length = 4;
            if (c == 0xF0) {
                secondMin = 0x90;
            } else if (c == 0xF4) {
                secondMax = 0x8F;
            }
        } else {
            return false;
        }
        if (i + length > bytes.size ()) {
            return false;
        }
        for (size_t j = 1; j < length; ++j) {
            const unsigned char next = static_cast<unsigned char> (bytes[i + j]);
            const bool valid = (j == 1) ? (next >= secondMin && next <= secondMax) : ((next & 0xC0) == 0x80);
            if (!valid) {
                return false;
            }
        }
        i += length;
    }
    return true;
}

static bool BytesToObjectState (const std::vector<char>& bytes, GS::ObjectState& os)
{
    if (bytes.empty () || !IsValidUTF8 (bytes)) {
        return false;
    }
    const GS::UniString text (bytes.data (), static_cast<GS::USize> (bytes.size ()), CC_UTF8);
    return JSON::ConvertToObjectState (text, os) == NoError && !os.IsEmpty ();
}

static std::vector<char> ReadAllBytes (GS::IBinaryChannel& channel)
{
    constexpr GS::USize BufferSize = 16 * 1024;
    std::vector<char> bytes;
    char buffer[BufferSize];

    try {
        while (true) {
            const GS::USize bytesRead = channel.Read (buffer, BufferSize);
            if (bytesRead == 0) {
                break;
            }
            bytes.insert (bytes.end (), buffer, buffer + bytesRead);
        }
    } catch (const GS::Exception&) {
        // End of input: the channel signalled it with an exception instead of returning 0.
    }

    return bytes;
}

// Splits "https://host/path?query" into "https://host" and "/path?query".
static bool SplitUrl (const GS::UniString& url, GS::UniString& origin, GS::UniString& pathAndQuery)
{
    const UIndex schemeEnd = url.FindFirst (GS::UniString ("://"));
    if (schemeEnd == MaxUIndex) {
        return false;
    }

    const UIndex pathStart = url.FindFirst (GS::UniChar ('/'), schemeEnd + 3);
    if (pathStart == MaxUIndex) {
        origin = url;
        pathAndQuery = "/";
        return true;
    }

    origin = GS::UniString (url.GetSubstring (0, pathStart));
    pathAndQuery = GS::UniString (url.GetSubstring (pathStart, url.GetLength () - pathStart));
    return true;
}

// Downloads the body of url, with the DevKit's HTTP client as VersionChecker uses it. error: why it failed.
static bool DownloadBytes (const GS::UniString& url, std::vector<char>& body, GS::UniString& error)
{
    GS::UniString origin;
    GS::UniString pathAndQuery;
    if (!SplitUrl (url, origin, pathAndQuery)) {
        error = GS::UniString::Printf (UpdateText (ID_AUTOUPDATE_ERROR_INVALID_URL), url.ToPrintf ());
        return false;
    }

    try {
        const IO::URI::URI connectionUrl (origin);
        HTTP::Client::ClientConnection clientConnection (connectionUrl);
        clientConnection.SetTimeout (CatalogueRequestTimeoutMs);
        clientConnection.Connect ();

        HTTP::Client::Request request (HTTP::MessageHeader::Method::Get, pathAndQuery);
        request.GetRequestHeaderFieldCollection ().Add (HTTP::MessageHeader::HeaderFieldName::UserAgent, "Tapir-Archicad-AddOn/" ADDON_VERSION);
        clientConnection.Send (request);

        HTTP::Client::Response response;
        body = ReadAllBytes (clientConnection.BeginReceive (response));
        const int statusCode = static_cast<int> (response.GetStatusCode ());
        clientConnection.FinishReceive ();
        clientConnection.Close (false);

        if (statusCode != 200) {
            error = GS::UniString::Printf (UpdateText (ID_AUTOUPDATE_ERROR_HTTP_STATUS), statusCode, origin.ToPrintf ());
            return false;
        }
        if (body.empty ()) {
            error = UpdateText (ID_AUTOUPDATE_ERROR_EMPTY);
            return false;
        }
        return true;
    } catch (const GS::Exception& e) {
        error = e.GetMessage ().IsEmpty () ? GS::UniString (e.GetName ()) : e.GetMessage ();
    } catch (...) {
        error = UpdateText (ID_AUTOUPDATE_ERROR_UNEXPECTED);
    }
    return false;
}

static bool ReadFileBytes (const IO::Location& location, std::vector<char>& bytes)
{
    bool exists = false;
    if (IO::fileSystem.Contains (location, &exists) != NoError || !exists) {
        return false;
    }
    IO::File file (location);
    if (file.GetStatus () != NoError || file.Open (IO::File::ReadMode) != NoError) {
        return false;
    }
    UInt64 length = 0;
    bool ok = file.GetDataLength (&length) == NoError && length > 0 && length < 64ull * 1024 * 1024;
    if (ok) {
        bytes.resize (static_cast<size_t> (length));
        ok = file.ReadBin (bytes.data (), static_cast<USize> (length)) == NoError;
    }
    file.Close ();
    return ok;
}

// The last good catalogue, kept as {"fetchedAt": <seconds since 1970>, "catalogue": <the catalogue as fetched>}.
static void WriteCatalogueCache (const std::vector<char>& catalogue, std::time_t fetchedAt)
{
    IO::Location folder = MarketplaceInstalls::GetMarketplaceFolder ();
    IO::fileSystem.CreateFolderTree (folder);

    IO::File file (GetCatalogueCacheLocation (), IO::File::OnNotFound::Create);
    if (file.GetStatus () != NoError || file.Open (IO::File::WriteEmptyMode) != NoError) {
        return;
    }
    const std::string prefix = "{\"fetchedAt\": " + std::to_string (static_cast<long long> (fetchedAt)) + ", \"catalogue\": ";
    const std::string suffix = "}\n";
    file.WriteBin (prefix.data (), static_cast<USize> (prefix.size ()));
    file.WriteBin (catalogue.data (), static_cast<USize> (catalogue.size ()));
    file.WriteBin (suffix.data (), static_cast<USize> (suffix.size ()));
    file.Close ();
}

// The catalogue's format is described at the top of tools/generate_catalogue.py. Keys not known here are ignored,
// and a script without a slug or a version is left out.
static bool ParseCatalogue (const GS::ObjectState& os, std::vector<ScriptManager::Script>& result)
{
    GS::Array<GS::ObjectState> scriptsOS;
    if (!os.Get ("scripts", scriptsOS)) {
        return false;
    }

    result.clear ();
    for (const GS::ObjectState& scriptOS : scriptsOS) {
        ScriptManager::Script script;
        scriptOS.Get ("slug", script.slug);
        scriptOS.Get ("name", script.name);
        scriptOS.Get ("summary", script.summary);
        scriptOS.Get ("category", script.category);
        scriptOS.Get ("tags", script.tags);
        GS::ObjectState authorOS;
        if (scriptOS.Get ("author", authorOS)) {
            authorOS.Get ("name", script.authorName);
            authorOS.Get ("github", script.authorGithub);
        }
        scriptOS.Get ("license", script.license);
        scriptOS.Get ("entry", script.entry);
        scriptOS.Get ("changes", script.changes);
        scriptOS.Get ("screenshots", script.screenshots);

        GS::Array<GS::ObjectState> versionsOS;
        scriptOS.Get ("versions", versionsOS);
        for (const GS::ObjectState& versionOS : versionsOS) {
            ScriptManager::ScriptVersion version;
            versionOS.Get ("version", version.version);
            GS::Array<Int32> archicadVersions;
            versionOS.Get ("archicadVersions", archicadVersions);
            for (Int32 archicadVersion : archicadVersions) {
                version.archicadVersions.push_back (static_cast<int> (archicadVersion));
            }
            versionOS.Get ("minTapirVersion", version.minTapirVersion);
            versionOS.Get ("changelog", version.changelog);
            versionOS.Get ("commit", version.commit);
            versionOS.Get ("url", version.url);
            versionOS.Get ("sha256", version.sha256);
            if (!version.version.IsEmpty ()) {
                script.versions.push_back (version);
            }
        }

        if (script.slug.IsEmpty () || script.versions.empty ()) {
            continue;
        }
        if (script.name.IsEmpty ()) {
            script.name = script.slug;
        }
        result.push_back (script);
    }
    return true;
}

// "today 10:42", or the date and time for an earlier day.
static GS::UniString FormatCatalogueTime (std::time_t time)
{
    const std::tm* timeTm = std::localtime (&time);
    if (timeTm == nullptr) {
        return GS::EmptyUniString;
    }
    const std::tm timeParts = *timeTm;   // localtime returns the same buffer each time

    const std::time_t now = std::time (nullptr);
    const std::tm* nowTm = std::localtime (&now);
    char buffer[32] = {};
    if (nowTm != nullptr && nowTm->tm_year == timeParts.tm_year && nowTm->tm_yday == timeParts.tm_yday) {
        std::strftime (buffer, sizeof (buffer), "%H:%M", &timeParts);
        return GS::UniString::Printf (ManagerText (ID_SCRIPT_MANAGER_STRINGS_TODAY), GS::UniString (buffer).ToPrintf ());
    }
    std::strftime (buffer, sizeof (buffer), "%Y-%m-%d %H:%M", &timeParts);
    return GS::UniString (buffer);
}

// "28, 29, 30"
static GS::UniString JoinArchicadVersions (const std::vector<int>& versions)
{
    GS::UniString text;
    for (size_t i = 0; i < versions.size (); ++i) {
        if (i > 0) {
            text += ", ";
        }
        text += GS::UniString::Printf ("%d", versions[i]);
    }
    return text;
}

// The badge of the catalogue's changes key, as the palette shows it; empty when the header has none.
static GS::UniString GetChangesBadge (const GS::UniString& changes)
{
    if (changes == "model") {
        return PaletteText (ID_PALETTE_STRINGS_BADGE_CHANGES_MODEL);
    }
    if (changes == "files") {
        return PaletteText (ID_PALETTE_STRINGS_BADGE_WRITES_FILES);
    }
    if (changes == "read-only") {
        return PaletteText (ID_PALETTE_STRINGS_BADGE_READ_ONLY);
    }
    return GS::EmptyUniString;
}

static bool WriteFileBytes (const IO::Location& location, const std::vector<char>& bytes)
{
    IO::File file (location, IO::File::OnNotFound::Create);
    if (file.GetStatus () != NoError || file.Open (IO::File::WriteEmptyMode) != NoError) {
        return false;
    }
    const bool ok = bytes.empty () || file.WriteBin (bytes.data (), static_cast<USize> (bytes.size ())) == NoError;
    return file.Close () == NoError && ok;
}

static GS::UniString LocationText (const IO::Location& location)
{
    return location.ToDisplayText ();
}

// Whether the script imports the helpers of the built-in scripts (see HelperFiles).
static bool ImportsHelpers (const std::vector<char>& code)
{
    const std::string text (code.begin (), code.end ());
    const char* const imports[] = { "import aclib", "from aclib", "import utilities", "from utilities" };
    for (const char* import : imports) {
        if (text.find (import) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// Whether candidate is a newer version than installed, both written as N.N.N; false when either is not.
static bool IsNewerVersion (const GS::UniString& candidate, const GS::UniString& installed)
{
    std::vector<std::uint32_t> candidateParts;
    std::vector<std::uint32_t> installedParts;
    if (!ScriptRequirements::ParseVersion (candidate.ToCStr (CC_UTF8).Get (), candidateParts) ||
        !ScriptRequirements::ParseVersion (installed.ToCStr (CC_UTF8).Get (), installedParts))
    {
        return false;
    }
    return candidateParts > installedParts;
}

// Whether location is folder or anything inside it.
static bool IsInFolder (const IO::Location& location, const IO::Location& folder)
{
    IO::Location current = location;
    for (int depth = 0; depth < 256 && !current.IsEmpty (); ++depth) {
        if (current == folder) {
            return true;
        }
        if (current.DeleteLastLocalName () != NoError) {
            break;
        }
    }
    return false;
}

// The Tapir Palette's shortcut slots that point at a file in folder are set to newLocation, or cleared when it is
// empty: on Update they follow the script to its new version's folder, on Remove they are cleared.
static void ReplaceShortcutSlotsIn (const IO::Location& folder, const IO::Location& newLocation)
{
    if (!TapirPalette::HasInstance ()) {
        return;
    }
    TapirPalette& palette = TapirPalette::Instance ();
    for (short slot = 0; slot < TapirPalette::ScriptShortcutSlotCount; ++slot) {
        const IO::Location assigned = palette.GetShortcutSlot (slot);
        if (!assigned.IsEmpty () && IsInFolder (assigned, folder)) {
            palette.SetShortcutSlot (slot, newLocation);
        }
    }
}

// Who is allowed once for all their scripts: the GitHub login when the listing has one, else the name.
static GS::UniString AuthorKey (const ScriptManager::Script& script)
{
    if (!script.authorGithub.IsEmpty ()) {
        return "github:" + ToLowerCased (script.authorGithub);
    }
    return "name:" + script.authorName;
}

ScriptManager::ScriptManager ()
    : DG::Palette (ACAPI_GetOwnResModule (), ID_SCRIPT_MANAGER, ACAPI_GetOwnResModule (), paletteGuid)
    , searchEdit (GetReference (), ID_SCRIPT_MANAGER_SEARCH)
    , availableTab (GetReference (), ID_SCRIPT_MANAGER_AVAILABLE_TAB)
    , installedTab (GetReference (), ID_SCRIPT_MANAGER_INSTALLED_TAB)
    , updatesTab (GetReference (), ID_SCRIPT_MANAGER_UPDATES_TAB)
    , catalogueTimeText (GetReference (), ID_SCRIPT_MANAGER_CATALOGUE_TIME)
    , refreshButton (GetReference (), ID_SCRIPT_MANAGER_REFRESH)
    , categoryList (GetReference (), ID_SCRIPT_MANAGER_CATEGORIES)
    , fitsCheckBox (GetReference (), ID_SCRIPT_MANAGER_FITS)
    , scriptList (GetReference (), ID_SCRIPT_MANAGER_SCRIPTS)
    , nameText (GetReference (), ID_SCRIPT_MANAGER_NAME)
    , bylineText (GetReference (), ID_SCRIPT_MANAGER_BYLINE)
    , summaryText (GetReference (), ID_SCRIPT_MANAGER_SUMMARY)
    , changesLabel (GetReference (), ID_SCRIPT_MANAGER_CHANGES_LABEL)
    , changesValue (GetReference (), ID_SCRIPT_MANAGER_CHANGES_VALUE)
    , archicadLabel (GetReference (), ID_SCRIPT_MANAGER_ARCHICAD_LABEL)
    , archicadValue (GetReference (), ID_SCRIPT_MANAGER_ARCHICAD_VALUE)
    , tapirLabel (GetReference (), ID_SCRIPT_MANAGER_TAPIR_LABEL)
    , tapirValue (GetReference (), ID_SCRIPT_MANAGER_TAPIR_VALUE)
    , licenceLabel (GetReference (), ID_SCRIPT_MANAGER_LICENCE_LABEL)
    , licenceValue (GetReference (), ID_SCRIPT_MANAGER_LICENCE_VALUE)
    , whatIsNewTitle (GetReference (), ID_SCRIPT_MANAGER_WHAT_IS_NEW)
    , changelogText (GetReference (), ID_SCRIPT_MANAGER_CHANGELOG)
    , shareButton (GetReference (), ID_SCRIPT_MANAGER_SHARE)
    , scriptsPageButton (GetReference (), ID_SCRIPT_MANAGER_SCRIPTS_PAGE)
    , closeButton (GetReference (), ID_SCRIPT_MANAGER_CLOSE)
    , enabledCheckBox (GetReference (), ID_SCRIPT_MANAGER_ENABLED)
    , installButton (GetReference (), ID_SCRIPT_MANAGER_INSTALL)
    , removeButton (GetReference (), ID_SCRIPT_MANAGER_REMOVE)
    , updateButton (GetReference (), ID_SCRIPT_MANAGER_UPDATE)
    , updateAllButton (GetReference (), ID_SCRIPT_MANAGER_UPDATE_ALL)
{
    SetGrowType (DG::Dialog::HVGrow);
    SetMinClientSize (GetOriginalClientWidth (), GetOriginalClientHeight ());

    availableTab.Select ();
    fitsCheckBox.Check ();
    scriptsPageButton.Hide ();

    // The categories: "All categories", then the 12 categories, each with the number of scripts beside it.
    const short categoryListWidth = categoryList.GetItemWidth ();
    const short countStart = static_cast<short> (categoryListWidth - 36);
    categoryList.SetTabFieldCount (2);
    categoryList.SetTabFieldProperties (CategoryNameTab, 0, countStart, DG::ListBox::Left, DG::ListBox::EndTruncate, false);
    categoryList.SetTabFieldProperties (CategoryCountTab, countStart, categoryListWidth, DG::ListBox::Right, DG::ListBox::NoTruncate, false);
    for (short category = 0; category <= CategoryCount; ++category) {
        categoryList.AppendItem ();
        categoryList.SetTabItemText (static_cast<short> (AllCategoriesItem + category), CategoryNameTab,
            ManagerText (ID_SCRIPT_MANAGER_STRINGS_ALL_CATEGORIES + category));
    }
    categoryList.SelectItem (AllCategoriesItem);

    SetupScriptListColumns ();
    // The installed scripts are listed even when no catalogue can be loaded.
    LoadInstalls ();
    RefreshAll ();

    // Attached once the controls are set up, so that setting them up sends no change events here.
    Attach (*this);
    enabledCheckBox.Attach (*this);
    installButton.Attach (*this);
    removeButton.Attach (*this);
    updateButton.Attach (*this);
    updateAllButton.Attach (*this);
    searchEdit.Attach (*this);
    availableTab.Attach (*this);
    installedTab.Attach (*this);
    updatesTab.Attach (*this);
    refreshButton.Attach (*this);
    categoryList.Attach (*this);
    fitsCheckBox.Attach (*this);
    scriptList.Attach (*this);
    shareButton.Attach (*this);
    scriptsPageButton.Attach (*this);
    closeButton.Attach (*this);

    BeginEventProcessing ();
}

ScriptManager::~ScriptManager ()
{
    searchEdit.Detach (*this);
    availableTab.Detach (*this);
    installedTab.Detach (*this);
    updatesTab.Detach (*this);
    refreshButton.Detach (*this);
    categoryList.Detach (*this);
    fitsCheckBox.Detach (*this);
    scriptList.Detach (*this);
    shareButton.Detach (*this);
    scriptsPageButton.Detach (*this);
    closeButton.Detach (*this);
    enabledCheckBox.Detach (*this);
    installButton.Detach (*this);
    removeButton.Detach (*this);
    updateButton.Detach (*this);
    updateAllButton.Detach (*this);
    EndEventProcessing ();
}

bool ScriptManager::HasInstance ()
{
    return instance != nullptr;
}

ScriptManager& ScriptManager::Instance ()
{
    if (!HasInstance ()) {
        instance = new ScriptManager ();
    }
    return *instance;
}

void ScriptManager::Show ()
{
    hiddenByArchicad = false;
    if (IsVisible ()) {
        BringToFront ();
        return;
    }
    DG::Palette::Show ();
    LoadCatalogue ();
}

void ScriptManager::Hide ()
{
    hiddenByArchicad = false;
    DG::Palette::Hide ();
}

void ScriptManager::LoadCatalogue ()
{
    const GS::UniString& configuredUrl = Config::Instance ().MarketplaceCatalogueUrl ();
    const GS::UniString url = configuredUrl.IsEmpty () ? GS::UniString (DefaultCatalogueUrl) : configuredUrl;

    std::vector<char> body;
    GS::UniString fetchError;
    std::vector<Script> fetchedScripts;
    bool fetched = false;
    if (DownloadBytes (url, body, fetchError)) {
        GS::ObjectState catalogueOS;
        if (BytesToObjectState (body, catalogueOS) && ParseCatalogue (catalogueOS, fetchedScripts)) {
            fetched = true;
        } else {
            fetchError = ManagerText (ID_SCRIPT_MANAGER_STRINGS_NOT_A_CATALOGUE);
        }
    }

    if (fetched) {
        shownScripts.clear ();   // the rows point into the old list
        scripts = std::move (fetchedScripts);
        hasCatalogue = true;
        catalogueTime = std::time (nullptr);
        WriteCatalogueCache (body, catalogueTime);
    } else {
        ACAPI_WriteReport (ManagerText (ID_SCRIPT_MANAGER_STRINGS_FETCH_FAILED_REPORT), false, url.ToPrintf (), fetchError.ToPrintf ());

        // The cached copy, with the time it was fetched. Without one, the window keeps what it shows already.
        std::vector<char> cached;
        GS::ObjectState cacheOS;
        GS::ObjectState cachedCatalogueOS;
        std::vector<Script> cachedScripts;
        if (ReadFileBytes (GetCatalogueCacheLocation (), cached) &&
            BytesToObjectState (cached, cacheOS) &&
            cacheOS.Get ("catalogue", cachedCatalogueOS) &&
            ParseCatalogue (cachedCatalogueOS, cachedScripts))
        {
            double fetchedAt = 0.0;
            cacheOS.Get ("fetchedAt", fetchedAt);
            shownScripts.clear ();
            scripts = std::move (cachedScripts);
            hasCatalogue = true;
            catalogueTime = static_cast<std::time_t> (fetchedAt);
        }
    }

    if (!hasCatalogue) {
        catalogueTimeText.SetText (ManagerText (ID_SCRIPT_MANAGER_STRINGS_NOT_LOADED));
        scriptsPageButton.Show ();
    } else {
        catalogueTimeText.SetText (GS::UniString::Printf (
            ManagerText (fetched ? ID_SCRIPT_MANAGER_STRINGS_CHECKED : ID_SCRIPT_MANAGER_STRINGS_CHECKED_OFFLINE),
            FormatCatalogueTime (catalogueTime).ToPrintf ()));
        scriptsPageButton.Hide ();
    }

    LoadInstalls ();
    RefreshAll ();
}

void ScriptManager::LoadInstalls ()
{
    installs = MarketplaceInstalls::Load ();
    MergeInstalledScripts ();
}

void ScriptManager::MergeInstalledScripts ()
{
    shownScripts.clear ();   // the rows point into the list changed here
    std::vector<Script> merged;
    for (Script& script : scripts) {
        if (script.inCatalogue) {
            merged.push_back (std::move (script));
        }
    }
    scripts = std::move (merged);

    for (const MarketplaceInstalls::InstalledScript& installed : installs.scripts) {
        bool listed = false;
        for (const Script& script : scripts) {
            if (script.slug == installed.slug) {
                listed = true;
                break;
            }
        }
        if (listed) {
            continue;
        }
        Script script;
        script.slug = installed.slug;
        script.name = installed.name;
        script.entry = installed.entry;
        script.inCatalogue = false;
        ScriptVersion version;
        version.version = installed.version;
        script.versions.push_back (version);
        scripts.push_back (script);
    }
}

const MarketplaceInstalls::InstalledScript* ScriptManager::GetInstalled (const Script& script) const
{
    return installs.Find (script.slug);
}

const ScriptManager::ScriptVersion* ScriptManager::GetUpdate (const Script& script) const
{
    const MarketplaceInstalls::InstalledScript* installed = GetInstalled (script);
    if (installed == nullptr || !script.inCatalogue) {
        return nullptr;
    }
    // The catalogue lists the versions newest first, so the ones before the installed version are newer. When the
    // catalogue no longer lists the installed version, the version numbers are compared.
    bool installedListed = false;
    for (const ScriptVersion& version : script.versions) {
        if (version.version == installed->version) {
            installedListed = true;
            break;
        }
    }
    for (const ScriptVersion& version : script.versions) {
        if (version.version == installed->version) {
            break;
        }
        if (!installedListed && !IsNewerVersion (version.version, installed->version)) {
            continue;
        }
        GS::UniString reason;
        if (!version.url.IsEmpty () && !version.sha256.IsEmpty () && FitsThisArchicad (version, reason)) {
            return &version;
        }
    }
    return nullptr;
}

ScriptManager::Tab ScriptManager::GetCurrentTab () const
{
    if (installedTab.IsSelected ()) {
        return Tab::Installed;
    }
    if (updatesTab.IsSelected ()) {
        return Tab::Updates;
    }
    return Tab::Available;
}

GS::UniString ScriptManager::GetSelectedCategory () const
{
    const short item = categoryList.GetSelectedItem ();
    if (item <= AllCategoriesItem || item > AllCategoriesItem + CategoryCount) {
        return GS::EmptyUniString;
    }
    return GS::UniString (CategoryKeys[item - AllCategoriesItem - 1]);
}

bool ScriptManager::FitsThisArchicad (const Script& script, GS::UniString& reason) const
{
    return FitsThisArchicad (script.versions.front (), reason);
}

bool ScriptManager::FitsThisArchicad (const ScriptVersion& version, GS::UniString& reason) const
{
    reason = GS::EmptyUniString;
    if (ScriptRequirements::CheckMinTapirVersion (version.minTapirVersion.ToCStr (CC_UTF8).Get (), ADDON_VERSION) ==
        ScriptRequirements::MinTapirVersionResult::TooOld)
    {
        reason = GS::UniString::Printf (PaletteText (ID_PALETTE_STRINGS_NEEDS_TAPIR_NOT_MET), version.minTapirVersion.ToPrintf (), ADDON_VERSION);
        return false;
    }
    const int archicadVersion = static_cast<int> (VersionChecker::ArchicadMainVersion ());
    if (!ScriptRequirements::IsArchicadVersionListed (version.archicadVersions, archicadVersion)) {
        reason = GS::UniString::Printf (PaletteText (ID_PALETTE_STRINGS_TESTED_ON_AC_NOT_MET),
            JoinArchicadVersions (version.archicadVersions).ToPrintf (), archicadVersion);
        return false;
    }
    return true;
}

bool ScriptManager::MatchesSearch (const Script& script, const GS::UniString& lowerCasedSearch) const
{
    if (lowerCasedSearch.IsEmpty ()) {
        return true;
    }
    if (ToLowerCased (script.name).Contains (lowerCasedSearch) || ToLowerCased (script.authorName).Contains (lowerCasedSearch) ||
        ToLowerCased (script.authorGithub).Contains (lowerCasedSearch))
    {
        return true;
    }
    for (const GS::UniString& tag : script.tags) {
        if (ToLowerCased (tag).Contains (lowerCasedSearch)) {
            return true;
        }
    }
    return false;
}

std::vector<size_t> ScriptManager::GetScriptsOfTabAndSearch () const
{
    std::vector<size_t> result;
    const Tab tab = GetCurrentTab ();
    const GS::UniString search = ToLowerCased (searchEdit.GetText ());
    const bool onlyFitting = fitsCheckBox.IsChecked ();
    for (size_t i = 0; i < scripts.size (); ++i) {
        if (!MatchesSearch (scripts[i], search)) {
            continue;
        }
        if (tab == Tab::Installed) {
            if (GetInstalled (scripts[i]) == nullptr) {
                continue;
            }
        } else if (tab == Tab::Updates) {
            // The installed scripts with a newer fitting version.
            if (GetUpdate (scripts[i]) == nullptr) {
                continue;
            }
        } else {
            GS::UniString reason;
            if (!scripts[i].inCatalogue || (onlyFitting && !FitsThisArchicad (scripts[i], reason))) {
                continue;
            }
        }
        result.push_back (i);
    }
    return result;
}

void ScriptManager::RefreshAll ()
{
    RefreshTabTexts ();
    RefreshCategoryList ();
    RefreshScriptList ();
}

void ScriptManager::RefreshTabTexts ()
{
    int availableCount = 0;
    int installedCount = 0;
    int updatesCount = 0;
    const GS::UniString search = ToLowerCased (searchEdit.GetText ());
    const bool onlyFitting = fitsCheckBox.IsChecked ();
    for (const Script& script : scripts) {
        if (!MatchesSearch (script, search)) {
            continue;
        }
        GS::UniString reason;
        if (script.inCatalogue && (!onlyFitting || FitsThisArchicad (script, reason))) {
            ++availableCount;
        }
        if (GetInstalled (script) != nullptr) {
            ++installedCount;
        }
        if (GetUpdate (script) != nullptr) {
            ++updatesCount;
        }
    }
    availableTab.SetText (GS::UniString::Printf (ManagerText (ID_SCRIPT_MANAGER_STRINGS_AVAILABLE_TAB), availableCount));
    installedTab.SetText (GS::UniString::Printf (ManagerText (ID_SCRIPT_MANAGER_STRINGS_INSTALLED_TAB), installedCount));
    updatesTab.SetText (GS::UniString::Printf (ManagerText (ID_SCRIPT_MANAGER_STRINGS_UPDATES_TAB), updatesCount));
    fitsCheckBox.SetVisibility (GetCurrentTab () == Tab::Available);
}

void ScriptManager::RefreshCategoryList ()
{
    const std::vector<size_t> tabScripts = GetScriptsOfTabAndSearch ();
    categoryList.SetTabItemText (AllCategoriesItem, CategoryCountTab, GS::UniString::Printf ("%d", static_cast<int> (tabScripts.size ())));
    for (short category = 0; category < CategoryCount; ++category) {
        int count = 0;
        for (size_t index : tabScripts) {
            if (scripts[index].category == CategoryKeys[category]) {
                ++count;
            }
        }
        categoryList.SetTabItemText (static_cast<short> (AllCategoriesItem + 1 + category), CategoryCountTab, GS::UniString::Printf ("%d", count));
    }
}

void ScriptManager::SetupScriptListColumns ()
{
    scriptList.SetTabFieldCount (ScriptTabCount);
    scriptList.SetHeaderSynchronState (false);
    scriptList.SetHeaderItemCount (ScriptTabCount);
    LayoutScriptListColumns ();
}

// Available and Installed: Name, Summary and Version, the summary taking what the others leave of the list's width.
// Updates: Name, Installed and New, the name taking the rest.
void ScriptManager::LayoutScriptListColumns ()
{
    const bool updates = GetCurrentTab () == Tab::Updates;
    scriptList.SetHeaderItemText (ScriptNameTab, ManagerText (ID_SCRIPT_MANAGER_STRINGS_NAME_COLUMN));
    scriptList.SetHeaderItemText (ScriptSummaryTab, ManagerText (updates ? ID_SCRIPT_MANAGER_STRINGS_INSTALLED_COLUMN : ID_SCRIPT_MANAGER_STRINGS_SUMMARY_COLUMN));
    scriptList.SetHeaderItemText (ScriptVersionTab, ManagerText (updates ? ID_SCRIPT_MANAGER_STRINGS_NEW_COLUMN : ID_SCRIPT_MANAGER_STRINGS_VERSION_COLUMN));

    const short width = scriptList.GetItemWidth ();
    constexpr short versionWidth = 58;
    const short versionStart = static_cast<short> (width - versionWidth);
    const short nameWidth = updates ? static_cast<short> (versionStart - versionWidth) : static_cast<short> (140);
    scriptList.SetTabFieldProperties (ScriptNameTab, 0, nameWidth, DG::ListBox::Left, DG::ListBox::EndTruncate, false);
    scriptList.SetTabFieldProperties (ScriptSummaryTab, nameWidth, versionStart, DG::ListBox::Left, DG::ListBox::EndTruncate, false);
    scriptList.SetTabFieldProperties (ScriptVersionTab, versionStart, width, DG::ListBox::Left, DG::ListBox::EndTruncate, false);
    scriptList.SetHeaderItemSize (ScriptNameTab, nameWidth);
    scriptList.SetHeaderItemSize (ScriptSummaryTab, static_cast<short> (versionStart - nameWidth));
    scriptList.SetHeaderItemSize (ScriptVersionTab, versionWidth);
}

void ScriptManager::RefreshScriptList ()
{
    // The selected script stays selected when it is still listed (selectedSlug, as the rows may point into a list
    // that was replaced since).
    const Tab currentTab = GetCurrentTab ();
    const bool listsInstalled = currentTab != Tab::Available;
    LayoutScriptListColumns ();   // the columns of Updates differ
    const GS::UniString category = GetSelectedCategory ();
    shownScripts.clear ();
    for (size_t index : GetScriptsOfTabAndSearch ()) {
        if (category.IsEmpty () || scripts[index].category == category) {
            shownScripts.push_back (index);
        }
    }

    const Gfx::Color grey (128, 128, 128);
    short itemToSelect = 0;
    scriptList.DisableDraw ();
    if (scriptList.GetItemCount () > 0) {
        scriptList.DeleteItem (DG::ListBox::AllItems);
    }
    for (size_t row = 0; row < shownScripts.size (); ++row) {
        const Script& script = scripts[shownScripts[row]];
        const short item = static_cast<short> (row + 1);
        scriptList.AppendItem ();
        GS::UniString reason;
        // On Installed, the installed version, and nothing greyed out: an installed script is listed in any case.
        const MarketplaceInstalls::InstalledScript* installed = GetInstalled (script);
        const bool fits = listsInstalled || FitsThisArchicad (script, reason);
        scriptList.SetTabItemText (item, ScriptNameTab, script.name);
        const ScriptVersion* update = (currentTab == Tab::Updates) ? GetUpdate (script) : nullptr;
        if (update != nullptr && installed != nullptr) {
            // Updates: the installed version, and the new one in bold.
            scriptList.SetTabItemText (item, ScriptSummaryTab, installed->version);
            scriptList.SetTabItemText (item, ScriptVersionTab, update->version);
            scriptList.SetTabItemFontStyle (item, ScriptVersionTab, DG::Font::Bold);
        } else {
            // A script that does not fit is only listed with Fits this Archicad off: greyed out, with the reason.
            scriptList.SetTabItemText (item, ScriptSummaryTab, fits ? script.summary : reason + " - " + script.summary);
            scriptList.SetTabItemText (item, ScriptVersionTab,
                (listsInstalled && installed != nullptr) ? installed->version : script.versions.front ().version);
        }
        if (!fits) {
            for (short tab = 1; tab <= ScriptTabCount; ++tab) {
                scriptList.SetTabItemColor (item, tab, grey);
            }
        }
        if (script.slug == selectedSlug) {
            itemToSelect = item;
        }
    }
    if (itemToSelect == 0 && !shownScripts.empty ()) {
        itemToSelect = 1;
    }
    if (itemToSelect > 0) {
        scriptList.SelectItem (itemToSelect);
    }
    scriptList.EnableDraw ();
    scriptList.Redraw ();

    RefreshDetails ();
}

const ScriptManager::Script* ScriptManager::GetSelectedScript () const
{
    const short item = scriptList.GetSelectedItem ();
    if (item < 1 || static_cast<size_t> (item) > shownScripts.size () || shownScripts[static_cast<size_t> (item - 1)] >= scripts.size ()) {
        return nullptr;
    }
    return &scripts[shownScripts[static_cast<size_t> (item - 1)]];
}

void ScriptManager::RefreshDetails ()
{
    const Script* script = GetSelectedScript ();
    DG::Item* const detailItems[] = {
        &nameText, &bylineText, &summaryText, &changesLabel, &changesValue, &archicadLabel, &archicadValue,
        &tapirLabel, &tapirValue, &licenceLabel, &licenceValue, &whatIsNewTitle, &changelogText,
        &enabledCheckBox, &installButton, &removeButton, &updateButton
    };
    for (DG::Item* item : detailItems) {
        item->SetVisibility (script != nullptr);
    }
    // Update all, under the list of the Updates tab.
    updateAllButton.SetVisibility (GetCurrentTab () == Tab::Updates);
    updateAllButton.SetStatus (!shownScripts.empty ());
    if (script == nullptr) {
        return;
    }
    selectedSlug = script->slug;

    const Tab tab = GetCurrentTab ();
    const MarketplaceInstalls::InstalledScript* installed = GetInstalled (*script);
    const ScriptVersion& newest = script->versions.front ();
    nameText.SetText (script->name);
    if (!script->inCatalogue) {
        bylineText.SetText (GS::UniString::Printf (ManagerText (ID_SCRIPT_MANAGER_STRINGS_BYLINE_NOT_IN_CATALOGUE),
            installed != nullptr ? installed->version.ToPrintf () : newest.version.ToPrintf ()));
    } else if (installed != nullptr) {
        bylineText.SetText (GS::UniString::Printf (ManagerText (ID_SCRIPT_MANAGER_STRINGS_BYLINE_INSTALLED),
            script->authorName.ToPrintf (), installed->version.ToPrintf (), newest.version.ToPrintf ()));
    } else {
        bylineText.SetText (GS::UniString::Printf (ManagerText (ID_SCRIPT_MANAGER_STRINGS_BYLINE),
            script->authorName.ToPrintf (), newest.version.ToPrintf ()));
    }
    summaryText.SetText (script->summary);

    // Enabled, for an installed script; Install on Available, for a script not installed yet; Remove on Installed
    // and Updates, with Update beside it when a newer fitting version exists.
    enabledCheckBox.SetVisibility (installed != nullptr);
    if (installed != nullptr) {
        enabledCheckBox.SetState (installed->enabled);
    }
    installButton.SetVisibility (tab == Tab::Available);
    installButton.SetStatus (installed == nullptr && script->inCatalogue && !newest.url.IsEmpty () && !newest.sha256.IsEmpty ());
    removeButton.SetVisibility (tab != Tab::Available && installed != nullptr);
    const ScriptVersion* update = GetUpdate (*script);
    updateButton.SetVisibility (tab != Tab::Available && update != nullptr);

    // What it changes: the badge of the entry script's header; the row is left out when the header has none.
    const GS::UniString badge = GetChangesBadge (script->changes);
    changesLabel.SetVisibility (!badge.IsEmpty ());
    changesValue.SetVisibility (!badge.IsEmpty ());
    changesValue.SetText (badge);

    archicadValue.SetText (JoinArchicadVersions (newest.archicadVersions));
    tapirValue.SetText (newest.minTapirVersion.IsEmpty () ? GS::EmptyUniString :
        GS::UniString::Printf (ManagerText (ID_SCRIPT_MANAGER_STRINGS_OR_NEWER), newest.minTapirVersion.ToPrintf ()));
    licenceValue.SetText (script->license);

    // On Available, what is new in the newest version. On Installed, the change log of each version newer than the
    // installed one, newest first (the catalogue lists the versions newest first), or the installed version's own
    // when there is none newer.
    GS::UniString changelog = newest.changelog;
    if (tab != Tab::Available && installed != nullptr) {
        size_t installedIndex = script->versions.size ();
        for (size_t i = 0; i < script->versions.size (); ++i) {
            if (script->versions[i].version == installed->version) {
                installedIndex = i;
                break;
            }
        }
        if (installedIndex > 0 && installedIndex < script->versions.size ()) {
            changelog = GS::EmptyUniString;
            for (size_t i = 0; i < installedIndex; ++i) {
                if (!changelog.IsEmpty ()) {
                    changelog += "\n";
                }
                changelog += GS::UniString::Printf (ManagerText (ID_SCRIPT_MANAGER_STRINGS_CHANGELOG_LINE),
                    script->versions[i].version.ToPrintf (), script->versions[i].changelog.ToPrintf ());
            }
        }
    }
    whatIsNewTitle.SetText (GS::UniString::Printf (ManagerText (ID_SCRIPT_MANAGER_STRINGS_WHAT_IS_NEW), newest.version.ToPrintf ()));
    changelogText.SetText (changelog);
}

void ScriptManager::ShowInstallError (const Script& script, bool isUpdate, const GS::UniString& reason) const
{
    DGAlert (DG_ERROR, ManagerText (ID_SCRIPT_MANAGER_STRINGS_ALERT_TITLE),
        GS::UniString::Printf (ManagerText (isUpdate ? ID_SCRIPT_MANAGER_STRINGS_UPDATE_FAILED : ID_SCRIPT_MANAGER_STRINGS_INSTALL_FAILED), script.name.ToPrintf ()),
        reason, ManagerText (ID_SCRIPT_MANAGER_STRINGS_OK));
}

bool ScriptManager::Install (const Script& script, const ScriptVersion& version)
{
    // An update when another version of the script is installed: that version's folder is removed afterwards.
    GS::UniString oldVersion;
    if (const MarketplaceInstalls::InstalledScript* current = GetInstalled (script)) {
        if (current->version != version.version) {
            oldVersion = current->version;
        }
    }
    const bool isUpdate = !oldVersion.IsEmpty ();

    if (version.url.IsEmpty () || version.sha256.IsEmpty ()) {
        ShowInstallError (script, isUpdate, GS::UniString::Printf (ManagerText (ID_SCRIPT_MANAGER_STRINGS_NO_DOWNLOAD), version.version.ToPrintf ()));
        return false;
    }
    // They become the path Documents/Tapir/marketplace/<slug>/<version>/<entry>.
    const GS::UniString* const names[] = { &script.slug, &version.version, &script.entry };
    for (const GS::UniString* name : names) {
        if (!MarketplaceInstalls::IsSafeName (*name)) {
            ShowInstallError (script, isUpdate, GS::UniString::Printf (ManagerText (ID_SCRIPT_MANAGER_STRINGS_UNSAFE_NAME), name->ToPrintf ()));
            return false;
        }
    }

    // Asked once per author, before their first script is installed.
    const GS::UniString authorKey = AuthorKey (script);
    if (!installs.IsAuthorAllowed (authorKey)) {
        const GS::UniString author = script.authorName.IsEmpty () ? script.authorGithub : script.authorName;
        const short response = DGAlert (DG_WARNING, ManagerText (ID_SCRIPT_MANAGER_STRINGS_ALERT_TITLE),
            GS::UniString::Printf (ManagerText (ID_SCRIPT_MANAGER_STRINGS_ALLOW_AUTHOR), author.ToPrintf (), author.ToPrintf ()),
            GS::EmptyUniString,
            ManagerText (ID_SCRIPT_MANAGER_STRINGS_ALLOW),
            ManagerText (ID_SCRIPT_MANAGER_STRINGS_CANCEL));
        if (response != DG_OK) {
            return false;
        }
    }

    // Everything is downloaded and checked before anything is written.
    std::vector<char> code;
    GS::UniString error;
    if (!DownloadBytes (version.url, code, error)) {
        ShowInstallError (script, isUpdate, GS::UniString::Printf (ManagerText (ID_SCRIPT_MANAGER_STRINGS_DOWNLOAD_FAILED), script.entry.ToPrintf (), error.ToPrintf ()));
        return false;
    }
    const GS::UniString actualHash (Sha256::HexDigest (code.data (), code.size ()).c_str ());
    const GS::UniString expectedHash = ToLowerCased (version.sha256);
    if (actualHash != expectedHash) {
        ShowInstallError (script, isUpdate, GS::UniString::Printf (ManagerText (ID_SCRIPT_MANAGER_STRINGS_HASH_MISMATCH),
            script.entry.ToPrintf (), actualHash.ToPrintf (), expectedHash.ToPrintf ()));
        return false;
    }

    // The helpers of the built-in scripts, from main, when the script imports them.
    std::vector<std::pair<IO::RelativeLocation, std::vector<char>>> helpers;
    if (ImportsHelpers (code)) {
        for (const auto& helper : HelperFiles) {
            const GS::UniString relativePath = GS::UniString (helper[0]) + "/" + helper[1];
            std::vector<char> content;
            if (!DownloadBytes (GS::UniString (HelpersBaseUrl) + relativePath, content, error)) {
                ShowInstallError (script, isUpdate, GS::UniString::Printf (ManagerText (ID_SCRIPT_MANAGER_STRINGS_DOWNLOAD_FAILED), relativePath.ToPrintf (), error.ToPrintf ()));
                return false;
            }
            IO::RelativeLocation relativeLocation;
            relativeLocation.Append (IO::Name (helper[0]));
            relativeLocation.Append (IO::Name (helper[1]));
            helpers.emplace_back (relativeLocation, std::move (content));
        }
    }

    // Documents/Tapir/marketplace/<slug>/<version>/, written from scratch; removed again when a file fails.
    const IO::Location versionFolder = MarketplaceInstalls::GetVersionFolder (script.slug, version.version);
    IO::fileSystem.Delete (versionFolder);
    IO::Location failedFile;
    bool written = IO::fileSystem.CreateFolderTree (versionFolder) == NoError;
    if (!written) {
        failedFile = versionFolder;
    }
    if (written) {
        IO::Location entryLocation = versionFolder;
        entryLocation.AppendToLocal (IO::Name (script.entry));
        if (!WriteFileBytes (entryLocation, code)) {
            written = false;
            failedFile = entryLocation;
        }
    }
    for (const auto& helper : helpers) {
        if (!written) {
            break;
        }
        IO::Location helperLocation = versionFolder;
        helperLocation.AppendToLocal (helper.first);
        IO::Location helperFolder = helperLocation;
        helperFolder.DeleteLastLocalName ();
        if (IO::fileSystem.CreateFolderTree (helperFolder) != NoError || !WriteFileBytes (helperLocation, helper.second)) {
            written = false;
            failedFile = helperLocation;
        }
    }
    if (!written) {
        IO::fileSystem.Delete (versionFolder);
        ShowInstallError (script, isUpdate, GS::UniString::Printf (ManagerText (ID_SCRIPT_MANAGER_STRINGS_WRITE_FAILED), LocationText (failedFile).ToPrintf ()));
        return false;
    }

    // installed.json, read again first so that nothing written since the window loaded it is lost.
    installs = MarketplaceInstalls::Load ();
    MarketplaceInstalls::InstalledScript* existing = installs.Find (script.slug);
    MarketplaceInstalls::InstalledScript previous;
    if (existing != nullptr) {
        previous = *existing;
    }
    MarketplaceInstalls::InstalledScript installed;
    installed.slug = script.slug;
    installed.name = script.name;
    installed.version = version.version;
    installed.entry = script.entry;
    installed.enabled = existing != nullptr ? existing->enabled : true;
    installed.authorAllowed = true;
    if (existing != nullptr) {
        *existing = installed;
    } else {
        installs.scripts.push_back (installed);
    }
    if (!installs.IsAuthorAllowed (authorKey)) {
        installs.allowedAuthors.Push (authorKey);
    }
    if (!MarketplaceInstalls::Save (installs)) {
        IO::Location installedFile = MarketplaceInstalls::GetMarketplaceFolder ();
        installedFile.AppendToLocal (IO::Name ("installed.json"));
        ShowInstallError (script, isUpdate, GS::UniString::Printf (ManagerText (ID_SCRIPT_MANAGER_STRINGS_WRITE_FAILED), LocationText (installedFile).ToPrintf ()));
        IO::fileSystem.Delete (versionFolder);
        LoadInstalls ();
        return false;
    }

    // An update: the old version's folder goes only now that the new one is written and listed, and a shortcut slot
    // that pointed at the old version points at the new one (Enabled was kept above).
    if (!previous.version.IsEmpty () && previous.version != version.version) {
        const IO::Location oldVersionFolder = MarketplaceInstalls::GetVersionFolder (script.slug, previous.version);
        IO::Location newEntryLocation = versionFolder;
        newEntryLocation.AppendToLocal (IO::Name (script.entry));
        ReplaceShortcutSlotsIn (oldVersionFolder, newEntryLocation);
        IO::fileSystem.Delete (oldVersionFolder);
        ACAPI_WriteReport (ManagerText (ID_SCRIPT_MANAGER_STRINGS_UPDATED_REPORT), false,
            script.name.ToPrintf (), previous.version.ToPrintf (), version.version.ToPrintf (), LocationText (versionFolder).ToPrintf ());
        return true;
    }

    ACAPI_WriteReport (ManagerText (ID_SCRIPT_MANAGER_STRINGS_INSTALLED_REPORT), false,
        script.name.ToPrintf (), version.version.ToPrintf (), LocationText (versionFolder).ToPrintf ());
    return true;
}

bool ScriptManager::UpdateAll ()
{
    // What the Updates tab lists, with its search and category. Copies: each update re-reads installed.json and
    // rebuilds the list the rows point into.
    std::vector<std::pair<Script, ScriptVersion>> updates;
    for (size_t index : shownScripts) {
        const ScriptVersion* update = index < scripts.size () ? GetUpdate (scripts[index]) : nullptr;
        if (update != nullptr) {
            updates.emplace_back (scripts[index], *update);
        }
    }
    bool anyUpdated = false;
    for (const auto& update : updates) {
        if (Install (update.first, update.second)) {
            anyUpdated = true;
            LoadInstalls ();
        }
    }
    return anyUpdated;
}

bool ScriptManager::Remove (const Script& script)
{
    const MarketplaceInstalls::InstalledScript* installed = GetInstalled (script);
    if (installed == nullptr) {
        return false;
    }
    const IO::Location scriptFolder = MarketplaceInstalls::GetScriptFolder (script.slug);
    const short response = DGAlert (DG_WARNING, ManagerText (ID_SCRIPT_MANAGER_STRINGS_ALERT_TITLE),
        GS::UniString::Printf (ManagerText (ID_SCRIPT_MANAGER_STRINGS_REMOVE_QUESTION), script.name.ToPrintf ()),
        GS::UniString::Printf (ManagerText (ID_SCRIPT_MANAGER_STRINGS_REMOVE_DETAILS), LocationText (scriptFolder).ToPrintf ()),
        ManagerText (ID_SCRIPT_MANAGER_STRINGS_REMOVE),
        ManagerText (ID_SCRIPT_MANAGER_STRINGS_CANCEL));
    if (response != DG_OK) {
        return false;
    }

    // The script leaves installed.json first, so it is never listed without its files.
    installs = MarketplaceInstalls::Load ();
    std::vector<MarketplaceInstalls::InstalledScript> kept;
    for (const MarketplaceInstalls::InstalledScript& other : installs.scripts) {
        if (other.slug != script.slug) {
            kept.push_back (other);
        }
    }
    installs.scripts = std::move (kept);
    if (!MarketplaceInstalls::Save (installs)) {
        DGAlert (DG_ERROR, ManagerText (ID_SCRIPT_MANAGER_STRINGS_ALERT_TITLE),
            GS::UniString::Printf (ManagerText (ID_SCRIPT_MANAGER_STRINGS_REMOVE_FAILED), script.name.ToPrintf ()),
            GS::EmptyUniString, ManagerText (ID_SCRIPT_MANAGER_STRINGS_OK));
        LoadInstalls ();
        return false;
    }
    // A shortcut slot pointing at the script would run nothing any more.
    ReplaceShortcutSlotsIn (scriptFolder, IO::Location ());
    IO::fileSystem.Delete (scriptFolder);
    ACAPI_WriteReport (ManagerText (ID_SCRIPT_MANAGER_STRINGS_REMOVED_REPORT), false, script.name.ToPrintf ());
    return true;
}

void ScriptManager::PanelCloseRequested (const DG::PanelCloseRequestEvent&, bool* accepted)
{
    Hide ();
    *accepted = true;
}

void ScriptManager::PanelResized (const DG::PanelResizeEvent& ev)
{
    const short h = ev.GetHorizontalChange ();
    const short v = ev.GetVerticalChange ();

    BeginMoveResizeItems ();
    // The top bar's right end, the list in the middle and the details on the right follow the window's width; the
    // lists and the change log follow its height, the buttons at the bottom stay at the bottom.
    catalogueTimeText.Move (h, 0);
    refreshButton.Move (h, 0);
    categoryList.Resize (0, v);
    scriptList.Resize (h, v);
    DG::Item* const detailItems[] = {
        &nameText, &bylineText, &summaryText, &changesLabel, &changesValue, &archicadLabel, &archicadValue,
        &tapirLabel, &tapirValue, &licenceLabel, &licenceValue, &whatIsNewTitle
    };
    for (DG::Item* item : detailItems) {
        item->Move (h, 0);
    }
    changelogText.MoveAndResize (h, 0, 0, v);
    shareButton.Move (0, v);
    scriptsPageButton.Move (0, v);
    closeButton.Move (h, v);
    enabledCheckBox.Move (h, v);
    installButton.Move (h, v);
    removeButton.Move (h, v);
    updateButton.Move (h, v);
    updateAllButton.Move (h, v);   // right-aligned under the list, which follows the width
    EndMoveResizeItems ();

    LayoutScriptListColumns ();
}

void ScriptManager::ButtonClicked (const DG::ButtonClickEvent& ev)
{
    if (ev.GetSource () == &refreshButton) {
        LoadCatalogue ();
    } else if (ev.GetSource () == &shareButton) {
        // The Share a script issue wires this button to its own dialog later.
        OpenWebpage (ShareScriptPageUrl);
    } else if (ev.GetSource () == &scriptsPageButton) {
        OpenWebpage (ScriptsPageUrl);
    } else if (ev.GetSource () == &closeButton) {
        Hide ();
    } else if (ev.GetSource () == &updateAllButton) {
        if (UpdateAll ()) {
            LoadInstalls ();
        }
        RefreshAll ();
    } else if (ev.GetSource () == &installButton || ev.GetSource () == &removeButton || ev.GetSource () == &updateButton) {
        const Script* selected = GetSelectedScript ();
        if (selected == nullptr) {
            return;
        }
        // Copies: installing, updating or removing rebuilds the list the selection points into.
        const Script script = *selected;
        bool changed = false;
        if (ev.GetSource () == &removeButton) {
            changed = Remove (script);
        } else if (ev.GetSource () == &installButton) {
            changed = Install (script, script.versions.front ());
        } else {
            const ScriptVersion* update = GetUpdate (script);
            if (update != nullptr) {
                const ScriptVersion version = *update;
                changed = Install (script, version);
            }
        }
        if (changed) {
            LoadInstalls ();
        }
        RefreshAll ();
    }
}

void ScriptManager::ListBoxSelectionChanged (const DG::ListBoxSelectionEvent& ev)
{
    if (ev.GetSource () == &categoryList) {
        RefreshScriptList ();
    } else if (ev.GetSource () == &scriptList) {
        RefreshDetails ();
    }
}

void ScriptManager::TextEditChanged (const DG::TextEditChangeEvent&)
{
    RefreshAll ();
}

void ScriptManager::CheckItemChanged (const DG::CheckItemChangeEvent& ev)
{
    if (ev.GetSource () != &enabledCheckBox) {
        RefreshAll ();
        return;
    }

    // Enabled in the Tapir Palette: unticked, the script stays installed but leaves the palette's list.
    const Script* selected = GetSelectedScript ();
    if (selected == nullptr) {
        return;
    }
    const GS::UniString slug = selected->slug;
    installs = MarketplaceInstalls::Load ();
    MarketplaceInstalls::InstalledScript* installed = installs.Find (slug);
    if (installed != nullptr) {
        installed->enabled = enabledCheckBox.IsChecked ();
        MarketplaceInstalls::Save (installs);
    }
    MergeInstalledScripts ();
    RefreshAll ();
}

void ScriptManager::RadioItemChanged (const DG::RadioItemChangeEvent&)
{
    RefreshAll ();
}

GSErrCode ScriptManager::PaletteControlCallBack (Int32, API_PaletteMessageID messageID, GS::IntPtr param)
{
    switch (messageID) {
        case APIPalMsg_OpenPalette:
            Instance ().Show ();
            break;

        case APIPalMsg_ClosePalette:
            if (HasInstance ()) {
                Instance ().Hide ();
            }
            break;

        // Archicad hides the palettes for a while (e.g. on a tab switch); only a window that was open comes back.
        case APIPalMsg_HidePalette_Begin:
            if (HasInstance () && Instance ().IsVisible ()) {
                Instance ().DG::Palette::Hide ();
                Instance ().hiddenByArchicad = true;
            }
            break;

        case APIPalMsg_HidePalette_End:
            if (HasInstance () && Instance ().hiddenByArchicad) {
                Instance ().hiddenByArchicad = false;
                if (!Instance ().IsVisible ()) {
                    Instance ().DG::Palette::Show ();
                }
            }
            break;

        case APIPalMsg_IsPaletteVisible:
            *(reinterpret_cast<bool*> (param)) = HasInstance () && Instance ().IsVisible ();
            break;

        default:
            break;
    }

    return NoError;
}

GSErrCode ScriptManager::RegisterPaletteControlCallBack ()
{
    return ACAPI_RegisterModelessWindow (
                    GS::CalculateHashValue (paletteGuid),
                    PaletteControlCallBack,
                    API_PalEnabled_FloorPlan + API_PalEnabled_Section + API_PalEnabled_Elevation +
                    API_PalEnabled_InteriorElevation + API_PalEnabled_3D + API_PalEnabled_Detail +
                    API_PalEnabled_Worksheet + API_PalEnabled_Layout + API_PalEnabled_DocumentFrom3D,
                    GSGuid2APIGuid (paletteGuid));
}
