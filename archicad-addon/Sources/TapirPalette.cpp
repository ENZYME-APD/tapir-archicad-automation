#include "TapirPalette.hpp"
#include "UvManager.hpp" 
#include "ResourceIds.hpp"
#include "VersionChecker.hpp"
#include "HTTP/Client/ClientConnection.hpp"
#include "HTTP/Client/Request.hpp"
#include "HTTP/Client/Response.hpp"
#include "JSON/Value.hpp"
#include "JSON/JDOMParser.hpp"
#include "IBinaryChannelUtilities.hpp"
#include "StringConversion.hpp"
#include "FileSystem.hpp"
#include "Folder.hpp"
#include "OnExit.hpp"
#include "MessageLoopExecutor.hpp"
#include "AddOnVersion.hpp"
#include "MigrationHelper.hpp"

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <map>
#include <regex>
#include <vector>

#if defined (WINDOWS)
#pragma warning (push)
#pragma warning (disable : 4995 4091)   // as in the DevKit's Win32ShellInterface.hpp
#include <shellapi.h>
#include <tlhelp32.h>
#pragma warning (pop)
#pragma comment (lib, "shell32.lib")
#endif

#if defined (macintosh)
#include <sys/sysctl.h>
#include <libproc.h>
#include <signal.h>
#include <unistd.h>
#endif

const GS::Guid        TapirPalette::paletteGuid("{2D42DF37-222F-40CD-BA86-B3279CCA1FEE}");
GS::Ref<TapirPalette> TapirPalette::instance;

// True while UpdateAddOn runs. Archicad handles its events during the installer download, so the palette's
// Run button and the shortcut slots must not start a script that the installer would then interrupt.
static bool isUpdatingAddOn = false;

static UShort GetConnectionPort ()
{
    UShort portNumber;
    ACAPI_Command_GetHttpConnectionPort (&portNumber);
    return portNumber;
}

static IO::Location GetTapirTemporaryFolder ()
{
    IO::Location tempFolder;
    IO::fileSystem.GetSpecialLocation (IO::FileSystem::TemporaryFolder, &tempFolder);
    tempFolder.AppendToLocal (IO::Name ("Tapir"));
    return tempFolder;
}

// The downloaded content is kept as raw bytes and written to disk unchanged.
// The repositories may contain binary files (images, pdf, zip, ...) next to the
// scripts, and those bytes are not valid UTF-8: converting them through
// GS::UniString corrupted the files and triggers a GSRoot assert in Archicad 30.
static IO::Location SaveBuiltInScript (const IO::Location& baseFolderLoc, const IO::RelativeLocation& relLoc, const std::vector<char>& content)
{
    IO::Location fileLoc = baseFolderLoc;
    fileLoc.AppendToLocal (relLoc);

    IO::Location folderToCreate = fileLoc;
    folderToCreate.DeleteLastLocalName ();

    IO::fileSystem.CreateFolderTree (folderToCreate);

    IO::File file (fileLoc, IO::File::OnNotFound::Create);
    if (file.Open (IO::File::WriteEmptyMode) != NoError) {
        return {};
    }

    if (!content.empty ()) {
        file.WriteBin (content.data (), (GS::USize) content.size ());
    }

    return fileLoc;
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

static std::vector<char> DownloadFileContent (const GS::UniString& fileDownloadUrl, const std::map<GS::UniString, GS::UniString>& headers = {})
{
    IO::URI::URI connectionUrl (fileDownloadUrl);
    HTTP::Client::ClientConnection clientConnection (connectionUrl);
    clientConnection.Connect ();

    HTTP::Client::Request getRequest (HTTP::MessageHeader::Method::Get, "");

    getRequest.GetRequestHeaderFieldCollection ().Add (HTTP::MessageHeader::HeaderFieldName::UserAgent,
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.99 Safari/537.36");
    for (const auto& kv : headers) {
        getRequest.GetRequestHeaderFieldCollection ().Add(kv.first, kv.second);
    }
    clientConnection.Send (getRequest);

    HTTP::Client::Response response;
    std::vector<char> body = ReadAllBytes (clientConnection.BeginReceive (response));

    clientConnection.FinishReceive ();
    clientConnection.Close (false);

    return body;
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

// Resolves the Location header of a redirect that origin + pathAndQuery answered with.
static GS::UniString ResolveRedirectLocation (const GS::UniString& location, const GS::UniString& origin, const GS::UniString& pathAndQuery)
{
    if (location.BeginsWith (GS::UniString ("https://")) || location.BeginsWith (GS::UniString ("http://"))) {
        return location;
    }
    if (location.BeginsWith (GS::UniString ("//"))) {
        return GS::UniString (origin.GetSubstring (0, origin.FindFirst (GS::UniChar (':')) + 1)) + location;
    }
    if (location.BeginsWith (GS::UniChar ('/'))) {
        return origin + location;
    }

    // Relative to the folder of the requested path
    UIndex pathEnd = pathAndQuery.FindFirst (GS::UniChar ('?'));
    if (pathEnd == MaxUIndex) {
        pathEnd = pathAndQuery.GetLength ();
    }
    const GS::UniString path (pathAndQuery.GetSubstring (0, pathEnd));
    return origin + GS::UniString (path.GetSubstring (0, path.FindLast (GS::UniChar ('/')) + 1)) + location;
}

static GS::UniString GetExceptionText (const GS::Exception& exception)
{
    return exception.GetMessage ().IsEmpty () ? GS::UniString (exception.GetName ()) : exception.GetMessage ();
}

// The body may end with an exception instead of a 0 byte read (see ReadAllBytes). Such an exception counts
// as the end of the body only when the response has a Content-Length: the length check in DownloadBinaryFile
// then tells a complete body from one that stopped early. Without a Content-Length nothing can tell them apart,
// so the exception is passed on and a dropped connection is reported instead of accepted as a complete file.
// The text of a swallowed exception is kept in readError, so an incomplete download can tell why it stopped.
static USize ReadBodyChunk (GS::IBinaryChannel& channel, char* buffer, USize bufferSize, bool hasContentLength, GS::UniString& readError)
{
    try {
        return channel.Read (buffer, bufferSize);
    } catch (const GS::Exception& e) {
        if (!hasContentLength) {
            throw;
        }
        readError = GetExceptionText (e);
        return 0;
    }
}

enum class DownloadResult {
    Succeeded,
    Failed,
    Cancelled
};

// Receives the downloaded size and the total size (0 when unknown); returns false to cancel the download.
using DownloadProgressCallback = std::function<bool (GS::UInt64 downloadedSize, GS::UInt64 totalSize)>;

// Downloads a binary file (e.g. a release asset). Redirects are followed here instead of by the client,
// so the signed query string of the redirect target is sent exactly as received on every Archicad version.
static DownloadResult DownloadBinaryFile (const GS::UniString& fileDownloadUrl, const IO::Location& fileLoc, const DownloadProgressCallback& progress, GS::UniString& error)
{
    constexpr int maxRedirectCount = 10;
    GS::UniString url = fileDownloadUrl;
    try {
        for (int redirectCount = 0; redirectCount <= maxRedirectCount; ++redirectCount) {
            GS::UniString origin;
            GS::UniString pathAndQuery;
            if (!SplitUrl (url, origin, pathAndQuery)) {
                error = "Invalid download URL: " + url;
                return DownloadResult::Failed;
            }
            // Lets the user cancel before every connection, the redirect hops included
            if (!progress (0, 0)) {
                return DownloadResult::Cancelled;
            }

            const IO::URI::URI connectionUrl (origin);
            HTTP::Client::ClientConnection clientConnection (connectionUrl);
            clientConnection.SetFollowRedirect (false);
            clientConnection.SetTimeout (60000); // 60 seconds
            clientConnection.Connect ();

            HTTP::Client::Request getRequest (HTTP::MessageHeader::Method::Get, pathAndQuery);
            getRequest.GetRequestHeaderFieldCollection ().Add (HTTP::MessageHeader::HeaderFieldName::UserAgent, "Tapir-Archicad-AddOn/" ADDON_VERSION);
            clientConnection.Send (getRequest);

            HTTP::Client::Response response;
            HTTP::Encoding::BodyIBinaryChannel& body = clientConnection.BeginReceive (response);
            // Closing the connection, also in its destructor, first reads the rest of the body: after Cancel the rest of
            // the installer, after a dropped connection until the timeout. So it is aborted unless it was closed normally.
            GS::OnExit abortConnection ([&clientConnection] () {
                try {
                    clientConnection.Abort ();
                } catch (...) {
                }
            });
            const int statusCode = static_cast<int> (response.GetStatusCode ());
            if (statusCode == 301 || statusCode == 302 || statusCode == 303 || statusCode == 307 || statusCode == 308) {
                const GS::UniString location = response.GetResponseHeaderFieldCollection ().GetHeaderValue (HTTP::MessageHeader::HeaderFieldName::Location);
                clientConnection.FinishReceive ();
                clientConnection.Close (false);
                abortConnection.Deactivate ();
                if (location.IsEmpty ()) {
                    error = "Redirect without a location from " + origin;
                    return DownloadResult::Failed;
                }
                url = ResolveRedirectLocation (location, origin, pathAndQuery);
                continue;
            }
            if (statusCode != 200) {
                error = GS::UniString::Printf ("HTTP status %d from %T", statusCode, origin.ToPrintf ());
                return DownloadResult::Failed;
            }

            GS::UInt64 contentLength = 0;
            const bool hasContentLength = response.GetResponseHeaderFieldCollection ().GetContentLength (contentLength);
            const GS::UInt64 totalSize = hasContentLength ? contentLength : 0;

            GS::UInt64 writtenSize = 0;
            GS::UniString readError;
            {
                IO::File file (fileLoc, IO::File::OnNotFound::Create);
                if (file.GetStatus () != NoError || file.Open (IO::File::WriteEmptyMode) != NoError) {
                    GS::UniString filePath;
                    fileLoc.ToPath (&filePath);
                    error = "Cannot write " + filePath;
                    return DownloadResult::Failed;
                }

                std::vector<char> buffer (64 * 1024);
                USize readSize = ReadBodyChunk (body, buffer.data (), static_cast<USize> (buffer.size ()), hasContentLength, readError);
                while (readSize > 0) {
                    if (file.WriteBin (buffer.data (), readSize) != NoError) {
                        error = "Failed to write the downloaded file.";
                        return DownloadResult::Failed;
                    }
                    writtenSize += readSize;
                    if (!progress (writtenSize, totalSize)) {
                        return DownloadResult::Cancelled;
                    }
                    readSize = ReadBodyChunk (body, buffer.data (), static_cast<USize> (buffer.size ()), hasContentLength, readError);
                }
                if (file.Close () != NoError) {
                    error = "Failed to write the downloaded file.";
                    return DownloadResult::Failed;
                }
            }

            if (hasContentLength && contentLength != writtenSize) {
                error = "The download is incomplete.";
                if (!readError.IsEmpty ()) {
                    error += " " + readError;
                }
                return DownloadResult::Failed;
            }
            if (writtenSize == 0) {
                error = "The downloaded file is empty.";
                return DownloadResult::Failed;
            }

            // The file is complete, so an error while closing the connection (e.g. after the body ended with an exception) does not matter.
            try {
                clientConnection.FinishReceive ();
                clientConnection.Close (false);
                abortConnection.Deactivate ();
            } catch (const GS::Exception&) {
            }
            return DownloadResult::Succeeded;
        }
        error = "Too many redirects.";
    } catch (const GS::Exception& e) {
        error = GetExceptionText (e);
    } catch (...) {
        error = "Unexpected error while downloading.";
    }

    return DownloadResult::Failed;
}

static std::map<GS::UniString, GS::UniString> GetFilesFromGitHubInRelativeLocation (
    const Config::Repository& repository,
    const GS::UniString& relativeLoc = GS::EmptyUniString)
{
    std::map<GS::UniString, GS::UniString> files;

    try {
        IO::URI::URI connectionUrl ("https://api.github.com");
        HTTP::Client::ClientConnection clientConnection (connectionUrl);
        clientConnection.Connect ();

        const GS::UniString& relativeLocation = relativeLoc.IsEmpty () ? repository.relativeLoc : relativeLoc;
        HTTP::Client::Request request (HTTP::MessageHeader::Method::Get, "/repos/" + repository.repoOwner + "/" + repository.repoName + "/contents/" + relativeLocation);
        request.GetRequestHeaderFieldCollection ().Add (HTTP::MessageHeader::HeaderFieldName::UserAgent,
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/135.0.0.0 Safari/537.36");
        request.GetRequestHeaderFieldCollection ().Add ("Accept", "application/vnd.github+json");
        if (!repository.token.IsEmpty ()) {
            request.GetRequestHeaderFieldCollection ().Add ("Authorization", "Bearer " + repository.token);
        }
        clientConnection.Send (request);

        HTTP::Client::Response response;
        JSON::JDOMParser parser;
        JSON::ValueRef parsed = parser.Parse (clientConnection.BeginReceive (response));

        if (response.GetStatusCode () == HTTP::MessageHeader::StatusCode::OK) {
            JSON::ArrayValueRef arrayValue = GS::DynamicCast<JSON::ArrayValue> (parsed);
            arrayValue->Enumerate ([&] (const JSON::ValueRef& assetValue) {
                JSON::ObjectValueRef objectValue = GS::DynamicCast<JSON::ObjectValue> (assetValue);
                JSON::StringValueRef typeValue = GS::DynamicCast<JSON::StringValue> (objectValue->Get ("type"));
                JSON::StringValueRef pathValue = GS::DynamicCast<JSON::StringValue> (objectValue->Get ("path"));

                if (typeValue->Get () == "dir") {
                    auto subFiles = GetFilesFromGitHubInRelativeLocation (repository, pathValue->Get ());
                    files.insert (subFiles.begin (), subFiles.end ());
                } else {
                    JSON::StringValueRef downloadUrlValue = GS::DynamicCast<JSON::StringValue> (objectValue->Get ("download_url"));
                    files[pathValue->Get ()] = downloadUrlValue->Get ();
                }
            });
        }

        clientConnection.Close (false);
    } catch (...) {
    }

    return files;
}

TapirPalette::TapirPalette ()
    : DG::Palette (ACAPI_GetOwnResModule (), ID_PALETTE, ACAPI_GetOwnResModule (), paletteGuid)
    , tapirButton (GetReference (), 1)
    , scriptSelectionPopUp (GetReference (), 2)
    , runScriptButton (GetReference (), 3)
    , openScriptButton (GetReference (), 4)
    , addScriptButton (GetReference (), 5)
    , delScriptButton (GetReference (), 6)
    , manageShortcutsButton (GetReference (), 7)
{
    Attach (*this);
    AttachToAllItems (*this);

    LoadScriptsToPopUp ();

    SetRunButtonIcon ();

    BeginEventProcessing ();
}

TapirPalette::~TapirPalette ()
{
    DetachFromAllItems (*this);
    EndEventProcessing ();
}

bool TapirPalette::HasInstance ()
{
    return instance != nullptr;
}

TapirPalette& TapirPalette::Instance ()
{
    if (!HasInstance ()) {
        instance = new TapirPalette ();
    }
    return *instance;
}

void TapirPalette::Show ()
{
    DG::Palette::Show ();
    SetMenuItemCheckedState (true);
}

void TapirPalette::Hide ()
{
    DG::Palette::Hide ();
    SetMenuItemCheckedState (false);
}

void TapirPalette::SetMenuItemCheckedState (bool isChecked)
{
    API_MenuItemRef    itemRef = {};
    GSFlags            itemFlags = {};

    itemRef.menuResID = ID_ADDON_MENU_FOR_PALETTE;
    itemRef.itemIndex = ID_ADDON_MENU_PALETTE;

    ACAPI_MenuItem_GetMenuItemFlags (&itemRef, &itemFlags);
    if (isChecked)
        itemFlags |= API_MenuItemChecked;
    else
        itemFlags &= ~API_MenuItemChecked;
    ACAPI_MenuItem_SetMenuItemFlags (&itemRef, &itemFlags);
}

void TapirPalette::PanelCloseRequested (const DG::PanelCloseRequestEvent&, bool* accepted)
{
    Hide ();
    *accepted = true;
}

void TapirPalette::PanelOpened (const DG::PanelOpenEvent&)
{
    auto itemToSelect = AddScriptsFromPreferences ();
    if (itemToSelect <= 0 || scriptSelectionPopUp.IsSeparator (itemToSelect) || scriptSelectionPopUp.GetItemCount () - 1 <= itemToSelect) {
        itemToSelect = scriptSelectionPopUp.GetSelectedItem ();
        if (scriptSelectionPopUp.IsSeparator (itemToSelect) || scriptSelectionPopUp.GetItemCount () - 1 <= itemToSelect) {
            itemToSelect = DG::PopUp::TopItem;
        }
    }
    scriptSelectionPopUp.SelectItem (itemToSelect);
    RefreshScriptListShortcutLabels ();
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

static void OpenFileInExplorer (const IO::Location& file)
{
    IO::Location folderLoc = file;
    folderLoc.DeleteLastLocalName ();

    GS::UniString pathStr;
    folderLoc.ToPath (&pathStr);
    const GS::UniString command = GS::UniString::Printf (
        "%s %T",
#ifdef WINDOWS
        "explorer",
#else
        "open",
#endif
        pathStr.ToPrintf ());
    system (command.ToCStr ().Get ());
}

void TapirPalette::ButtonClicked (const DG::ButtonClickEvent& ev)
{
    if (ev.GetSource () == &runScriptButton) {
        if (IsProcessRunning ()) {
            KillRunningProcess ();
        } else {
            if (isUpdatingAddOn) {
                return;
            }
            if (Config::Instance().AskUpdatingAddOnBeforeEachExecution () && UpdateAddOn ()) {
                return;
            }

            GS::Ref<PopUpItemData> popUpItemData = GS::DynamicCast<PopUpItemData> (scriptSelectionPopUp.GetItemObjectData (scriptSelectionPopUp.GetSelectedItem ()));
            if (popUpItemData != nullptr) {
                ExecuteScript (*popUpItemData);
            }
        }
    } else if (ev.GetSource () == &tapirButton) {
        OpenWebpage ("https://github.com/ENZYME-APD/tapir-archicad-automation");
    } else if (ev.GetSource () == &openScriptButton) {
        GS::Ref<PopUpItemData> popUpItemData = GS::DynamicCast<PopUpItemData> (scriptSelectionPopUp.GetItemObjectData (scriptSelectionPopUp.GetSelectedItem ()));
        if (popUpItemData->repoRelLoc.IsEmpty ()) {
            OpenFileInExplorer (popUpItemData->fileLocation);
        } else {
            OpenWebpage ("https://github.com/" + popUpItemData->repo->repoOwner + "/" + popUpItemData->repo->repoName + "/blob/main/" + popUpItemData->repoRelLoc);
        }
    } else if (ev.GetSource () == &addScriptButton) {
        AddNewScript ();
    } else if (ev.GetSource () == &delScriptButton) {
        DeleteScriptFromPopUp ();
    } else if (ev.GetSource () == &manageShortcutsButton) {
        OpenShortcutsDialog ();
    }
}

bool TapirPalette::IsPopUpContainsFile (const IO::Location& fileLocation) const
{
    for (short i = 1; i <= scriptSelectionPopUp.GetItemCount () - 1; ++i) {
        if (scriptSelectionPopUp.IsSeparator (i)) {
            continue;
        }

        GS::Ref<PopUpItemData> popUpItemData = GS::DynamicCast<PopUpItemData> (scriptSelectionPopUp.GetItemObjectData (i));
        if (popUpItemData != nullptr && popUpItemData->fileLocation == fileLocation) {
            return true;
        }
    }
    return false;
}

void TapirPalette::PopUpChanged (const DG::PopUpChangeEvent& ev)
{
    if (ev.GetSource () == &scriptSelectionPopUp) {
        const short selectedItem = scriptSelectionPopUp.GetSelectedItem ();

        if (selectedItem == scriptSelectionPopUp.GetItemCount ()) {
            LoadScriptsToPopUp ();

            if (!scriptSelectionPopUp.IsSeparator (ev.GetPreviousSelection ()) &&
                ev.GetPreviousSelection () < scriptSelectionPopUp.GetItemCount ()) {
                scriptSelectionPopUp.SelectItem (ev.GetPreviousSelection ());
            } else {
                scriptSelectionPopUp.SelectItem (DG::PopUp::TopItem);
            }
        }

        SaveScriptsToPreferences ();
        SetDeleteScriptButtonStatus ();
    }
}

void TapirPalette::RunShortcutSlot (short slotIndex)
{
    // Uses DG_ERROR (pops an alert) rather than DG_WARNING (silently logged) for every failure
    // path here on purpose: a keyboard-shortcut-triggered action needs to be impossible to miss —
    // a report-log-only warning looks identical to "nothing happened" if the Report window isn't open.
    if (slotIndex < 0 || slotIndex >= ScriptShortcutSlotCount || scriptShortcutSlots[slotIndex].IsEmpty ()) {
        WriteReport (DG_ERROR, "No script assigned to shortcut slot %d.", (int) (slotIndex + 1));
        return;
    }
    if (IsProcessRunning ()) {
        WriteReport (DG_ERROR, "A script is already running - wait for it to finish before using a shortcut slot.");
        return;
    }
    if (isUpdatingAddOn) {
        return;
    }
    if (Config::Instance ().AskUpdatingAddOnBeforeEachExecution () && UpdateAddOn ()) {
        return;
    }

    for (short i = 1; i <= scriptSelectionPopUp.GetItemCount () - 1; ++i) {
        if (scriptSelectionPopUp.IsSeparator (i)) {
            continue;
        }
        GS::Ref<PopUpItemData> popUpItemData = GS::DynamicCast<PopUpItemData> (scriptSelectionPopUp.GetItemObjectData (i));
        if (popUpItemData != nullptr && popUpItemData->fileLocation == scriptShortcutSlots[slotIndex]) {
            ExecuteScript (*popUpItemData);
            return;
        }
    }
    WriteReport (DG_ERROR, "Script assigned to shortcut slot %d could not be found (%d scripts currently listed).",
        (int) (slotIndex + 1), (int) (scriptSelectionPopUp.GetItemCount () - 1));
}

void TapirPalette::GetAvailableScripts (GS::Array<std::pair<GS::UniString, IO::Location>>& outScripts)
{
    outScripts.Clear ();
    for (short i = 1; i <= scriptSelectionPopUp.GetItemCount () - 1; ++i) {
        if (scriptSelectionPopUp.IsSeparator (i)) {
            continue;
        }
        GS::Ref<PopUpItemData> popUpItemData = GS::DynamicCast<PopUpItemData> (scriptSelectionPopUp.GetItemObjectData (i));
        if (popUpItemData != nullptr) {
            outScripts.Push ({ scriptSelectionPopUp.GetItemText (i), popUpItemData->fileLocation });
        }
    }
}

IO::Location TapirPalette::GetShortcutSlot (short slotIndex) const
{
    if (slotIndex < 0 || slotIndex >= ScriptShortcutSlotCount) {
        return IO::Location ();
    }
    return scriptShortcutSlots[slotIndex];
}

void TapirPalette::SetShortcutSlot (short slotIndex, const IO::Location& location)
{
    if (slotIndex < 0 || slotIndex >= ScriptShortcutSlotCount) {
        return;
    }
    // A script only ever occupies one slot — clear it from any other slot first.
    if (!location.IsEmpty ()) {
        for (short slot = 0; slot < ScriptShortcutSlotCount; ++slot) {
            if (scriptShortcutSlots[slot] == location) {
                scriptShortcutSlots[slot] = IO::Location ();
            }
        }
    }
    scriptShortcutSlots[slotIndex] = location;
    SaveScriptsToPreferences ();
    RefreshScriptListShortcutLabels ();
    // Refresh every slot: the dedup loop above may have cleared another one.
    ApplyAllShortcutMenuItemTexts ();
}

GS::UniString TapirPalette::GetShortcutLabel (short slotIndex) const
{
    if (slotIndex < 0 || slotIndex >= ScriptShortcutSlotCount) {
        return GS::EmptyUniString;
    }
    return scriptShortcutLabels[slotIndex];
}

void TapirPalette::SetShortcutLabel (short slotIndex, const GS::UniString& label)
{
    if (slotIndex < 0 || slotIndex >= ScriptShortcutSlotCount) {
        return;
    }
    if (scriptShortcutLabels[slotIndex] == label) {
        return;
    }
    scriptShortcutLabels[slotIndex] = label;
    ApplyShortcutMenuItemText (slotIndex);
    SaveScriptsToPreferences ();
}

// scriptSelectionPopUp items get "  [Shortcut N]" appended when the script backing them is assigned
// to a slot, so both "which slot is this" and "what's already taken" are visible while just browsing
// the normal script dropdown, not only inside the dedicated Shortcuts dialog.
void TapirPalette::RefreshScriptListShortcutLabels ()
{
    for (short i = 1; i <= scriptSelectionPopUp.GetItemCount () - 1; ++i) {
        if (scriptSelectionPopUp.IsSeparator (i)) {
            continue;
        }
        GS::Ref<PopUpItemData> popUpItemData = GS::DynamicCast<PopUpItemData> (scriptSelectionPopUp.GetItemObjectData (i));
        if (popUpItemData == nullptr) {
            continue;
        }
        GS::UniString label = popUpItemData->baseDisplayText;
        for (short slot = 0; slot < ScriptShortcutSlotCount; ++slot) {
            if (!scriptShortcutSlots[slot].IsEmpty () && scriptShortcutSlots[slot] == popUpItemData->fileLocation) {
                label += GS::UniString::Printf ("   [Shortcut %d]", (int) (slot + 1));
                break;
            }
        }
        scriptSelectionPopUp.SetItemText (i, label);
    }
}

void TapirPalette::ApplyShortcutMenuItemText (short slotIndex)
{
    static const short menuResIds[ScriptShortcutSlotCount] = {
        ID_ADDON_MENU_RUNSCRIPT_1, ID_ADDON_MENU_RUNSCRIPT_2, ID_ADDON_MENU_RUNSCRIPT_3,
        ID_ADDON_MENU_RUNSCRIPT_4, ID_ADDON_MENU_RUNSCRIPT_5, ID_ADDON_MENU_RUNSCRIPT_6
    };
    if (slotIndex < 0 || slotIndex >= ScriptShortcutSlotCount) {
        return;
    }

    GS::UniString label = scriptShortcutLabels[slotIndex];
    if (label.IsEmpty ()) {
        label = GS::UniString::Printf ("Run Script Shortcut %d", (int) (slotIndex + 1));
    }

    API_MenuItemRef itemRef = {};
    itemRef.menuResID = menuResIds[slotIndex];
    itemRef.itemIndex = ID_ADDON_MENU_RUNSCRIPT_ITEM;
    ACAPI_MenuItem_SetMenuItemText (&itemRef, nullptr, &label);

    // The API offers no way to hide add-on menu items, so slots without an
    // assigned script are disabled (greyed out) instead.
    GSFlags itemFlags = {};
    ACAPI_MenuItem_GetMenuItemFlags (&itemRef, &itemFlags);
    if (scriptShortcutSlots[slotIndex].IsEmpty ()) {
        itemFlags |= API_MenuItemDisabled;
    } else {
        itemFlags &= ~API_MenuItemDisabled;
    }
    ACAPI_MenuItem_SetMenuItemFlags (&itemRef, &itemFlags);
}

void TapirPalette::ApplyAllShortcutMenuItemTexts ()
{
    for (short slot = 0; slot < ScriptShortcutSlotCount; ++slot) {
        ApplyShortcutMenuItemText (slot);
    }
}

namespace {

// Dedicated dialog for assigning scripts to shortcut slots: each row shows the slot's currently
// assigned script directly in its own popup (and "(unassigned)" when empty), plus an editable label
// for the text shown in Archicad's menu/toolbar for that shortcut, so both "which slot is this" and
// "what's already taken" are visible in one screen, instead of blind slot numbers.
class TapirShortcutsDialog : public DG::ModalDialog,
    public DG::ButtonItemObserver,
    public DG::PopUpObserver,
    public DG::TextEditBaseObserver
{
public:
    TapirShortcutsDialog ()
        : DG::ModalDialog (ACAPI_GetOwnResModule (), ID_SHORTCUTS_DIALOG, ACAPI_GetOwnResModule ())
        , slotPopUp1 (GetReference (), ID_SHORTCUTS_DIALOG_POPUP_1)
        , slotPopUp2 (GetReference (), ID_SHORTCUTS_DIALOG_POPUP_2)
        , slotPopUp3 (GetReference (), ID_SHORTCUTS_DIALOG_POPUP_3)
        , slotPopUp4 (GetReference (), ID_SHORTCUTS_DIALOG_POPUP_4)
        , slotPopUp5 (GetReference (), ID_SHORTCUTS_DIALOG_POPUP_5)
        , slotPopUp6 (GetReference (), ID_SHORTCUTS_DIALOG_POPUP_6)
        , labelEdit1 (GetReference (), ID_SHORTCUTS_DIALOG_LABELEDIT_1)
        , labelEdit2 (GetReference (), ID_SHORTCUTS_DIALOG_LABELEDIT_2)
        , labelEdit3 (GetReference (), ID_SHORTCUTS_DIALOG_LABELEDIT_3)
        , labelEdit4 (GetReference (), ID_SHORTCUTS_DIALOG_LABELEDIT_4)
        , labelEdit5 (GetReference (), ID_SHORTCUTS_DIALOG_LABELEDIT_5)
        , labelEdit6 (GetReference (), ID_SHORTCUTS_DIALOG_LABELEDIT_6)
        , closeButton (GetReference (), ID_SHORTCUTS_DIALOG_CLOSE_BUTTON)
    {
        slotPopUps[0] = &slotPopUp1;
        slotPopUps[1] = &slotPopUp2;
        slotPopUps[2] = &slotPopUp3;
        slotPopUps[3] = &slotPopUp4;
        slotPopUps[4] = &slotPopUp5;
        slotPopUps[5] = &slotPopUp6;

        labelEdits[0] = &labelEdit1;
        labelEdits[1] = &labelEdit2;
        labelEdits[2] = &labelEdit3;
        labelEdits[3] = &labelEdit4;
        labelEdits[4] = &labelEdit5;
        labelEdits[5] = &labelEdit6;

        closeButton.Attach (*this);
        for (short row = 0; row < TapirPalette::ScriptShortcutSlotCount; ++row) {
            slotPopUps[row]->Attach (*this);
            labelEdits[row]->Attach (*this);
        }

        TapirPalette::Instance ().GetAvailableScripts (scripts);
        for (short row = 0; row < TapirPalette::ScriptShortcutSlotCount; ++row) {
            BuildRowItems (*slotPopUps[row]);
        }
        RefreshAllRows ();
    }

    ~TapirShortcutsDialog ()
    {
        closeButton.Detach (*this);
        for (short row = 0; row < TapirPalette::ScriptShortcutSlotCount; ++row) {
            slotPopUps[row]->Detach (*this);
            labelEdits[row]->Detach (*this);
        }
    }

private:
    DG::PopUp    slotPopUp1, slotPopUp2, slotPopUp3, slotPopUp4, slotPopUp5, slotPopUp6;
    DG::TextEdit labelEdit1, labelEdit2, labelEdit3, labelEdit4, labelEdit5, labelEdit6;
    DG::Button   closeButton;
    DG::PopUp*     slotPopUps[TapirPalette::ScriptShortcutSlotCount];
    DG::TextEdit*  labelEdits[TapirPalette::ScriptShortcutSlotCount];
    GS::Array<std::pair<GS::UniString, IO::Location>> scripts;

    void BuildRowItems (DG::PopUp& popUp)
    {
        popUp.AppendItem ();
        popUp.SetItemText (DG::PopUp::BottomItem, "(unassigned)");
        for (const auto& script : scripts) {
            popUp.AppendItem ();
            popUp.SetItemText (DG::PopUp::BottomItem, script.first);
        }
    }

    void RefreshAllRows ()
    {
        for (short row = 0; row < TapirPalette::ScriptShortcutSlotCount; ++row) {
            const IO::Location assigned = TapirPalette::Instance ().GetShortcutSlot (row);
            short itemToSelect = 1;   // "(unassigned)"
            if (!assigned.IsEmpty ()) {
                for (UIndex i = 0; i < scripts.GetSize (); ++i) {
                    if (scripts[i].second == assigned) {
                        itemToSelect = (short) (i + 2);
                        break;
                    }
                }
            }
            slotPopUps[row]->SelectItem (itemToSelect);

            GS::UniString label = TapirPalette::Instance ().GetShortcutLabel (row);
            if (label.IsEmpty ()) {
                label = GS::UniString::Printf ("Run Script Shortcut %d", (int) (row + 1));
            }
            labelEdits[row]->SetText (label);
        }
    }

    virtual void ButtonClicked (const DG::ButtonClickEvent& ev) override
    {
        if (ev.GetSource () == &closeButton) {
            PostCloseRequest (DG::ModalDialog::Accept);
        }
    }

    virtual void PopUpChanged (const DG::PopUpChangeEvent& ev) override
    {
        for (short row = 0; row < TapirPalette::ScriptShortcutSlotCount; ++row) {
            if (ev.GetSource () == slotPopUps[row]) {
                const short selectedItem = slotPopUps[row]->GetSelectedItem ();
                IO::Location newLocation;   // empty = unassigned
                if (selectedItem >= 2 && (UIndex) (selectedItem - 2) < scripts.GetSize ()) {
                    newLocation = scripts[selectedItem - 2].second;
                }
                TapirPalette::Instance ().SetShortcutSlot (row, newLocation);
                RefreshAllRows ();   // another row may have just lost this script to the dedup rule
                return;
            }
        }
    }

    virtual void TextEditChanged (const DG::TextEditChangeEvent& ev) override
    {
        for (short row = 0; row < TapirPalette::ScriptShortcutSlotCount; ++row) {
            if (ev.GetSource () == labelEdits[row]) {
                TapirPalette::Instance ().SetShortcutLabel (row, labelEdits[row]->GetText ());
                return;
            }
        }
    }
};

} // namespace

void TapirPalette::OpenShortcutsDialog ()
{
    TapirShortcutsDialog dialog;
    dialog.Invoke ();
}

GSErrCode TapirPalette::PaletteControlCallBack (Int32, API_PaletteMessageID messageID, GS::IntPtr param)
{
    switch (messageID) {
        case APIPalMsg_OpenPalette:
            Instance ().Show ();
            break;

        case APIPalMsg_ClosePalette:
            if (!HasInstance ())
                break;
            Instance ().Hide ();
            break;

        case APIPalMsg_HidePalette_Begin:
            if (HasInstance () && Instance ().IsVisible ())
                Instance ().Hide ();
            break;

        case APIPalMsg_HidePalette_End:
            if (HasInstance () && !Instance ().IsVisible ())
                Instance ().Show ();
            break;

        case APIPalMsg_IsPaletteVisible:
            *(reinterpret_cast<bool*> (param)) = HasInstance () && Instance ().IsVisible ();
            break;

        default:
            break;
    }

    return NoError;
}

GSErrCode TapirPalette::RegisterPaletteControlCallBack ()
{
    return ACAPI_RegisterModelessWindow (
                    GS::CalculateHashValue (paletteGuid),
                    PaletteControlCallBack,
                    API_PalEnabled_FloorPlan + API_PalEnabled_Section + API_PalEnabled_Elevation +
                    API_PalEnabled_InteriorElevation + API_PalEnabled_3D + API_PalEnabled_Detail +
                    API_PalEnabled_Worksheet + API_PalEnabled_Layout + API_PalEnabled_DocumentFrom3D,
                    GSGuid2APIGuid (paletteGuid));
}

// ---- Stopping a script ----
//
// GS::Process::Kill ends only the process the palette started. For a Python script that is uv, and the Python
// process(es) uv started kept running without a parent (issue #769): a script waiting for something, such as a
// ShowScriptUI page, went on in the background, and a second run of it competed with it for the same Archicad.
// So Stop ends the started process together with all of its descendants. GS::Process gives no process id: the
// started process is found among the child processes of this Archicad by the file name of its executable, which
// is unambiguous because the palette runs one script at a time and the Add-On waits for its other uv runs.
// A process id can be reused, so a process is taken as the child of another only when it started after it.

#if defined (WINDOWS)
using ProcessId = DWORD;
#else
using ProcessId = pid_t;
#endif

struct ProcessInfo {
    ProcessId     id = 0;
    ProcessId     parentId = 0;
    GS::UInt64    startTime = 0;     // in platform units, only compared with each other
    GS::UniString executableName;    // the file name without the folder
};

#if defined (WINDOWS)

static ProcessId GetThisProcessId ()
{
    return GetCurrentProcessId ();
}

static GS::Array<ProcessInfo> ListProcesses ()
{
    GS::Array<ProcessInfo> processes;
    const HANDLE snapshot = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return processes;
    }
    PROCESSENTRY32W entry = {};
    entry.dwSize = static_cast<DWORD> (sizeof (entry));
    for (BOOL hasEntry = Process32FirstW (snapshot, &entry); hasEntry; hasEntry = Process32NextW (snapshot, &entry)) {
        ProcessInfo info;
        info.id = entry.th32ProcessID;
        info.parentId = entry.th32ParentProcessID;
        info.executableName = GS::UniString (reinterpret_cast<const GS::UniChar::Layout*> (entry.szExeFile), static_cast<USize> (wcslen (entry.szExeFile)));
        const HANDLE processHandle = OpenProcess (PROCESS_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ProcessID);
        if (processHandle != nullptr) {
            FILETIME creationTime = {}, exitTime = {}, kernelTime = {}, userTime = {};
            if (GetProcessTimes (processHandle, &creationTime, &exitTime, &kernelTime, &userTime)) {
                info.startTime = (static_cast<GS::UInt64> (creationTime.dwHighDateTime) << 32) | creationTime.dwLowDateTime;
            }
            CloseHandle (processHandle);
        }
        processes.Push (info);
    }
    CloseHandle (snapshot);
    return processes;
}

static void KillProcess (ProcessId id)
{
    const HANDLE processHandle = OpenProcess (PROCESS_TERMINATE, FALSE, id);
    if (processHandle != nullptr) {
        TerminateProcess (processHandle, 1);
        CloseHandle (processHandle);
    }
}

// Windows file names are case-insensitive, and the command may name uv without its .exe extension
static GS::UniString NormalizeExecutableName (const GS::UniString& name)
{
    constexpr USize extensionLength = 4;
    if (name.GetLength () > extensionLength &&
        GS::UniString (name.GetSubstring (name.GetLength () - extensionLength, extensionLength)).Compare (".exe", CaseInsensitive) == GS::UniString::Equal) {
        return GS::UniString (name.GetSubstring (0, name.GetLength () - extensionLength));
    }
    return name;
}

static bool IsSameExecutableName (const GS::UniString& name1, const GS::UniString& name2)
{
    return NormalizeExecutableName (name1).Compare (NormalizeExecutableName (name2), CaseInsensitive) == GS::UniString::Equal;
}

#else

static ProcessId GetThisProcessId ()
{
    return getpid ();
}

static GS::Array<ProcessInfo> ListProcesses ()
{
    GS::Array<ProcessInfo> processes;
    int name[] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
    size_t size = 0;
    if (sysctl (name, 4, nullptr, &size, nullptr, 0) != 0) {
        return processes;
    }
    std::vector<kinfo_proc> entries (size / sizeof (kinfo_proc) + 32);   // room for processes started since the size was asked
    size = entries.size () * sizeof (kinfo_proc);
    if (sysctl (name, 4, entries.data (), &size, nullptr, 0) != 0) {
        return processes;
    }
    entries.resize (size / sizeof (kinfo_proc));
    for (const kinfo_proc& entry : entries) {
        ProcessInfo info;
        info.id = entry.kp_proc.p_pid;
        info.parentId = entry.kp_eproc.e_ppid;
        info.startTime = static_cast<GS::UInt64> (entry.kp_proc.p_starttime.tv_sec) * 1000000 + static_cast<GS::UInt64> (entry.kp_proc.p_starttime.tv_usec);
        char path[PROC_PIDPATHINFO_MAXSIZE] = {};
        const char* executableName = entry.kp_proc.p_comm;   // the name cut to 16 characters, for processes whose path cannot be read
        if (proc_pidpath (entry.kp_proc.p_pid, path, sizeof (path)) > 0) {
            const char* lastSlash = strrchr (path, '/');
            executableName = lastSlash != nullptr ? lastSlash + 1 : path;
        }
        info.executableName = GS::UniString (executableName, static_cast<USize> (strlen (executableName)), CC_UTF8);
        processes.Push (info);
    }
    return processes;
}

static void KillProcess (ProcessId id)
{
    kill (id, SIGKILL);
}

static bool IsSameExecutableName (const GS::UniString& name1, const GS::UniString& name2)
{
    return name1 == name2;
}

#endif

// The file name of the command's executable: the command is an absolute path, or on Windows the bare "uv"
static GS::UniString GetExecutableName (const GS::UniString& command)
{
    UIndex nameStart = 0;
    for (UIndex i = 0; i < command.GetLength (); ++i) {
        const GS::UniChar ch = command.GetChar (i);
        if (ch == '/' || ch == '\\') {
            nameStart = i + 1;
        }
    }
    return GS::UniString (command.GetSubstring (nameStart, command.GetLength () - nameStart));
}

static bool IsChildOf (const ProcessInfo& process, const ProcessInfo& parent)
{
    return process.parentId == parent.id && process.id != parent.id && process.startTime >= parent.startTime;
}

static void CollectDescendants (const GS::Array<ProcessInfo>& processes, const ProcessInfo& parent, GS::Array<ProcessId>& descendants)
{
    for (const ProcessInfo& process : processes) {
        if (IsChildOf (process, parent) && !descendants.Contains (process.id)) {
            descendants.Push (process.id);
            CollectDescendants (processes, process, descendants);
        }
    }
}

// The processes started by the process this Archicad started for the command, the processes those started, and so on
static GS::Array<ProcessId> FindDescendantsOfStartedProcess (const GS::UniString& command)
{
    GS::Array<ProcessId> descendants;
    const GS::Array<ProcessInfo> processes = ListProcesses ();
    const GS::UniString executableName = GetExecutableName (command);

    const ProcessInfo* thisProcess = nullptr;
    for (const ProcessInfo& process : processes) {
        if (process.id == GetThisProcessId ()) {
            thisProcess = &process;
        }
    }
    if (thisProcess == nullptr) {
        return descendants;
    }

    const ProcessInfo* startedProcess = nullptr;   // the most recently started one, should there be more
    for (const ProcessInfo& process : processes) {
        if (IsChildOf (process, *thisProcess) && IsSameExecutableName (process.executableName, executableName) &&
            (startedProcess == nullptr || process.startTime > startedProcess->startTime)) {
            startedProcess = &process;
        }
    }
    if (startedProcess != nullptr) {
        CollectDescendants (processes, *startedProcess, descendants);
    }
    return descendants;
}

void TapirPalette::KillRunningProcess ()
{
    // Found before Kill: afterwards the descendants have no parent any more (macOS gives them to launchd), and
    // the id of the ended process may be reused
    const GS::Array<ProcessId> descendants = FindDescendantsOfStartedProcess (runningCommand);
    process.Kill ();
    for (const ProcessId descendant : descendants) {
        KillProcess (descendant);
    }
}

void TapirPalette::ExecuteScript (const PopUpItemData& popUpItemData)
{
    GS::UniString filePath;
    popUpItemData.fileLocation.ToPath (&filePath);

    class UIUpdaterThread : public GS::Runnable
    {
        GS::Process& process;

        class IconUpdateTask : public GS::Runnable {
        public:
            IconUpdateTask () = default;
            virtual void Run ()
            {
                TapirPalette::Instance ().SetRunButtonIcon ();
            }
        };
        class OutputUpdateTask : public GS::Runnable {
            short type;
            GS::UniString text;
        public:
            OutputUpdateTask (short _type, const GS::UniString& _text) : text(_text), type(_type)
            {}
            virtual void Run ()
            {
                TapirPalette::Instance ().WriteReport (type, text);
            }
        };

        void ProcessStderrBlock (const GS::UniString& stderrContent)
        {
            if (stderrContent.IsEmpty ()) {
                return;
            }

#ifdef ServerMainVers_3000
            const GS::UniString lowerContent = stderrContent.GetLowerCased ();
#else
            const GS::UniString lowerContent = stderrContent.ToLowerCase ();
#endif

            if (lowerContent.Contains ("error:") || lowerContent.Contains ("failed") || lowerContent.Contains ("traceback (most recent call last)")) {
                GS::MessageLoopExecutor ().Execute (new OutputUpdateTask (DG_ERROR, stderrContent));
                return;
            }
            if (lowerContent.Contains ("warning:")) {
                GS::MessageLoopExecutor ().Execute (new OutputUpdateTask (DG_WARNING, stderrContent));
                return;
            }
            GS::MessageLoopExecutor ().Execute (new OutputUpdateTask (DG_INFORMATION, stderrContent));
        }

    public:
        explicit UIUpdaterThread (GS::Process& p) : process(p)
        {
        }
        GS::UniString ReadFromChannel (GS::IBinaryChannel& channel)
        {
            if (channel.GetAvailable () <= 0) {
                return GS::EmptyUniString;
            }

            const GS::USize uSize = static_cast<GS::USize> (channel.GetAvailable ());
            std::unique_ptr<char> buffer;
            buffer.reset (new char[uSize + 1]);

            GS::IBinaryChannelUtilities::ReadFully (channel, buffer.get (), uSize);
            return GS::UniString (buffer.get (), uSize, CC_UTF8);
        }
        void ReadFromChannels ()
        {
            const GS::UniString stdError = ReadFromChannel (process.GetStandardErrorChannel ());
            ProcessStderrBlock (stdError);

            const GS::UniString stdOutput = ReadFromChannel (process.GetStandardOutputChannel ());
            if (!stdOutput.IsEmpty ()) {
                GS::MessageLoopExecutor ().Execute (new OutputUpdateTask (DG_INFORMATION, stdOutput));
            }
        }
        virtual void Run () override final
        {
            GS::MessageLoopExecutor ().Execute (new IconUpdateTask ());

            const GS::Timeout Timeout = {0, 0, 0, 10 /*ms*/};
            while (!process.WaitFor (Timeout)) {
                ReadFromChannels ();
            }

            const int exitCode = process.GetExitCode ();
            ReadFromChannels ();
            process = {}; // Reset the process to an invalid state

            GS::MessageLoopExecutor ().Execute (new IconUpdateTask ());
            GS::MessageLoopExecutor ().Execute (new OutputUpdateTask (DG_INFORMATION, "ExitCode: " + GS::ValueToUniString (exitCode)));
        }
    };

    try {
        WriteReport (DG_INFORMATION, "Executing %T with uv", filePath.ToPrintf ());

        GS::UniString command;
        GS::Array<GS::UniString> argv;
        if (filePath.EndsWith (".py")) {
            const GS::UniString uvCommand = uvManager.GetUvExecutablePath ();
            if (uvCommand.IsEmpty ()) {
                // An alert was already shown to the user inside the manager, so we just exit.
                return;
            }
            command = uvCommand;
            argv = {"run"};
            argv.Append (uvManager.GetPythonSelectionArgs ());
            argv.Append ({"--script", filePath, "--port", GS::ValueToUniString (GetConnectionPort ())});
            if (popUpItemData.repo != nullptr && !popUpItemData.repo->token.IsEmpty ()) {
                argv.Append ({"--token", popUpItemData.repo->token});
            }
        } else {
            command = filePath;
            argv = {"--port", GS::ValueToUniString (GetConnectionPort ())};
        }

        constexpr bool redirectStandardOutput = true;
        constexpr bool redirectStandardInput = false;
        constexpr bool redirectStandardError = true;
        runningCommand = command;
        process = GS::Process::Create (command, argv, GS::Process::CreateNoWindow, redirectStandardOutput, redirectStandardInput, redirectStandardError);
        if (!process.IsValid ()) {
            WriteReport (DG_ERROR, "Failed to start uv process. Ensure it is installed and executable.");
        }

        executor.Execute (new UIUpdaterThread (process));
    } catch (const GS::ProcessException&) {
        WriteReport (DG_ERROR, "Failed to execute %T", filePath.ToPrintf ());
    } catch (...) {
        WriteReport (DG_ERROR, "Unexpected issue while executing %T", filePath.ToPrintf ());
    }
}

bool TapirPalette::AddScriptToPopUp (GS::Ref<PopUpItemData> popUpData, short index)
{
    if (popUpData == nullptr || !IsValidLocation (popUpData->fileLocation)) {
        return false;
    }

    IO::Name name;
    popUpData->fileLocation.GetLastLocalName (&name);

#if defined (macintosh)
    if (name.ToString ().BeginsWith ('.')) {
        return false;
    }
#endif

    scriptSelectionPopUp.InsertItem (index);
    if (popUpData->repo) {
        auto* repo = popUpData->repo;
        if (repo->displayName.IsEmpty ()) {
            popUpData->baseDisplayText = name.ToString () + " (" + repo->repoOwner + "/" + repo->repoName + ")";
        } else {
            popUpData->baseDisplayText = name.ToString () + " (" + repo->displayName + ")";
        }
    } else if (!popUpData->sourceName.IsEmpty ()) {
        popUpData->baseDisplayText = name.ToString () + " (" + popUpData->sourceName + ")";
    } else {
        popUpData->baseDisplayText = name.ToString ();
    }
    scriptSelectionPopUp.SetItemText (index, popUpData->baseDisplayText);
    scriptSelectionPopUp.SetItemObjectData (index, popUpData);

    return true;
}

void TapirPalette::AddScriptsFromRepositories ()
{
    IO::Location tapirTempFolder = GetTapirTemporaryFolder ();
    IO::fileSystem.Delete (tapirTempFolder);

    const auto& repositories = Config::Instance ().Repositories ();
    for (auto& repo : repositories) {
        const std::unique_ptr<std::regex> excludeFromDownloadRegex = repo.excludeFromDownloadPattern.IsEmpty () ? nullptr : std::make_unique<std::regex> (repo.excludeFromDownloadPattern.ToCStr ().Get ());
        const std::unique_ptr<std::regex> excludeRegex = repo.excludePattern.IsEmpty () ? nullptr : std::make_unique<std::regex> (repo.excludePattern.ToCStr ().Get ());
        const std::unique_ptr<std::regex> includeRegex = repo.includePattern.IsEmpty () ? nullptr : std::make_unique<std::regex> (repo.includePattern.ToCStr ().Get ());

        IO::RelativeLocation repoRelativeLoc (repo.repoOwner);
        repoRelativeLoc.Append (IO::Name (repo.repoName));
        IO::RelativeLocation repoFolderRelativeLoc (repoRelativeLoc);
        if (!repo.relativeLoc.IsEmpty ()) {
            repoFolderRelativeLoc.Append (IO::RelativeLocation (repo.relativeLoc));
        };
        std::map<GS::UniString, GS::UniString> headers;
        if (!repo.token.IsEmpty ()) {
            headers.emplace ("Authorization", "Bearer " + repo.token);
        }
        ACAPI_WriteReport ("Tapir is downloading content from GitHub repository: " + repo.repoOwner + "/" + repo.repoName, false);
        for (auto kv : GetFilesFromGitHubInRelativeLocation (repo)) {
            const GS::UniString repoLoc = repo.repoOwner + "/" + repo.repoName + "/" + kv.first;
            if (excludeFromDownloadRegex && std::regex_match (repoLoc.ToCStr ().Get (), *excludeFromDownloadRegex)) {
                ACAPI_WriteReport ("Skipping download of " + repoLoc + " due to exclude pattern", false);
                continue;
            }
            const auto content = DownloadFileContent (kv.second, headers);
            IO::RelativeLocation relLoc = repoRelativeLoc;
            relLoc.Append (IO::RelativeLocation (kv.first));
            const IO::Location fileLoc = SaveBuiltInScript (tapirTempFolder, relLoc, content);
            ACAPI_WriteReport ("Downloaded " + repoLoc, false);
            if (excludeRegex) {
                if (std::regex_match (kv.first.ToCStr ().Get (), *excludeRegex)) {
                    ACAPI_WriteReport ("Skipping use of " + repoLoc + " due to exclude pattern", false);
                    continue;
                }
            } else if (includeRegex) {
                if (!std::regex_match (kv.first.ToCStr ().Get (), *includeRegex)) {
                    ACAPI_WriteReport ("Skipping use of " + repoLoc + " due to include pattern", false);
                    continue;
                }
            } else {
                IO::Name fileName;
                if (relLoc.GetLength () != (1 + repoFolderRelativeLoc.GetLength ()) ||
                    fileLoc.GetLastLocalName (&fileName) != NoError ||
#ifdef ServerMainVers_3000
                    fileName.GetExtension ().GetLowerCased () != "py") {
#else
                    fileName.GetExtension ().ToLowerCase () != "py") {
#endif
                    continue;
                }
            }
            ACAPI_WriteReport ("Added to the popup: " + repoLoc, false);
            AddScriptToPopUp (GS::NewRef<PopUpItemData> (fileLoc, kv.first, &repo), DG::PopUp::BottomItem);
        }
    }
}

// Collects every file below folderLoc as (folder-relative path with '/' separators, file location),
// mirroring what GetFilesFromGitHubInRelativeLocation returns for a repository.
static void CollectFilesInFolder (const IO::Location& folderLoc, const GS::UniString& relativePrefix, std::map<GS::UniString, IO::Location>& files)
{
    IO::Folder folder (folderLoc);
    if (folder.GetStatus () != NoError) {
        return;
    }

    folder.Enumerate ([&] (const IO::Name& name, bool isFolder) {
        const GS::UniString relativePath = relativePrefix.IsEmpty () ? name.ToString () : relativePrefix + "/" + name.ToString ();
        const IO::Location entryLoc (folderLoc, name);
        if (isFolder) {
            CollectFilesInFolder (entryLoc, relativePath, files);
        } else {
            files[relativePath] = entryLoc;
        }
    });
}

void TapirPalette::AddScriptsFromLocalFolders ()
{
    const auto& localFolders = Config::Instance ().LocalFolders ();
    for (auto& localFolder : localFolders) {
        if (localFolder.path.IsEmpty ()) {
            continue;
        }

        const IO::Location folderLoc (localFolder.path);
        if (!IsValidLocation (folderLoc)) {
            ACAPI_WriteReport ("Tapir: Local script folder not found, skipping it: " + localFolder.path, false);
            continue;
        }

        const std::unique_ptr<std::regex> excludeRegex = localFolder.excludePattern.IsEmpty () ? nullptr : std::make_unique<std::regex> (localFolder.excludePattern.ToCStr ().Get ());
        const std::unique_ptr<std::regex> includeRegex = localFolder.includePattern.IsEmpty () ? nullptr : std::make_unique<std::regex> (localFolder.includePattern.ToCStr ().Get ());
        GS::UniString sourceName = localFolder.displayName;
        if (sourceName.IsEmpty ()) {
            IO::Name folderName;
            sourceName = folderLoc.GetLastLocalName (&folderName) == NoError ? folderName.ToString () : localFolder.path;
        }

        ACAPI_WriteReport ("Tapir is loading scripts from local folder: " + localFolder.path, false);
        std::map<GS::UniString, IO::Location> files;
        CollectFilesInFolder (folderLoc, GS::EmptyUniString, files);
        for (const auto& kv : files) {
            if (excludeRegex) {
                if (std::regex_match (kv.first.ToCStr ().Get (), *excludeRegex)) {
                    ACAPI_WriteReport ("Skipping use of " + kv.first + " due to exclude pattern", false);
                    continue;
                }
            } else if (includeRegex) {
                if (!std::regex_match (kv.first.ToCStr ().Get (), *includeRegex)) {
                    ACAPI_WriteReport ("Skipping use of " + kv.first + " due to include pattern", false);
                    continue;
                }
            } else {
                // Same default as for repositories: only the .py files directly in the folder.
                IO::Name fileName;
                if (kv.first.Contains ("/") ||
                    kv.second.GetLastLocalName (&fileName) != NoError ||
#ifdef ServerMainVers_3000
                    fileName.GetExtension ().GetLowerCased () != "py") {
#else
                    fileName.GetExtension ().ToLowerCase () != "py") {
#endif
                    continue;
                }
            }
            ACAPI_WriteReport ("Added to the popup: " + kv.first + " (" + sourceName + ")", false);
            GS::Ref<PopUpItemData> popUpItemData = GS::NewRef<PopUpItemData> (kv.second);
            popUpItemData->sourceName = sourceName;
            AddScriptToPopUp (popUpItemData, DG::PopUp::BottomItem);
        }
    }
}

void TapirPalette::AddScriptsFromCustomScriptsFolder ()
{
    IO::Location docsFolderLoc;
    IO::fileSystem.GetSpecialLocation (IO::FileSystem::UserDocuments, &docsFolderLoc);
    IO::Location customScriptsFolderLoc = docsFolderLoc;
    customScriptsFolderLoc.AppendToLocal (IO::Name ("Tapir"));
    customScriptsFolderLoc.AppendToLocal (IO::Name ("custom-scripts"));

    IO::fileSystem.CreateFolderTree (customScriptsFolderLoc);

    IO::Folder customScriptsFolder (customScriptsFolderLoc);
    customScriptsFolder.Enumerate ([&] (const IO::Name& name, bool isFolder) {
        if (isFolder) {
            return;
        }

        if (!hasCustomScript) {
            scriptSelectionPopUp.InsertSeparator (DG::PopUp::TopItem);
            hasCustomScript = true;
        }

        IO::Location file (customScriptsFolderLoc, name);
        AddScriptToPopUp (GS::NewRef<PopUpItemData> (file));
    });
}

void TapirPalette::LoadScriptsToPopUp ()
{
    scriptSelectionPopUp.DeleteItem (DG_ALL_ITEMS);

    hasCustomScript = false;
    hasAddedScript = false;

    AddScriptsFromCustomScriptsFolder ();
    auto itemToSelect = AddScriptsFromPreferences ();
    AddScriptsFromLocalFolders ();
    AddScriptsFromRepositories ();

    scriptSelectionPopUp.AppendSeparator ();
    scriptSelectionPopUp.AppendItem ();
    scriptSelectionPopUp.SetItemText (DG::PopUp::BottomItem, "Reload scripts...");

    if (itemToSelect <= 0 || scriptSelectionPopUp.IsSeparator (itemToSelect) || scriptSelectionPopUp.GetItemCount () - 1 <= itemToSelect) {
        itemToSelect = DG::PopUp::TopItem;
    }
    scriptSelectionPopUp.SelectItem (itemToSelect);

    RefreshScriptListShortcutLabels ();
    SetDeleteScriptButtonStatus ();
}

void TapirPalette::SetRunButtonIcon ()
{
    short resId = ID_RUN_BUTTON_ICON;
    if (IsProcessRunning ()) {
        resId = ID_KILL_BUTTON_ICON;
    } else if (VersionChecker::IsUsingLatestVersion ()) {
        resId = ID_RUN_BUTTON_ICON;
    } else {
        resId = ID_RUNWITHUPDATE_BUTTON_ICON;
    }
    runScriptButton.SetIcon (DG::Icon (ACAPI_GetOwnResModule (), resId));
}

#if defined (WINDOWS)

// Quotes an argument so that CommandLineToArgvW (and the C runtime) of the started program gives it back unchanged.
static GS::UniString QuoteWindowsArgument (const GS::UniString& argument)
{
    GS::UniString quoted ("\"");
    USize backslashCount = 0;
    for (UIndex i = 0; i < argument.GetLength (); ++i) {
        const GS::UniChar ch = argument.GetChar (i);
        if (ch == '\\') {
            ++backslashCount;
            continue;
        }
        // Backslashes are only special in front of a quote
        if (ch == '"') {
            backslashCount = backslashCount * 2 + 1;
        }
        for (; backslashCount > 0; --backslashCount) {
            quoted += GS::UniChar ('\\');
        }
        quoted += ch;
    }
    // Trailing backslashes must not escape the closing quote
    for (backslashCount *= 2; backslashCount > 0; --backslashCount) {
        quoted += GS::UniChar ('\\');
    }
    quoted += GS::UniChar ('"');
    return quoted;
}

// Whether the installer can replace the Add-On with the user's rights: it creates the new file next to the
// Add-On and renames it onto the Add-On, which a read-only file does not allow.
static bool CanReplaceAddOnWithoutElevation (const IO::Location& addOnFile)
{
    IO::Location probeFile = addOnFile;
    probeFile.DeleteLastLocalName ();
    probeFile.AppendToLocal (IO::Name (".TapirWriteCheck_" + GS::ValueToUniString (static_cast<GS::UInt32> (GetCurrentProcessId ()))));

    GS::UniString addOnPath;
    addOnFile.ToPath (&addOnPath);
    GS::UniString probePath;
    probeFile.ToPath (&probePath);
    const auto addOnPathUStr = addOnPath.ToUStr ();
    const auto probePathUStr = probePath.ToUStr ();

    // Deleted when closed, so it also checks that the user may delete files there
    const HANDLE probeHandle = CreateFileW (reinterpret_cast<LPCWSTR> (probePathUStr.Get ()), GENERIC_WRITE | DELETE, 0, nullptr,
        CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (probeHandle == INVALID_HANDLE_VALUE) {
        return false;
    }
    CloseHandle (probeHandle);

    const DWORD attributes = GetFileAttributesW (reinterpret_cast<LPCWSTR> (addOnPathUStr.Get ()));
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_READONLY) == 0;
}

// The installer asks for administrator rights in its manifest (--uac-admin). The RunAsInvoker compatibility layer
// starts it with the user's rights instead, without the prompt. Windows takes the layer from the environment, which
// the installer's own child process (PyInstaller's one-file mode) inherits too. So it is set in Archicad's environment
// only while the installer is started, and Archicad's own value is put back.
static bool StartWithoutElevation (const GS::UniString& programPath, const GS::UniString& parameters, DWORD& lastError)
{
    const wchar_t* compatLayerName = L"__COMPAT_LAYER";
    const DWORD oldValueSize = GetEnvironmentVariableW (compatLayerName, nullptr, 0);
    std::vector<wchar_t> oldValue (oldValueSize + 1);
    const bool hasOldValue = oldValueSize > 0 && GetEnvironmentVariableW (compatLayerName, oldValue.data (), oldValueSize) < oldValueSize;

    const GS::UniString commandLine = QuoteWindowsArgument (programPath) + " " + parameters;
    const auto programPathUStr = programPath.ToUStr ();
    const auto commandLineUStr = commandLine.ToUStr ();
    // CreateProcessW may write into the command line, so it gets a copy
    const wchar_t* commandLineChars = reinterpret_cast<const wchar_t*> (commandLineUStr.Get ());
    std::vector<wchar_t> commandLineBuffer (commandLineChars, commandLineChars + commandLine.GetLength () + 1);

    STARTUPINFOW startupInfo = {};
    startupInfo.cb = static_cast<DWORD> (sizeof (startupInfo));
    PROCESS_INFORMATION processInfo = {};
    SetEnvironmentVariableW (compatLayerName, L"RunAsInvoker");
    const BOOL started = CreateProcessW (reinterpret_cast<LPCWSTR> (programPathUStr.Get ()), commandLineBuffer.data (),
        nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startupInfo, &processInfo);
    lastError = started ? ERROR_SUCCESS : GetLastError ();
    SetEnvironmentVariableW (compatLayerName, hasOldValue ? oldValue.data () : nullptr);
    if (!started) {
        return false;
    }

    CloseHandle (processInfo.hThread);
    CloseHandle (processInfo.hProcess);
    return true;
}

// The installer requires administrator rights, so the shell starts it with the administrator (UAC) prompt. A standard
// user can only decline that prompt, but an Add-On in a folder the user can write to needs no administrator rights:
// then the installer is started with the user's rights instead, as the legacy update script updates such an Add-On.
// Returns false with an empty error when the prompt was declined and the Add-On cannot be replaced without it.
static bool StartInstaller (const IO::Location& installerFile, const IO::Location& addOnFile, const GS::Array<GS::UniString>& arguments, GS::UniString& error)
{
    GS::UniString installerPath;
    installerFile.ToPath (&installerPath);

    GS::UniString parameters;
    for (const GS::UniString& argument : arguments) {
        if (!parameters.IsEmpty ()) {
            parameters += GS::UniChar (' ');
        }
        parameters += QuoteWindowsArgument (argument);
    }

    // The UStr objects own the UTF-16 strings, they must live until ShellExecuteExW returns
    const auto installerPathUStr = installerPath.ToUStr ();
    const auto parametersUStr = parameters.ToUStr ();

    SHELLEXECUTEINFOW executeInfo = {};
    executeInfo.cbSize = static_cast<DWORD> (sizeof (executeInfo));
    executeInfo.fMask = SEE_MASK_NOASYNC;
    executeInfo.hwnd = GetForegroundWindow ();
    executeInfo.lpVerb = L"runas";
    executeInfo.lpFile = reinterpret_cast<LPCWSTR> (installerPathUStr.Get ());
    executeInfo.lpParameters = reinterpret_cast<LPCWSTR> (parametersUStr.Get ());
    executeInfo.nShow = SW_SHOWNORMAL;
    if (ShellExecuteExW (&executeInfo)) {
        return true;
    }

    DWORD lastError = GetLastError ();
    // ERROR_CANCELLED: the prompt was declined, ERROR_ACCESS_DENIED: a policy denies the user administrator rights
    if (lastError == ERROR_CANCELLED || lastError == ERROR_ACCESS_DENIED) {
        if (CanReplaceAddOnWithoutElevation (addOnFile)) {
            if (StartWithoutElevation (installerPath, parameters, lastError)) {
                return true;
            }
        } else if (lastError == ERROR_CANCELLED) {
            return false;
        }
    }

    error = "Windows error code " + GS::ValueToUniString (static_cast<GS::UInt32> (lastError));
    return false;
}

#endif

#if defined (macintosh)

// GS::Process does not search the PATH, so command must be an absolute path.
static bool RunAndWait (const GS::UniString& command, const GS::Array<GS::UniString>& arguments)
{
    try {
        GS::Process childProcess = GS::Process::Create (command, arguments, GS::Process::CreateNoWindow);
        if (!childProcess.IsValid ()) {
            return false;
        }
        childProcess.WaitFor ();
        return childProcess.GetExitCode () == 0;
    } catch (...) {
        return false;
    }
}

// Whether this Mac has an Apple silicon processor, also when Archicad runs translated by Rosetta.
static bool IsAppleSiliconMac ()
{
#if defined (__arm64__) || defined (__aarch64__)
    return true;
#else
    int isTranslated = 0;
    size_t size = sizeof (isTranslated);
    return sysctlbyname ("sysctl.proc_translated", &isTranslated, &size, nullptr, 0) == 0 && isTranslated == 1;
#endif
}

static GS::UInt32 ReadBigEndianUInt32 (const unsigned char* bytes)
{
    return (static_cast<GS::UInt32> (bytes[0]) << 24) | (static_cast<GS::UInt32> (bytes[1]) << 16) |
        (static_cast<GS::UInt32> (bytes[2]) << 8) | static_cast<GS::UInt32> (bytes[3]);
}

// Whether the Mach-O executable surely has no code for Intel processors. Anything that cannot be read is left
// to open. The values are those of <mach-o/fat.h>, <mach-o/loader.h> and <mach/machine.h>.
static bool LacksIntelCode (const IO::Location& executableFile)
{
    constexpr GS::UInt32 fatMagic = 0xcafebabe;             // FAT_MAGIC
    constexpr GS::UInt32 fatMagic64 = 0xcafebabf;           // FAT_MAGIC_64
    constexpr GS::UInt32 swappedMachMagic64 = 0xcffaedfe;   // MH_CIGAM_64: MH_MAGIC_64 of a little-endian executable
    constexpr GS::UInt32 cpuTypeX86_64 = 0x01000007;        // CPU_TYPE_X86_64

    unsigned char header[4096] = {};
    USize readSize = 0;
    IO::File file (executableFile);
    if (file.GetStatus () != NoError || file.Open (IO::File::ReadMode) != NoError) {
        return false;
    }
    file.ReadBin (reinterpret_cast<char*> (header), sizeof (header), &readSize);
    file.Close ();
    if (readSize < 8 || readSize > sizeof (header)) {
        return false;
    }

    const GS::UInt32 magic = ReadBigEndianUInt32 (header);
    if (magic == fatMagic || magic == fatMagic64) {
        // A universal executable: a big-endian list of fat_arch (or fat_arch_64) entries, each starting with its CPU type
        const GS::UInt64 archCount = ReadBigEndianUInt32 (header + 4);
        const GS::UInt64 archSize = magic == fatMagic ? 20 : 32;
        if (archCount == 0 || 8 + archCount * archSize > readSize) {
            return false;
        }
        for (GS::UInt64 i = 0; i < archCount; ++i) {
            if (ReadBigEndianUInt32 (header + 8 + i * archSize) == cpuTypeX86_64) {
                return false;
            }
        }
        return true;
    }
    if (magic == swappedMachMagic64) {
        // An executable for a single CPU type, its header is little-endian
        const unsigned char cpuTypeBytes[4] = { header[7], header[6], header[5], header[4] };
        return ReadBigEndianUInt32 (cpuTypeBytes) != cpuTypeX86_64;
    }
    return false;
}

// The installer app is started through LaunchServices, so it is not a child of Archicad and keeps running when Archicad quits.
// The Add-On is replaced by the installer, which asks for the administrator password itself when it needs to.
static bool StartInstaller (const IO::Location& installerFile, const IO::Location& /*addOnFile*/, const GS::Array<GS::UniString>& arguments, GS::UniString& error)
{
    IO::Location installerFolder = installerFile;
    installerFolder.DeleteLastLocalName ();
    IO::Location installerApp = installerFolder;
    installerApp.AppendToLocal (IO::Name ("TapirInstaller.app"));

    GS::UniString installerPath;
    installerFile.ToPath (&installerPath);
    GS::UniString installerFolderPath;
    installerFolder.ToPath (&installerFolderPath);
    GS::UniString installerAppPath;
    installerApp.ToPath (&installerAppPath);

    // ditto keeps the symbolic links and the signature of the app bundle intact
    bool installerAppExists = false;
    if (!RunAndWait ("/usr/bin/ditto", { "-x", "-k", installerPath, installerFolderPath }) ||
        IO::fileSystem.Contains (installerApp, &installerAppExists) != NoError || !installerAppExists) {
        error = "Failed to extract " + installerPath;
        return false;
    }

    // The installer of a release may be built for Apple silicon only, while the Add-On also runs on Intel Macs.
    // open would then fail, or only show the system's alert, without telling that the update can be installed manually.
    IO::Location installerExecutable = installerApp;
    installerExecutable.AppendToLocal (IO::Name ("Contents"));
    installerExecutable.AppendToLocal (IO::Name ("MacOS"));
    installerExecutable.AppendToLocal (IO::Name ("TapirInstaller"));
    if (!IsAppleSiliconMac () && LacksIntelCode (installerExecutable)) {
        error = "This Tapir Installer does not run on Macs with an Intel processor.";
        return false;
    }

    // -n: start a new instance even if an installer is already running, otherwise the arguments would be ignored
    GS::Array<GS::UniString> openArguments = { "-n", installerAppPath, "--args" };
    openArguments.Append (arguments);
    if (!RunAndWait ("/usr/bin/open", openArguments)) {
        error = "Failed to open " + installerAppPath;
        return false;
    }

    return true;
}

#endif

// Every update downloads the installer into a new folder, named "<creation time>_<process id>": the installer
// of an earlier update may still be open (its window stays open after a failure), and its files must not be
// overwritten (Windows locks the running exe) or deleted (on macOS it would go on running from deleted files).
// Not in the Tapir temporary folder: that one is deleted whenever the scripts are reloaded.
static bool CreateInstallerFolder (const GS::UniString& processIdStr, IO::Location& installerFolder, GS::UniString& error)
{
    IO::Location installersFolder;
    IO::fileSystem.GetSpecialLocation (IO::FileSystem::TemporaryFolder, &installersFolder);
    installersFolder.AppendToLocal (IO::Name ("TapirInstaller"));

    const GS::Int64 now = static_cast<GS::Int64> (std::time (nullptr));

    // Folders younger than a day are kept, their installer may still be open. Deleting may fail, e.g. for the
    // folder of a running installer on Windows; that folder is tried again next time.
    GS::Array<IO::Location> oldFolders;
    IO::Folder folder (installersFolder);
    if (folder.GetStatus () == NoError) {
        folder.Enumerate ([&] (const IO::Name& name, bool isFolder) {
            const auto nameCStr = name.ToString ().ToCStr ();
            char* nameEnd = nullptr;
            const GS::Int64 creationTime = std::strtoll (nameCStr.Get (), &nameEnd, 10);
            if (isFolder && nameEnd != nameCStr.Get () && *nameEnd == '_' && now - creationTime > 24 * 60 * 60) {
                oldFolders.Push (IO::Location (installersFolder, name));
            }
        });
    }
    for (const IO::Location& oldFolder : oldFolders) {
        IO::fileSystem.Delete (oldFolder);
    }

    const GS::UniString folderName = GS::ValueToUniString (now) + "_" + processIdStr;
    installerFolder = installersFolder;
    installerFolder.AppendToLocal (IO::Name (folderName));
    if (IO::fileSystem.CreateFolderTree (installerFolder) != NoError) {
        GS::UniString installerFolderPath;
        installerFolder.ToPath (&installerFolderPath);
        error = "Cannot create " + installerFolderPath;
        return false;
    }

    return true;
}

// Archicad's process window with a progress bar and a Cancel button, open while this object exists, so it is
// closed on every return path. Checking for Cancel also lets Archicad handle its events during the download,
// otherwise Windows marks Archicad "Not Responding" on a slow connection.
class DownloadProcessWindow {
public:
    DownloadProcessWindow (const GS::UniString& title, const GS::UniString& subtitle)
        : isOpen (false)
        , percent (0)
    {
        Int32 phaseCount = 1;
        // A menu command could start a second update or a script during the download
        API_ProcessControlTypeID controlType = API_MenuCommandDisabled;
        isOpen = ACAPI_ProcessWindow_InitProcessWindow (&title, &phaseCount, &controlType) == NoError;
        if (isOpen) {
            Int32 maxValue = 100;
            // Archicad passes maxValue on to the progress bar only with showPercent (seen in the Archicad 29 API),
            // otherwise the bar's maximum is 0
            bool showPercent = true;
            ACAPI_ProcessWindow_SetNextProcessPhase (&subtitle, &maxValue, &showPercent);
        }
    }

    ~DownloadProcessWindow ()
    {
        if (isOpen) {
            ACAPI_ProcessWindow_CloseProcessWindow ();
        }
    }

    DownloadProcessWindow (const DownloadProcessWindow&) = delete;
    DownloadProcessWindow& operator= (const DownloadProcessWindow&) = delete;

    // Returns false when the user cancelled.
    bool SetProgress (GS::UInt64 downloadedSize, GS::UInt64 totalSize)
    {
        if (!isOpen) {
            return true;
        }
        if (totalSize > 0) {
            Int32 newPercent = downloadedSize >= totalSize ? 100 : static_cast<Int32> (downloadedSize * 100 / totalSize);
            if (newPercent != percent) {
                percent = newPercent;
                ACAPI_ProcessWindow_SetProcessValue (&newPercent);
            }
        }
        return ACAPI_ProcessWindow_IsProcessCanceled () != APIERR_CANCEL;
    }

private:
    bool  isOpen;
    Int32 percent;
};

bool TapirPalette::UpdateAddOn ()
{
    // A second call during the download (e.g. from the menu) returns true, so the callers do not run a script
    if (isUpdatingAddOn) {
        return true;
    }
    isUpdatingAddOn = true;
    const GS::OnExit resetIsUpdatingAddOn ([] () { isUpdatingAddOn = false; });

    if (VersionChecker::IsUsingLatestVersion ()) {
        return false;
    }

    short response = DGAlert (DG_WARNING,
        RSGetIndString (ID_AUTOUPDATE_STRINGS, ID_AUTOUPDATE_NEWVERSION_ALERT_TITLE, ACAPI_GetOwnResModule ()),
        GS::UniString::Printf (RSGetIndString (ID_AUTOUPDATE_STRINGS, ID_AUTOUPDATE_NEWVERSION_ALERT_TEXT1, ACAPI_GetOwnResModule ()),
            ADDON_VERSION, VersionChecker::LatestVersion ().ToPrintf ()),
        RSGetIndString (ID_AUTOUPDATE_STRINGS, ID_AUTOUPDATE_NEWVERSION_ALERT_TEXT2, ACAPI_GetOwnResModule ()),
        RSGetIndString (ID_AUTOUPDATE_STRINGS, ID_AUTOUPDATE_NEWVERSION_ALERT_BUTTON1, ACAPI_GetOwnResModule ()),
        RSGetIndString (ID_AUTOUPDATE_STRINGS, ID_AUTOUPDATE_NEWVERSION_ALERT_BUTTON2, ACAPI_GetOwnResModule ()));

    if (response != DG_OK) {
        return false;
    }

    const GS::UniString& installerDownloadUrl = VersionChecker::LatestInstallerDownloadUrl ();
    if (installerDownloadUrl.IsEmpty ()) {
        DGAlert (DG_ERROR, "Tapir Update", "The Tapir Installer is missing from the latest release.",
            "Download the update from https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest", "OK");
        return false;
    }

#if defined (WINDOWS)
    const GS::UInt32 processId = static_cast<GS::UInt32> (GetCurrentProcessId ());
#else
    const GS::Int32 processId = static_cast<GS::Int32> (getpid ());
#endif
    const GS::UniString processIdStr = GS::ValueToUniString (processId);

    GS::UniString error;
    IO::Location installerFolder;
    if (!CreateInstallerFolder (processIdStr, installerFolder, error)) {
        DGAlert (DG_ERROR, "Tapir Update", "Failed to download the Tapir Installer.", error, "OK");
        return false;
    }

    IO::Location installerFile = installerFolder;
#if defined (WINDOWS)
    installerFile.AppendToLocal (IO::Name ("TapirInstaller_Win.exe"));
#else
    installerFile.AppendToLocal (IO::Name ("TapirInstaller_Mac.zip"));
#endif

    DownloadResult downloadResult = DownloadResult::Failed;
    {
        DownloadProcessWindow processWindow ("Tapir Update", "Downloading the Tapir Installer...");
        downloadResult = DownloadBinaryFile (installerDownloadUrl, installerFile, [&processWindow] (GS::UInt64 downloadedSize, GS::UInt64 totalSize) {
            return processWindow.SetProgress (downloadedSize, totalSize);
        }, error);
    }
    if (downloadResult == DownloadResult::Cancelled) {
        return false;
    }
    if (downloadResult != DownloadResult::Succeeded) {
        DGAlert (DG_ERROR, "Tapir Update", "Failed to download the Tapir Installer.", error, "OK");
        return false;
    }

    IO::Location addOnLocation;
    ACAPI_GetOwnLocation (&addOnLocation);
    GS::UniString addOnLocationStr;
    addOnLocation.ToPath (&addOnLocationStr);

    // The installer quits this Archicad, replaces exactly this Add-On and starts Archicad again.
    const GS::Array<GS::UniString> installerArguments = {
        "--addOnFile", addOnLocationStr,
        "--versions", GS::ValueToUniString (VersionChecker::ArchicadMainVersion ()),
        "--archicadPort", GS::ValueToUniString (GetConnectionPort ()),
        "--archicadPid", processIdStr };
    if (!StartInstaller (installerFile, addOnLocation, installerArguments, error)) {
        const GS::UniString manualUpdateText = "You can also download the new version from https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest and replace the Add-On manually.";
        if (error.IsEmpty ()) {
            // No error (Windows only): the user declined the administrator prompt, which the Add-On's folder needs.
            DGAlert (DG_INFORMATION, "Tapir Update", "The update was cancelled.",
                "The Tapir Add-On is in a folder that only an administrator can change. " + manualUpdateText, "OK");
        } else {
            DGAlert (DG_ERROR, "Tapir Update", "Failed to start the Tapir Installer.", error + "\n\n" + manualUpdateText, "OK");
        }
        return false;
    }

    return true;
}

#define PREFERENCES_VERSION 12
#define PREFERENCES_VERSION_BEFORE_SHORTCUT_LABELS 11
#define PREFERENCES_VERSION_BEFORE_SHORTCUT_SLOTS 10
static const GS::UniString ShortcutSlotEmptyMarker ("(unassigned)");   // never a valid file path; "" would be dropped by SkipEmptyParts
static const GS::UniString ShortcutLabelDefaultMarker ("(default)");   // means "use the default 'Run Script Shortcut N' text"; "" would be dropped by SkipEmptyParts

void TapirPalette::SaveScriptsToPreferences ()
{
    GS::UniString preferencesStr;
    if (hasAddedScript) {
        for (short i = 1; i <= scriptSelectionPopUp.GetItemCount () - 1 && !scriptSelectionPopUp.IsSeparator (i); ++i) {
            GS::Ref<PopUpItemData> popUpItemData = GS::DynamicCast<PopUpItemData> (scriptSelectionPopUp.GetItemObjectData (i));
            if (popUpItemData == nullptr) {
                continue;
            }

            GS::UniString pathStr;
            popUpItemData->fileLocation.ToPath (&pathStr);
            preferencesStr += pathStr + '\n';
        }
    }
    preferencesStr += GS::ValueToUniString (scriptSelectionPopUp.GetSelectedItem ());
    for (short slot = 0; slot < ScriptShortcutSlotCount; ++slot) {
        GS::UniString slotPathStr = ShortcutSlotEmptyMarker;
        if (!scriptShortcutSlots[slot].IsEmpty ()) {
            scriptShortcutSlots[slot].ToPath (&slotPathStr);
        }
        preferencesStr += '\n' + slotPathStr;
    }
    for (short slot = 0; slot < ScriptShortcutSlotCount; ++slot) {
        const GS::UniString& labelStr = scriptShortcutLabels[slot].IsEmpty () ? ShortcutLabelDefaultMarker : scriptShortcutLabels[slot];
        preferencesStr += '\n' + labelStr;
    }
    auto cStr = preferencesStr.ToCStr ();
    ACAPI_SetPreferences (PREFERENCES_VERSION, (GSSize)strlen (cStr.Get()), cStr.Get());
}

bool TapirPalette::IsValidLocation (const IO::Location& location)
{
    if (location.IsEmpty ()) {
        return false;
    }

    bool exists = false;
    GSErrCode err = IO::fileSystem.Contains (location, &exists);
    return err == NoError && exists;
}

short TapirPalette::AddScriptsFromPreferences ()
{
    if (hasAddedScript) {
        while (!scriptSelectionPopUp.IsSeparator (DG::PopUp::TopItem)) {
            scriptSelectionPopUp.DeleteItem (DG::PopUp::TopItem);
        }
    }
    for (short slot = 0; slot < ScriptShortcutSlotCount; ++slot) {
        scriptShortcutSlots[slot] = IO::Location ();
        scriptShortcutLabels[slot] = GS::EmptyUniString;
    }

    Int32 version;
    GSSize nBytes;

    ACAPI_GetPreferences (&version, &nBytes, nullptr);
    if (nBytes <= 0 || (version != PREFERENCES_VERSION && version != PREFERENCES_VERSION_BEFORE_SHORTCUT_LABELS && version != PREFERENCES_VERSION_BEFORE_SHORTCUT_SLOTS)) {
        return DG::PopUp::TopItem;
    }

    std::unique_ptr<char> data(new char[nBytes]);
    ACAPI_GetPreferences (&version, &nBytes, data.get ());
    GS::Array<GS::UniString> scriptPathArray;
    GS::UniString (data.get ()).Split ("\n", GS::UniString::SkipEmptyParts, &scriptPathArray);

    if (version == PREFERENCES_VERSION && scriptPathArray.GetSize () >= 2 * ScriptShortcutSlotCount) {
        // Labels were appended last, so they must be popped first.
        for (short slot = ScriptShortcutSlotCount - 1; slot >= 0; --slot) {
            GS::UniString labelStr = scriptPathArray.GetLast ();
            scriptPathArray.DeleteLast ();
            if (labelStr != ShortcutLabelDefaultMarker) {
                scriptShortcutLabels[slot] = labelStr;
            }
        }
        for (short slot = ScriptShortcutSlotCount - 1; slot >= 0; --slot) {
            GS::UniString slotPathStr = scriptPathArray.GetLast ();
            scriptPathArray.DeleteLast ();
            if (slotPathStr != ShortcutSlotEmptyMarker) {
                scriptShortcutSlots[slot] = IO::Location (slotPathStr);
            }
        }
    } else if (version == PREFERENCES_VERSION_BEFORE_SHORTCUT_LABELS && scriptPathArray.GetSize () >= ScriptShortcutSlotCount) {
        for (short slot = ScriptShortcutSlotCount - 1; slot >= 0; --slot) {
            GS::UniString slotPathStr = scriptPathArray.GetLast ();
            scriptPathArray.DeleteLast ();
            if (slotPathStr != ShortcutSlotEmptyMarker) {
                scriptShortcutSlots[slot] = IO::Location (slotPathStr);
            }
        }
    }

    ApplyAllShortcutMenuItemTexts ();

    short selectedItemBeforeClose = -1;
    if (!scriptPathArray.IsEmpty () && GS::UniStringToValue<short> (scriptPathArray.GetLast (), selectedItemBeforeClose, GS::ToValueMode::Strict)) {
        scriptPathArray.DeleteLast();
    }
    for (auto&& scriptPath : scriptPathArray) {
        IO::Location ownScript (scriptPath);
        if (!IsValidLocation (ownScript)) {
            continue;
        }
        if (!hasAddedScript) {
            scriptSelectionPopUp.InsertSeparator (DG::PopUp::TopItem);
            hasAddedScript = true;
        }
        AddScriptToPopUp (GS::NewRef<PopUpItemData> (ownScript));
    }
    return selectedItemBeforeClose;
}

bool TapirPalette::AddNewScript ()
{
    FTM::FileTypeManager	ftMan ("Python Script");
    FTM::GroupID			gid = ftMan.AddGroup ("Python Script");
    ftMan.AddType (FTM::FileType ("Python Script", "py", '    ', '    ', 0), gid);
    DG::FileDialog dlg (DG::FileDialog::OpenMultiFile);
    dlg.AddFilter (gid);
    dlg.SetHeader (RSGetIndString (ID_PALETTE_STRINGS, ID_PALETTE_STRINGS_SELECT_SCRIPTS_TITLE, ACAPI_GetOwnResModule ()));
    dlg.SetOKButtonText (RSGetIndString (ID_PALETTE_STRINGS, ID_PALETTE_STRINGS_SELECT_SCRIPTS_BUTTON, ACAPI_GetOwnResModule ()));
    if (!dlg.Invoke ()) {
        return false;
    }
    bool hasNew = false;
    for (UIndex i = 0; i < dlg.GetSelectionCount (); ++i) {
        IO::Location file = dlg.GetSelectedFile (i);
        if (IsPopUpContainsFile (file)) {
            continue;
        }
        if (!hasAddedScript) {
            scriptSelectionPopUp.InsertSeparator (DG::PopUp::TopItem);
            hasAddedScript = true;
        }
        AddScriptToPopUp (GS::NewRef<PopUpItemData> (file));
        hasNew = true;
    }

    if (hasNew) {
        scriptSelectionPopUp.SelectItem (DG::PopUp::TopItem);
        SaveScriptsToPreferences ();
        SetDeleteScriptButtonStatus ();
    }

    return hasNew;
}

void TapirPalette::DeleteScriptFromPopUp ()
{
    short selectedItem = scriptSelectionPopUp.GetSelectedItem ();
    scriptSelectionPopUp.DeleteItem (selectedItem);
    if (scriptSelectionPopUp.IsSeparator (DG::PopUp::TopItem)) {
        scriptSelectionPopUp.DeleteItem (DG::PopUp::TopItem);
        hasAddedScript = false;
    }
    if (scriptSelectionPopUp.IsSeparator (scriptSelectionPopUp.GetSelectedItem ())) {
        scriptSelectionPopUp.SelectItem (scriptSelectionPopUp.GetSelectedItem () - 1);
    }

    SaveScriptsToPreferences ();
    SetDeleteScriptButtonStatus ();
}

void TapirPalette::SetDeleteScriptButtonStatus ()
{
    if (!hasAddedScript) {
        delScriptButton.Disable ();
        return;
    }

    for (short i = scriptSelectionPopUp.GetSelectedItem (); i >= DG_POPUP_TOP; --i) {
        if (scriptSelectionPopUp.IsSeparator (i)) {
            delScriptButton.Disable ();
            return;
        }
    }

    delScriptButton.Enable ();
}