#pragma once

#include "DGModule.hpp"
#include "UniString.hpp"
#include "Location.hpp"
#include "ScriptInfo.hpp"

#include <functional>

// The scripts panel of the Tapir Palette, below its bar (#2407). The palette is one palette in two sizes: the bar
// (BarHeight tall), and the bar with the panel under it. The panel opens when the palette is dragged taller than the
// bar, or with the expand arrow, the last control at the bar's right end, which opens it at its last height. The
// palette's size and whether the panel is open are kept in Documents/Tapir/scripts-panel.json; Archicad keeps its
// position with the palette's guid.
//
// The panel lists the scripts of the bar's dropdown, grouped by the category of their `# /// tapir` header, with a
// search field and a category filter above the list and the selected script's details below it. Under the details,
// Run and Stop start and stop the selected script through the palette's own run and stop functions, so a script
// started here is the same run as one started from the bar, and the result of its last run shows next to them:
// "Running...", "Finished", "Stopped", or an error in plain words with Details (the last lines of its output) and a
// button that offers a fix (Install uv, Try again, or Report a problem).
//
// The panel's controls are the items of ID_PALETTE below y BarHeight. TapirPalette creates the panel, forwards its
// resize and the button and tooltip events to it, gives it the script list and its run and stop functions (Actions),
// and tells it when a script starts, writes output, is stopped and ends. The panel observes its search field,
// category filter and list itself.
class ScriptsPanel : public DG::TextEditBaseObserver,
    public DG::PopUpObserver,
    public DG::ListBoxObserver
{
public:
    static constexpr short BarHeight = 28;
    // The palette's client height when the panel is opened with the arrow and was never open before.
    static constexpr short DefaultOpenHeight = 520;
    // A palette at least this much taller than the bar has the panel open; a lower one counts as the bar.
    static constexpr short MinOpenGrowth = 8;
    // A palette closed or left at a smaller height than this does not change the height the arrow opens it at.
    static constexpr short MinRememberedOpenHeight = BarHeight + 120;

    // A script of the bar's list, as the panel shows it.
    struct Script
    {
        IO::Location  fileLocation;
        GS::UniString name;          // the docstring's title, or the file name when it has none
        GS::UniString fileName;
        GS::UniString source;        // Built-in, Marketplace, My scripts, or the folder's or repository's name
        GS::UniString badge;         // Changes the model, Writes files, Read only, or empty
        GS::UniString requirements;  // "Needs Tapir ..." and "Tested on Archicad ...", one per line, as in the bar's tooltip
        ScriptInfo    info;
    };
    using ScriptsGetter = std::function<GS::Array<Script> ()>;

    // What the panel asks of the palette.
    struct Actions
    {
        ScriptsGetter                               getScripts;
        std::function<bool (const IO::Location&)>   isRunning;     // the script runs now
        std::function<void (const IO::Location&)>   run;           // starts it, as the bar's Run button does
        std::function<void (const IO::Location&)>   stop;          // stops it with the processes it started
        std::function<void (const GS::UniString&)>  openWebpage;   // opens the URL in the browser
    };

    // The item numbers of the expand arrow and of the panel's controls in ID_PALETTE, in their order there:
    // separator, search field, category filter, list, name, badge, details, Run, Stop, result, Details, fix button.
    ScriptsPanel (DG::Palette& palette, short expandButtonItem, short firstPanelItem, const Actions& actions);
    ~ScriptsPanel ();

    bool IsOpen () const;
    // Resizes the palette to the bar or to the panel's last height, and remembers it.
    void SetOpen (bool open);
    // Applies the saved size and open state. The palette calls it once, from its constructor.
    void RestoreState ();
    // Places the arrow and the panel's controls for the palette's current size, and shows the controls when the
    // panel is open.
    void Layout ();
    // After the palette was resized: Layout, and remember the size.
    void Resized ();
    // The bar's script list changed (Add, Remove, Reload scripts): reads it again and refills the list.
    void ScriptListChanged ();

    // The palette forwards the events of every item; these return true when the item was the panel's.
    bool ButtonClicked (const DG::ButtonClickEvent& ev);
    bool ToolTipRequested (const DG::ItemHelpEvent& ev, GS::UniString* toolTipText) const;

    // What the palette tells the panel about the scripts it runs, wherever they were started (the bar, a shortcut or
    // the panel). All of them are called on the main thread.
    void ScriptStarted (const IO::Location& fileLocation, const GS::UniString& scriptName);
    // The script could not be started; uvMissing: because uv is not installed or could not be started.
    void ScriptNotStarted (const IO::Location& fileLocation, const GS::UniString& scriptName, bool uvMissing);
    // A part of what the script wrote to its standard output or error, as it was read.
    void ScriptOutput (const IO::Location& fileLocation, const GS::UniString& text);
    // Stop was used on the script: its exit code then means that it was stopped, not that it failed.
    void ScriptStopRequested (const IO::Location& fileLocation);
    void ScriptEnded (const IO::Location& fileLocation, int exitCode);

    // The number of output lines Details shows and a problem report includes.
    static constexpr UIndex OutputLineCount = 20;

private:
    enum class RunState { Running, Finished, Stopped, Failed };
    enum class Failure { None, UvMissing, NotStarted, ConnectionBusy, ScriptError };

    // The last run of a script since Archicad started.
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

    RunResult* FindResult (const IO::Location& fileLocation);
    const RunResult* FindResult (const IO::Location& fileLocation) const;
    RunResult& ResetResult (const IO::Location& fileLocation, const GS::UniString& scriptName);
    static void AddOutputLine (RunResult& result, const GS::UniString& line);
    static GS::UniString GetLastLines (const RunResult& result, UIndex skippedLines = 0);
    // Shows the selected script's last result with the buttons it offers, and enables Run and Stop for the script.
    void ShowResult ();
    void ShowOutputDetails (const RunResult& result) const;
    void ReportProblem (const RunResult& result) const;

    void SetExpandButtonIcon (bool open);
    void SaveState ();
    static IO::Location GetStateFileLocation ();

    void FillCategoryPopUp ();
    void FillList ();
    void ShowDetails ();
    const Script* GetSelectedScript () const;

    virtual void TextEditChanged (const DG::TextEditChangeEvent& ev) override;
    virtual void PopUpChanged (const DG::PopUpChangeEvent& ev) override;
    virtual void ListBoxSelectionChanged (const DG::ListBoxSelectionEvent& ev) override;

    DG::Palette&         palette;
    DG::IconButton       expandButton;
    DG::Separator        separator;
    DG::TextEdit         searchEdit;
    DG::PopUp            categoryPopUp;
    DG::SingleSelListBox scriptList;
    DG::LeftText         nameText;
    DG::LeftText         badgeText;
    DG::LeftText         detailsText;
    DG::Button           runButton;
    DG::Button           stopButton;
    DG::LeftText         resultText;
    DG::Button           outputButton;   // Details: the last lines of the script's output
    DG::Button           fixButton;      // Install uv, Try again or Report a problem

    Actions              actions;
    GS::Array<RunResult> results;
    bool                 outputButtonShown = false;
    bool                 fixButtonShown = false;
    GS::Array<Script>    scripts;              // as GetScripts returned them last; a list row's value is its index + 1
    IO::Location         selectedLocation;     // the selected script, kept when the list is refilled

    short openHeight = DefaultOpenHeight;   // the client height the arrow opens the panel at
    bool  expandButtonShowsOpen = false;    // the arrow's icon is the up chevron

    // What SaveState wrote last, so that a resize that changed nothing writes nothing.
    bool  hasSavedState = false;
    short savedWidth = 0;
    short savedOpenHeight = 0;
    bool  savedOpen = false;
};
