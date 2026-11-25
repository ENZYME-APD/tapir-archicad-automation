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
#include "DGMenu.hpp"
#include "DGMenuItem.hpp"
#include "DGCommandDescriptor.hpp"
#include "DGCommandEvent.hpp"

#include <cstdio>
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
// Below this client width the window has one column instead of three; it cannot be made smaller than the minimum.
static constexpr short NarrowWidth = 780;
static constexpr short MinWidth = 360;
static constexpr short MinHeight = 520;

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


// The commands of the Scripts tab's right-click menu. Each is a DG::Command of Tapir's 'MDID' (RFIX/AddOnFix.grc), so
// that it is never taken for a command of Archicad or another add-on.
static constexpr ULong TapirDeveloperId = 826567491;
static constexpr ULong TapirLocalId = 368333322;
enum LocalMenuCommand : ULong {
    RunCommand = 1,
    StopCommand,
    AssignShortcutCommand,
    EditCommand,
    ShowInFolderCommand,
    OpenOnGitHubCommand,
    RemoveCommand
};

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
// An empty body is an error unless allowEmpty: a helper file may be empty (utilities/__init__.py is, #3045).
static bool DownloadBytes (const GS::UniString& url, std::vector<char>& body, GS::UniString& error, bool allowEmpty = false)
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
        if (body.empty () && !allowEmpty) {
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

// "28, 29, 30": the catalogue's versions (std::vector<int>) or a header's (GS::Array<Int32>).
template <typename Versions>
static GS::UniString JoinArchicadVersions (const Versions& versions)
{
    GS::UniString text;
    for (const auto version : versions) {
        if (!text.IsEmpty ()) {
            text += ", ";
        }
        text += GS::UniString::Printf ("%d", static_cast<int> (version));
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

// text as a JavaScript string literal that is safe inside an HTML <script> element: besides the quote, the backslash
// and the control characters, <, > and & are escaped (so the text can never close the element) and so are U+2028
// and U+2029 (line terminators in older JavaScript).
static std::string ToJavaScriptString (const GS::UniString& text)
{
    const std::string utf8 = text.ToCStr (CC_UTF8).Get ();
    std::string result = "\"";
    for (size_t i = 0; i < utf8.size (); ++i) {
        const unsigned char c = static_cast<unsigned char> (utf8[i]);
        if (c == 0xE2 && i + 2 < utf8.size () && static_cast<unsigned char> (utf8[i + 1]) == 0x80 &&
            (static_cast<unsigned char> (utf8[i + 2]) == 0xA8 || static_cast<unsigned char> (utf8[i + 2]) == 0xA9))
        {
            result += (static_cast<unsigned char> (utf8[i + 2]) == 0xA8) ? "\\u2028" : "\\u2029";
            i += 2;
        } else if (c == '"' || c == '\\') {
            result += '\\';
            result += static_cast<char> (c);
        } else if (c < 0x20 || c == '<' || c == '>' || c == '&') {
            char escaped[8] = {};
            std::snprintf (escaped, sizeof (escaped), "\\u%04x", static_cast<unsigned int> (c));
            result += escaped;
        } else {
            result += static_cast<char> (c);
        }
    }
    result += "\"";
    return result;
}

// The folder of a version's entry file on the web: its url up to the last slash, which tools/generate_catalogue.py
// writes as https://raw.githubusercontent.com/<repo>/<commit>/marketplace/<slug>/<entry>. The README.md and the
// screenshots/ of the script are in the same folder at the same commit. Empty when the url is not https.
static GS::UniString GetVersionWebFolder (const ScriptManager::ScriptVersion& version)
{
    if (!version.url.BeginsWith (GS::UniString ("https://"))) {
        return GS::EmptyUniString;
    }
    const UIndex lastSlash = version.url.FindLast (GS::UniChar ('/'));
    if (lastSlash == MaxUIndex || lastSlash < 8) {
        return GS::EmptyUniString;
    }
    return GS::UniString (version.url.GetSubstring (0, lastSlash));
}

// The page of the README browser: the README as plain text, then the screenshots. The page fetches them itself, so
// the window never waits for them. Every text from the catalogue reaches the page as an escaped string and is
// shown with textContent or as an image address, never as HTML.
static GS::UniString BuildReadmePage (const GS::UniString& folder, const GS::Array<GS::UniString>& screenshots)
{
    std::string shots = "[";
    for (UIndex i = 0; i < screenshots.GetSize (); ++i) {
        if (i > 0) {
            shots += ",";
        }
        shots += ToJavaScriptString (screenshots[i]);
    }
    shots += "]";

    std::string page =
        "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><style>"
        "html,body{margin:0;padding:0;background:#fff;color:#263238;}"
        "body{font:12px -apple-system,'Segoe UI',Helvetica,Arial,sans-serif;padding:6px;}"
        "pre{white-space:pre-wrap;word-wrap:break-word;font:12px -apple-system,'Segoe UI',Helvetica,Arial,sans-serif;margin:0 0 8px 0;}"
        "h4{margin:8px 0 4px 0;font-size:12px;}"
        "img{display:block;max-width:100%;height:auto;margin:0 0 8px 0;border:1px solid #cfd8dc;}"
        "</style></head><body><pre id=\"readme\"></pre><div id=\"shots\"></div><script>(function () {"
        "var folder = " + ToJavaScriptString (folder) + ";"
        "var screenshots = " + shots + ";"
        "var readme = document.getElementById ('readme');"
        "readme.textContent = " + ToJavaScriptString (ManagerText (ID_SCRIPT_MANAGER_STRINGS_README_LOADING)) + ";"
        "var failed = " + ToJavaScriptString (ManagerText (ID_SCRIPT_MANAGER_STRINGS_README_FAILED)) + ";"
        "var request = new XMLHttpRequest ();"
        "request.onload = function () { readme.textContent = (request.status === 200) ? request.responseText : failed; };"
        "request.onerror = function () { readme.textContent = failed; };"
        "request.open ('GET', folder + '/README.md');"
        "request.send ();"
        "var shotsElement = document.getElementById ('shots');"
        "if (screenshots.length > 0) {"
        "  var heading = document.createElement ('h4');"
        "  heading.textContent = " + ToJavaScriptString (ManagerText (ID_SCRIPT_MANAGER_STRINGS_SCREENSHOTS)) + ";"
        "  shotsElement.appendChild (heading);"
        "}"
        "for (var i = 0; i < screenshots.length; ++i) {"
        "  var image = document.createElement ('img');"
        "  image.alt = screenshots[i];"
        "  image.src = folder + '/screenshots/' + encodeURIComponent (screenshots[i]);"
        "  shotsElement.appendChild (image);"
        "}"
        "}) ();</script></body></html>";
    return GS::UniString (page.c_str (), CC_UTF8);
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
    , readmeBrowser (GetReference (), ID_SCRIPT_MANAGER_README)
    , scriptsTab (GetReference (), ID_SCRIPT_MANAGER_SCRIPTS_TAB)
    , resultText (GetReference (), ID_SCRIPT_MANAGER_RESULT)
    , runButton (GetReference (), ID_SCRIPT_MANAGER_RUN)
    , stopButton (GetReference (), ID_SCRIPT_MANAGER_STOP)
    , outputButton (GetReference (), ID_SCRIPT_MANAGER_OUTPUT)
    , fixButton (GetReference (), ID_SCRIPT_MANAGER_FIX)
    , addScriptButton (GetReference (), ID_SCRIPT_MANAGER_ADD_SCRIPT)
    , categoryPopUp (GetReference (), ID_SCRIPT_MANAGER_CATEGORY_POPUP)
    , scriptsFolderButton (GetReference (), ID_SCRIPT_MANAGER_SCRIPTS_FOLDER)
{
    SetGrowType (DG::Dialog::HVGrow);
    SetMinClientSize (MinWidth, MinHeight);

    scriptsTab.Select ();
    fitsCheckBox.Check ();
    licenceLabelText = licenceLabel.GetText ();
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
    for (short category = 0; category <= CategoryCount; ++category) {
        categoryPopUp.AppendItem ();   // RefreshCategoryList gives the items their texts and counts
    }

    SetupScriptListColumns ();
    // The scripts of the Tapir Palette are listed first, and the installed ones even when no catalogue can be loaded.
    localActions = TapirPalette::Instance ().GetLocalScriptActions ();
    LoadLocalScripts ();
    LoadInstalls ();
    LayoutItems ();
    RefreshAll ();

    // Attached once the controls are set up, so that setting them up sends no change events here.
    Attach (*this);
    enabledCheckBox.Attach (*this);
    installButton.Attach (*this);
    removeButton.Attach (*this);
    updateButton.Attach (*this);
    updateAllButton.Attach (*this);
    searchEdit.Attach (*this);
    scriptsTab.Attach (*this);
    availableTab.Attach (*this);
    installedTab.Attach (*this);
    updatesTab.Attach (*this);
    refreshButton.Attach (*this);
    runButton.Attach (*this);
    stopButton.Attach (*this);
    outputButton.Attach (*this);
    fixButton.Attach (*this);
    addScriptButton.Attach (*this);
    scriptsFolderButton.Attach (*this);
    categoryPopUp.Attach (*this);
    categoryList.Attach (*this);
    fitsCheckBox.Attach (*this);
    scriptList.Attach (*this);
    shareButton.Attach (*this);
    scriptsPageButton.Attach (*this);
    closeButton.Attach (*this);

    // A script that was started from the bar, a shortcut or this window changes the result line shown here.
    ScriptRunResults::Instance ().SetChangedHandler ([this] () { ShowRunResult (); });

    // Without this DG sends no drag and drop events, and a .py file dropped on the Scripts tab's list does nothing
    EnableDragAndDropEvent ();

    BeginEventProcessing ();
}

ScriptManager::~ScriptManager ()
{
    ScriptRunResults::Instance ().SetChangedHandler (nullptr);
    searchEdit.Detach (*this);
    scriptsTab.Detach (*this);
    runButton.Detach (*this);
    stopButton.Detach (*this);
    outputButton.Detach (*this);
    fixButton.Detach (*this);
    addScriptButton.Detach (*this);
    scriptsFolderButton.Detach (*this);
    categoryPopUp.Detach (*this);
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

void ScriptManager::Release ()
{
    instance = nullptr;
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
    // Archicad gives the window the size it had when it was last closed, which sends no resize event
    LayoutItems ();
    LocalScriptsChanged ();
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
    NotifyPalette ();
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

void ScriptManager::NotifyPalette () const
{
    GS::Array<GS::UniString> updatableSlugs;
    for (const Script& script : scripts) {
        if (GetUpdate (script) != nullptr) {
            updatableSlugs.Push (script.slug);
        }
    }
    MarketplaceInstalls::SetUpdatableSlugs (updatableSlugs);
    if (TapirPalette::HasInstance ()) {
        TapirPalette::Instance ().ReloadMarketplaceScripts ();
    }
}

ScriptManager::Tab ScriptManager::GetCurrentTab () const
{
    if (scriptsTab.IsSelected ()) {
        return Tab::Scripts;
    }
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

// The Scripts tab's search: the name, the file name, what the script does and where it comes from.
static bool MatchesLocalSearch (const LocalScript& script, const GS::UniString& lowerCasedSearch)
{
    return lowerCasedSearch.IsEmpty () || ToLowerCased (script.name).Contains (lowerCasedSearch) ||
        ToLowerCased (script.fileName).Contains (lowerCasedSearch) || ToLowerCased (script.info.summary).Contains (lowerCasedSearch) ||
        ToLowerCased (script.source).Contains (lowerCasedSearch);
}

// The scripts of the current tab and search: indexes into localScripts on the Scripts tab, into scripts on the others.
std::vector<size_t> ScriptManager::GetScriptsOfTabAndSearch () const
{
    std::vector<size_t> result;
    const Tab tab = GetCurrentTab ();
    const GS::UniString search = ToLowerCased (searchEdit.GetText ());
    if (tab == Tab::Scripts) {
        for (UIndex i = 0; i < localScripts.GetSize (); ++i) {
            const LocalScript& script = localScripts[i];
            if (MatchesLocalSearch (script, search)) {
                result.push_back (i);
            }
        }
        return result;
    }
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
    int scriptsCount = 0;
    for (const LocalScript& script : localScripts) {
        if (MatchesLocalSearch (script, search)) {
            ++scriptsCount;
        }
    }
    scriptsTab.SetText (GS::UniString::Printf (ManagerText (ID_SCRIPT_MANAGER_STRINGS_SCRIPTS_TAB), scriptsCount));
    availableTab.SetText (GS::UniString::Printf (ManagerText (ID_SCRIPT_MANAGER_STRINGS_AVAILABLE_TAB), availableCount));
    installedTab.SetText (GS::UniString::Printf (ManagerText (ID_SCRIPT_MANAGER_STRINGS_INSTALLED_TAB), installedCount));
    updatesTab.SetText (GS::UniString::Printf (ManagerText (ID_SCRIPT_MANAGER_STRINGS_UPDATES_TAB), updatesCount));
    const Tab tab = GetCurrentTab ();
    fitsCheckBox.SetVisibility (tab == Tab::Available);
    // The catalogue is the marketplace's: its time and Refresh do not belong to the Scripts tab.
    catalogueTimeText.SetVisibility (tab != Tab::Scripts && !isNarrow);
    refreshButton.SetVisibility (tab != Tab::Scripts);
    scriptsFolderButton.SetVisibility (tab == Tab::Scripts);   // in Refresh's place
}

void ScriptManager::RefreshCategoryList ()
{
    const std::vector<size_t> tabScripts = GetScriptsOfTabAndSearch ();
    const int allCount = static_cast<int> (tabScripts.size ());
    categoryList.SetTabItemText (AllCategoriesItem, CategoryCountTab, GS::UniString::Printf ("%d", allCount));
    categoryPopUp.SetItemText (AllCategoriesItem, GS::UniString::Printf ("%T (%d)", ManagerText (ID_SCRIPT_MANAGER_STRINGS_ALL_CATEGORIES).ToPrintf (), allCount));
    const bool onScriptsTab = GetCurrentTab () == Tab::Scripts;
    for (short category = 0; category < CategoryCount; ++category) {
        int count = 0;
        for (size_t index : tabScripts) {
            const GS::UniString& scriptCategory = onScriptsTab ? localScripts[static_cast<UIndex> (index)].info.category : scripts[index].category;
            if (scriptCategory == CategoryKeys[category]) {
                ++count;
            }
        }
        const short item = static_cast<short> (AllCategoriesItem + 1 + category);
        categoryList.SetTabItemText (item, CategoryCountTab, GS::UniString::Printf ("%d", count));
        categoryPopUp.SetItemText (item, GS::UniString::Printf ("%T (%d)", ManagerText (ID_SCRIPT_MANAGER_STRINGS_ALL_CATEGORIES + 1 + category).ToPrintf (), count));
    }
    categoryPopUp.SelectItem (categoryList.GetSelectedItem () > 0 ? categoryList.GetSelectedItem () : AllCategoriesItem);
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
    const bool listsScripts = GetCurrentTab () == Tab::Scripts;
    scriptList.SetHeaderItemText (ScriptNameTab, ManagerText (ID_SCRIPT_MANAGER_STRINGS_NAME_COLUMN));
    scriptList.SetHeaderItemText (ScriptSummaryTab, ManagerText (updates ? ID_SCRIPT_MANAGER_STRINGS_INSTALLED_COLUMN : ID_SCRIPT_MANAGER_STRINGS_SUMMARY_COLUMN));
    scriptList.SetHeaderItemText (ScriptVersionTab, ManagerText (updates ? ID_SCRIPT_MANAGER_STRINGS_NEW_COLUMN : (listsScripts ? ID_SCRIPT_MANAGER_STRINGS_SOURCE_COLUMN : ID_SCRIPT_MANAGER_STRINGS_VERSION_COLUMN)));

    const short width = scriptList.GetItemWidth ();
    const short versionWidth = listsScripts ? 92 : 58;
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
    if (GetCurrentTab () == Tab::Scripts) {
        RefreshLocalScriptList ();
        return;
    }
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
    RefreshDetailsOfTab ();
    ApplyNarrowVisibility ();
}

void ScriptManager::ApplyNarrowVisibility ()
{
    if (!isNarrow) {
        return;
    }
    for (DG::Item* item : { static_cast<DG::Item*> (&archicadLabel), static_cast<DG::Item*> (&archicadValue), static_cast<DG::Item*> (&tapirLabel),
                            static_cast<DG::Item*> (&tapirValue), static_cast<DG::Item*> (&licenceLabel), static_cast<DG::Item*> (&licenceValue),
                            static_cast<DG::Item*> (&whatIsNewTitle), static_cast<DG::Item*> (&changelogText), static_cast<DG::Item*> (&catalogueTimeText) }) {
        item->SetVisibility (false);
    }
}

void ScriptManager::RefreshDetailsOfTab ()
{
    if (GetCurrentTab () == Tab::Scripts) {
        RefreshLocalDetails ();
        return;
    }
    // The controls of the Scripts tab, and the label it renamed, are the marketplace's again.
    for (DG::Item* item : { static_cast<DG::Item*> (&resultText), static_cast<DG::Item*> (&runButton), static_cast<DG::Item*> (&stopButton),
                            static_cast<DG::Item*> (&outputButton), static_cast<DG::Item*> (&fixButton), static_cast<DG::Item*> (&addScriptButton) }) {
        item->SetVisibility (false);
    }
    licenceLabel.SetText (licenceLabelText);
    summaryText.SetVisibility (true);
    const Script* script = GetSelectedScript ();
    DG::Item* const detailItems[] = {
        &nameText, &bylineText, &summaryText, &changesLabel, &changesValue, &archicadLabel, &archicadValue,
        &tapirLabel, &tapirValue, &licenceLabel, &licenceValue, &whatIsNewTitle, &changelogText,
        &enabledCheckBox, &installButton, &removeButton, &updateButton
    };
    for (DG::Item* item : detailItems) {
        item->SetVisibility (script != nullptr);
    }
    RefreshReadme (script);
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

void ScriptManager::RefreshReadme (const Script* script)
{
    // The README and the screenshots at the commit of the newest version: the catalogue's screenshots are the ones
    // of that commit, the newest of the script's folder (see tools/generate_catalogue.py).
    const GS::UniString folder = (script != nullptr && script->inCatalogue) ? GetVersionWebFolder (script->versions.front ()) : GS::EmptyUniString;
    readmeBrowser.SetVisibility (!folder.IsEmpty ());
    // The page is only loaded again for another script: the details are refreshed on every key typed in the search.
    if (folder == shownReadmeFolder) {
        return;
    }
    shownReadmeFolder = folder;
    readmeBrowser.LoadHTML (folder.IsEmpty () ? GS::UniString ("<!DOCTYPE html><html><body></body></html>") : BuildReadmePage (folder, script->screenshots));
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
            if (!DownloadBytes (GS::UniString (HelpersBaseUrl) + relativePath, content, error, true)) {
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

// ---- The Scripts tab ----

void ScriptManager::LoadLocalScripts ()
{
    localScripts = localActions.getScripts ();
}

void ScriptManager::LocalScriptsChanged ()
{
    if (!IsVisible ()) {
        return;   // Show loads them again
    }
    LoadLocalScripts ();
    RefreshAll ();
}

const LocalScript* ScriptManager::GetLocalScriptOfRow (short row) const
{
    if (row <= 0 || row > scriptList.GetItemCount ()) {
        return nullptr;
    }
    const DGUserData value = scriptList.GetItemValue (row);
    if (value <= 0 || static_cast<UIndex> (value) > localScripts.GetSize ()) {
        return nullptr;
    }
    return &localScripts[static_cast<UIndex> (value - 1)];
}

const LocalScript* ScriptManager::GetSelectedLocalScript () const
{
    if (GetCurrentTab () != Tab::Scripts) {
        return nullptr;
    }
    return GetLocalScriptOfRow (scriptList.GetSelectedItem ());
}

// The scripts of the Tapir Palette's list, by category and search; the row's value is the script's index + 1.
void ScriptManager::RefreshLocalScriptList ()
{
    LayoutScriptListColumns ();   // the columns of the other tabs differ
    const GS::UniString category = GetSelectedCategory ();
    scriptList.DisableDraw ();
    if (scriptList.GetItemCount () > 0) {
        scriptList.DeleteItem (DG::ListBox::AllItems);
    }
    short itemToSelect = 0;
    for (size_t index : GetScriptsOfTabAndSearch ()) {
        const LocalScript& script = localScripts[static_cast<UIndex> (index)];
        if (!category.IsEmpty () && script.info.category != category) {
            continue;
        }
        scriptList.AppendItem ();
        const short item = scriptList.GetItemCount ();
        scriptList.SetTabItemText (item, ScriptNameTab, script.name);
        scriptList.SetTabItemText (item, ScriptSummaryTab, script.info.summary);
        scriptList.SetTabItemText (item, ScriptVersionTab, script.source);
        scriptList.SetItemValue (item, static_cast<DGUserData> (index + 1));
        if (!selectedLocalLocation.IsEmpty () && script.fileLocation == selectedLocalLocation) {
            itemToSelect = item;
        }
    }
    if (itemToSelect == 0 && scriptList.GetItemCount () > 0) {
        itemToSelect = 1;
    }
    if (itemToSelect > 0) {
        scriptList.SelectItem (itemToSelect);
    }
    scriptList.EnableDraw ();
    scriptList.Redraw ();

    RefreshLocalDetails ();
}

static GS::UniString HtmlEscaped (const GS::UniString& text)
{
    const std::string bytes (text.ToCStr (CC_UTF8).Get ());
    std::string escaped;
    for (const char c : bytes) {
        switch (c) {
            case '&': escaped += "&amp;"; break;
            case '<': escaped += "&lt;"; break;
            case '>': escaped += "&gt;"; break;
            default: escaped += c; break;
        }
    }
    return GS::UniString (escaped.c_str (), CC_UTF8);
}

// What the selected script does, in the browser control that shows a marketplace script's README.
void ScriptManager::RefreshLocalDetails ()
{
    const LocalScript* script = GetSelectedLocalScript ();

    // The details of a marketplace script that the Scripts tab does not use.
    for (DG::Item* item : { static_cast<DG::Item*> (&summaryText), static_cast<DG::Item*> (&enabledCheckBox), static_cast<DG::Item*> (&installButton),
                            static_cast<DG::Item*> (&removeButton), static_cast<DG::Item*> (&updateButton), static_cast<DG::Item*> (&updateAllButton) }) {
        item->SetVisibility (false);
    }
    addScriptButton.SetVisibility (true);
    for (DG::Item* item : { static_cast<DG::Item*> (&nameText), static_cast<DG::Item*> (&bylineText), static_cast<DG::Item*> (&licenceLabel),
                            static_cast<DG::Item*> (&licenceValue), static_cast<DG::Item*> (&whatIsNewTitle), static_cast<DG::Item*> (&changelogText) }) {
        item->SetVisibility (script != nullptr);
    }
    if (script == nullptr) {
        for (DG::Item* item : { static_cast<DG::Item*> (&changesLabel), static_cast<DG::Item*> (&changesValue), static_cast<DG::Item*> (&archicadLabel),
                                static_cast<DG::Item*> (&archicadValue), static_cast<DG::Item*> (&tapirLabel), static_cast<DG::Item*> (&tapirValue) }) {
            item->SetVisibility (false);
        }
        readmeBrowser.SetVisibility (false);
        shownReadmeFolder = GS::EmptyUniString;
        ShowRunResult ();
        return;
    }
    selectedLocalLocation = script->fileLocation;

    nameText.SetText (script->name);
    GS::UniString byline;
    if (!script->info.author.IsEmpty ()) {
        byline = GS::UniString::Printf (LocalScriptText (ID_LOCAL_SCRIPT_STRINGS_BY_AUTHOR), script->info.author.ToPrintf ());
    }
    if (!script->info.version.IsEmpty ()) {
        if (!byline.IsEmpty ()) {
            byline += ", ";
        }
        byline += GS::UniString::Printf (LocalScriptText (ID_LOCAL_SCRIPT_STRINGS_VERSION), script->info.version.ToPrintf ());
    }
    bylineText.SetText (byline);

    // What it changes, and the Archicad and Tapir versions it names; a row without a value is left out.
    const GS::UniString badge = GetChangesBadge (script->info.changes);
    changesLabel.SetVisibility (!badge.IsEmpty ());
    changesValue.SetVisibility (!badge.IsEmpty ());
    changesValue.SetText (badge);
    const bool hasArchicad = !script->info.archicadVersions.IsEmpty ();
    archicadLabel.SetVisibility (hasArchicad);
    archicadValue.SetVisibility (hasArchicad);
    archicadValue.SetText (JoinArchicadVersions (script->info.archicadVersions));
    const bool hasTapir = !script->info.minTapirVersion.IsEmpty ();
    tapirLabel.SetVisibility (hasTapir);
    tapirValue.SetVisibility (hasTapir);
    tapirValue.SetText (hasTapir ? GS::UniString::Printf (ManagerText (ID_SCRIPT_MANAGER_STRINGS_OR_NEWER), script->info.minTapirVersion.ToPrintf ()) : GS::EmptyUniString);

    // Where it comes from, and its file.
    licenceLabel.SetText (ManagerText (ID_SCRIPT_MANAGER_STRINGS_SOURCE_LABEL));
    licenceValue.SetText (script->source);
    whatIsNewTitle.SetText (ManagerText (ID_SCRIPT_MANAGER_STRINGS_FILE_LABEL));
    changelogText.SetText (script->fileName);

    // The description, whole.
    GS::UniString filePath;
    script->fileLocation.ToPath (&filePath);
    const GS::UniString pageKey = "local:" + filePath;
    readmeBrowser.SetVisibility (!script->info.summary.IsEmpty ());
    if (pageKey != shownReadmeFolder) {
        shownReadmeFolder = pageKey;
        readmeBrowser.LoadHTML (GS::UniString (
            "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><style>"
            "html,body{margin:0;padding:0;background:#fff;color:#263238;}"
            "body{font:12px -apple-system,'Segoe UI',Helvetica,Arial,sans-serif;padding:6px;}"
            "p{margin:0;white-space:pre-wrap;word-wrap:break-word;}"
            "</style></head><body><p>") + HtmlEscaped (script->info.summary) + GS::UniString ("</p></body></html>"));
    }
    ShowRunResult ();
}

// The Run and Stop buttons and the result of the selected script's last run, wherever it was started.
void ScriptManager::ShowRunResult ()
{
    const bool onScriptsTab = GetCurrentTab () == Tab::Scripts;
    const LocalScript* script = GetSelectedLocalScript ();
    const bool hasScript = onScriptsTab && script != nullptr;
    const bool isRunning = hasScript && localActions.isRunning (script->fileLocation);

    runButton.SetVisibility (onScriptsTab);
    stopButton.SetVisibility (onScriptsTab);
    resultText.SetVisibility (onScriptsTab);
    runButton.SetStatus (hasScript && !isRunning);
    stopButton.SetStatus (isRunning);

    ScriptRunResults::Display display;
    if (hasScript) {
        display = ScriptRunResults::Instance ().Describe (script->fileLocation, isRunning);
    }
    resultText.SetText (display.text);
    if (display.fixTextIndex != 0) {
        fixButton.SetText (LocalScriptText (display.fixTextIndex));
    }
    outputButton.SetVisibility (onScriptsTab && display.showOutputButton);
    fixButton.SetVisibility (onScriptsTab && display.fixTextIndex != 0);
}

// Run, Stop, Details, the fix button, Add script and Scripts folder. True when the button was one of them.
bool ScriptManager::HandleLocalButton (const DG::ButtonClickEvent& ev)
{
    if (ev.GetSource () == &addScriptButton) {
        localActions.addScriptWithDialog ();   // refills the palette's list, and with it this one
        return true;
    }
    if (ev.GetSource () == &scriptsFolderButton) {
        localActions.openScriptsFolder ();
        return true;
    }
    const bool isRunButton = ev.GetSource () == &runButton;
    const bool isStopButton = ev.GetSource () == &stopButton;
    const bool isOutputButton = ev.GetSource () == &outputButton;
    const bool isFixButton = ev.GetSource () == &fixButton;
    if (!isRunButton && !isStopButton && !isOutputButton && !isFixButton) {
        return false;
    }

    const LocalScript* script = GetSelectedLocalScript ();
    if (script == nullptr) {
        return true;
    }
    // A copy: running or stopping the script changes the results.
    // run and stop do nothing when the script already runs or does not run.
    const IO::Location fileLocation = script->fileLocation;

    if (isRunButton) {
        localActions.run (fileLocation);
    } else if (isStopButton) {
        localActions.stop (fileLocation);
    } else {
        const ScriptRunResults::RunResult* result = ScriptRunResults::Instance ().Find (fileLocation);
        if (result == nullptr) {
            return true;
        }
        if (isOutputButton) {
            ScriptRunResults::Instance ().ShowOutputDetails (*result);
        } else {
            switch (result->failure) {
                case ScriptRunResults::Failure::UvMissing:
                    localActions.openWebpage ("https://docs.astral.sh/uv/getting-started/installation/");
                    break;
                case ScriptRunResults::Failure::ConnectionBusy:
                    localActions.run (fileLocation);
                    break;
                case ScriptRunResults::Failure::NotStarted:
                case ScriptRunResults::Failure::ScriptError:
                    ScriptRunResults::Instance ().ReportProblem (*result, localActions.openWebpage);
                    break;
                case ScriptRunResults::Failure::None:
                    break;
            }
        }
    }
    ShowRunResult ();
    return true;
}

void ScriptManager::ListBoxContextMenuRequested (const DG::ListBoxContextMenuEvent& ev, bool* processed)
{
    if (ev.GetSource () != &scriptList || GetCurrentTab () != Tab::Scripts) {
        return;
    }
    // The script under the mouse becomes the selected one, so the details show what the menu acts on.
    const short row = ev.GetItem ();
    const LocalScript* script = GetLocalScriptOfRow (row);
    if (script == nullptr) {
        return;
    }
    selectedLocalLocation = script->fileLocation;
    scriptList.SelectItem (row);
    RefreshDetails ();

    // A copy: a command may refill the list (Remove) and with it the scripts.
    const LocalScript clickedScript = *script;
    ShowLocalContextMenu (clickedScript);
    if (processed != nullptr) {
        *processed = true;
    }
}

// Run (Stop while it runs), Assign Shortcut, Edit and Show in Folder for a local file or Open on GitHub for a
// repository's script, and Remove for a script the user added.
void ScriptManager::ShowLocalContextMenu (const LocalScript& script)
{
    const bool isRunning = localActions.isRunning (script.fileLocation);
    const bool isLocalFile = script.webUrl.IsEmpty ();

    DG::Menu menu ("-");
    DG::CommandTable commandTable;
    const auto addCommandWithText = [&menu, &commandTable] (ULong commandId, const GS::UniString& text) {
        const DG::Command command (commandId, TapirDeveloperId, TapirLocalId);
        commandTable.Add (command, new DG::CommandDescriptor (command, text));
        menu.AddMenuItem (DG::MenuSimpleItem (command));
    };
    const auto addCommand = [&addCommandWithText] (ULong commandId, Int32 textIndex) {
        addCommandWithText (commandId, LocalScriptText (textIndex));
    };
    if (isRunning) {
        addCommand (StopCommand, ID_LOCAL_SCRIPT_STRINGS_MENU_STOP);
    } else {
        addCommand (RunCommand, ID_LOCAL_SCRIPT_STRINGS_MENU_RUN);
    }
    // The key the script's shortcut slot has in Work Environment (Archicad 29 and later), e.g. "Assign Shortcut... (Ctrl+Alt+H)".
    const GS::UniString shortcutKeys = localActions.shortcutKeys ? localActions.shortcutKeys (script.fileLocation) : GS::UniString ();
    if (shortcutKeys.IsEmpty ()) {
        addCommand (AssignShortcutCommand, ID_LOCAL_SCRIPT_STRINGS_MENU_ASSIGN_SHORTCUT);
    } else {
        addCommandWithText (AssignShortcutCommand, GS::UniString::Printf (LocalScriptText (ID_LOCAL_SCRIPT_STRINGS_MENU_ASSIGN_SHORTCUT_KEY), shortcutKeys.ToPrintf ()));
    }
    menu.AddMenuItem (DG::MenuSeparatorItem ());
    if (isLocalFile) {
        addCommand (EditCommand, ID_LOCAL_SCRIPT_STRINGS_MENU_EDIT);
        addCommand (ShowInFolderCommand, ID_LOCAL_SCRIPT_STRINGS_MENU_SHOW_IN_FOLDER);
    } else {
        addCommand (OpenOnGitHubCommand, ID_LOCAL_SCRIPT_STRINGS_MENU_OPEN_ON_GITHUB);
    }
    if (script.isAdded) {
        menu.AddMenuItem (DG::MenuSeparatorItem ());
        addCommand (RemoveCommand, ID_LOCAL_SCRIPT_STRINGS_MENU_REMOVE);
    }

    ULong chosenCommand = 0;
    {
        DG::ContextMenu contextMenu ("-", &menu);
        contextMenu.SetEnabledCommands (commandTable);
        DG::MousePosData mousePos;
        mousePos.Retrieve ();
        DG::CommandEvent* commandEvent = contextMenu.Display (mousePos.GetMouseOffsetInNativeUnits ());
        if (commandEvent != nullptr) {
            chosenCommand = commandEvent->GetCommand ().GetCommandId ();
            delete commandEvent;
        }
    }
    for (auto it = commandTable.EnumerateValues (); it != nullptr; ++it) {
        delete *it;
    }

    switch (chosenCommand) {
        case RunCommand:
            localActions.run (script.fileLocation);
            break;
        case StopCommand:
            localActions.stop (script.fileLocation);
            break;
        case AssignShortcutCommand:
            localActions.openShortcuts ();
            break;
        case EditCommand:
            localActions.editFile (script.fileLocation);
            break;
        case ShowInFolderCommand:
            localActions.showInFolder (script.fileLocation);
            break;
        case OpenOnGitHubCommand:
            localActions.openWebpage (script.webUrl);
            break;
        case RemoveCommand:
            localActions.remove (script.fileLocation);   // refills the palette's list, and with it this one
            break;
        default:
            break;
    }
    ShowRunResult ();
}

GS::Array<IO::Location> ScriptManager::GetDroppedScripts (const DG::ListBoxDropTargetEvent& ev)
{
    GS::Array<IO::Location> files;
    const ULong count = ev.GetLocationCount ();
    for (ULong i = 0; i < count; ++i) {
        IO::Location location;
        IO::Name name;
        if (!ev.GetLocation (i, location) || location.GetLastLocalName (&name) != NoError) {
            continue;
        }
        if (ToLowerCased (name.ToString ()).EndsWith (GS::UniString (".py"))) {
            files.Push (location);
        }
    }
    return files;
}

// Copy when .py files are dragged over the Scripts tab's list, else Reject.
DG::DragDrop::Effect ScriptManager::GetDropEffect (const DG::ListBoxDropTargetEvent& ev) const
{
    return (GetCurrentTab () == Tab::Scripts && !GetDroppedScripts (ev).IsEmpty ()) ? DG::DragDrop::Effect::Copy : DG::DragDrop::Effect::Reject;
}

void ScriptManager::ListBoxDragEntered (const DG::ListBoxDropTargetEvent& ev, DG::DragDrop::Effect* effect, bool*)
{
    if (ev.GetSource () == &scriptList && effect != nullptr) {
        *effect = GetDropEffect (ev);
    }
}

void ScriptManager::ListBoxDragEntered (const DG::ListBoxDropTargetEvent& ev, DG::DragDrop::Effect* effect, bool* defBehaviour, bool*)
{
    ListBoxDragEntered (ev, effect, defBehaviour);
}

void ScriptManager::ListBoxDragMoved (const DG::ListBoxDropTargetEvent& ev, DG::DragDrop::Effect* effect, DG::DragDrop::Feedback* denyFeedback)
{
    if (ev.GetSource () != &scriptList) {
        return;
    }
    // A file is dropped on the list, not between its rows: no insertion mark.
    if (denyFeedback != nullptr) {
        *denyFeedback = DG::DragDrop::RejectSelAndIns;
    }
    if (effect != nullptr) {
        *effect = GetDropEffect (ev);
    }
}

void ScriptManager::ListBoxDropped (const DG::ListBoxDropTargetEvent& ev, DG::DragDrop::Effect* effect)
{
    if (ev.GetSource () != &scriptList) {
        return;
    }
    const GS::Array<IO::Location> files = GetDroppedScripts (ev);
    const bool accepted = GetCurrentTab () == Tab::Scripts && !files.IsEmpty ();
    if (effect != nullptr) {
        *effect = accepted ? DG::DragDrop::Effect::Copy : DG::DragDrop::Effect::Reject;
    }
    if (accepted) {
        localActions.addScripts (files);   // refills the palette's list, and with it this one
    }
}

void ScriptManager::PanelCloseRequested (const DG::PanelCloseRequestEvent&, bool* accepted)
{
    Hide ();
    *accepted = true;
}

void ScriptManager::PanelResized (const DG::PanelResizeEvent&)
{
    LayoutItems ();
}

// Wide: the categories on the left, the list in the middle and the details on the right; the list and the README follow
// the window's size, the buttons stay at its right and bottom edges. Narrow: one column, so that the window can be
// docked in a side panel: the search and the tabs on top, the categories as a dropdown, the list, then the details and
// the buttons under it.
void ScriptManager::LayoutItems ()
{
    const short width = GetClientWidth ();
    const short height = GetClientHeight ();
    const bool wasNarrow = isNarrow;
    isNarrow = width < NarrowWidth;

    const auto place = [] (DG::Item& item, short x, short y, short w, short h) {
        item.SetPosition (x, y);
        item.SetSize (w, h);
    };

    // Not between BeginMoveResizeItems and EndMoveResizeItems: moves there are applied twice
    if (!isNarrow) {
        place (searchEdit, 10, 10, 220, 22);
        place (scriptsTab, 240, 12, 108, 18);
        place (availableTab, 350, 12, 108, 18);
        place (installedTab, 460, 12, 108, 18);
        place (updatesTab, 570, 12, 108, 18);
        place (catalogueTimeText, static_cast<short> (width - 256), 12, 142, 18);
        place (refreshButton, static_cast<short> (width - 110), 9, 100, 24);
        {
            // The catalogue's time is hidden on its tab. Right of the Updates tab (ends at 678), narrower when needed.
            const short folderX = static_cast<short> (width - 140 > 688 ? width - 140 : 688);
            const short folderWidth = static_cast<short> (width - 10 - folderX < 130 ? width - 10 - folderX : 130);
            place (scriptsFolderButton, folderX, 9, folderWidth, 24);
        }
        place (categoryList, 10, 44, 200, static_cast<short> (height - 94));
        place (fitsCheckBox, 220, 44, 410, 18);
        place (scriptList, 220, 66, static_cast<short> (width - 530), static_cast<short> (height - 116));
        const short x = static_cast<short> (width - 300);
        const short valueX = static_cast<short> (x + 105);
        place (nameText, x, 44, 290, 20);
        place (bylineText, x, 66, 290, 32);
        place (summaryText, x, 100, 290, 48);
        place (changesLabel, x, 152, 100, 18);
        place (changesValue, valueX, 152, 185, 18);
        place (archicadLabel, x, 172, 100, 18);
        place (archicadValue, valueX, 172, 185, 18);
        place (tapirLabel, x, 192, 100, 18);
        place (tapirValue, valueX, 192, 185, 18);
        place (licenceLabel, x, 212, 100, 18);
        place (licenceValue, valueX, 212, 185, 18);
        place (whatIsNewTitle, x, 238, 290, 18);
        place (changelogText, x, 258, 290, 48);
        place (readmeBrowser, x, 312, 290, static_cast<short> (height - 414));
        place (shareButton, 10, static_cast<short> (height - 40), 140, 24);
        place (scriptsPageButton, 160, static_cast<short> (height - 40), 200, 24);
        place (closeButton, static_cast<short> (width - 110), static_cast<short> (height - 40), 100, 24);
        place (enabledCheckBox, x, static_cast<short> (height - 96), 290, 18);
        place (resultText, x, static_cast<short> (height - 96), 290, 18);
        place (installButton, static_cast<short> (width - 110), static_cast<short> (height - 74), 100, 24);
        place (removeButton, static_cast<short> (width - 110), static_cast<short> (height - 74), 100, 24);
        place (updateButton, static_cast<short> (width - 220), static_cast<short> (height - 74), 100, 24);
        place (updateAllButton, static_cast<short> (width - 430), static_cast<short> (height - 40), 120, 24);
        place (runButton, x, static_cast<short> (height - 74), 64, 24);
        place (stopButton, static_cast<short> (x + 68), static_cast<short> (height - 74), 64, 24);
        place (outputButton, static_cast<short> (x + 136), static_cast<short> (height - 74), 70, 24);
        place (fixButton, x, static_cast<short> (height - 40), 170, 24);
        place (addScriptButton, static_cast<short> (width - 430), static_cast<short> (height - 40), 120, 24);
    } else {
        constexpr short margin = 8;
        const short contentWidth = static_cast<short> (width - 2 * margin);
        place (searchEdit, margin, margin, contentWidth, 22);
        const short tabWidth = static_cast<short> (contentWidth / 4);
        place (scriptsTab, margin, 36, tabWidth, 18);
        place (availableTab, static_cast<short> (margin + tabWidth), 36, tabWidth, 18);
        place (installedTab, static_cast<short> (margin + 2 * tabWidth), 36, tabWidth, 18);
        place (updatesTab, static_cast<short> (margin + 3 * tabWidth), 36, tabWidth, 18);
        // Refresh and, on the Scripts tab, Scripts folder in its place, beside the categories
        place (categoryPopUp, margin, 62, static_cast<short> (contentWidth - 118), 22);
        place (refreshButton, static_cast<short> (width - margin - 110), 61, 110, 24);
        place (scriptsFolderButton, static_cast<short> (width - margin - 110), 61, 110, 24);

        // The bottom rows: what the selected script's tab offers, the buttons, and Share, Update all and Close.
        const short rowC = static_cast<short> (height - 34);
        const short rowB = static_cast<short> (height - 62);
        const short rowA = static_cast<short> (height - 86);
        const short bottomTop = static_cast<short> (rowA - 4);
        constexpr short listTop = 90;
        constexpr short detailsHeader = 94;   // name, author line, summary and what it changes
        short listHeight = static_cast<short> ((bottomTop - listTop - detailsHeader - 6) * 55 / 100);
        if (listHeight < 70) {
            listHeight = 70;
        }
        place (scriptList, margin, listTop, contentWidth, listHeight);
        const short detailsTop = static_cast<short> (listTop + listHeight + 6);
        place (nameText, margin, detailsTop, contentWidth, 20);
        place (bylineText, margin, static_cast<short> (detailsTop + 20), contentWidth, 16);
        place (summaryText, margin, static_cast<short> (detailsTop + 38), contentWidth, 32);
        place (changesLabel, margin, static_cast<short> (detailsTop + 72), 100, 18);
        place (changesValue, static_cast<short> (margin + 105), static_cast<short> (detailsTop + 72), static_cast<short> (contentWidth - 105), 18);
        const short browserTop = static_cast<short> (detailsTop + detailsHeader);
        short browserHeight = static_cast<short> (bottomTop - browserTop);
        if (browserHeight < 40) {
            browserHeight = 40;
        }
        place (readmeBrowser, margin, browserTop, contentWidth, browserHeight);

        place (resultText, margin, rowA, static_cast<short> (contentWidth - 138), 20);
        place (fixButton, static_cast<short> (width - margin - 130), static_cast<short> (rowA - 2), 130, 24);
        place (enabledCheckBox, margin, rowA, contentWidth, 18);
        place (fitsCheckBox, margin, rowA, contentWidth, 18);
        place (runButton, margin, rowB, 64, 24);
        place (stopButton, static_cast<short> (margin + 68), rowB, 64, 24);
        place (outputButton, static_cast<short> (margin + 136), rowB, 70, 24);
        place (addScriptButton, static_cast<short> (width - margin - 120), rowB, 120, 24);
        place (installButton, static_cast<short> (width - margin - 100), rowB, 100, 24);
        place (removeButton, static_cast<short> (width - margin - 100), rowB, 100, 24);
        place (updateButton, static_cast<short> (width - margin - 208), rowB, 100, 24);
        place (shareButton, margin, rowC, 150, 24);
        place (scriptsPageButton, margin, rowC, 150, 24);
        place (updateAllButton, static_cast<short> (width - margin - 80 - 4 - 110), rowC, 110, 24);
        place (closeButton, static_cast<short> (width - margin - 80), rowC, 80, 24);
    }
    categoryList.SetVisibility (!isNarrow);
    categoryPopUp.SetVisibility (isNarrow);
    if (wasNarrow != isNarrow) {
        // The rows and the header that only one of the layouts has.
        RefreshTabTexts ();
        RefreshDetails ();
    }
    ApplyNarrowVisibility ();
    LayoutScriptListColumns ();

    // DG does not repaint the items that were moved or resized by itself: without this part of the window keeps what
    // the other layout drew.
    DG::Item* const allItems[] = {
        &searchEdit, &scriptsTab, &availableTab, &installedTab, &updatesTab, &catalogueTimeText, &refreshButton, &categoryList,
        &categoryPopUp, &fitsCheckBox, &scriptList, &nameText, &bylineText, &summaryText, &changesLabel, &changesValue,
        &archicadLabel, &archicadValue, &tapirLabel, &tapirValue, &licenceLabel, &licenceValue, &whatIsNewTitle, &changelogText,
        &shareButton, &scriptsPageButton, &closeButton, &enabledCheckBox, &installButton, &removeButton, &updateButton,
        &updateAllButton, &readmeBrowser, &resultText, &runButton, &stopButton, &outputButton, &fixButton, &addScriptButton,
        &scriptsFolderButton
    };
    for (DG::Item* item : allItems) {
        item->Invalidate ();
    }
}

void ScriptManager::ButtonClicked (const DG::ButtonClickEvent& ev)
{
    if (HandleLocalButton (ev)) {
        return;
    }
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
            NotifyPalette ();
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
            NotifyPalette ();
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
    NotifyPalette ();
}

void ScriptManager::RadioItemChanged (const DG::RadioItemChangeEvent&)
{
    RefreshAll ();
}

void ScriptManager::PopUpChanged (const DG::PopUpChangeEvent& ev)
{
    if (ev.GetSource () != &categoryPopUp) {
        return;
    }
    categoryList.SelectItem (categoryPopUp.GetSelectedItem ());
    RefreshScriptList ();
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
