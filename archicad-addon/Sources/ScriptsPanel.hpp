#pragma once

#include "DGModule.hpp"
#include "UniString.hpp"
#include "Location.hpp"
#include "ScriptInfo.hpp"

#include <functional>

// The content of the Tapir Scripts palette (#2407): the list of every script with what it does, its badge and Run and
// Stop. It is a palette of its own (ScriptsPalette), opened from the Tapir Palette's bar and from the Tapir menu, so
// that the bar keeps its size and its place in the toolbar. The palette's size is kept in
// Documents/Tapir/scripts-panel.json; Archicad keeps its position with the palette's guid.
//
// The panel lists the scripts of the Tapir Palette's dropdown, grouped by the category of their `# /// tapir` header,
// with a search field and a category filter above the list and the selected script's details below it. Under the
// details, Run and Stop start and stop the selected script through the Tapir Palette's own run and stop functions, so
// a script started here is the same run as one started from the bar, and the result of its last run shows next to
// them: "Running...", "Finished", "Stopped", or an error in plain words with Details (the last lines of its output)
// and a button that offers a fix (Install uv, Try again, or Report a problem).
//
// A right-click on a script offers Run (Stop while it runs), Assign Shortcut, Edit and Show in Folder for a local
// file or Open on GitHub for a repository's script, and Remove for a script the user added. A .py file dropped on the
// list is added as the bar's Add button adds it. While the user has no script of their own, the list starts with
// "New to Tapir? Try Project Health Check", which selects that built-in script. Settings, in the panel's footer,
// opens Documents/Tapir/config.json.
//
// The panel's controls are items 1-12 of ID_SCRIPTS_PALETTE. ScriptsPalette creates the panel, forwards its resize
// and button events to it, gives it the script list and its run and stop functions (Actions), and tells it when a
// script starts, writes output, is stopped and ends. The panel observes its search field, category filter and list
// itself.
class ScriptsPanel : public DG::TextEditBaseObserver,
    public DG::PopUpObserver,
    public DG::ListBoxObserver
{
public:
    // The palette can not be made smaller than this.
    static constexpr short MinWidth = 420;
    static constexpr short MinHeight = 320;

    // A script of the bar's list, as the panel shows it.
    struct Script
    {
        IO::Location  fileLocation;
        GS::UniString name;          // the docstring's title, or the file name when it has none
        GS::UniString fileName;
        GS::UniString source;        // Built-in, Marketplace, My scripts, or the folder's or repository's name
        GS::UniString badge;         // Changes the model, Writes files, Read only, or empty
        GS::UniString requirements;  // "Needs Tapir ..." and "Tested on Archicad ...", one per line, as in the bar's tooltip
        GS::UniString webUrl;        // the script's page on GitHub for a repository's script; empty for a local file
        bool          isAdded = false;     // added with the bar's Add button (or dropped on the panel): Remove removes it
        bool          isUsersOwn = false;  // added, or in the custom-scripts folder
        ScriptInfo    info;
    };
    using ScriptsGetter = std::function<GS::Array<Script> ()>;

    // What the panel asks of the Tapir Palette.
    struct Actions
    {
        ScriptsGetter                               getScripts;
        std::function<bool (const IO::Location&)>   isRunning;     // the script runs now
        std::function<void (const IO::Location&)>   run;           // starts it, as the bar's Run button does
        std::function<void (const IO::Location&)>   stop;          // stops it with the processes it started
        std::function<void (const GS::UniString&)>  openWebpage;   // opens the URL in the browser
        std::function<void ()>                      openShortcuts; // the Shortcuts dialog of the bar
        std::function<void (const IO::Location&)>   editFile;      // opens the file in a text editor
        std::function<void (const IO::Location&)>   showInFolder;  // opens the file's folder
        std::function<void (const IO::Location&)>   remove;        // removes a script the user added, as the bar's Remove button does
        std::function<void (const GS::Array<IO::Location>&)>  addScripts;   // adds the files, as the bar's Add button does
        std::function<void ()>                      openSettings;  // opens Documents/Tapir/config.json
    };

    // The items of the palette, in their order: search field, category filter, list, name, badge, details, Run, Stop,
    // result, Details, fix button, Settings.
    ScriptsPanel (DG::Palette& palette, const Actions& actions);
    ~ScriptsPanel ();

    // Applies the saved size. The palette calls it once, from its constructor.
    void RestoreState ();
    // Places the controls for the palette's current size.
    void Layout ();
    // After the palette was resized: Layout, and remember the size.
    void Resized ();
    // The bar's script list changed (Add, Remove, Reload scripts): reads it again and refills the list.
    void ScriptListChanged ();

    // The palette forwards the button events; this returns true when the button was the panel's.
    bool ButtonClicked (const DG::ButtonClickEvent& ev);

    // What the Tapir Palette tells the panel about the scripts it runs, wherever they were started (the bar, a shortcut or
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

    void SaveState ();
    static IO::Location GetStateFileLocation ();

    void FillCategoryPopUp ();
    void FillList ();
    void ShowDetails ();
    const Script* GetSelectedScript () const;
    const Script* GetScriptOfRow (short row) const;   // nullptr for a group's row and the other rows without a script
    // Selects the built-in Project Health Check, for the first-run row.
    void SelectFirstRunScript ();
    void ShowContextMenu (const Script& script);
    // The .py files of a drag over or a drop on the list.
    static GS::Array<IO::Location> GetDroppedScripts (const DG::ListBoxDropTargetEvent& ev);

    virtual void TextEditChanged (const DG::TextEditChangeEvent& ev) override;
    virtual void PopUpChanged (const DG::PopUpChangeEvent& ev) override;
    virtual void ListBoxSelectionChanged (const DG::ListBoxSelectionEvent& ev) override;
    virtual void ListBoxContextMenuRequested (const DG::ListBoxContextMenuEvent& ev, bool* processed) override;
    // Both overloads of ListBoxDragEntered, so that whichever DG calls accepts the files.
    virtual void ListBoxDragEntered (const DG::ListBoxDropTargetEvent& ev, DG::DragDrop::Effect* effect, bool* defBehaviour) override;
    virtual void ListBoxDragEntered (const DG::ListBoxDropTargetEvent& ev, DG::DragDrop::Effect* effect, bool* defBehaviour,
        bool* rightDragMenu) override;
    virtual void ListBoxDragMoved (const DG::ListBoxDropTargetEvent& ev, DG::DragDrop::Effect* effect,
        DG::DragDrop::Feedback* denyFeedback) override;
    virtual void ListBoxDropped (const DG::ListBoxDropTargetEvent& ev, DG::DragDrop::Effect* effect) override;

    DG::Palette&         palette;
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
    DG::Button           settingsButton; // the footer's Settings: opens Documents/Tapir/config.json

    Actions              actions;
    GS::Array<RunResult> results;
    bool                 outputButtonShown = false;
    bool                 fixButtonShown = false;
    GS::Array<Script>    scripts;              // as GetScripts returned them last; a list row's value is its index + 1
    IO::Location         selectedLocation;     // the selected script, kept when the list is refilled
    short                firstRunRow = 0;      // the "New to Tapir? Try Project Health Check" row; 0: not shown

    // What SaveState wrote last, so that a resize that changed nothing writes nothing.
    bool  hasSavedState = false;
    short savedWidth = 0;
    short savedHeight = 0;
};
