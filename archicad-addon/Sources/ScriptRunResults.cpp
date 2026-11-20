#include "ScriptRunResults.hpp"
#include "ResourceIds.hpp"
#include "APIEnvir.h"
#include "ACAPinc.h"
#include "DGModule.hpp"
#include "AddOnVersion.hpp"
#include "VersionChecker.hpp"

#include <string>

// The texts of the results, from the localized resources (RINT/AddOn.grc, see LOCALIZATION.md)
static GS::UniString RunText (Int32 index)
{
    return RSGetIndString (ID_LOCAL_SCRIPT_STRINGS, index, ACAPI_GetOwnResModule ());
}

static GS::UniString LowerCased (const GS::UniString& text)
{
    GS::UniString result = text;
    result.SetToLowerCase ();
    return result;
}

ScriptRunResults& ScriptRunResults::Instance ()
{
    static ScriptRunResults instance;
    return instance;
}

void ScriptRunResults::SetChangedHandler (const std::function<void ()>& handler)
{
    changedHandler = handler;
}

void ScriptRunResults::Changed () const
{
    if (changedHandler) {
        changedHandler ();
    }
}

const ScriptRunResults::RunResult* ScriptRunResults::Find (const IO::Location& fileLocation) const
{
    for (const RunResult& result : results) {
        if (result.fileLocation == fileLocation) {
            return &result;
        }
    }
    return nullptr;
}

ScriptRunResults::RunResult* ScriptRunResults::FindMutable (const IO::Location& fileLocation)
{
    for (RunResult& result : results) {
        if (result.fileLocation == fileLocation) {
            return &result;
        }
    }
    return nullptr;
}

ScriptRunResults::RunResult& ScriptRunResults::ResetResult (const IO::Location& fileLocation, const GS::UniString& scriptName)
{
    RunResult* result = FindMutable (fileLocation);
    if (result == nullptr) {
        results.Push (RunResult ());
        result = &results.GetLast ();
    } else {
        *result = RunResult ();
    }
    result->fileLocation = fileLocation;
    result->scriptName = scriptName;
    return *result;
}

void ScriptRunResults::ScriptStarted (const IO::Location& fileLocation, const GS::UniString& scriptName)
{
    ResetResult (fileLocation, scriptName);
    Changed ();
}

void ScriptRunResults::ScriptNotStarted (const IO::Location& fileLocation, const GS::UniString& scriptName, bool uvMissing)
{
    RunResult& result = ResetResult (fileLocation, scriptName);
    result.state = RunState::Failed;
    result.failure = uvMissing ? Failure::UvMissing : Failure::NotStarted;
    Changed ();
}

// What a failed connection to Archicad leaves in the output of a script that uses aclib or the archicad package:
// urllib's and http.client's exceptions, in lower case.
static bool ShowsConnectionError (const GS::UniString& lowerCasedText)
{
    static const char* const markers[] = {
        "connectionrefusederror",
        "connectionreseterror",
        "connectionabortederror",
        "remotedisconnected",
        "urlopen error",
        "timed out",
        "timeouterror"
    };
    for (const char* marker : markers) {
        if (lowerCasedText.Contains (GS::UniString (marker))) {
            return true;
        }
    }
    return false;
}

static constexpr USize MaxOutputLineLength = 300;

void ScriptRunResults::AddOutputLine (RunResult& result, const GS::UniString& line)
{
    GS::UniString shortLine = line;
    if (shortLine.EndsWith (GS::UniString ("\r"))) {
        shortLine = GS::UniString (shortLine.GetSubstring (0, shortLine.GetLength () - 1));
    }
    if (shortLine.GetLength () > MaxOutputLineLength) {
        shortLine = GS::UniString (shortLine.GetSubstring (0, MaxOutputLineLength)) + "...";
    }
    result.lastLines.Push (shortLine);
    while (result.lastLines.GetSize () > OutputLineCount) {
        result.lastLines.Delete (0);
    }
}

void ScriptRunResults::ScriptOutput (const IO::Location& fileLocation, const GS::UniString& text)
{
    RunResult* result = FindMutable (fileLocation);
    if (result == nullptr || text.IsEmpty ()) {
        return;
    }
    if (ShowsConnectionError (LowerCased (text))) {
        result->sawConnectionError = true;
    }

    // Only whole lines are kept; the rest waits for the next part of the output.
    const GS::UniString output = result->partialLine + text;
    const USize length = output.GetLength ();
    UIndex lineStart = 0;
    for (UIndex i = 0; i < length; ++i) {
        if (output[i] == '\n') {
            AddOutputLine (*result, GS::UniString (output.GetSubstring (lineStart, i - lineStart)));
            lineStart = i + 1;
        }
    }
    result->partialLine = lineStart < length ? GS::UniString (output.GetSubstring (lineStart, length - lineStart)) : GS::EmptyUniString;
    if (result->partialLine.GetLength () > 4 * MaxOutputLineLength) {
        AddOutputLine (*result, result->partialLine);
        result->partialLine.Clear ();
    }
}

void ScriptRunResults::ScriptStopRequested (const IO::Location& fileLocation)
{
    RunResult* result = FindMutable (fileLocation);
    if (result != nullptr && result->state == RunState::Running) {
        result->stopRequested = true;
    }
}

void ScriptRunResults::ScriptEnded (const IO::Location& fileLocation, int exitCode)
{
    RunResult* result = FindMutable (fileLocation);
    if (result == nullptr || result->state != RunState::Running) {
        return;
    }
    if (!result->partialLine.IsEmpty ()) {
        AddOutputLine (*result, result->partialLine);
        result->partialLine.Clear ();
    }
    result->exitCode = exitCode;
    if (result->stopRequested) {
        result->state = RunState::Stopped;
    } else if (exitCode == 0) {
        result->state = RunState::Finished;
    } else {
        result->state = RunState::Failed;
        result->failure = result->sawConnectionError ? Failure::ConnectionBusy : Failure::ScriptError;
    }
    Changed ();
}

GS::UniString ScriptRunResults::GetLastLines (const RunResult& result, UIndex skippedLines)
{
    GS::UniString text;
    for (UIndex i = skippedLines; i < result.lastLines.GetSize (); ++i) {
        if (!text.IsEmpty ()) {
            text += "\n";
        }
        text += result.lastLines[i];
    }
    return text;
}

ScriptRunResults::Display ScriptRunResults::Describe (const IO::Location& fileLocation, bool isRunning) const
{
    Display display;
    const RunResult* result = Find (fileLocation);
    if (result == nullptr) {
        return display;
    }
    switch (result->state) {
        case RunState::Running:
            display.text = RunText (ID_LOCAL_SCRIPT_STRINGS_RUNNING);
            break;
        case RunState::Finished:
            display.text = RunText (ID_LOCAL_SCRIPT_STRINGS_FINISHED);
            break;
        case RunState::Stopped:
            display.text = RunText (ID_LOCAL_SCRIPT_STRINGS_STOPPED);
            break;
        case RunState::Failed:
            switch (result->failure) {
                case Failure::UvMissing:
                    display.text = RunText (ID_LOCAL_SCRIPT_STRINGS_ERROR_UV_MISSING);
                    display.fixTextIndex = ID_LOCAL_SCRIPT_STRINGS_INSTALL_UV;
                    break;
                case Failure::NotStarted:
                    display.text = RunText (ID_LOCAL_SCRIPT_STRINGS_ERROR_NOT_STARTED);
                    display.fixTextIndex = ID_LOCAL_SCRIPT_STRINGS_REPORT_PROBLEM;
                    break;
                case Failure::ConnectionBusy:
                    display.text = RunText (ID_LOCAL_SCRIPT_STRINGS_ERROR_CONNECTION_BUSY);
                    display.showOutputButton = true;
                    display.fixTextIndex = ID_LOCAL_SCRIPT_STRINGS_TRY_AGAIN;
                    break;
                case Failure::ScriptError:
                case Failure::None:
                    display.text = GS::UniString::Printf (RunText (ID_LOCAL_SCRIPT_STRINGS_ERROR_EXIT_CODE), result->exitCode);
                    display.showOutputButton = true;
                    display.fixTextIndex = ID_LOCAL_SCRIPT_STRINGS_REPORT_PROBLEM;
                    break;
            }
            break;
    }
    // Try again starts the script, so it is offered only while the script does not run.
    if (isRunning && display.fixTextIndex == ID_LOCAL_SCRIPT_STRINGS_TRY_AGAIN) {
        display.fixTextIndex = 0;
    }
    return display;
}

void ScriptRunResults::ShowOutputDetails (const RunResult& result) const
{
    const GS::UniString lines = GetLastLines (result);
    DGAlert (DG_INFORMATION,
        RunText (ID_LOCAL_SCRIPT_STRINGS_OUTPUT_TITLE),
        GS::UniString::Printf (RunText (ID_LOCAL_SCRIPT_STRINGS_OUTPUT_HEADER), result.scriptName.ToPrintf ()),
        lines.IsEmpty () ? RunText (ID_LOCAL_SCRIPT_STRINGS_OUTPUT_EMPTY) : lines,
        RSGetIndString (ID_PALETTE_STRINGS, ID_PALETTE_STRINGS_OK, ACAPI_GetOwnResModule ()));
}

// The text percent-encoded as UTF-8 for a URL's query, every byte but the unreserved characters encoded.
static GS::UniString PercentEncoded (const GS::UniString& text)
{
    static const char hexDigits[] = "0123456789ABCDEF";
    const std::string bytes (text.ToCStr (CC_UTF8).Get ());
    std::string encoded;
    for (const char c : bytes) {
        const unsigned char byte = static_cast<unsigned char> (c);
        if ((byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z') || (byte >= '0' && byte <= '9') ||
            byte == '-' || byte == '_' || byte == '.' || byte == '~') {
            encoded += static_cast<char> (byte);
        } else {
            encoded += '%';
            encoded += hexDigits[byte >> 4];
            encoded += hexDigits[byte & 0x0F];
        }
    }
    return GS::UniString (encoded.c_str ());
}

// The oldest lines are left out when the address would be too long for the browser or GitHub.
void ScriptRunResults::ReportProblem (const RunResult& result, const std::function<void (const GS::UniString&)>& openWebpage) const
{
    static constexpr USize MaxEncodedBodyLength = 6000;
    const GS::UniString title = GS::UniString::Printf (RunText (ID_LOCAL_SCRIPT_STRINGS_ISSUE_TITLE), result.scriptName.ToPrintf ());
    const GS::UniString tapirVersion (ADDON_VERSION);
    GS::UniString encodedBody;
    for (UIndex skippedLines = 0; skippedLines <= result.lastLines.GetSize (); ++skippedLines) {
        const GS::UniString body = GS::UniString::Printf (RunText (ID_LOCAL_SCRIPT_STRINGS_ISSUE_BODY),
            result.scriptName.ToPrintf (), tapirVersion.ToPrintf (), static_cast<int> (VersionChecker::ArchicadMainVersion ()),
            GetLastLines (result, skippedLines).ToPrintf ());
        encodedBody = PercentEncoded (body);
        if (encodedBody.GetLength () <= MaxEncodedBodyLength) {
            break;
        }
    }
    openWebpage ("https://github.com/ENZYME-APD/tapir-archicad-automation/issues/new?title=" +
        PercentEncoded (title) + "&body=" + encodedBody);
}
