#include "ShareDialog.hpp"

#include "APIEnvir.h"
#include "ACAPinc.h"

#include "AddOnVersion.hpp"
#include "ResourceIds.hpp"
#include "VersionChecker.hpp"
#include "Config.hpp"

#include "HTTP/Client/ClientConnection.hpp"
#include "HTTP/Client/Request.hpp"
#include "HTTP/Client/Response.hpp"
#include "JSON/Value.hpp"
#include "JSON/JDOMParser.hpp"
#include "JSON/JDOMWriter.hpp"
#include "File.hpp"
#include "FileSystem.hpp"
#include "FileTypeManager.hpp"
#include "Process.hpp"

#include <cstring>
#include <ctime>
#include <functional>
#include <vector>

#if defined (WINDOWS)
#pragma warning (push)
#pragma warning (disable : 4995 4091)   // as in the DevKit's Win32ShellInterface.hpp
#include <shellapi.h>
#pragma warning (pop)
#pragma comment (lib, "shell32.lib")
#endif

// The share service. The "shareServiceUrl" key of Documents/Tapir/config.json overrides it, e.g. with
// "http://localhost:8787" to test against a local server.
static const char* DefaultShareServiceUrl = "https://share.archicad-api.com";
// Opened when the share service does not answer: it explains sharing on Discord and by pull request.
static const char* ShareWebsiteUrl = "https://archicad-api.com/scripts/share/";
// Only links to the project's issues are opened from the service's answer.
static const char* IssueUrlPrefix = "https://github.com/ENZYME-APD/tapir-archicad-automation/issues/";

static constexpr int ShareRequestTimeoutMs = 15 * 1000;
// A script is sent whole in one request; anything larger is not a script to share.
static constexpr GS::USize MaxScriptSize = 1024 * 1024;
// The answer of the service is a small JSON object.
static constexpr GS::USize MaxAnswerSize = 64 * 1024;

static GS::UniString ShareText (Int32 index)
{
    return RSGetIndString (ID_SHARE_STRINGS, index, ACAPI_GetOwnResModule ());
}

// Whether the bytes are well-formed UTF-8 (Table 3-7 of the Unicode Standard) without NUL characters. Converting
// anything else to a GS::UniString would fire GSRoot's UTF-8 assert (see SafeUTF8String in TapirPalette.cpp).
static bool IsValidUTF8Text (const std::vector<char>& bytes)
{
    const GS::USize size = static_cast<GS::USize> (bytes.size ());
    GS::USize i = 0;
    while (i < size) {
        const unsigned char c = static_cast<unsigned char> (bytes[i]);
        GS::USize seqLen = 0;
        unsigned char secondMin = 0x80;
        unsigned char secondMax = 0xBF;
        if (c == 0) {
            return false;
        } else if (c < 0x80) {
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
        } else {
            return false;
        }
        if (i + seqLen > size) {
            return false;
        }
        for (GS::USize j = 1; j < seqLen; ++j) {
            const unsigned char next = static_cast<unsigned char> (bytes[i + j]);
            const bool validNext = (j == 1) ? (next >= secondMin && next <= secondMax) : ((next & 0xC0) == 0x80);
            if (!validNext) {
                return false;
            }
        }
        i += seqLen;
    }
    return true;
}

// The UTF-8 text of bytes checked by IsValidUTF8Text, without a leading byte order mark.
static GS::UniString UTF8BytesToString (const std::vector<char>& bytes)
{
    std::vector<char> text (bytes);
    if (text.size () >= 3 && static_cast<unsigned char> (text[0]) == 0xEF && static_cast<unsigned char> (text[1]) == 0xBB && static_cast<unsigned char> (text[2]) == 0xBF) {
        text.erase (text.begin (), text.begin () + 3);
    }
    text.push_back ('\0');
    return GS::UniString (text.data (), CC_UTF8);
}

enum class ReadResult {
    Ok,
    CannotRead,
    TooLarge,
    NotText
};

static ReadResult ReadTextFile (const IO::Location& location, GS::USize maxSize, GS::UniString& text)
{
    IO::File file (location);
    if (file.GetStatus () != NoError || file.Open (IO::File::ReadMode) != NoError) {
        return ReadResult::CannotRead;
    }

    GS::USize length = 0;
    if (file.GetDataLength (&length) != NoError) {
        file.Close ();
        return ReadResult::CannotRead;
    }
    if (length > maxSize) {
        file.Close ();
        return ReadResult::TooLarge;
    }

    std::vector<char> bytes (length);
    GS::USize readSize = 0;
    const GSErrCode err = length > 0 ? file.ReadBin (bytes.data (), length, &readSize) : NoError;
    file.Close ();
    if (err != NoError || readSize != length) {
        return ReadResult::CannotRead;
    }
    if (!IsValidUTF8Text (bytes)) {
        return ReadResult::NotText;
    }

    text = UTF8BytesToString (bytes);
    return ReadResult::Ok;
}

static IO::Location GetTapirDocumentsFolder ()
{
    IO::Location folder;
    IO::fileSystem.GetSpecialLocation (IO::FileSystem::UserDocuments, &folder);
    folder.AppendToLocal (IO::Name ("Tapir"));
    return folder;
}

static IO::Location GetConfigFileLocation ()
{
    IO::Location configFile = GetTapirDocumentsFolder ();
    configFile.AppendToLocal (IO::Name ("config.json"));
    return configFile;
}

// Documents/Tapir/config.json as a JSON object, or nullptr when it cannot be read.
static JSON::ObjectValueRef ReadConfigObject ()
{
    // Creates config.json with its defaults when it does not exist yet
    Config::Instance ();

    GS::UniString text;
    if (ReadTextFile (GetConfigFileLocation (), 16 * 1024 * 1024, text) != ReadResult::Ok) {
        return JSON::ObjectValueRef ();
    }

    try {
        JSON::JDOMStringParser parser;
        return GS::DynamicCast<JSON::ObjectValue> (parser.Parse (text));
    } catch (...) {
        return JSON::ObjectValueRef ();
    }
}

static GS::UniString GetConfigString (const JSON::ObjectValueRef& config, const char* key)
{
    GS::UniString value;
    if (config == nullptr) {
        return value;
    }
    try {
        config->GetOptionalMember (key, value);
    } catch (...) {
        value.Clear ();
    }
    return value;
}

// Changes Documents/Tapir/config.json, keeping every key it does not change, so that the settings of the
// palette and of the user are kept as they are.
static bool UpdateConfig (const std::function<void (JSON::ObjectValue&)>& change)
{
    JSON::ObjectValueRef config = ReadConfigObject ();
    if (config == nullptr) {
        ACAPI_WriteReport (RSGetIndString (ID_REPORT_STRINGS, ID_REPORT_STRINGS_CONFIG_SAVE_FAILED, ACAPI_GetOwnResModule ()), false,
            GetConfigFileLocation ().ToDisplayText ().ToPrintf ());
        return false;
    }

    GS::UniString text;
    try {
        change (*config);
        JSON::JDOMStringWriter writer (*config, text, true);
    } catch (...) {
        return false;
    }

    const GS::UniString::CStr utf8 = text.ToCStr (CC_UTF8);
    const GS::USize length = static_cast<GS::USize> (std::strlen (utf8.Get ()));

    IO::File file (GetConfigFileLocation (), IO::File::OnNotFound::Create);
    GSErrCode err = file.Open (IO::File::WriteEmptyMode);
    if (err == NoError) {
        err = file.WriteBin (utf8.Get (), length);
        const GSErrCode closeErr = file.Close ();
        if (err == NoError) {
            err = closeErr;
        }
    }
    if (err != NoError) {
        ACAPI_WriteReport (RSGetIndString (ID_REPORT_STRINGS, ID_REPORT_STRINGS_CONFIG_SAVE_FAILED, ACAPI_GetOwnResModule ()), false,
            GetConfigFileLocation ().ToDisplayText ().ToPrintf ());
        return false;
    }
    return true;
}

static void SetConfigString (JSON::ObjectValue& config, const char* key, const GS::UniString& value)
{
    config.Remove (key);
    config.Add (key, value);
}

static GS::UniString CurrentUtcTime ()
{
    const std::time_t now = std::time (nullptr);
    const std::tm* utc = std::gmtime (&now);
    char buffer[32] = {};
    if (utc == nullptr || std::strftime (buffer, sizeof (buffer), "%Y-%m-%dT%H:%M:%SZ", utc) == 0) {
        return GS::EmptyUniString;
    }
    return GS::UniString (buffer);
}

// Opens an http(s) link in the default browser, without a shell, so no character of it is interpreted.
static void OpenUrl (const GS::UniString& url)
{
    if (!url.BeginsWith ("https://") && !url.BeginsWith ("http://")) {
        return;
    }
#if defined (WINDOWS)
    const auto urlUStr = url.ToUStr ();
    ShellExecuteW (nullptr, L"open", reinterpret_cast<LPCWSTR> (urlUStr.Get ()), nullptr, nullptr, SW_SHOWNORMAL);
#else
    try {
        GS::Array<GS::UniString> arguments;
        arguments.Push (url);
        GS::Process::Create ("/usr/bin/open", arguments, GS::Process::CreateNoWindow);
    } catch (...) {
    }
#endif
}

// Splits "https://host[:port][/base]" into "https://host[:port]" and "/base" (empty when there is none).
static bool SplitServiceUrl (GS::UniString url, GS::UniString& origin, GS::UniString& basePath)
{
    url.Trim ();
    while (url.EndsWith ("/")) {
        url.DeleteLast ();
    }
    if (!url.BeginsWith ("https://") && !url.BeginsWith ("http://")) {
        return false;
    }

    const UIndex schemeEnd = url.FindFirst (GS::UniString ("://"));
    const UIndex pathStart = url.FindFirst (GS::UniChar ('/'), schemeEnd + 3);
    if (pathStart == MaxUIndex) {
        origin = url;
        basePath.Clear ();
    } else {
        origin = GS::UniString (url.GetSubstring (0, pathStart));
        basePath = GS::UniString (url.GetSubstring (pathStart, url.GetLength () - pathStart));
    }
    return origin.GetLength () > schemeEnd + 3;
}

// The share service: the "shareServiceUrl" key of config.json, else the project's service.
static GS::UniString GetShareServiceUrl ()
{
    GS::UniString serviceUrl = GetConfigString (ReadConfigObject (), "shareServiceUrl");
    serviceUrl.Trim ();
    if (serviceUrl.IsEmpty ()) {
        serviceUrl = DefaultShareServiceUrl;
    }
    return serviceUrl;
}

// Percent-encodes a path segment or query value (RFC 3986: only the unreserved characters are kept).
static GS::UniString EncodeUrlComponent (const GS::UniString& text)
{
    static const char* HexDigits = "0123456789ABCDEF";
    const GS::UniString::CStr utf8 = text.ToCStr (CC_UTF8);
    GS::UniString encoded;
    for (const char* p = utf8.Get (); *p != '\0'; ++p) {
        const unsigned char c = static_cast<unsigned char> (*p);
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '.' || c == '_' || c == '~') {
            encoded.Append (GS::UniChar (static_cast<char> (c)));
        } else {
            encoded.Append (GS::UniChar ('%'));
            encoded.Append (GS::UniChar (HexDigits[c >> 4]));
            encoded.Append (GS::UniChar (HexDigits[c & 0x0F]));
        }
    }
    return encoded;
}

enum class ServiceCallResult {
    // A 2xx answer that is a JSON object (or any 2xx answer, when anyAnswer is set)
    Answered,
    // The service was reached but its answer was something else
    BadAnswer,
    // No connection or a timeout
    NoConnection
};

// One request to the share service, with the path below the service's URL (e.g. "/share"). The request body, when
// given, is sent as JSON. With anyAnswer (the reply and confirm calls) every 2xx answer counts, whatever its body.
static ServiceCallResult CallShareService (HTTP::MessageHeader::Method::Id method, const GS::UniString& pathAndQuery, const GS::UniString* requestBody,
                                           bool anyAnswer, JSON::ObjectValueRef& answerObject)
{
    answerObject = JSON::ObjectValueRef ();

    GS::UniString origin;
    GS::UniString basePath;
    if (!SplitServiceUrl (GetShareServiceUrl (), origin, basePath)) {
        return ServiceCallResult::NoConnection;
    }

    int statusCode = 0;
    std::vector<char> answerBytes;
    try {
        const IO::URI::URI connectionUrl (origin);
        HTTP::Client::ClientConnection clientConnection (connectionUrl);
        clientConnection.SetTimeout (ShareRequestTimeoutMs);
        clientConnection.Connect ();

        HTTP::Client::Request request (method, basePath + pathAndQuery);
        request.GetRequestHeaderFieldCollection ().Add (HTTP::MessageHeader::HeaderFieldName::UserAgent, "Tapir-Archicad-AddOn/" ADDON_VERSION);
        if (requestBody != nullptr) {
            request.GetRequestHeaderFieldCollection ().Add (HTTP::MessageHeader::HeaderFieldName::ContentType, "application/json; charset=utf-8");
            const GS::UniString::CStr body = requestBody->ToCStr (CC_UTF8);
            clientConnection.Send (request, body.Get (), static_cast<GS::USize> (std::strlen (body.Get ())));
        } else {
            clientConnection.Send (request);
        }

        HTTP::Client::Response response;
        HTTP::Encoding::BodyIBinaryChannel& responseBody = clientConnection.BeginReceive (response);
        statusCode = static_cast<int> (response.GetStatusCode ());

        constexpr GS::USize BufferSize = 4096;
        char buffer[BufferSize];
        try {
            while (answerBytes.size () <= MaxAnswerSize) {
                const GS::USize bytesRead = responseBody.Read (buffer, BufferSize);
                if (bytesRead == 0) {
                    break;
                }
                answerBytes.insert (answerBytes.end (), buffer, buffer + bytesRead);
            }
        } catch (const GS::Exception&) {
            // End of input: the channel signalled it with an exception instead of returning 0.
        }
        clientConnection.FinishReceive ();
        clientConnection.Close (false);
    } catch (...) {
        return statusCode == 0 ? ServiceCallResult::NoConnection : ServiceCallResult::BadAnswer;
    }

    if (statusCode < 200 || statusCode > 299 || answerBytes.size () > MaxAnswerSize || !IsValidUTF8Text (answerBytes)) {
        return ServiceCallResult::BadAnswer;
    }

    GS::UniString answerText = UTF8BytesToString (answerBytes);
    answerText.Trim ();
    if (!answerText.IsEmpty ()) {
        try {
            JSON::JDOMStringParser parser;
            answerObject = GS::DynamicCast<JSON::ObjectValue> (parser.Parse (answerText));
        } catch (...) {
            answerObject = JSON::ObjectValueRef ();
        }
    }
    return (answerObject != nullptr || anyAnswer) ? ServiceCallResult::Answered : ServiceCallResult::BadAnswer;
}

// A string member of an answer, or empty when it is missing or not a string.
static GS::UniString GetAnswerString (const JSON::ObjectValueRef& answerObject, const char* key)
{
    return GetConfigString (answerObject, key);
}

struct ShareAnswer {
    GS::UniString issueUrl;
    GS::UniString shareId;
    GS::UniString key;
};

// Sends the share to the service. Returns false when it does not answer (no connection, a timeout or any answer
// other than {"issueUrl", "shareId", "key"} with a link to an issue of the project).
static bool SendShare (const GS::UniString& requestBody, ShareAnswer& answer)
{
    JSON::ObjectValueRef answerObject;
    if (CallShareService (HTTP::MessageHeader::Method::Post, "/share", &requestBody, false, answerObject) != ServiceCallResult::Answered) {
        return false;
    }
    answer.issueUrl = GetAnswerString (answerObject, "issueUrl");
    answer.shareId = GetAnswerString (answerObject, "shareId");
    answer.key = GetAnswerString (answerObject, "key");

    return answer.issueUrl.BeginsWith (IssueUrlPrefix) && answer.issueUrl.GetLength () > GS::UniString (IssueUrlPrefix).GetLength () &&
        !answer.shareId.IsEmpty () && !answer.key.IsEmpty ();
}

static GS::UniString BuildShareRequestBody (const GS::UniString& fileName, const GS::UniString& content, const GS::UniString& summary,
                                            const GS::UniString& creditName, const GS::UniString& email)
{
    JSON::ObjectValueRef file (new JSON::ObjectValue ());
    file->Add ("name", fileName);
    file->Add ("content", content);

    JSON::ObjectValueRef body (new JSON::ObjectValue ());
    body->Add ("version", static_cast<Int32> (1));
    body->AddValue ("file", file);
    body->Add ("summary", summary);
    body->Add ("creditName", creditName);
    body->Add ("email", email);
    body->Add ("archicadVersion", static_cast<Int32> (VersionChecker::ArchicadMainVersion ()));
    body->Add ("tapirVersion", GS::UniString (ADDON_VERSION));
#if defined (WINDOWS)
    body->Add ("os", GS::UniString ("Windows"));
#else
    body->Add ("os", GS::UniString ("macOS"));
#endif

    GS::UniString text;
    JSON::JDOMStringWriter writer (*body, text);
    return text;
}

void ShareDialog::Open (const IO::Location& script)
{
    GS::UniString content;
    const ReadResult result = ReadTextFile (script, MaxScriptSize, content);
    if (result != ReadResult::Ok) {
        IO::Name fileName;
        script.GetLastLocalName (&fileName);
        const Int32 messageIndex =
            result == ReadResult::TooLarge ? ID_SHARE_STRINGS_FILE_TOO_LARGE :
            result == ReadResult::NotText ? ID_SHARE_STRINGS_FILE_NOT_TEXT :
            ID_SHARE_STRINGS_FILE_CANNOT_READ;
        DGAlert (DG_ERROR, ShareText (ID_SHARE_STRINGS_TITLE),
            GS::UniString::Printf (ShareText (messageIndex), fileName.ToString ().ToPrintf ()), "", ShareText (ID_SHARE_STRINGS_OK));
        return;
    }

    ShareDialog dialog (script, content);
    dialog.Invoke ();
}

void ShareDialog::OpenWithFileDialog ()
{
    FTM::FileTypeManager ftMan ("Python Script");
    FTM::GroupID gid = ftMan.AddGroup ("Python Script");
    ftMan.AddType (FTM::FileType ("Python Script", "py", '    ', '    ', 0), gid);
    DG::FileDialog dlg (DG::FileDialog::OpenFile);
    dlg.AddFilter (gid);
    dlg.SetHeader (ShareText (ID_SHARE_STRINGS_SELECT_SCRIPT_TITLE));
    dlg.SetOKButtonText (ShareText (ID_SHARE_STRINGS_SELECT_SCRIPT_BUTTON));

    IO::Location customScriptsFolder = GetTapirDocumentsFolder ();
    customScriptsFolder.AppendToLocal (IO::Name ("custom-scripts"));
    bool exists = false;
    if (IO::fileSystem.Contains (customScriptsFolder, &exists) == NoError && exists) {
        dlg.SetFolder (customScriptsFolder);
    }

    if (!dlg.Invoke () || dlg.GetSelectionCount () == 0) {
        return;
    }
    Open (dlg.GetSelectedFile (0));
}

ShareDialog::ShareDialog (const IO::Location& scriptIn, const GS::UniString& contentIn)
    : DG::ModalDialog (ACAPI_GetOwnResModule (), ID_SHARE_DIALOG, ACAPI_GetOwnResModule ())
    , script (scriptIn)
    , content (contentIn)
    , shareButton (GetReference (), ID_SHARE_DIALOG_SHARE_BUTTON)
    , cancelButton (GetReference (), ID_SHARE_DIALOG_CANCEL_BUTTON)
    , summaryEdit (GetReference (), ID_SHARE_DIALOG_SUMMARY_EDIT)
    , creditNameEdit (GetReference (), ID_SHARE_DIALOG_CREDIT_NAME_EDIT)
    , emailEdit (GetReference (), ID_SHARE_DIALOG_EMAIL_EDIT)
    , licenceCheck (GetReference (), ID_SHARE_DIALOG_LICENCE_CHECK)
    , statusText (GetReference (), ID_SHARE_DIALOG_STATUS_TEXT)
    , openIssueButton (GetReference (), ID_SHARE_DIALOG_OPEN_ISSUE_BUTTON)
    , yourSharesButton (GetReference (), ID_SHARE_DIALOG_YOUR_SHARES_BUTTON)
    , contentEdit (GetReference (), DG::Rect (10, 32, 630, 330), DG::MultiLineEdit::VScroll,
        DG::EditControl::Frame, DG::EditControl::Update, DG::EditControl::ReadOnly)
{
    contentEdit.SetText (content);
    contentEdit.Show ();

    creditNameEdit.SetText (GetConfigString (ReadConfigObject (), "shareCreditName"));
    licenceCheck.Uncheck ();
    statusText.SetText (GS::EmptyUniString);
    openIssueButton.Hide ();

    shareButton.Attach (*this);
    cancelButton.Attach (*this);
    openIssueButton.Attach (*this);
    yourSharesButton.Attach (*this);
    licenceCheck.Attach (*this);
    summaryEdit.Attach (*this);
    creditNameEdit.Attach (*this);

    UpdateShareButton ();
}

ShareDialog::~ShareDialog ()
{
    shareButton.Detach (*this);
    cancelButton.Detach (*this);
    openIssueButton.Detach (*this);
    yourSharesButton.Detach (*this);
    licenceCheck.Detach (*this);
    summaryEdit.Detach (*this);
    creditNameEdit.Detach (*this);
}

void ShareDialog::ButtonClicked (const DG::ButtonClickEvent& ev)
{
    if (ev.GetSource () == &shareButton) {
        Share ();
    } else if (ev.GetSource () == &cancelButton) {
        PostCloseRequest (shared ? DG::ModalDialog::Accept : DG::ModalDialog::Cancel);
    } else if (ev.GetSource () == &openIssueButton) {
        OpenUrl (issueUrl);
    } else if (ev.GetSource () == &yourSharesButton) {
        YourSharesDialog::Open ();
    }
}

void ShareDialog::CheckItemChanged (const DG::CheckItemChangeEvent&)
{
    UpdateShareButton ();
}

void ShareDialog::TextEditChanged (const DG::TextEditChangeEvent&)
{
    UpdateShareButton ();
}

// Share stays disabled until the licence is accepted and the sentence and the name are filled in, and after the
// script was shared.
void ShareDialog::UpdateShareButton ()
{
    GS::UniString summary = summaryEdit.GetText ();
    GS::UniString creditName = creditNameEdit.GetText ();
    summary.Trim ();
    creditName.Trim ();
    if (!shared && licenceCheck.IsChecked () && !summary.IsEmpty () && !creditName.IsEmpty ()) {
        shareButton.Enable ();
    } else {
        shareButton.Disable ();
    }
}

void ShareDialog::Share ()
{
    GS::UniString summary = summaryEdit.GetText ();
    GS::UniString creditName = creditNameEdit.GetText ();
    GS::UniString email = emailEdit.GetText ();
    summary.Trim ();
    creditName.Trim ();
    email.Trim ();
    if (shared || !licenceCheck.IsChecked () || summary.IsEmpty () || creditName.IsEmpty ()) {
        return;
    }

    // The name is remembered for next time, whether or not the service answers.
    UpdateConfig ([&] (JSON::ObjectValue& config) {
        SetConfigString (config, "shareCreditName", creditName);
    });

    IO::Name fileName;
    script.GetLastLocalName (&fileName);
    const GS::UniString requestBody = BuildShareRequestBody (fileName.ToString (), content, summary, creditName, email);

    shareButton.Disable ();
    statusText.SetText (ShareText (ID_SHARE_STRINGS_SENDING));
    statusText.Redraw ();

    ShareAnswer answer;
    if (!SendShare (requestBody, answer)) {
        // Nothing is stored: the website explains the other ways to share.
        statusText.SetText (ShareText (ID_SHARE_STRINGS_SERVICE_NOT_AVAILABLE));
        UpdateShareButton ();
        OpenUrl (ShareWebsiteUrl);
        return;
    }

    shared = true;
    issueUrl = answer.issueUrl;

    // Only this person can follow the share up, with its key.
    const GS::UniString sharedAt = CurrentUtcTime ();
    UpdateConfig ([&] (JSON::ObjectValue& config) {
        JSON::ObjectValueRef share (new JSON::ObjectValue ());
        share->Add ("shareId", answer.shareId);
        share->Add ("key", answer.key);
        share->Add ("issueUrl", answer.issueUrl);
        share->Add ("file", fileName.ToString ());
        share->Add ("sharedAt", sharedAt);

        JSON::ArrayValueRef shares;
        if (!config.GetOptionalMember ("shares", shares) || shares == nullptr) {
            config.Remove ("shares");
            shares = JSON::ArrayValueRef (new JSON::ArrayValue ());
            config.AddValue ("shares", shares);
        }
        shares->AddValue (share);
    });

    statusText.SetText (ShareText (ID_SHARE_STRINGS_SHARED));
    openIssueButton.Show ();
    cancelButton.SetText (ShareText (ID_SHARE_STRINGS_CLOSE));
    UpdateShareButton ();
}

// --- Your shares ---------------------------------------------------------------------------------------------------

// An answer to the reviewer's questions is a few paragraphs, not a document.
static constexpr GS::USize MaxAnswerLength = 4000;

enum {
    FileColumn   = 1,
    DateColumn   = 2,
    StatusColumn = 3
};

// Links in the service's answer are only opened when they point to the project's website or repository.
static bool IsProjectUrl (const GS::UniString& url)
{
    return url.BeginsWith ("https://archicad-api.com/") || url.BeginsWith ("https://github.com/ENZYME-APD/tapir-archicad-automation/");
}

static GS::UniString ShareStatusPath (const GS::UniString& shareId, const GS::UniString& key, const char* action)
{
    GS::UniString path = "/share/" + EncodeUrlComponent (shareId);
    if (action != nullptr) {
        path += "/";
        path += action;
    }
    return path + "?key=" + EncodeUrlComponent (key);
}

void YourSharesDialog::Open ()
{
    YourSharesDialog dialog;
    dialog.Invoke ();
}

YourSharesDialog::YourSharesDialog ()
    : DG::ModalDialog (ACAPI_GetOwnResModule (), ID_YOUR_SHARES_DIALOG, ACAPI_GetOwnResModule ())
    , closeButton (GetReference (), ID_YOUR_SHARES_DIALOG_CLOSE_BUTTON)
    , sharesList (GetReference (), ID_YOUR_SHARES_DIALOG_LIST)
    , detailsText (GetReference (), ID_YOUR_SHARES_DIALOG_DETAILS_TEXT)
    , openIssueButton (GetReference (), ID_YOUR_SHARES_DIALOG_OPEN_ISSUE_BUTTON)
    , openListingButton (GetReference (), ID_YOUR_SHARES_DIALOG_OPEN_LISTING_BUTTON)
    , refreshButton (GetReference (), ID_YOUR_SHARES_DIALOG_REFRESH_BUTTON)
    , questionsLabel (GetReference (), ID_YOUR_SHARES_DIALOG_QUESTIONS_LABEL)
    , answerLabel (GetReference (), ID_YOUR_SHARES_DIALOG_ANSWER_LABEL)
    , sendAnswerButton (GetReference (), ID_YOUR_SHARES_DIALOG_SEND_ANSWER_BUTTON)
    , messageText (GetReference (), ID_YOUR_SHARES_DIALOG_MESSAGE_TEXT)
    , confirmText (GetReference (), ID_YOUR_SHARES_DIALOG_CONFIRM_TEXT)
    , confirmButton (GetReference (), ID_YOUR_SHARES_DIALOG_CONFIRM_BUTTON)
    , questionsEdit (GetReference (), DG::Rect (10, 228, 630, 318), DG::MultiLineEdit::VScroll,
        DG::EditControl::Frame, DG::EditControl::Update, DG::EditControl::ReadOnly)
    , answerEdit (GetReference (), DG::Rect (10, 350, 630, 400), DG::MultiLineEdit::VScroll,
        DG::EditControl::Frame, DG::EditControl::Update, DG::EditControl::Editable)
{
    sharesList.SetTabFieldCount (3);
    sharesList.SetTabFieldProperties (FileColumn, 0, 330, DG::ListBox::Left, DG::ListBox::EndTruncate, false, true);
    sharesList.SetTabFieldProperties (DateColumn, 330, 440, DG::ListBox::Left, DG::ListBox::EndTruncate, false, true);
    sharesList.SetTabFieldProperties (StatusColumn, 440, 600, DG::ListBox::Left, DG::ListBox::EndTruncate, false, true);
    sharesList.SetHeaderItemText (FileColumn, ShareText (ID_SHARE_STRINGS_COLUMN_FILE));
    sharesList.SetHeaderItemText (DateColumn, ShareText (ID_SHARE_STRINGS_COLUMN_DATE));
    sharesList.SetHeaderItemText (StatusColumn, ShareText (ID_SHARE_STRINGS_COLUMN_STATUS));

    questionsEdit.Show ();
    answerEdit.Show ();
    messageText.SetText (GS::EmptyUniString);

    LoadShares ();
    FillList ();
    UpdateDetails ();

    Attach (*this);
    closeButton.Attach (*this);
    openIssueButton.Attach (*this);
    openListingButton.Attach (*this);
    refreshButton.Attach (*this);
    sendAnswerButton.Attach (*this);
    confirmButton.Attach (*this);
    sharesList.Attach (*this);
    answerEdit.Attach (*this);
}

YourSharesDialog::~YourSharesDialog ()
{
    Detach (*this);
    closeButton.Detach (*this);
    openIssueButton.Detach (*this);
    openListingButton.Detach (*this);
    refreshButton.Detach (*this);
    sendAnswerButton.Detach (*this);
    confirmButton.Detach (*this);
    sharesList.Detach (*this);
    answerEdit.Detach (*this);
}

// The newest share first. Entries without an id or a key cannot be followed up and are left out.
void YourSharesDialog::LoadShares ()
{
    shares.clear ();
    const JSON::ObjectValueRef config = ReadConfigObject ();
    if (config == nullptr) {
        return;
    }

    try {
        JSON::ArrayValueRef storedShares;
        if (!config->GetOptionalMember ("shares", storedShares) || storedShares == nullptr) {
            return;
        }
        for (UIndex i = storedShares->GetSize (); i > 0; --i) {
            const JSON::ObjectValueRef stored = GS::DynamicCast<JSON::ObjectValue> (storedShares->Get (i - 1));
            if (stored == nullptr) {
                continue;
            }
            ShareEntry share;
            share.shareId = GetConfigString (stored, "shareId");
            share.key = GetConfigString (stored, "key");
            share.issueUrl = GetConfigString (stored, "issueUrl");
            share.file = GetConfigString (stored, "file");
            share.sharedAt = GetConfigString (stored, "sharedAt");
            if (share.shareId.IsEmpty () || share.key.IsEmpty ()) {
                continue;
            }
            shares.push_back (share);
        }
    } catch (...) {
        // A config.json edited by hand: the shares read so far are shown.
    }
}

void YourSharesDialog::FillList ()
{
    sharesList.DisableDraw ();
    sharesList.DeleteItem (DG::ListBox::AllItems);
    for (size_t i = 0; i < shares.size (); ++i) {
        sharesList.AppendItem ();
        UpdateRow (static_cast<short> (i + 1));
    }
    if (!shares.empty ()) {
        sharesList.SelectItem (1);
    }
    sharesList.EnableDraw ();
    sharesList.Redraw ();
}

void YourSharesDialog::UpdateRow (short listItem)
{
    if (listItem < 1 || static_cast<size_t> (listItem) > shares.size ()) {
        return;
    }
    const ShareEntry& share = shares[listItem - 1];

    Int32 statusIndex = ID_SHARE_STRINGS_STATUS_UNKNOWN;
    switch (share.status) {
        case Status::Checking:  statusIndex = ID_SHARE_STRINGS_STATUS_CHECKING; break;
        case Status::Unknown:   statusIndex = ID_SHARE_STRINGS_STATUS_UNKNOWN; break;
        case Status::Waiting:   statusIndex = ID_SHARE_STRINGS_STATUS_WAITING; break;
        case Status::Questions: statusIndex = ID_SHARE_STRINGS_STATUS_QUESTIONS; break;
        case Status::Accepted:  statusIndex = ID_SHARE_STRINGS_STATUS_ACCEPTED; break;
        case Status::Declined:  statusIndex = ID_SHARE_STRINGS_STATUS_DECLINED; break;
    }

    // sharedAt is "YYYY-MM-DDTHH:MM:SSZ"; the date is enough here.
    const GS::UniString date = share.sharedAt.GetLength () >= 10 ? GS::UniString (share.sharedAt.GetSubstring (0, 10)) : share.sharedAt;

    sharesList.SetTabItemText (listItem, FileColumn, share.file);
    sharesList.SetTabItemText (listItem, DateColumn, date);
    sharesList.SetTabItemText (listItem, StatusColumn, ShareText (statusIndex));
}

YourSharesDialog::ShareEntry* YourSharesDialog::GetSelectedShare ()
{
    const short selected = sharesList.GetSelectedItem ();
    if (selected < 1 || static_cast<size_t> (selected) > shares.size ()) {
        return nullptr;
    }
    return &shares[selected - 1];
}

void YourSharesDialog::UpdateDetails ()
{
    const ShareEntry* share = GetSelectedShare ();

    if (share == nullptr) {
        detailsText.SetText (ShareText (shares.empty () ? ID_SHARE_STRINGS_NO_SHARES : ID_SHARE_STRINGS_DETAILS_CHECKING));
        openIssueButton.Disable ();
        openListingButton.Hide ();
        refreshButton.SetStatus (!shares.empty ());
    } else {
        Int32 detailsIndex = ID_SHARE_STRINGS_DETAILS_UNKNOWN;
        switch (share->status) {
            case Status::Checking:  detailsIndex = ID_SHARE_STRINGS_DETAILS_CHECKING; break;
            case Status::Unknown:   detailsIndex = ID_SHARE_STRINGS_DETAILS_UNKNOWN; break;
            case Status::Waiting:   detailsIndex = ID_SHARE_STRINGS_DETAILS_WAITING; break;
            case Status::Questions: detailsIndex = ID_SHARE_STRINGS_DETAILS_QUESTIONS; break;
            case Status::Accepted:  detailsIndex = ID_SHARE_STRINGS_DETAILS_ACCEPTED; break;
            case Status::Declined:  detailsIndex = ID_SHARE_STRINGS_DETAILS_DECLINED; break;
        }
        detailsText.SetText (ShareText (detailsIndex));
        openIssueButton.SetStatus (share->issueUrl.BeginsWith (IssueUrlPrefix));
        openListingButton.SetVisibility (IsProjectUrl (share->listingUrl));
        refreshButton.Enable ();
    }

    const bool hasQuestions = share != nullptr && share->status == Status::Questions;
    GS::UniString questions;
    if (hasQuestions) {
        for (const GS::UniString& question : share->questions) {
            if (!questions.IsEmpty ()) {
                questions += "\r\n\r\n";
            }
            questions += question;
        }
    }
    questionsEdit.SetText (questions);
    questionsLabel.SetVisibility (hasQuestions);
    questionsEdit.SetVisibility (hasQuestions);
    answerLabel.SetVisibility (hasQuestions);
    answerEdit.SetVisibility (hasQuestions);
    sendAnswerButton.SetVisibility (hasQuestions);

    GS::UniString answer = answerEdit.GetText ();
    answer.Trim ();
    sendAnswerButton.SetStatus (hasQuestions && !answer.IsEmpty () && answer.GetLength () <= MaxAnswerLength);

    const bool accepted = share != nullptr && share->status == Status::Accepted;
    confirmText.SetVisibility (accepted);
    confirmButton.SetVisibility (accepted);
    confirmButton.SetStatus (accepted && !share->confirmed);
}

bool YourSharesDialog::RefreshStatus (ShareEntry& share)
{
    share.questions.clear ();
    share.listingUrl.Clear ();
    share.status = Status::Unknown;

    JSON::ObjectValueRef answerObject;
    const ServiceCallResult result =
        CallShareService (HTTP::MessageHeader::Method::Get, ShareStatusPath (share.shareId, share.key, nullptr), nullptr, false, answerObject);
    if (result == ServiceCallResult::NoConnection) {
        return false;
    }
    if (result != ServiceCallResult::Answered) {
        return true;
    }

    const GS::UniString status = GetAnswerString (answerObject, "status");
    if (status == "waiting") {
        share.status = Status::Waiting;
    } else if (status == "questions") {
        share.status = Status::Questions;
    } else if (status == "accepted") {
        share.status = Status::Accepted;
    } else if (status == "declined") {
        share.status = Status::Declined;
    } else {
        return true;
    }

    share.listingUrl = GetAnswerString (answerObject, "listingUrl");
    try {
        JSON::ArrayValueRef questions;
        if (answerObject->GetOptionalMember ("questions", questions) && questions != nullptr) {
            for (UIndex i = 0; i < questions->GetSize (); ++i) {
                const JSON::StringValueRef question = GS::DynamicCast<JSON::StringValue> (questions->Get (i));
                if (question != nullptr) {
                    share.questions.push_back (question->Get ());
                }
            }
        }
    } catch (...) {
        share.questions.clear ();
    }
    return true;
}

void YourSharesDialog::RefreshAllStatuses ()
{
    if (shares.empty ()) {
        return;
    }

    messageText.SetText (GS::EmptyUniString);
    for (size_t i = 0; i < shares.size (); ++i) {
        shares[i].status = Status::Checking;
        UpdateRow (static_cast<short> (i + 1));
    }
    UpdateDetails ();
    sharesList.Redraw ();
    detailsText.Redraw ();

    bool reachable = true;
    for (size_t i = 0; i < shares.size (); ++i) {
        if (reachable) {
            reachable = RefreshStatus (shares[i]);
        } else {
            shares[i].status = Status::Unknown;
        }
        UpdateRow (static_cast<short> (i + 1));
    }
    UpdateDetails ();
}

void YourSharesDialog::PanelOpened (const DG::PanelOpenEvent&)
{
    RefreshAllStatuses ();
}

void YourSharesDialog::ButtonClicked (const DG::ButtonClickEvent& ev)
{
    ShareEntry* share = GetSelectedShare ();
    if (ev.GetSource () == &closeButton) {
        PostCloseRequest (DG::ModalDialog::Accept);
    } else if (ev.GetSource () == &refreshButton) {
        RefreshAllStatuses ();
    } else if (ev.GetSource () == &openIssueButton) {
        if (share != nullptr && share->issueUrl.BeginsWith (IssueUrlPrefix)) {
            OpenUrl (share->issueUrl);
        }
    } else if (ev.GetSource () == &openListingButton) {
        if (share != nullptr && IsProjectUrl (share->listingUrl)) {
            OpenUrl (share->listingUrl);
        }
    } else if (ev.GetSource () == &sendAnswerButton) {
        SendAnswer ();
    } else if (ev.GetSource () == &confirmButton) {
        Confirm ();
    }
}

void YourSharesDialog::ListBoxSelectionChanged (const DG::ListBoxSelectionEvent&)
{
    answerEdit.SetText (GS::EmptyUniString);
    messageText.SetText (GS::EmptyUniString);
    UpdateDetails ();
}

// Send Answer stays disabled while the answer is empty or too long.
void YourSharesDialog::TextEditChanged (const DG::TextEditChangeEvent&)
{
    const ShareEntry* share = GetSelectedShare ();
    GS::UniString answer = answerEdit.GetText ();
    answer.Trim ();
    sendAnswerButton.SetStatus (share != nullptr && share->status == Status::Questions && !answer.IsEmpty () && answer.GetLength () <= MaxAnswerLength);
}

void YourSharesDialog::SendAnswer ()
{
    ShareEntry* share = GetSelectedShare ();
    GS::UniString answer = answerEdit.GetText ();
    answer.Trim ();
    if (share == nullptr || share->status != Status::Questions || answer.IsEmpty () || answer.GetLength () > MaxAnswerLength) {
        return;
    }

    sendAnswerButton.Disable ();
    messageText.SetText (ShareText (ID_SHARE_STRINGS_SENDING));
    messageText.Redraw ();

    JSON::ObjectValue body;
    body.Add ("text", answer);
    GS::UniString requestBody;
    JSON::JDOMStringWriter writer (body, requestBody);

    JSON::ObjectValueRef answerObject;
    const ServiceCallResult result =
        CallShareService (HTTP::MessageHeader::Method::Post, ShareStatusPath (share->shareId, share->key, "reply"), &requestBody, true, answerObject);
    if (result != ServiceCallResult::Answered) {
        messageText.SetText (ShareText (ID_SHARE_STRINGS_ANSWER_FAILED));
        UpdateDetails ();
        return;
    }

    answerEdit.SetText (GS::EmptyUniString);
    RefreshStatus (*share);
    UpdateRow (sharesList.GetSelectedItem ());
    UpdateDetails ();
    messageText.SetText (ShareText (ID_SHARE_STRINGS_ANSWER_SENT));
}

void YourSharesDialog::Confirm ()
{
    ShareEntry* share = GetSelectedShare ();
    if (share == nullptr || share->status != Status::Accepted || share->confirmed) {
        return;
    }

    confirmButton.Disable ();
    messageText.SetText (ShareText (ID_SHARE_STRINGS_SENDING));
    messageText.Redraw ();

    const GS::UniString requestBody = "{}";
    JSON::ObjectValueRef answerObject;
    const ServiceCallResult result =
        CallShareService (HTTP::MessageHeader::Method::Post, ShareStatusPath (share->shareId, share->key, "confirm"), &requestBody, true, answerObject);
    if (result != ServiceCallResult::Answered) {
        messageText.SetText (ShareText (ID_SHARE_STRINGS_CONFIRM_FAILED));
        UpdateDetails ();
        return;
    }

    share->confirmed = true;
    RefreshStatus (*share);
    UpdateRow (sharesList.GetSelectedItem ());
    UpdateDetails ();
    messageText.SetText (ShareText (ID_SHARE_STRINGS_CONFIRM_SENT));
}
