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
#include "ScriptRequirements.hpp"
#include "ScriptUIPalette.hpp"

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <initializer_list>
#include <map>
#include <regex>
#include <string>
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
std::atomic<bool>     TapirPalette::quitting (false);
std::mutex                    TapirPalette::postedTasksMutex;
std::vector<GS::RunnableTask> TapirPalette::postedTasks;
std::vector<GS::RunnableTask> TapirPalette::finishedTasks;

// True while UpdateAddOn runs. Archicad handles its events during the installer download, so the palette's
// Run button and the shortcut slots must not start a script that the installer would then interrupt.
static bool isUpdatingAddOn = false;

// True while the palette is hidden by Archicad (APIPalMsg_HidePalette_Begin, e.g. on a tab switch or Hide All Palettes)
// and was visible before. Only then does APIPalMsg_HidePalette_End show it again: a palette the user closed stays
// closed (#1915). Showing or hiding it any other way clears it.
static bool isHiddenByArchicad = false;

// The script repositories are read from GitHub while Archicad starts, on the main thread. Without a timeout
// a connection that GitHub accepts but never answers kept Archicad from starting or opening a project (#798).
static constexpr int RepositoryRequestTimeoutMs = 15 * 1000;

// The texts of the user interface, from the localized resources (RINT/AddOn.grc, see LOCALIZATION.md)
static GS::UniString PaletteText (Int32 index)
{
    return RSGetIndString (ID_PALETTE_STRINGS, index, ACAPI_GetOwnResModule ());
}

static GS::UniString SearchText (Int32 index)
{
    return RSGetIndString (ID_PALETTE_SEARCH_STRINGS, index, ACAPI_GetOwnResModule ());
}

static GS::UniString UpdateText (Int32 index)
{
    return RSGetIndString (ID_AUTOUPDATE_STRINGS, index, ACAPI_GetOwnResModule ());
}

static GS::UniString ReportText (Int32 index)
{
    return RSGetIndString (ID_REPORT_STRINGS, index, ACAPI_GetOwnResModule ());
}

static UShort GetConnectionPort ()
{
    UShort portNumber;
    ACAPI_Command_GetHttpConnectionPort (&portNumber);
    return portNumber;
}

// Builds a UniString from the raw bytes of a script's output, replacing invalid UTF-8
// sequences with '?'. GSRoot's UTF-8 assert (Unicode.cpp:343) must never fire: its report
// path converts the same bad string again and can recurse into a stack overflow that kills
// Archicad (WER 0xc00000fd in GSRoot.dll). Script output is not always valid UTF-8: Python
// writes to a pipe in the system code page (for example cp932 tracebacks on Japanese
// Windows), and a multi-byte character can be split at a read-chunk boundary.
static GS::UniString SafeUTF8String (const char* bytes, GS::USize size)
{
    std::string clean;
    clean.reserve (size);
    GS::USize i = 0;
    while (i < size) {
        const unsigned char c = static_cast<unsigned char> (bytes[i]);
        // Well-formed sequences as in Table 3-7 of the Unicode Standard: the lead byte gives
        // the length, and the range of the second byte also excludes overlong encodings,
        // UTF-16 surrogates and code points above U+10FFFF.
        GS::USize seqLen = 0;
        unsigned char secondMin = 0x80;
        unsigned char secondMax = 0xBF;
        if (c < 0x80) {
            seqLen = 1;
        } else if (c >= 0xC2 && c <= 0xDF) {
            seqLen = 2;
        } else if (c >= 0xE0 && c <= 0xEF) {
            seqLen = 3;
            if (c == 0xE0) {
                secondMin = 0xA0;
            } else if (c == 0xED) {
                secondMax = 0x9F;
            }
        } else if (c >= 0xF0 && c <= 0xF4) {
            seqLen = 4;
            if (c == 0xF0) {
                secondMin = 0x90;
            } else if (c == 0xF4) {
                secondMax = 0x8F;
            }
        }
        bool valid = seqLen > 0 && i + seqLen <= size;
        for (GS::USize j = 1; valid && j < seqLen; ++j) {
            const unsigned char next = static_cast<unsigned char> (bytes[i + j]);
            valid = (j == 1) ? (next >= secondMin && next <= secondMax) : ((next & 0xC0) == 0x80);
        }
        if (valid) {
            clean.append (bytes + i, seqLen);
            i += seqLen;
        } else {
            clean.push_back ('?');
            ++i;
        }
    }
    return GS::UniString (clean.c_str (), static_cast<GS::USize> (clean.size ()), CC_UTF8);
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
    clientConnection.SetTimeout (RepositoryRequestTimeoutMs);
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
                error = GS::UniString::Printf (UpdateText (ID_AUTOUPDATE_ERROR_INVALID_URL), url.ToPrintf ());
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
                    error = GS::UniString::Printf (UpdateText (ID_AUTOUPDATE_ERROR_REDIRECT_NO_LOCATION), origin.ToPrintf ());
                    return DownloadResult::Failed;
                }
                url = ResolveRedirectLocation (location, origin, pathAndQuery);
                continue;
            }
            if (statusCode != 200) {
                error = GS::UniString::Printf (UpdateText (ID_AUTOUPDATE_ERROR_HTTP_STATUS), statusCode, origin.ToPrintf ());
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
                    error = GS::UniString::Printf (UpdateText (ID_AUTOUPDATE_ERROR_CANNOT_WRITE), filePath.ToPrintf ());
                    return DownloadResult::Failed;
                }

                std::vector<char> buffer (64 * 1024);
                USize readSize = ReadBodyChunk (body, buffer.data (), static_cast<USize> (buffer.size ()), hasContentLength, readError);
                while (readSize > 0) {
                    if (file.WriteBin (buffer.data (), readSize) != NoError) {
                        error = UpdateText (ID_AUTOUPDATE_ERROR_WRITE_FAILED);
                        return DownloadResult::Failed;
                    }
                    writtenSize += readSize;
                    if (!progress (writtenSize, totalSize)) {
                        return DownloadResult::Cancelled;
                    }
                    readSize = ReadBodyChunk (body, buffer.data (), static_cast<USize> (buffer.size ()), hasContentLength, readError);
                }
                if (file.Close () != NoError) {
                    error = UpdateText (ID_AUTOUPDATE_ERROR_WRITE_FAILED);
                    return DownloadResult::Failed;
                }
            }

            if (hasContentLength && contentLength != writtenSize) {
                error = UpdateText (ID_AUTOUPDATE_ERROR_INCOMPLETE);
                if (!readError.IsEmpty ()) {
                    error += " " + readError;
                }
                return DownloadResult::Failed;
            }
            if (writtenSize == 0) {
                error = UpdateText (ID_AUTOUPDATE_ERROR_EMPTY);
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
        error = UpdateText (ID_AUTOUPDATE_ERROR_TOO_MANY_REDIRECTS);
    } catch (const GS::Exception& e) {
        error = GetExceptionText (e);
    } catch (...) {
        error = UpdateText (ID_AUTOUPDATE_ERROR_UNEXPECTED);
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
        clientConnection.SetTimeout (RepositoryRequestTimeoutMs);
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
    } catch (const GS::Exception& e) {
        ACAPI_WriteReport (ReportText (ID_REPORT_STRINGS_LIST_FAILED_WITH_ERROR), false,
            repository.repoOwner.ToPrintf (), repository.repoName.ToPrintf (), GetExceptionText (e).ToPrintf ());
    } catch (...) {
        ACAPI_WriteReport (ReportText (ID_REPORT_STRINGS_LIST_FAILED), false, repository.repoOwner.ToPrintf (), repository.repoName.ToPrintf ());
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
    , installedVersionText (GetReference (), 8)
    , checkForUpdatesButton (GetReference (), 9)
    , searchEdit (GetReference (), ID_PALETTE_SEARCH_EDIT)
    , favouriteButton (GetReference (), ID_PALETTE_FAVOURITE_BUTTON)
    , searchButton (GetReference (), ID_PALETTE_SEARCH_BUTTON)
{
    Attach (*this);
    AttachToAllItems (*this);

    // The search field opens with the search button, in the left part of the dropdown's place (SetSearchOpen)
    searchEdit.Hide ();
    // The bar has no Open in Browser button any more; its dialog item stays, hidden, so that the items after it
    // keep their numbers, and its room went to the dropdown.
    openScriptButton.Hide ();

    paletteState.Load ();
    LoadScriptsToPopUp ();

    installedVersionText.SetText (GS::UniString::Printf (
        UpdateText (ID_AUTOUPDATE_INSTALLED_VERSION_TEXT), ADDON_VERSION));
    SetRunButtonIcon ();
    SetUpdateButtonIcon ();
    SetSearchButtonIcon ();

    BeginEventProcessing ();
}

TapirPalette::~TapirPalette ()
{
    SetSearchHotKeys (false);
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

void TapirPalette::StopRunningScriptOnQuit ()
{
    if (!HasInstance ()) {
        return;
    }
    // The output reader checks this before it touches the process or posts anything to the message loop: a task
    // posted now would only run after the add-on has been unloaded.
    quitting = true;
    // Every running script ends, with the processes it started; then each output reader is given up to 2 seconds.
    TapirPalette& palette = Instance ();
    for (const std::unique_ptr<RunningScript>& script : palette.runningScripts) {
        if (!script->ended) {
            try {
                palette.KillScript (*script, false);
            } catch (...) {
            }
        }
    }
    for (const std::unique_ptr<RunningScript>& script : palette.runningScripts) {
        script->executor.Wait (GS::Timeout (0, 0, 2));
    }

    // Output posted while Archicad was already closing is still queued, and the message loop does not run it any
    // more: Archicad would destroy those tasks after this add-on is unloaded and crash in their destructors. Keep
    // them alive on purpose (never freed), so that discarding them later does not call into the unloaded add-on.
    std::lock_guard<std::mutex> lock (postedTasksMutex);
    finishedTasks.clear ();
    if (!postedTasks.empty ()) {
        new std::vector<GS::RunnableTask> (std::move (postedTasks));
        postedTasks.clear ();
    }
}

void TapirPalette::Release ()
{
    if (!HasInstance ()) {
        return;
    }
    // An output reader that did not stop in StopRunningScriptOnQuit still uses its script and the palette:
    // keep them alive on purpose (never freed) rather than destroying its executor under the running thread.
    for (const std::unique_ptr<RunningScript>& script : instance->runningScripts) {
        if (!script->executor.Wait (GS::Timeout (0, 0, 0, 0))) {
            new GS::Ref<TapirPalette> (instance);
            break;
        }
    }
    instance = nullptr;
}

void TapirPalette::KeepPostedTask (const GS::RunnableTask& task)
{
    std::lock_guard<std::mutex> lock (postedTasksMutex);
    finishedTasks.clear ();
    postedTasks.push_back (task);
}

void TapirPalette::ForgetPostedTask (const GS::Runnable* task)
{
    std::lock_guard<std::mutex> lock (postedTasksMutex);
    for (auto it = postedTasks.begin (); it != postedTasks.end (); ++it) {
        if (it->operator-> () == task) {
            finishedTasks.push_back (*it);
            postedTasks.erase (it);
            return;
        }
    }
}

void TapirPalette::Show ()
{
    isHiddenByArchicad = false;
    DG::Palette::Show ();
    SetMenuItemCheckedState (true);
}

void TapirPalette::Hide ()
{
    isHiddenByArchicad = false;
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
    // The saved selection, else the one before (SelectScript finds it by its file), else the first script
    const GS::Ref<PopUpItemData> selectedBefore = GetSelectedScript ();
    const short savedItem = AddScriptsFromPreferences ();
    GS::Ref<PopUpItemData> scriptToSelect = GetSavedSelection (savedItem);
    if (scriptToSelect == nullptr) {
        scriptToSelect = selectedBefore;
    }
    RebuildScriptPopUp (scriptToSelect);
    RefreshScriptListShortcutLabels ();
    SetRunButtonIcon ();
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
        GS::Ref<PopUpItemData> popUpItemData = GetSelectedScript ();
        if (popUpItemData != nullptr) {
            RunOrStopScript (*popUpItemData);
        }
    } else if (ev.GetSource () == &tapirButton) {
        OpenWebpage ("https://tapir-api.com");   // redirects to the Tapir website
    } else if (ev.GetSource () == &openScriptButton) {
        GS::Ref<PopUpItemData> popUpItemData = GetSelectedScript ();
        if (popUpItemData == nullptr) {
            return;
        }
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
    } else if (ev.GetSource () == &checkForUpdatesButton) {
        CheckForUpdates ();
    } else if (ev.GetSource () == &favouriteButton) {
        ToggleFavourite ();
    } else if (ev.GetSource () == &searchButton) {
        SetSearchOpen (!searchEdit.IsVisible ());
    }
}

void TapirPalette::ToggleFavourite ()
{
    const GS::Ref<PopUpItemData> selectedScript = GetSelectedScript ();
    if (selectedScript == nullptr) {
        return;
    }
    const GS::UniString scriptPath = GetScriptPath (*selectedScript);
    paletteState.SetFavourite (scriptPath, !paletteState.IsFavourite (scriptPath));
    RebuildScriptPopUp (selectedScript);   // it moves into or out of the favourites section, and stays selected
    SelectedScriptChanged ();              // palette.json gets the favourites as well
}

void TapirPalette::SetFavouriteButtonIcon ()
{
    const GS::Ref<PopUpItemData> selectedScript = GetSelectedScript ();
    const bool isFavourite = selectedScript != nullptr && paletteState.IsFavourite (GetScriptPath (*selectedScript));
    favouriteButton.SetIcon (DG::Icon (ACAPI_GetOwnResModule (), isFavourite ? ID_STARFILLED_BUTTON_ICON : ID_STAR_BUTTON_ICON));
    if (selectedScript != nullptr) {
        favouriteButton.Enable ();
    } else {
        favouriteButton.Disable ();
    }
}

void TapirPalette::SelectedScriptChanged ()
{
    SaveScriptsToPreferences ();
    SetDeleteScriptButtonStatus ();
    SetRunButtonIcon ();   // Stop when the newly selected script runs
    SetFavouriteButtonIcon ();
}

void TapirPalette::MoveSelection (bool down)
{
    const Int32 itemCount = static_cast<Int32> (visibleItems.GetSize ());
    const Int32 step = down ? 1 : -1;
    for (Int32 index = static_cast<Int32> (scriptSelectionPopUp.GetSelectedItem ()) - DG::PopUp::TopItem + step;
        index >= 0 && index < itemCount; index += step) {
        if (visibleItems[static_cast<UIndex> (index)].kind == VisibleItem::Kind::Script) {
            scriptSelectionPopUp.SelectItem (static_cast<short> (DG::PopUp::TopItem + index));
            SelectedScriptChanged ();
            return;
        }
    }
}

// The search: filters the dropdown as it is typed. The selected script stays selected while it matches.
void TapirPalette::TextEditChanged (const DG::TextEditChangeEvent& ev)
{
    if (ev.GetSource () != &searchEdit) {
        return;
    }
    RebuildScriptPopUp (GetSelectedScript ());
    SelectedScriptChanged ();
    SetSearchButtonIcon ();
}

GS::UniString TapirPalette::GetSearchText () const
{
    GS::UniString search = searchEdit.GetText ();
    search.Trim ();
    return search;
}

// The field takes the left part of the dropdown's place and the dropdown gets narrower, so the bar keeps its width
// (it may be docked in a toolbar). Closing it keeps the filter; the search button shows that one is applied.
void TapirPalette::SetSearchOpen (bool open)
{
    if (open == searchEdit.IsVisible ()) {
        if (open) {
            searchEdit.SetFocus ();
        }
        return;
    }
    const short shift = static_cast<short> (searchEdit.GetWidth () + 4);
    if (open) {
        scriptSelectionPopUp.MoveAndResize (shift, 0, -shift, 0);
        searchEdit.Show ();
        searchEdit.SetFocus ();
    } else {
        searchEdit.Hide ();
        scriptSelectionPopUp.MoveAndResize (-shift, 0, shift, 0);
    }
}

void TapirPalette::ClearSearch ()
{
    searchEdit.SetText (GS::EmptyUniString);
    RebuildScriptPopUp (GetSelectedScript ());
    SelectedScriptChanged ();
    SetSearchButtonIcon ();
    SetSearchOpen (false);
}

void TapirPalette::SetSearchButtonIcon ()
{
    const short resId = GetSearchText ().IsEmpty () ? ID_SEARCH_BUTTON_ICON : ID_SEARCHACTIVE_BUTTON_ICON;
    searchButton.SetIcon (DG::Icon (ACAPI_GetOwnResModule (), resId));
}

void TapirPalette::SetSearchHotKeys (bool registered)
{
    short* const hotKeys[] = { &returnHotKey, &enterHotKey, &escapeHotKey };
    if (!registered) {
        for (short* hotKey : hotKeys) {
            if (*hotKey != 0) {
                UnregisterHotKey (*hotKey);
                *hotKey = 0;
            }
        }
        if (searchKeyHandlerInstalled) {
            DG::LowLevelKeyboardHandler::RemoveHandler (searchKeyHandler);
            searchKeyHandlerInstalled = false;
        }
        return;
    }
    if (returnHotKey == 0) {
        returnHotKey = RegisterHotKey (DG::Key::Return);
    }
    if (enterHotKey == 0) {
        enterHotKey = RegisterHotKey (DG::Key::Enter);
    }
    if (escapeHotKey == 0) {
        escapeHotKey = RegisterHotKey (DG::Key::Escape);
    }
    // Tracked here: DG::LowLevelKeyboardHandler::IsHandlerInstalled exists only from Archicad 28.
    if (!searchKeyHandlerInstalled) {
        DG::LowLevelKeyboardHandler::InstallHandler (searchKeyHandler);
        searchKeyHandlerInstalled = true;
    }
}

DG::EventHandlerResultType TapirPalette::SearchKeyHandler::KeyPressed (const DG::Key::Code& code, DG::Item** /*item*/)
{
    if (!palette.searchEdit.IsFocused () || !code.IsSpecial ()) {
        return DG::NotProcessedEvent;
    }
    if (code.GetSpecialKey () == DG::Key::Up || code.GetSpecialKey () == DG::Key::Down) {
        palette.MoveSelection (code.GetSpecialKey () == DG::Key::Down);
        return DG::ProcessedEvent;
    }
    return DG::NotProcessedEvent;
}

// The keys are registered only while the search field has the focus, so the arrows and Enter work as usual
// everywhere else.
void TapirPalette::ItemFocusGained (const DG::ItemFocusEvent& ev)
{
    if (ev.GetSource () == &searchEdit) {
        SetSearchHotKeys (true);
    }
}

void TapirPalette::ItemFocusLost (const DG::ItemFocusEvent& ev)
{
    if (ev.GetSource () == &searchEdit) {
        SetSearchHotKeys (false);
    }
}

// While the search field has the focus: Enter runs the selected script, Escape clears the search and closes the
// field. Up and Down come through searchKeyHandler.
void TapirPalette::PanelHotkeyPressed (const DG::PanelHotKeyEvent& ev, bool* processed)
{
    if (processed == nullptr || !searchEdit.IsFocused ()) {
        return;
    }
    const short keyId = ev.GetKeyId ();
    if (keyId == 0) {
        return;
    }
    if (keyId == escapeHotKey) {
        ClearSearch ();
        *processed = true;
    } else if (keyId == returnHotKey || keyId == enterHotKey) {
        GS::Ref<PopUpItemData> popUpItemData = GetSelectedScript ();
        if (popUpItemData != nullptr) {
            RunOrStopScript (*popUpItemData);
        }
        *processed = true;
    }
}

bool TapirPalette::IsPopUpContainsFile (const IO::Location& fileLocation) const
{
    for (const GS::Ref<PopUpItemData>& popUpItemData : GetAllScripts ()) {
        if (popUpItemData->fileLocation == fileLocation) {
            return true;
        }
    }
    return false;
}

TapirPalette::ScriptList TapirPalette::GetAllScripts () const
{
    ScriptList scripts;
    for (const ScriptList* list : { &addedScripts, &customScripts, &listedScripts }) {
        for (const GS::Ref<PopUpItemData>& popUpItemData : *list) {
            scripts.Push (popUpItemData);
        }
    }
    return scripts;
}

static GS::UniString LowerCased (const GS::UniString& text)
{
#ifdef ServerMainVers_3000
    return text.GetLowerCased ();
#else
    return text.ToLowerCase ();
#endif
}

// The dropdown's sections: each list followed by its separator when shown, then "Reload scripts...".
TapirPalette::VisibleItemList TapirPalette::BuildVisibleItems (bool forDropdown) const
{
    GS::UniString search;
    if (forDropdown) {
        search = LowerCased (GetSearchText ());
    }
    const bool isSearching = !search.IsEmpty ();

    // The scripts of the list that are shown in the section: the favourites only in their own section, and while
    // searching only the matching ones, those whose name matches first, then those whose summary does.
    const auto getSectionScripts = [&] (const ScriptList& scripts, bool favouritesSection) {
        ScriptList nameMatches;
        ScriptList summaryMatches;
        for (const GS::Ref<PopUpItemData>& popUpItemData : scripts) {
            const bool isFavourite = forDropdown && paletteState.IsFavourite (GetScriptPath (*popUpItemData));
            if (isFavourite != favouritesSection) {
                continue;
            }
            if (!isSearching || LowerCased (popUpItemData->baseDisplayText).Contains (search)) {
                nameMatches.Push (popUpItemData);
            } else if (LowerCased (popUpItemData->info.summary).Contains (search)) {
                summaryMatches.Push (popUpItemData);
            }
        }
        nameMatches.Append (summaryMatches);
        return nameMatches;
    };

    VisibleItemList items;
    bool hasScript = false;
    // A section and its separator. In the dropdown a separator follows a section only when it shows a script;
    // otherwise, as every Tapir version did, when the section exists.
    const auto appendSection = [&] (const ScriptList& scripts, bool hasSeparator) {
        for (const GS::Ref<PopUpItemData>& popUpItemData : scripts) {
            items.Push (VisibleItem (VisibleItem::Kind::Script, popUpItemData));
            hasScript = true;
        }
        if (forDropdown ? !scripts.IsEmpty () : hasSeparator) {
            items.Push (VisibleItem (VisibleItem::Kind::Separator));
        }
    };
    if (forDropdown) {
        appendSection (getSectionScripts (GetAllScripts (), true), true);
    }
    appendSection (getSectionScripts (addedScripts, false), hasAddedScript);
    appendSection (getSectionScripts (customScripts, false), hasCustomScript);
    const ScriptList listed = getSectionScripts (listedScripts, false);
    for (const GS::Ref<PopUpItemData>& popUpItemData : listed) {
        items.Push (VisibleItem (VisibleItem::Kind::Script, popUpItemData));
        hasScript = true;
    }

    if (isSearching && !hasScript) {
        items.Push (VisibleItem (VisibleItem::Kind::NoMatch));
    }
    items.Push (VisibleItem (VisibleItem::Kind::Separator));
    items.Push (VisibleItem (VisibleItem::Kind::Reload));

    // The dropdown never shows two separators in a row (a section followed by an empty one)
    if (forDropdown) {
        for (UIndex i = items.GetSize () - 1; i > 0; --i) {
            if (items[i].kind == VisibleItem::Kind::Separator && items[i - 1].kind == VisibleItem::Kind::Separator) {
                items.Delete (i);
            }
        }
    }
    return items;
}

const TapirPalette::VisibleItem* TapirPalette::GetVisibleItem (short item) const
{
    if (item < DG::PopUp::TopItem || static_cast<UIndex> (item - DG::PopUp::TopItem) >= visibleItems.GetSize ()) {
        return nullptr;
    }
    return &visibleItems[static_cast<UIndex> (item - DG::PopUp::TopItem)];
}

GS::Ref<TapirPalette::PopUpItemData> TapirPalette::GetScriptOfPopUpItem (short item) const
{
    const VisibleItem* visibleItem = GetVisibleItem (item);
    if (visibleItem == nullptr || visibleItem->kind != VisibleItem::Kind::Script) {
        return GS::Ref<PopUpItemData> ();
    }
    return visibleItem->script;
}

GS::Ref<TapirPalette::PopUpItemData> TapirPalette::GetSelectedScript () const
{
    return GetScriptOfPopUpItem (scriptSelectionPopUp.GetSelectedItem ());
}

bool TapirPalette::IsAddedScript (const GS::Ref<PopUpItemData>& script) const
{
    if (script == nullptr) {
        return false;
    }
    for (const GS::Ref<PopUpItemData>& popUpItemData : addedScripts) {
        if (popUpItemData == script) {
            return true;
        }
    }
    return false;
}

GS::UniString TapirPalette::GetScriptLabel (const PopUpItemData& popUpData) const
{
    GS::UniString label = popUpData.baseDisplayText;
    for (short slot = 0; slot < ScriptShortcutSlotCount; ++slot) {
        if (!scriptShortcutSlots[slot].IsEmpty () && scriptShortcutSlots[slot] == popUpData.fileLocation) {
            label += "   " + GS::UniString::Printf (PaletteText (ID_PALETTE_STRINGS_SHORTCUT_SUFFIX), (int) (slot + 1));
            break;
        }
    }
    return label;
}

void TapirPalette::RebuildScriptPopUp (const GS::Ref<PopUpItemData>& scriptToSelect)
{
    visibleItems = BuildVisibleItems (true);
    scriptSelectionPopUp.DeleteItem (DG_ALL_ITEMS);
    for (const VisibleItem& visibleItem : visibleItems) {
        switch (visibleItem.kind) {
            case VisibleItem::Kind::Script:
                scriptSelectionPopUp.AppendItem ();
                scriptSelectionPopUp.SetItemText (DG::PopUp::BottomItem, GetScriptLabel (*visibleItem.script));
                break;
            case VisibleItem::Kind::Reload:
                scriptSelectionPopUp.AppendItem ();
                scriptSelectionPopUp.SetItemText (DG::PopUp::BottomItem, PaletteText (ID_PALETTE_STRINGS_RELOAD_SCRIPTS));
                break;
            case VisibleItem::Kind::NoMatch:
                scriptSelectionPopUp.AppendItem ();
                scriptSelectionPopUp.SetItemText (DG::PopUp::BottomItem, SearchText (ID_PALETTE_SEARCH_STRINGS_NO_MATCH));
                scriptSelectionPopUp.DisableItem (DG::PopUp::BottomItem);
                break;
            case VisibleItem::Kind::Separator:
            default:
                scriptSelectionPopUp.AppendSeparator ();
                break;
        }
    }

    SelectScript (scriptToSelect);
    RefreshRunningMarks ();
    SetRunButtonIcon ();   // the selected script may have changed
    SetFavouriteButtonIcon ();
}

void TapirPalette::SelectScript (const GS::Ref<PopUpItemData>& script)
{
    short itemToSelect = 0;
    short firstScriptItem = 0;
    for (UIndex i = 0; i < visibleItems.GetSize (); ++i) {
        const VisibleItem& visibleItem = visibleItems[i];
        if (visibleItem.kind != VisibleItem::Kind::Script) {
            continue;
        }
        const short item = static_cast<short> (DG::PopUp::TopItem + i);
        if (firstScriptItem == 0) {
            firstScriptItem = item;
        }
        if (script != nullptr && visibleItem.script->fileLocation == script->fileLocation) {
            itemToSelect = item;
            break;
        }
    }
    if (itemToSelect == 0) {
        itemToSelect = firstScriptItem;
    }
    // Without a script the first item, as before: "Reload scripts..." is never selected, so choosing it is a change
    scriptSelectionPopUp.SelectItem (itemToSelect != 0 ? itemToSelect : static_cast<short> (DG::PopUp::TopItem));
}

// DG::PopUp has no right-aligned column or tab stop for a mark after the name, so the mark is the item's icon,
// drawn before the name. Every other item gets an empty icon of the same size, so the names stay aligned.
void TapirPalette::RefreshRunningMarks ()
{
    const DG::Icon runningIcon (ACAPI_GetOwnResModule (), ID_SCRIPT_RUNNING_ICON);
    const DG::Icon notRunningIcon (ACAPI_GetOwnResModule (), ID_SCRIPT_NOT_RUNNING_ICON);
    for (UIndex i = 0; i < visibleItems.GetSize (); ++i) {
        const VisibleItem& visibleItem = visibleItems[i];
        if (visibleItem.kind == VisibleItem::Kind::Separator) {
            continue;
        }
        const bool isRunning = visibleItem.kind == VisibleItem::Kind::Script && IsScriptRunning (visibleItem.script->fileLocation);
        scriptSelectionPopUp.SetItemIcon (static_cast<short> (DG::PopUp::TopItem + i), isRunning ? runningIcon : notRunningIcon);
    }
}

void TapirPalette::PopUpChanged (const DG::PopUpChangeEvent& ev)
{
    if (ev.GetSource () == &scriptSelectionPopUp) {
        const VisibleItem* selectedItem = GetVisibleItem (scriptSelectionPopUp.GetSelectedItem ());

        if (selectedItem != nullptr && selectedItem->kind == VisibleItem::Kind::Reload) {
            // The script selected before stays selected while it is still listed, found by its file
            const GS::Ref<PopUpItemData> previousScript = GetScriptOfPopUpItem (ev.GetPreviousSelection ());
            LoadScriptsToPopUp ();
            SelectScript (previousScript);
        }

        SelectedScriptChanged ();
    }
}

// min-tapir-version of the script's header against the installed Tapir (ScriptRequirements.hpp).
static ScriptRequirements::MinTapirVersionResult CheckMinTapirVersion (const ScriptInfo& info)
{
    return ScriptRequirements::CheckMinTapirVersion (info.minTapirVersion.ToCStr (CC_UTF8).Get (), ADDON_VERSION);
}

// False only when the script's header lists Archicad versions and the running one is not among them.
static bool IsRunningArchicadListed (const ScriptInfo& info)
{
    std::vector<int> versions;
    for (UIndex i = 0; i < info.archicadVersions.GetSize (); ++i) {
        versions.push_back (static_cast<int> (info.archicadVersions[i]));
    }
    return ScriptRequirements::IsArchicadVersionListed (versions, static_cast<int> (VersionChecker::ArchicadMainVersion ()));
}

// "28, 29, 30"
static GS::UniString JoinArchicadVersions (const GS::Array<Int32>& versions)
{
    GS::UniString result;
    for (UIndex i = 0; i < versions.GetSize (); ++i) {
        if (i > 0) {
            result += ", ";
        }
        result += GS::ValueToUniString (versions[i]);
    }
    return result;
}

GS::UniString TapirPalette::GetScriptDescription (const ScriptInfo& info)
{
    GS::UniString badge;
    if (info.ChangesModel ()) {
        badge = PaletteText (ID_PALETTE_STRINGS_BADGE_CHANGES_MODEL);
    } else if (info.WritesFiles ()) {
        badge = PaletteText (ID_PALETTE_STRINGS_BADGE_WRITES_FILES);
    } else if (info.IsReadOnly ()) {
        badge = PaletteText (ID_PALETTE_STRINGS_BADGE_READ_ONLY);
    }

    GS::UniString description = info.summary;
    const auto addLine = [&description] (const GS::UniString& line) {
        if (line.IsEmpty ()) {
            return;
        }
        if (!description.IsEmpty ()) {
            description += "\n";
        }
        description += line;
    };
    addLine (badge);

    // The requirements of the `# /// tapir` header, each with what the running Tapir or Archicad is when it does not
    // satisfy it. A key that is absent or malformed shows nothing.
    switch (CheckMinTapirVersion (info)) {
        case ScriptRequirements::MinTapirVersionResult::Satisfied:
            addLine (GS::UniString::Printf (PaletteText (ID_PALETTE_STRINGS_NEEDS_TAPIR),
                info.minTapirVersion.ToPrintf ()));
            break;
        case ScriptRequirements::MinTapirVersionResult::TooOld:
            addLine (GS::UniString::Printf (PaletteText (ID_PALETTE_STRINGS_NEEDS_TAPIR_NOT_MET),
                info.minTapirVersion.ToPrintf (), ADDON_VERSION));
            break;
        default:
            break;
    }
    if (!info.archicadVersions.IsEmpty ()) {
        const GS::UniString versionList = JoinArchicadVersions (info.archicadVersions);
        if (IsRunningArchicadListed (info)) {
            addLine (GS::UniString::Printf (PaletteText (ID_PALETTE_STRINGS_TESTED_ON_AC),
                versionList.ToPrintf ()));
        } else {
            addLine (GS::UniString::Printf (PaletteText (ID_PALETTE_STRINGS_TESTED_ON_AC_NOT_MET),
                versionList.ToPrintf (), static_cast<int> (VersionChecker::ArchicadMainVersion ())));
        }
    }
    return description;
}

// Runs before every script start (ExecuteScript), so the palette's Run button and the shortcut slots follow the same
// rule. A script that needs a newer Tapir does not run; one not tested on this Archicad runs after a warning.
bool TapirPalette::ConfirmScriptRequirements (const PopUpItemData& popUpItemData)
{
    const ScriptInfo& info = popUpItemData.info;

    GS::UniString scriptName;
    IO::Name name;
    if (popUpItemData.fileLocation.GetLastLocalName (&name) == NoError) {
        scriptName = name.ToString ();
    }
    const GS::UniString title = PaletteText (ID_PALETTE_STRINGS_REQUIREMENT_TITLE);

    switch (CheckMinTapirVersion (info)) {
        case ScriptRequirements::MinTapirVersionResult::Malformed:
            WriteReport (DG_WARNING, PaletteText (ID_PALETTE_STRINGS_MIN_TAPIR_MALFORMED),
                scriptName.ToPrintf (), info.minTapirVersion.ToPrintf ());
            break;
        case ScriptRequirements::MinTapirVersionResult::TooOld:
            {
                const short response = DGAlert (DG_WARNING, title,
                    GS::UniString::Printf (PaletteText (ID_PALETTE_STRINGS_TAPIR_TOO_OLD_TEXT),
                        scriptName.ToPrintf (), info.minTapirVersion.ToPrintf (), ADDON_VERSION),
                    GS::EmptyUniString,
                    PaletteText (ID_PALETTE_STRINGS_CHECK_FOR_UPDATES),
                    PaletteText (ID_PALETTE_STRINGS_CANCEL));
                if (response == DG_OK) {
                    CheckForUpdates ();
                }
            }
            return false;
        default:
            break;
    }

    if (!IsRunningArchicadListed (info)) {
        const short response = DGAlert (DG_WARNING, title,
            GS::UniString::Printf (PaletteText (ID_PALETTE_STRINGS_AC_NOT_LISTED_TEXT1),
                scriptName.ToPrintf (), JoinArchicadVersions (info.archicadVersions).ToPrintf (), static_cast<int> (VersionChecker::ArchicadMainVersion ())),
            PaletteText (ID_PALETTE_STRINGS_AC_NOT_LISTED_TEXT2),
            PaletteText (ID_PALETTE_STRINGS_RUN_ANYWAY),
            PaletteText (ID_PALETTE_STRINGS_CANCEL));
        if (response != DG_OK) {
            return false;
        }
    }
    return true;
}

// The selected script's summary and badge, on the script list and on the Run button (while the selected script
// does not run, when the button is Stop), and the update button's "Check for Updates" or "Update to <version>".
void TapirPalette::ItemToolTipRequested (const DG::ItemHelpEvent& ev, GS::UniString* toolTipText)
{
    if (toolTipText != nullptr && ev.GetSource () == &checkForUpdatesButton) {
        if (VersionChecker::IsUsingLatestVersion ()) {
            *toolTipText = UpdateText (ID_AUTOUPDATE_CHECK_BUTTON_TEXT);
        } else {
            *toolTipText = GS::UniString::Printf (UpdateText (ID_AUTOUPDATE_UPDATE_BUTTON_TEXT), VersionChecker::LatestVersion ().ToPrintf ());
        }
        return;
    }
    if (toolTipText != nullptr && ev.GetSource () == &favouriteButton) {
        const GS::Ref<PopUpItemData> selectedScript = GetSelectedScript ();
        const bool isFavourite = selectedScript != nullptr && paletteState.IsFavourite (GetScriptPath (*selectedScript));
        *toolTipText = SearchText (isFavourite ? ID_PALETTE_SEARCH_STRINGS_REMOVE_FAVOURITE : ID_PALETTE_SEARCH_STRINGS_ADD_FAVOURITE);
        return;
    }
    // DG's text edit has no placeholder text, so the field tells what it is for in its tooltip
    if (toolTipText != nullptr && ev.GetSource () == &searchEdit) {
        *toolTipText = SearchText (ID_PALETTE_SEARCH_STRINGS_PLACEHOLDER);
        return;
    }
    // The search button says what the list is filtered by, also while the field is closed
    if (toolTipText != nullptr && ev.GetSource () == &searchButton) {
        const GS::UniString search = GetSearchText ();
        *toolTipText = search.IsEmpty ()
            ? SearchText (ID_PALETTE_SEARCH_STRINGS_PLACEHOLDER)
            : GS::UniString::Printf (SearchText (ID_PALETTE_SEARCH_STRINGS_FILTERED), search.ToPrintf ());
        return;
    }

    const bool isPopUp = ev.GetSource () == &scriptSelectionPopUp;
    const bool isRunButton = ev.GetSource () == &runScriptButton;
    if (toolTipText == nullptr || !(isPopUp || isRunButton)) {
        return;
    }

    GS::Ref<PopUpItemData> popUpItemData = GetSelectedScript ();
    if (popUpItemData == nullptr || (isRunButton && IsScriptRunning (popUpItemData->fileLocation))) {
        return;
    }

    const GS::UniString description = GetScriptDescription (popUpItemData->info);
    if (description.IsEmpty ()) {
        return;
    }
    if (isRunButton) {
        *toolTipText = PaletteText (ID_PALETTE_STRINGS_RUN_SCRIPT) + "\n" + description;
    } else {
        *toolTipText = description;
    }
}

void TapirPalette::RunShortcutSlot (short slotIndex)
{
    // Uses DG_ERROR (pops an alert) rather than DG_WARNING (silently logged) for every failure
    // path here on purpose: a keyboard-shortcut-triggered action needs to be impossible to miss —
    // a report-log-only warning looks identical to "nothing happened" if the Report window isn't open.
    if (slotIndex < 0 || slotIndex >= ScriptShortcutSlotCount || scriptShortcutSlots[slotIndex].IsEmpty ()) {
        WriteReport (DG_ERROR, PaletteText (ID_PALETTE_STRINGS_SLOT_EMPTY), (int) (slotIndex + 1));
        return;
    }

    // Like the Run button: the slot stops its script while it runs, and other scripts may run meanwhile.
    const ScriptList scripts = GetAllScripts ();
    for (const GS::Ref<PopUpItemData>& popUpItemData : scripts) {
        if (popUpItemData->fileLocation == scriptShortcutSlots[slotIndex]) {
            RunOrStopScript (*popUpItemData);
            return;
        }
    }
    RunningScript* runningScript = FindRunningScript (scriptShortcutSlots[slotIndex]);
    if (runningScript != nullptr) {
        // still running after the script list was reloaded without it
        KillScript (*runningScript, true);
        return;
    }
    WriteReport (DG_ERROR, PaletteText (ID_PALETTE_STRINGS_SLOT_SCRIPT_NOT_FOUND),
        (int) (slotIndex + 1), (int) scripts.GetSize ());
}

void TapirPalette::GetAvailableScripts (GS::Array<std::pair<GS::UniString, IO::Location>>& outScripts)
{
    outScripts.Clear ();
    for (const GS::Ref<PopUpItemData>& popUpItemData : GetAllScripts ()) {
        outScripts.Push ({ GetScriptLabel (*popUpItemData), popUpItemData->fileLocation });
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
    for (UIndex i = 0; i < visibleItems.GetSize (); ++i) {
        if (visibleItems[i].kind == VisibleItem::Kind::Script) {
            scriptSelectionPopUp.SetItemText (static_cast<short> (DG::PopUp::TopItem + i), GetScriptLabel (*visibleItems[i].script));
        }
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
        label = GS::UniString::Printf (PaletteText (ID_PALETTE_STRINGS_SHORTCUT_DEFAULT_LABEL), (int) (slotIndex + 1));
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
        popUp.SetItemText (DG::PopUp::BottomItem, PaletteText (ID_PALETTE_STRINGS_UNASSIGNED));
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
                label = GS::UniString::Printf (PaletteText (ID_PALETTE_STRINGS_SHORTCUT_DEFAULT_LABEL), (int) (row + 1));
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
            if (HasInstance () && Instance ().IsVisible ()) {
                Instance ().Hide ();
                isHiddenByArchicad = true;
            }
            break;

        case APIPalMsg_HidePalette_End:
            if (isHiddenByArchicad && HasInstance () && !Instance ().IsVisible ())
                Instance ().Show ();
            isHiddenByArchicad = false;
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
// started process is found just after the start, among the child processes of this Archicad that did not run
// before it was started and are not the started process of another running script, by the file name of its
// executable. Scripts run at the same time (issue #1819), but they are started one after the other on the main
// thread, and the Add-On waits for its other uv runs, so this is unambiguous. A script that is not Python may run
// as its interpreter (a shell script as bash, a .bat as cmd.exe), so when no file name matches, the first started
// of those child processes is taken. Stop then finds the process again by its id and start time. A process id
// can be reused, so a process is taken as the child of another only when it started after it.

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

static const ProcessInfo* FindThisProcess (const GS::Array<ProcessInfo>& processes)
{
    for (const ProcessInfo& process : processes) {
        if (process.id == GetThisProcessId ()) {
            return &process;
        }
    }
    return nullptr;
}

// The ids of the running processes this Archicad started
static GS::Array<GS::UInt64> ListChildProcessIds ()
{
    GS::Array<GS::UInt64> childIds;
    const GS::Array<ProcessInfo> processes = ListProcesses ();
    const ProcessInfo* thisProcess = FindThisProcess (processes);
    if (thisProcess == nullptr) {
        return childIds;
    }
    for (const ProcessInfo& process : processes) {
        if (IsChildOf (process, *thisProcess)) {
            childIds.Push (static_cast<GS::UInt64> (process.id));
        }
    }
    return childIds;
}

// The process this Archicad has just started for the command: a child process that did not run before it was
// started (childIdsBeforeStart) and is not the started process of another running script (otherStartedIds).
// Returns false when it is not found.
static bool FindStartedProcess (const GS::UniString& command, const GS::Array<GS::UInt64>& childIdsBeforeStart,
    const GS::Array<GS::UInt64>& otherStartedIds, ProcessInfo& result)
{
    const GS::Array<ProcessInfo> processes = ListProcesses ();
    const GS::UniString executableName = GetExecutableName (command);

    const ProcessInfo* thisProcess = FindThisProcess (processes);
    if (thisProcess == nullptr) {
        return false;
    }

    const ProcessInfo* startedProcess = nullptr;   // the most recently started one, should there be more
    const ProcessInfo* firstNewChild = nullptr;
    for (const ProcessInfo& process : processes) {
        const GS::UInt64 id = static_cast<GS::UInt64> (process.id);
        if (!IsChildOf (process, *thisProcess) || childIdsBeforeStart.Contains (id) || otherStartedIds.Contains (id)) {
            continue;
        }
        if (IsSameExecutableName (process.executableName, executableName) &&
            (startedProcess == nullptr || process.startTime > startedProcess->startTime)) {
            startedProcess = &process;
        }
        if (firstNewChild == nullptr || process.startTime < firstNewChild->startTime) {
            firstNewChild = &process;
        }
    }
    if (startedProcess == nullptr) {
        startedProcess = firstNewChild;
    }
    if (startedProcess == nullptr) {
        return false;
    }
    result = *startedProcess;
    return true;
}

// The processes started by the given process (found by its id and start time, as the id may be reused), the
// processes those started, and so on. Returns false when the process is not found.
static bool FindDescendantsOfProcess (GS::UInt64 id, GS::UInt64 startTime, GS::Array<ProcessId>& descendants)
{
    const GS::Array<ProcessInfo> processes = ListProcesses ();
    for (const ProcessInfo& process : processes) {
        if (static_cast<GS::UInt64> (process.id) == id && process.startTime == startTime) {
            CollectDescendants (processes, process, descendants);
            return true;
        }
    }
    return false;
}

void TapirPalette::KillScript (RunningScript& script, bool reportNotFound)
{
    // Found before Kill: afterwards the descendants have no parent any more (macOS gives them to launchd), and
    // the id of the ended process may be reused
    GS::Array<ProcessId> descendants;
    const bool found = script.startedProcessFound &&
        FindDescendantsOfProcess (script.startedProcessId, script.startedProcessStartTime, descendants);
    script.process.Kill ();
    if (!found && reportNotFound) {
        WriteReport (DG_WARNING, "[%T] %T", script.scriptName.ToPrintf (), PaletteText (ID_PALETTE_STRINGS_PROCESSES_NOT_FOUND).ToPrintf ());
    }
    for (const ProcessId descendant : descendants) {
        KillProcess (descendant);
    }
}

TapirPalette::RunningScript* TapirPalette::FindRunningScript (const IO::Location& fileLocation) const
{
    for (const std::unique_ptr<RunningScript>& script : runningScripts) {
        if (!script->ended && script->fileLocation == fileLocation) {
            return script.get ();
        }
    }
    return nullptr;
}

void TapirPalette::RunOrStopScript (const PopUpItemData& popUpItemData)
{
    RunningScript* runningScript = FindRunningScript (popUpItemData.fileLocation);
    if (runningScript != nullptr) {
        KillScript (*runningScript, true);
        return;
    }
    if (isUpdatingAddOn) {
        return;
    }
    if (Config::Instance ().AskUpdatingAddOnBeforeEachExecution () && UpdateAddOn ()) {
        return;
    }
    ExecuteScript (popUpItemData);
}

void TapirPalette::RemoveEndedScript (const GS::UniString& scriptId)
{
    // The output reader posted this as its last step, so it returns at once. A reader that still does not
    // return in time is kept, and removed with the next ended script; an ended script no longer counts as running.
    for (auto it = runningScripts.begin (); it != runningScripts.end ();) {
        RunningScript& script = **it;
        const bool isEnded = script.ended || script.scriptId == scriptId;
        if (isEnded && script.executor.Wait (GS::Timeout (0, 0, 0, 500 /*ms*/))) {
            it = runningScripts.erase (it);
        } else {
            ++it;
        }
    }
}

// Sets an environment variable in Archicad's environment while this object exists, so that only the process
// started meanwhile inherits it, then puts back the value Archicad had. The value must be ASCII.
class ScopedEnvironmentVariable {
public:
    ScopedEnvironmentVariable (const char* _name, const GS::UniString& value) : name (_name)
    {
        const std::string valueStr (value.ToCStr ().Get ());
#if defined (WINDOWS)
        const std::wstring nameW (name.begin (), name.end ());
        const DWORD oldValueSize = GetEnvironmentVariableW (nameW.c_str (), nullptr, 0);
        std::vector<wchar_t> oldValueW (oldValueSize + 1);
        hasOldValue = oldValueSize > 0 && GetEnvironmentVariableW (nameW.c_str (), oldValueW.data (), oldValueSize) < oldValueSize;
        if (hasOldValue) {
            oldValue = std::wstring (oldValueW.data ());
        }
        const std::wstring valueW (valueStr.begin (), valueStr.end ());
        SetEnvironmentVariableW (nameW.c_str (), valueW.c_str ());
#else
        const char* old = getenv (name.c_str ());
        hasOldValue = old != nullptr;
        if (hasOldValue) {
            oldValue = old;
        }
        setenv (name.c_str (), valueStr.c_str (), 1);
#endif
    }

    ~ScopedEnvironmentVariable ()
    {
#if defined (WINDOWS)
        const std::wstring nameW (name.begin (), name.end ());
        SetEnvironmentVariableW (nameW.c_str (), hasOldValue ? oldValue.c_str () : nullptr);
#else
        if (hasOldValue) {
            setenv (name.c_str (), oldValue.c_str (), 1);
        } else {
            unsetenv (name.c_str ());
        }
#endif
    }

    ScopedEnvironmentVariable (const ScopedEnvironmentVariable&) = delete;
    ScopedEnvironmentVariable& operator= (const ScopedEnvironmentVariable&) = delete;

private:
    const std::string name;
    bool hasOldValue = false;
#if defined (WINDOWS)
    std::wstring oldValue;
#else
    std::string oldValue;
#endif
};

// Starts every line of the text with "[<script name>] ", so that the output of scripts running at the same time
// can be told apart in the Report window.
static GS::UniString PrefixLines (const GS::UniString& scriptName, const GS::UniString& text)
{
    const GS::UniString prefix = "[" + scriptName + "] ";
    const USize length = text.GetLength ();
    GS::UniString result;
    UIndex lineStart = 0;
    for (UIndex i = 0; i < length; ++i) {
        if (text[i] == '\n') {
            result += prefix + GS::UniString (text.GetSubstring (lineStart, i + 1 - lineStart));
            lineStart = i + 1;
        }
    }
    if (lineStart < length) {
        result += prefix + GS::UniString (text.GetSubstring (lineStart, length - lineStart));
    }
    return result;
}

void TapirPalette::ExecuteScript (const PopUpItemData& popUpItemData)
{
    if (!ConfirmScriptRequirements (popUpItemData)) {
        return;
    }

    paletteState.AddRecent (GetScriptPath (popUpItemData));
    paletteState.Save ();

    GS::UniString filePath;
    popUpItemData.fileLocation.ToPath (&filePath);

    // Reads the output of one script. Several run at the same time, one for each running script, each on the
    // thread of its script's executor. The RunningScript is removed only after this has returned.
    class UIUpdaterThread : public GS::Runnable
    {
        RunningScript& script;
        GS::Process& process;
        const GS::UniString scriptId;
        const GS::UniString scriptName;
        const bool runWithUv;

        // The script's Script UI window is free for other scripts once it ended or was stopped, and the palette
        // forgets the script, so its Run button and shortcut slot start it again.
        class ScriptEndedTask : public GS::Runnable {
            GS::UniString scriptId;
        public:
            explicit ScriptEndedTask (const GS::UniString& _scriptId) : scriptId(_scriptId)
            {}
            virtual void Run ()
            {
                ForgetPostedTask (this);
                ScriptUIPalette::ReleaseScript (scriptId);
                TapirPalette::Instance ().RemoveEndedScript (scriptId);
                TapirPalette::Instance ().RefreshRunningMarks ();
                TapirPalette::Instance ().SetRunButtonIcon ();
            }
        };

        class IconUpdateTask : public GS::Runnable {
        public:
            IconUpdateTask () = default;
            virtual void Run ()
            {
                ForgetPostedTask (this);
                TapirPalette::Instance ().SetRunButtonIcon ();
            }
        };
        // The text is the script's output, never a format string.
        class OutputUpdateTask : public GS::Runnable {
            short type;
            GS::UniString text;
        public:
            OutputUpdateTask (short _type, const GS::UniString& _text) : text(_text), type(_type)
            {}
            virtual void Run ()
            {
                ForgetPostedTask (this);
                TapirPalette::Instance ().WriteReport (type, "%T", text.ToPrintf ());
            }
        };
        // The text is read from the resources in Run, on the main thread, not on the reader thread.
        class ExitCodeReportTask : public GS::Runnable {
            GS::UniString scriptName;
            int exitCode;
        public:
            ExitCodeReportTask (const GS::UniString& _scriptName, int _exitCode) : scriptName(_scriptName), exitCode(_exitCode)
            {}
            virtual void Run ()
            {
                ForgetPostedTask (this);
                const GS::UniString text = GS::UniString::Printf (PaletteText (ID_PALETTE_STRINGS_EXIT_CODE), exitCode);
                TapirPalette::Instance ().WriteReport (DG_INFORMATION, "%T", PrefixLines (scriptName, text).ToPrintf ());
            }
        };
#if defined (macintosh)
        // The hint for a uv that aborted (exit code 134), also read from the resources on the main thread.
        class AbortedUvHintTask : public GS::Runnable {
            GS::UniString scriptName;
        public:
            explicit AbortedUvHintTask (const GS::UniString& _scriptName) : scriptName(_scriptName)
            {}
            virtual void Run ()
            {
                ForgetPostedTask (this);
                TapirPalette::Instance ().WriteReport (DG_ERROR, "%T", PrefixLines (scriptName, UvManager::GetAbortedUvHint ()).ToPrintf ());
            }
        };
#endif

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

            const GS::UniString text = PrefixLines (scriptName, stderrContent);
            if (lowerContent.Contains ("error:") || lowerContent.Contains ("failed") || lowerContent.Contains ("traceback (most recent call last)")) {
                Post (new OutputUpdateTask (DG_ERROR, text));
                return;
            }
            if (lowerContent.Contains ("warning:")) {
                Post (new OutputUpdateTask (DG_WARNING, text));
                return;
            }
            Post (new OutputUpdateTask (DG_INFORMATION, text));
        }

        // A task posted while Archicad is closing would only run after the add-on has been unloaded.
        static void Post (const GS::RunnableTask& task)
        {
            if (!quitting) {
                KeepPostedTask (task);
                GS::MessageLoopExecutor ().Execute (task);
            }
        }

    public:
        UIUpdaterThread (RunningScript& _script, bool _runWithUv)
            : script(_script), process(_script.process), scriptId(_script.scriptId), scriptName(_script.scriptName), runWithUv(_runWithUv)
        {
        }
        GS::UniString ReadFromChannel (GS::IBinaryChannel& channel)
        {
            if (channel.GetAvailable () <= 0) {
                return GS::EmptyUniString;
            }

            const GS::USize uSize = static_cast<GS::USize> (channel.GetAvailable ());
            std::unique_ptr<char[]> buffer (new char[uSize + 1]);

            GS::IBinaryChannelUtilities::ReadFully (channel, buffer.get (), uSize);
            return SafeUTF8String (buffer.get (), uSize);
        }
        void ReadFromChannels ()
        {
            const GS::UniString stdError = ReadFromChannel (process.GetStandardErrorChannel ());
            ProcessStderrBlock (stdError);

            const GS::UniString stdOutput = ReadFromChannel (process.GetStandardOutputChannel ());
            if (!stdOutput.IsEmpty ()) {
                Post (new OutputUpdateTask (DG_INFORMATION, PrefixLines (scriptName, stdOutput)));
            }
        }
        virtual void Run () override final
        {
            Post (new IconUpdateTask ());

            const GS::Timeout Timeout = {0, 0, 0, 10 /*ms*/};
            while (!process.WaitFor (Timeout)) {
                if (quitting) {
                    return;   // StopRunningScriptOnQuit kills the process and waits for this thread
                }
                ReadFromChannels ();
            }
            if (quitting) {
                return;
            }

            const int exitCode = process.GetExitCode ();
            ReadFromChannels ();
            script.ended = true;   // no longer running; the last thing done with script

            Post (new ExitCodeReportTask (scriptName, exitCode));
#if defined (macintosh)
            // Exit code 134 is an abort (SIGABRT). uv builds that need a newer macOS than the one running
            // abort at start with "macOS <x> or later required" (issue #392).
            if (runWithUv && exitCode == 134) {
                Post (new AbortedUvHintTask (scriptName));
            }
#endif
            Post (new ScriptEndedTask (scriptId));   // last: it removes the script once this has returned
        }
    };

    IO::Name scriptFileName;
    popUpItemData.fileLocation.GetLastLocalName (&scriptFileName);
    const GS::UniString scriptName = scriptFileName.ToString ();
    GS::UniString scriptId;

    try {
        WriteReport (DG_INFORMATION, "%T", PrefixLines (scriptName,
            GS::UniString::Printf (PaletteText (ID_PALETTE_STRINGS_EXECUTING_WITH_UV), filePath.ToPrintf ())).ToPrintf ());

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
            if (popUpItemData.uvProjectPath.IsEmpty ()) {
                argv.Append ({"--script", filePath});
            } else {
                // uv ignores pyproject.toml with --script; --project runs the script in the configured project
                // (a script with inline metadata still runs with its own dependencies).
                argv.Append ({"--project", popUpItemData.uvProjectPath, filePath});
            }
            argv.Append ({"--port", GS::ValueToUniString (GetConnectionPort ())});
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
        const GS::Array<GS::UInt64> childProcessIdsBeforeStart = ListChildProcessIds ();

        // Every started script gets its own id in TAPIR_SCRIPT_ID, which aclib passes to the Script UI commands as
        // scriptId, so that each script has its own Script UI window, titled with the script's name (issue #1819).
        // The id of this Archicad's process keeps the ids of two Archicads apart.
        scriptId = "tapir-" + GS::ValueToUniString (static_cast<GS::UInt32> (GetThisProcessId ())) +
            "-" + GS::ValueToUniString (++startedScriptCount);
        ScriptUIPalette::SetScriptName (scriptId, scriptName);

        std::unique_ptr<RunningScript> script (new RunningScript ());
        script->fileLocation = popUpItemData.fileLocation;
        script->scriptName = scriptName;
        script->scriptId = scriptId;
        {
            const ScopedEnvironmentVariable scriptIdVariable ("TAPIR_SCRIPT_ID", scriptId);
#if defined (WINDOWS)
            // Python writes to a pipe in the system code page unless told otherwise (cp932 on Japanese
            // Windows), while the output is read as UTF-8. PYTHONIOENCODING makes the script's standard
            // streams UTF-8, so non-ASCII output stays readable. It is set in Archicad's environment only
            // while the process is started, and only when the user has not set it.
            const wchar_t* pythonIOEncodingName = L"PYTHONIOENCODING";
            const bool setPythonIOEncoding = GetEnvironmentVariableW (pythonIOEncodingName, nullptr, 0) == 0 &&
                SetEnvironmentVariableW (pythonIOEncodingName, L"utf-8") != 0;
            const GS::OnExit restorePythonIOEncoding ([&] () {
                if (setPythonIOEncoding) {
                    SetEnvironmentVariableW (pythonIOEncodingName, nullptr);
                }
            });
#endif
            script->process = GS::Process::Create (command, argv, GS::Process::CreateNoWindow, redirectStandardOutput, redirectStandardInput, redirectStandardError);
        }
        if (!script->process.IsValid ()) {
            ScriptUIPalette::ReleaseScript (scriptId);
            WriteReport (DG_ERROR, PaletteText (ID_PALETTE_STRINGS_UV_START_FAILED));
            return;
        }

        // The started process is found now, while it surely runs and before another script is started: later the
        // processes started meanwhile for other scripts would be among the new child processes too. Scripts are
        // only started here, on the main thread, one after the other.
        GS::Array<GS::UInt64> otherStartedIds;
        for (const std::unique_ptr<RunningScript>& other : runningScripts) {
            if (other->startedProcessFound) {
                otherStartedIds.Push (other->startedProcessId);
            }
        }
        ProcessInfo startedProcess;
        if (FindStartedProcess (command, childProcessIdsBeforeStart, otherStartedIds, startedProcess)) {
            script->startedProcessFound = true;
            script->startedProcessId = static_cast<GS::UInt64> (startedProcess.id);
            script->startedProcessStartTime = startedProcess.startTime;
        }

        RunningScript& startedScript = *script;
        runningScripts.push_back (std::move (script));
        startedScript.executor.Execute (new UIUpdaterThread (startedScript, filePath.EndsWith (".py")));
    } catch (const GS::ProcessException&) {
        if (!scriptId.IsEmpty ()) {
            ScriptUIPalette::ReleaseScript (scriptId);
        }
        WriteReport (DG_ERROR, PaletteText (ID_PALETTE_STRINGS_EXECUTE_FAILED), filePath.ToPrintf ());
    } catch (...) {
        if (!scriptId.IsEmpty ()) {
            ScriptUIPalette::ReleaseScript (scriptId);
        }
        WriteReport (DG_ERROR, PaletteText (ID_PALETTE_STRINGS_EXECUTE_UNEXPECTED), filePath.ToPrintf ());
    }
    RefreshRunningMarks ();
    SetRunButtonIcon ();
}

bool TapirPalette::PrepareScript (PopUpItemData& popUpData)
{
    if (!IsValidLocation (popUpData.fileLocation)) {
        return false;
    }

    IO::Name name;
    popUpData.fileLocation.GetLastLocalName (&name);

#if defined (macintosh)
    if (name.ToString ().BeginsWith ('.')) {
        return false;
    }
#endif

    if (popUpData.repo) {
        auto* repo = popUpData.repo;
        if (repo->displayName.IsEmpty ()) {
            popUpData.baseDisplayText = name.ToString () + " (" + repo->repoOwner + "/" + repo->repoName + ")";
        } else {
            popUpData.baseDisplayText = name.ToString () + " (" + repo->displayName + ")";
        }
    } else if (!popUpData.sourceName.IsEmpty ()) {
        popUpData.baseDisplayText = name.ToString () + " (" + popUpData.sourceName + ")";
    } else {
        popUpData.baseDisplayText = name.ToString ();
    }
    popUpData.info = ReadScriptInfo (popUpData.fileLocation);

    return true;
}

// The folder of the uv project configured as uvProject (a path relative to baseFolderLoc, with '/' or '\'
// separators, "." for baseFolderLoc itself, optionally ending in pyproject.toml), or empty when none is
// configured or the path leaves baseFolderLoc.
static GS::UniString GetUvProjectPath (const IO::Location& baseFolderLoc, const GS::UniString& configuredUvProject)
{
    GS::UniString uvProject = configuredUvProject;
    uvProject.Trim ();
    if (uvProject.IsEmpty ()) {
        return GS::EmptyUniString;
    }

    IO::Location projectLoc = baseFolderLoc;
    GS::UniString lastName;
    UIndex nameStart = 0;
    for (UIndex i = 0; i <= uvProject.GetLength (); ++i) {
        if (i < uvProject.GetLength () && uvProject[i] != '/' && uvProject[i] != '\\') {
            continue;
        }
        GS::UniString name (uvProject.GetSubstring (nameStart, i - nameStart));
        name.Trim ();
        nameStart = i + 1;
        if (name.IsEmpty () || name == ".") {
            continue;
        }
        if (name == "..") {
            ACAPI_WriteReport (ReportText (ID_REPORT_STRINGS_UV_PROJECT_OUTSIDE), false, uvProject.ToPrintf ());
            return GS::EmptyUniString;
        }
        projectLoc.AppendToLocal (IO::Name (name));
        lastName = name;
    }
    if (lastName.Compare ("pyproject.toml", CaseInsensitive) == GS::UniString::Equal) {
        projectLoc.DeleteLastLocalName ();
    }

    GS::UniString projectPath;
    projectLoc.ToPath (&projectPath);
    return projectPath;
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
        // Only the files below relativeLoc are downloaded (under their path in the repository), so the project is
        // looked for below that folder.
        IO::Location repoFolderLoc = tapirTempFolder;
        repoFolderLoc.AppendToLocal (repoFolderRelativeLoc);
        const GS::UniString uvProjectPath = GetUvProjectPath (repoFolderLoc, repo.uvProject);
        std::map<GS::UniString, GS::UniString> headers;
        if (!repo.token.IsEmpty ()) {
            headers.emplace ("Authorization", "Bearer " + repo.token);
        }
        ACAPI_WriteReport (ReportText (ID_REPORT_STRINGS_DOWNLOADING_REPOSITORY), false, repo.repoOwner.ToPrintf (), repo.repoName.ToPrintf ());
        for (auto kv : GetFilesFromGitHubInRelativeLocation (repo)) {
            const GS::UniString repoLoc = repo.repoOwner + "/" + repo.repoName + "/" + kv.first;
            if (excludeFromDownloadRegex && std::regex_match (repoLoc.ToCStr ().Get (), *excludeFromDownloadRegex)) {
                ACAPI_WriteReport (ReportText (ID_REPORT_STRINGS_SKIP_DOWNLOAD_EXCLUDED), false, repoLoc.ToPrintf ());
                continue;
            }
            // A failed download (e.g. the timeout) skips this file only: an exception here used to abort the
            // creation of the whole palette, which runs during the Add-On's Initialize.
            std::vector<char> content;
            try {
                content = DownloadFileContent (kv.second, headers);
            } catch (const GS::Exception& e) {
                ACAPI_WriteReport (ReportText (ID_REPORT_STRINGS_DOWNLOAD_FAILED_WITH_ERROR), false, repoLoc.ToPrintf (), GetExceptionText (e).ToPrintf ());
                continue;
            } catch (...) {
                ACAPI_WriteReport (ReportText (ID_REPORT_STRINGS_DOWNLOAD_FAILED), false, repoLoc.ToPrintf ());
                continue;
            }
            IO::RelativeLocation relLoc = repoRelativeLoc;
            relLoc.Append (IO::RelativeLocation (kv.first));
            const IO::Location fileLoc = SaveBuiltInScript (tapirTempFolder, relLoc, content);
            ACAPI_WriteReport (ReportText (ID_REPORT_STRINGS_DOWNLOADED), false, repoLoc.ToPrintf ());
            if (excludeRegex) {
                if (std::regex_match (kv.first.ToCStr ().Get (), *excludeRegex)) {
                    ACAPI_WriteReport (ReportText (ID_REPORT_STRINGS_SKIP_USE_EXCLUDED), false, repoLoc.ToPrintf ());
                    continue;
                }
            } else if (includeRegex) {
                if (!std::regex_match (kv.first.ToCStr ().Get (), *includeRegex)) {
                    ACAPI_WriteReport (ReportText (ID_REPORT_STRINGS_SKIP_USE_NOT_INCLUDED), false, repoLoc.ToPrintf ());
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
            ACAPI_WriteReport (ReportText (ID_REPORT_STRINGS_ADDED_TO_POPUP), false, repoLoc.ToPrintf ());
            GS::Ref<PopUpItemData> popUpItemData = GS::NewRef<PopUpItemData> (fileLoc, kv.first, &repo);
            popUpItemData->uvProjectPath = uvProjectPath;
            if (PrepareScript (*popUpItemData)) {
                listedScripts.Push (popUpItemData);
            }
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
            ACAPI_WriteReport (ReportText (ID_REPORT_STRINGS_LOCAL_FOLDER_NOT_FOUND), false, localFolder.path.ToPrintf ());
            continue;
        }

        const std::unique_ptr<std::regex> excludeRegex = localFolder.excludePattern.IsEmpty () ? nullptr : std::make_unique<std::regex> (localFolder.excludePattern.ToCStr ().Get ());
        const std::unique_ptr<std::regex> includeRegex = localFolder.includePattern.IsEmpty () ? nullptr : std::make_unique<std::regex> (localFolder.includePattern.ToCStr ().Get ());
        GS::UniString sourceName = localFolder.displayName;
        if (sourceName.IsEmpty ()) {
            IO::Name folderName;
            sourceName = folderLoc.GetLastLocalName (&folderName) == NoError ? folderName.ToString () : localFolder.path;
        }

        const GS::UniString uvProjectPath = GetUvProjectPath (folderLoc, localFolder.uvProject);

        ACAPI_WriteReport (ReportText (ID_REPORT_STRINGS_LOADING_LOCAL_FOLDER), false, localFolder.path.ToPrintf ());
        std::map<GS::UniString, IO::Location> files;
        CollectFilesInFolder (folderLoc, GS::EmptyUniString, files);
        for (const auto& kv : files) {
            if (excludeRegex) {
                if (std::regex_match (kv.first.ToCStr ().Get (), *excludeRegex)) {
                    ACAPI_WriteReport (ReportText (ID_REPORT_STRINGS_SKIP_USE_EXCLUDED), false, kv.first.ToPrintf ());
                    continue;
                }
            } else if (includeRegex) {
                if (!std::regex_match (kv.first.ToCStr ().Get (), *includeRegex)) {
                    ACAPI_WriteReport (ReportText (ID_REPORT_STRINGS_SKIP_USE_NOT_INCLUDED), false, kv.first.ToPrintf ());
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
            ACAPI_WriteReport (ReportText (ID_REPORT_STRINGS_ADDED_TO_POPUP_FROM), false, kv.first.ToPrintf (), sourceName.ToPrintf ());
            GS::Ref<PopUpItemData> popUpItemData = GS::NewRef<PopUpItemData> (kv.second);
            popUpItemData->sourceName = sourceName;
            popUpItemData->uvProjectPath = uvProjectPath;
            if (PrepareScript (*popUpItemData)) {
                listedScripts.Push (popUpItemData);
            }
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

        hasCustomScript = true;

        // Each one goes first, as when they were inserted at the top of the dropdown.
        GS::Ref<PopUpItemData> popUpItemData = GS::NewRef<PopUpItemData> (IO::Location (customScriptsFolderLoc, name));
        if (PrepareScript (*popUpItemData)) {
            customScripts.Insert (0, popUpItemData);
        }
    });
}

void TapirPalette::LoadScriptsToPopUp ()
{
    addedScripts.Clear ();
    customScripts.Clear ();
    listedScripts.Clear ();
    hasCustomScript = false;
    hasAddedScript = false;

    AddScriptsFromCustomScriptsFolder ();
    const short savedItem = AddScriptsFromPreferences ();
    AddScriptsFromLocalFolders ();
    AddScriptsFromRepositories ();

    RebuildScriptPopUp (GetSavedSelection (savedItem));
    SetDeleteScriptButtonStatus ();
}

void TapirPalette::SetRunButtonIcon ()
{
    // Stop while the selected script runs; other scripts may run meanwhile.
    const GS::Ref<PopUpItemData> selectedScript = GetSelectedScript ();
    short resId = ID_RUN_BUTTON_ICON;
    if (selectedScript != nullptr && IsScriptRunning (selectedScript->fileLocation)) {
        resId = ID_KILL_BUTTON_ICON;
    } else if (VersionChecker::IsUsingLatestVersion ()) {
        resId = ID_RUN_BUTTON_ICON;
    } else {
        resId = ID_RUNWITHUPDATE_BUTTON_ICON;
    }
    runScriptButton.SetIcon (DG::Icon (ACAPI_GetOwnResModule (), resId));
}

// Its tooltip, "Check for Updates" or "Update to <version>", is set by ItemToolTipRequested.
void TapirPalette::SetUpdateButtonIcon ()
{
    const short resId = VersionChecker::IsUsingLatestVersion () ? ID_CHECKFORUPDATES_BUTTON_ICON : ID_UPDATEAVAILABLE_BUTTON_ICON;
    checkForUpdatesButton.SetIcon (DG::Icon (ACAPI_GetOwnResModule (), resId));
}

void TapirPalette::ApplyUpdateMenuItemState ()
{
    API_MenuItemRef itemRef = {};
    itemRef.menuResID = ID_ADDON_MENU_FOR_UPDATE;
    itemRef.itemIndex = ID_ADDON_MENU_UPDATE;

    // The text from the resource ("Check for Updates...") stays until a newer version is found.
    if (!VersionChecker::IsUsingLatestVersion ()) {
        GS::UniString label = GS::UniString::Printf (
            UpdateText (ID_AUTOUPDATE_UPDATE_MENU_TEXT),
            VersionChecker::LatestVersion ().ToPrintf ());
        ACAPI_MenuItem_SetMenuItemText (&itemRef, nullptr, &label);
    }

    // Nothing disables this item on purpose: make sure it never shows greyed out.
    // (Before Archicad 27 these calls are macros that end with a semicolon, so their result is not used.)
    GSFlags itemFlags = {};
    ACAPI_MenuItem_GetMenuItemFlags (&itemRef, &itemFlags);
    if ((itemFlags & API_MenuItemDisabled) != 0) {
        itemFlags &= ~API_MenuItemDisabled;
        ACAPI_MenuItem_SetMenuItemFlags (&itemRef, &itemFlags);
    }
}

void TapirPalette::CheckForUpdates ()
{
    if (!VersionChecker::IsCheckCompleted ()) {
        DGAlert (DG_INFORMATION, UpdateText (ID_AUTOUPDATE_ALERT_TITLE),
            UpdateText (ID_AUTOUPDATE_STILL_CHECKING_TEXT1),
            UpdateText (ID_AUTOUPDATE_STILL_CHECKING_TEXT2),
            UpdateText (ID_AUTOUPDATE_OK_BUTTON));
    } else if (!VersionChecker::IsUsingLatestVersion ()) {
        UpdateAddOn ();
    } else if (VersionChecker::IsNewerVersionWithoutAddOn ()) {
        DGAlert (DG_INFORMATION, UpdateText (ID_AUTOUPDATE_ALERT_TITLE),
            GS::UniString::Printf (UpdateText (ID_AUTOUPDATE_NOT_YET_FOR_AC_TEXT1),
                VersionChecker::LatestVersion ().ToPrintf (), static_cast<int> (VersionChecker::ArchicadMainVersion ())),
            GS::UniString::Printf (UpdateText (ID_AUTOUPDATE_NOT_YET_FOR_AC_TEXT2), ADDON_VERSION),
            UpdateText (ID_AUTOUPDATE_OK_BUTTON));
    } else {
        DGAlert (DG_INFORMATION,
            UpdateText (ID_AUTOUPDATE_LATESTVERSION_ALERT_TITLE),
            GS::UniString::Printf (UpdateText (ID_AUTOUPDATE_LATESTVERSION_ALERT_TEXT), ADDON_VERSION),
            GS::EmptyUniString,
            UpdateText (ID_AUTOUPDATE_LATESTVERSION_ALERT_BUTTON));
    }
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

    error = GS::UniString::Printf (UpdateText (ID_AUTOUPDATE_ERROR_WINDOWS_CODE), GS::ValueToUniString (static_cast<GS::UInt32> (lastError)).ToPrintf ());
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
        error = GS::UniString::Printf (UpdateText (ID_AUTOUPDATE_ERROR_EXTRACT_FAILED), installerPath.ToPrintf ());
        return false;
    }

    // The installer of a release may be built for Apple silicon only, while the Add-On also runs on Intel Macs.
    // open would then fail, or only show the system's alert, without telling that the update can be installed manually.
    IO::Location installerExecutable = installerApp;
    installerExecutable.AppendToLocal (IO::Name ("Contents"));
    installerExecutable.AppendToLocal (IO::Name ("MacOS"));
    installerExecutable.AppendToLocal (IO::Name ("TapirInstaller"));
    if (!IsAppleSiliconMac () && LacksIntelCode (installerExecutable)) {
        error = UpdateText (ID_AUTOUPDATE_ERROR_NO_INTEL_CODE);
        return false;
    }

    // -n: start a new instance even if an installer is already running, otherwise the arguments would be ignored
    GS::Array<GS::UniString> openArguments = { "-n", installerAppPath, "--args" };
    openArguments.Append (arguments);
    if (!RunAndWait ("/usr/bin/open", openArguments)) {
        error = GS::UniString::Printf (UpdateText (ID_AUTOUPDATE_ERROR_OPEN_FAILED), installerAppPath.ToPrintf ());
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
        error = GS::UniString::Printf (UpdateText (ID_AUTOUPDATE_ERROR_CANNOT_CREATE), installerFolderPath.ToPrintf ());
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
        UpdateText (ID_AUTOUPDATE_NEWVERSION_ALERT_TITLE),
        GS::UniString::Printf (UpdateText (ID_AUTOUPDATE_NEWVERSION_ALERT_TEXT1),
            ADDON_VERSION, VersionChecker::LatestVersion ().ToPrintf ()),
        UpdateText (ID_AUTOUPDATE_NEWVERSION_ALERT_TEXT2),
        UpdateText (ID_AUTOUPDATE_NEWVERSION_ALERT_BUTTON1),
        UpdateText (ID_AUTOUPDATE_NEWVERSION_ALERT_BUTTON2));

    if (response != DG_OK) {
        return false;
    }

    const GS::UniString& installerDownloadUrl = VersionChecker::LatestInstallerDownloadUrl ();
    if (installerDownloadUrl.IsEmpty ()) {
        DGAlert (DG_ERROR, UpdateText (ID_AUTOUPDATE_ALERT_TITLE), UpdateText (ID_AUTOUPDATE_INSTALLER_MISSING_TEXT1),
            UpdateText (ID_AUTOUPDATE_INSTALLER_MISSING_TEXT2), UpdateText (ID_AUTOUPDATE_OK_BUTTON));
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
        DGAlert (DG_ERROR, UpdateText (ID_AUTOUPDATE_ALERT_TITLE), UpdateText (ID_AUTOUPDATE_DOWNLOAD_FAILED), error, UpdateText (ID_AUTOUPDATE_OK_BUTTON));
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
        DownloadProcessWindow processWindow (UpdateText (ID_AUTOUPDATE_ALERT_TITLE), UpdateText (ID_AUTOUPDATE_DOWNLOADING));
        downloadResult = DownloadBinaryFile (installerDownloadUrl, installerFile, [&processWindow] (GS::UInt64 downloadedSize, GS::UInt64 totalSize) {
            return processWindow.SetProgress (downloadedSize, totalSize);
        }, error);
    }
    if (downloadResult == DownloadResult::Cancelled) {
        return false;
    }
    if (downloadResult != DownloadResult::Succeeded) {
        DGAlert (DG_ERROR, UpdateText (ID_AUTOUPDATE_ALERT_TITLE), UpdateText (ID_AUTOUPDATE_DOWNLOAD_FAILED), error, UpdateText (ID_AUTOUPDATE_OK_BUTTON));
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
        const GS::UniString manualUpdateText = UpdateText (ID_AUTOUPDATE_MANUAL_UPDATE);
        if (error.IsEmpty ()) {
            // No error (Windows only): the user declined the administrator prompt, which the Add-On's folder needs.
            DGAlert (DG_INFORMATION, UpdateText (ID_AUTOUPDATE_ALERT_TITLE), UpdateText (ID_AUTOUPDATE_CANCELLED),
                UpdateText (ID_AUTOUPDATE_NEEDS_ADMINISTRATOR) + " " + manualUpdateText, UpdateText (ID_AUTOUPDATE_OK_BUTTON));
        } else {
            DGAlert (DG_ERROR, UpdateText (ID_AUTOUPDATE_ALERT_TITLE), UpdateText (ID_AUTOUPDATE_INSTALLER_START_FAILED), error + "\n\n" + manualUpdateText, UpdateText (ID_AUTOUPDATE_OK_BUTTON));
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
    for (const GS::Ref<PopUpItemData>& popUpItemData : addedScripts) {
        GS::UniString pathStr;
        popUpItemData->fileLocation.ToPath (&pathStr);
        preferencesStr += pathStr + '\n';
    }
    // The dropdown's item index, as before, so that every Tapir version reads the blob the same way
    const GS::Ref<PopUpItemData> selectedScript = GetSelectedScript ();
    preferencesStr += GS::ValueToUniString (GetPreferencesItemIndex (selectedScript));
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
    // +1 keeps the terminating null in the stored blob; older builds stored strlen bytes
    // only, which made the load side read past its buffer (see AddScriptsFromPreferences).
    ACAPI_SetPreferences (PREFERENCES_VERSION, (GSSize)strlen (cStr.Get()) + 1, cStr.Get());

    // The selection by its path as well, which outlives a change of the list (GetSavedSelection)
    paletteState.SetSelected (selectedScript != nullptr ? GetScriptPath (*selectedScript) : GS::EmptyUniString);
    paletteState.Save ();
}

GS::UniString TapirPalette::GetScriptPath (const PopUpItemData& popUpData)
{
    GS::UniString path;
    popUpData.fileLocation.ToPath (&path);
    return path;
}

GS::Ref<TapirPalette::PopUpItemData> TapirPalette::GetSavedSelection (short savedItem) const
{
    const GS::UniString& selectedPath = paletteState.Selected ();
    if (!selectedPath.IsEmpty ()) {
        for (const GS::Ref<PopUpItemData>& popUpItemData : GetAllScripts ()) {
            if (GetScriptPath (*popUpItemData) == selectedPath) {
                return popUpItemData;
            }
        }
    }

    const VisibleItemList items = BuildVisibleItems (false);
    if (savedItem >= DG::PopUp::TopItem && static_cast<UIndex> (savedItem - DG::PopUp::TopItem) < items.GetSize ()) {
        const VisibleItem& item = items[static_cast<UIndex> (savedItem - DG::PopUp::TopItem)];
        if (item.kind == VisibleItem::Kind::Script) {
            return item.script;
        }
    }
    return GS::Ref<PopUpItemData> ();
}

short TapirPalette::GetPreferencesItemIndex (const GS::Ref<PopUpItemData>& script) const
{
    if (script != nullptr) {
        const VisibleItemList items = BuildVisibleItems (false);
        for (UIndex i = 0; i < items.GetSize (); ++i) {
            if (items[i].kind == VisibleItem::Kind::Script && items[i].script == script) {
                return static_cast<short> (DG::PopUp::TopItem + i);
            }
        }
    }
    return DG::PopUp::TopItem;
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
    // The caller shows the result with RebuildScriptPopUp. hasAddedScript stays as it was, as the separator did.
    addedScripts.Clear ();
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

    std::unique_ptr<char[]> data (new char[nBytes + 1]);
    ACAPI_GetPreferences (&version, &nBytes, data.get ());
    // Blobs stored by older builds have no terminating null (they stored strlen bytes),
    // so the length must come from nBytes, never from scanning for a null. strnlen still
    // trims the terminator that current builds do store. The code page stays the one
    // SaveScriptsToPreferences writes with (ToCStr's default), so older blobs read as before.
    data.get ()[nBytes] = '\0';
    const GS::USize textLen = static_cast<GS::USize> (strnlen (data.get (), static_cast<size_t> (nBytes)));
    GS::Array<GS::UniString> scriptPathArray;
    GS::UniString (data.get (), textLen).Split ("\n", GS::UniString::SkipEmptyParts, &scriptPathArray);

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
        hasAddedScript = true;
        // Each one goes first, as when they were inserted at the top of the dropdown.
        GS::Ref<PopUpItemData> popUpItemData = GS::NewRef<PopUpItemData> (ownScript);
        if (PrepareScript (*popUpItemData)) {
            addedScripts.Insert (0, popUpItemData);
        }
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
    dlg.SetHeader (PaletteText (ID_PALETTE_STRINGS_SELECT_SCRIPTS_TITLE));
    dlg.SetOKButtonText (PaletteText (ID_PALETTE_STRINGS_SELECT_SCRIPTS_BUTTON));
    if (!dlg.Invoke ()) {
        return false;
    }
    bool hasNew = false;
    for (UIndex i = 0; i < dlg.GetSelectionCount (); ++i) {
        IO::Location file = dlg.GetSelectedFile (i);
        if (IsPopUpContainsFile (file)) {
            continue;
        }
        hasAddedScript = true;
        GS::Ref<PopUpItemData> popUpItemData = GS::NewRef<PopUpItemData> (file);
        if (PrepareScript (*popUpItemData)) {
            addedScripts.Insert (0, popUpItemData);
        }
        hasNew = true;
    }

    if (hasNew) {
        // The newest added script, the first of its section
        RebuildScriptPopUp (addedScripts.IsEmpty () ? GS::Ref<PopUpItemData> () : addedScripts[0]);
        SaveScriptsToPreferences ();
        SetDeleteScriptButtonStatus ();
    }

    return hasNew;
}

// The selected script is one of addedScripts: SetDeleteScriptButtonStatus enables the button only then.
void TapirPalette::DeleteScriptFromPopUp ()
{
    const GS::Ref<PopUpItemData> selectedScript = GetSelectedScript ();
    const UIndex index = IsAddedScript (selectedScript) ? addedScripts.FindFirst (selectedScript) : MaxUIndex;
    if (index >= addedScripts.GetSize ()) {
        return;
    }
    addedScripts.Delete (index);

    // The next added script takes the deleted one's place; after the last added one the one above it is selected.
    // When none is left, the section and its separator go, and the first script is selected.
    GS::Ref<PopUpItemData> scriptToSelect;
    if (addedScripts.IsEmpty ()) {
        hasAddedScript = false;
    } else {
        scriptToSelect = addedScripts[index < addedScripts.GetSize () ? index : addedScripts.GetSize () - 1];
    }
    RebuildScriptPopUp (scriptToSelect);

    SaveScriptsToPreferences ();
    SetDeleteScriptButtonStatus ();
}

void TapirPalette::SetDeleteScriptButtonStatus ()
{
    if (IsAddedScript (GetSelectedScript ())) {
        delScriptButton.Enable ();
    } else {
        delScriptButton.Disable ();
    }
}