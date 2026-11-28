#pragma once

#include "UniString.hpp"
#include "Location.hpp"

#include <functional>

// What became of the scripts the Tapir Palette ran since Archicad started: the last run of each script, its output
// and how it ended, wherever it was started (the bar, a shortcut or the Script Manager). The Script Manager shows the
// result of the selected script next to its Run button: "Running...", "Finished", "Stopped", or an error in plain
// words, with Details (the last lines of the output) and a button that offers a fix (Install uv, Try again, or Report
// a problem).
// A text of the Scripts tab and its results (ID_LOCAL_SCRIPT_STRINGS of RINT/AddOn.grc).
GS::UniString LocalScriptText (Int32 index);

class ScriptRunResults
{
public:
    static ScriptRunResults& Instance ();

    // The number of output lines Details shows and a problem report includes.
    static constexpr UIndex OutputLineCount = 20;

    enum class RunState { Running, Finished, Stopped, Failed };
    enum class Failure { None, UvMissing, NotStarted, ConnectionBusy, ScriptError };

    struct RunResult
    {
        IO::Location             fileLocation;
        GS::UniString            scriptName;
        RunState                 state = RunState::Running;
        Failure                  failure = Failure::None;
        int                      exitCode = 0;
        bool                     stopRequested = false;
        bool                     sawConnectionError = false;   // the output shows that the connection to Archicad failed
        GS::Array<GS::UniString> lastLines;                    // at most OutputLineCount complete lines
        GS::UniString            partialLine;                  // the output after its last line break
    };

    // What the Script Manager shows for one script.
    struct Display
    {
        GS::UniString text;                  // the result line; empty when the script was not run
        bool          showOutputButton = false;
        Int32         fixTextIndex = 0;      // the index of the fix button's text in the strings; 0: no fix button
    };

    // The Script Manager sets this to be told when a result changed. Called on the main thread.
    void SetChangedHandler (const std::function<void ()>& handler);

    // What the Tapir Palette tells about the scripts it runs. All of them are called on the main thread.
    void ScriptStarted (const IO::Location& fileLocation, const GS::UniString& scriptName);
    // The script could not be started; uvMissing: because uv is not installed or could not be started.
    void ScriptNotStarted (const IO::Location& fileLocation, const GS::UniString& scriptName, bool uvMissing);
    // A part of what the script wrote to its standard output or error, as it was read.
    void ScriptOutput (const IO::Location& fileLocation, const GS::UniString& text);
    // Stop was used on the script: its exit code then means that it was stopped, not that it failed.
    void ScriptStopRequested (const IO::Location& fileLocation);
    void ScriptEnded (const IO::Location& fileLocation, int exitCode);

    const RunResult* Find (const IO::Location& fileLocation) const;
    // The result line of the script's last run with the buttons it offers. isRunning: the script runs now, which hides
    // Try again.
    Display Describe (const IO::Location& fileLocation, bool isRunning) const;

    // Details: an alert with the last lines of the output.
    void ShowOutputDetails (const RunResult& result) const;
    // Opens a new issue of this repository with the script's name, the Tapir and Archicad versions and the last lines of
    // its output filled in.
    void ReportProblem (const RunResult& result, const std::function<void (const GS::UniString&)>& openWebpage) const;

private:
    ScriptRunResults () = default;

    RunResult* FindMutable (const IO::Location& fileLocation);
    RunResult& ResetResult (const IO::Location& fileLocation, const GS::UniString& scriptName);
    static void AddOutputLine (RunResult& result, const GS::UniString& line);
    static GS::UniString GetLastLines (const RunResult& result, UIndex skippedLines = 0);
    void Changed () const;

    GS::Array<RunResult>       results;
    std::function<void ()>     changedHandler;
};
