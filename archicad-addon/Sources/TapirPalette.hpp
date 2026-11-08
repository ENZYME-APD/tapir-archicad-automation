#pragma once

#include "APIEnvir.h"
#include "ACAPinc.h"
#include "DGModule.hpp"
#include "ThreadedExecutor.hpp"
#include "Process.hpp"
#include "Config.hpp"
#include "UvManager.hpp"
#include "ScriptInfo.hpp"
#include "ResourceIds.hpp"
#include "ScriptsPanel.hpp"

#include <atomic>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

class TapirPalette final : public DG::Palette,
    public DG::PanelObserver,
    public DG::ButtonItemObserver,
    public DG::PopUpObserver,
    public DG::CompoundItemObserver
{
public:
    virtual ~TapirPalette ();

    static bool          HasInstance ();
    static TapirPalette& Instance ();

    void Show ();
    void Hide ();
    // The "Scripts Panel" menu item: shows the palette with its scripts panel open.
    void ShowScriptsPanel ();

    bool UpdateAddOn ();
    void SetRunButtonIcon ();   // also called once the VersionChecker's lookup completes
    // The "Check for Updates..." menu item and the palette's update button: updates when a newer version exists,
    // otherwise tells why there is nothing to update to. Also offered when a script needs a newer Tapir.
    void CheckForUpdates ();
    // The update button's icon: a mark on it when a newer version exists. Also called once the VersionChecker's
    // lookup completes.
    void SetUpdateButtonIcon ();
    // Shows "Update Tapir to <version>..." on the update menu item when a newer version exists, and keeps the item
    // enabled. Needs no palette, so it is static.
    static void ApplyUpdateMenuItemState ();

    static constexpr short ScriptShortcutSlotCount = 6;
    void RunShortcutSlot (short slotIndex);   // 0-based, 0..ScriptShortcutSlotCount-1

    void GetAvailableScripts (GS::Array<std::pair<GS::UniString, IO::Location>>& outScripts);
    IO::Location GetShortcutSlot (short slotIndex) const;   // 0-based; empty location = unassigned
    void SetShortcutSlot (short slotIndex, const IO::Location& location);   // empty location clears it
    GS::UniString GetShortcutLabel (short slotIndex) const;   // 0-based; empty = using the default "Run Script Shortcut N" text
    void SetShortcutLabel (short slotIndex, const GS::UniString& label);   // empty resets to the default text
    void OpenShortcutsDialog ();

    static GSErrCode RegisterPaletteControlCallBack ();

    // Called from FreeData: kills every script that is still running, with the processes it started, and waits for
    // their output reader threads, so that no thread outlives the add-on (Archicad crashed on quit while a palette
    // script was running).
    static void StopRunningScriptOnQuit ();

private:
    struct PopUpItemData : public GS::Object
    {
        IO::Location fileLocation;
        const GS::UniString repoRelLoc;
        const Config::Repository* repo = nullptr;
        GS::UniString sourceName;        // shown after the file name for scripts from a configured local folder; empty for repository and user-added scripts
        GS::UniString baseDisplayText;   // label as computed by PrepareScript, before any shortcut-slot suffix
        GS::UniString uvProjectPath;     // the folder of the uv project (uvProject in the config) the script runs in; empty: run it with --script
        ScriptInfo info;                 // the script's docstring summary and `# /// tapir` header, read by PrepareScript

        explicit PopUpItemData (
            const IO::Location& _fileLocation,
            const GS::UniString& _repoRelLoc = GS::EmptyUniString,
            const Config::Repository* _repo = nullptr)
            : fileLocation(_fileLocation), repoRelLoc(_repoRelLoc), repo(_repo) {}
    };

    DG::IconButton tapirButton;
    DG::PopUp      scriptSelectionPopUp;
    DG::IconButton runScriptButton;
    DG::IconButton openScriptButton;
    DG::IconButton addScriptButton;
    DG::IconButton delScriptButton;
    DG::IconButton manageShortcutsButton;
    DG::LeftText   installedVersionText;
    DG::IconButton checkForUpdatesButton;
    ScriptsPanel   scriptsPanel;   // the expand arrow and the controls below the bar

    // A script started by the palette, until it ended or was stopped. Any number of scripts run at the same time,
    // but a script at most once: Run, and its shortcut slot, stop it while it runs (issue #1819).
    struct RunningScript
    {
        IO::Location fileLocation;   // the script, as in the script list
        GS::UniString scriptName;    // its file name, which starts each of its lines in the Report window
        GS::UniString scriptId;      // its TAPIR_SCRIPT_ID
        GS::Process process;
        // The process this Archicad started for the script, found among its child processes just after the start
        // (the palette gives no process id). Stop ends it together with its descendants.
        bool startedProcessFound = false;
        GS::UInt64 startedProcessId = 0;
        GS::UInt64 startedProcessStartTime = 0;
        std::atomic<bool> ended { false };   // set by the output reader once the process ended and its output is read
        GS::ThreadedExecutor executor;       // runs the output reader of this script
    };

    std::vector<std::unique_ptr<RunningScript>> runningScripts;   // touched on the main thread only
    GS::UInt32 startedScriptCount = 0;   // numbers the TAPIR_SCRIPT_ID of each started script
    // The script list. scriptSelectionPopUp only shows it (RebuildScriptPopUp), in this order: the scripts the user
    // added, a separator when hasAddedScript, the scripts of the custom-scripts folder, a separator when
    // hasCustomScript, the scripts of the local folders and repositories, a separator and "Reload scripts...".
    using ScriptList = GS::Array<GS::Ref<PopUpItemData>>;
    ScriptList addedScripts;    // added with the Add button, saved in the preferences; the newest first
    ScriptList customScripts;   // Documents/Tapir/custom-scripts
    ScriptList listedScripts;   // localFolders and repositories of the config
    bool hasCustomScript = false;   // the custom-scripts section and its separator are shown
    bool hasAddedScript = false;    // the added section and its separator are shown
    UvManager uvManager;
    IO::Location scriptShortcutSlots[ScriptShortcutSlotCount];    // empty = unassigned
    GS::UniString scriptShortcutLabels[ScriptShortcutSlotCount];  // empty = using the default menu text

    void SetMenuItemCheckedState (bool);
    void ExecuteScript (const PopUpItemData& popUpItemData);   // every path that starts a script calls this
    bool ConfirmScriptRequirements (const PopUpItemData& popUpItemData);   // min-tapir-version and archicad-versions; false: do not run
    // Ends the script's process together with the processes it started; reportNotFound: warn when they cannot
    // be found.
    void KillScript (RunningScript& script, bool reportNotFound);
    RunningScript* FindRunningScript (const IO::Location& fileLocation) const;   // nullptr: the script does not run
    bool IsScriptRunning (const IO::Location& fileLocation) const {
        return FindRunningScript (fileLocation) != nullptr;
    }
    // Run, the Run button and the shortcut slots: stops the script when it runs, otherwise starts it.
    void RunOrStopScript (const PopUpItemData& popUpItemData);
    // Called on the main thread once the output reader of the script finished.
    void RemoveEndedScript (const GS::UniString& scriptId);
    bool PrepareScript (PopUpItemData& popUpData);   // sets its label and reads its info; false: the file cannot be listed
    void AddScriptsFromRepositories ();
    void AddScriptsFromLocalFolders ();
    void AddScriptsFromCustomScriptsFolder ();
    void LoadScriptsToPopUp ();
    void RebuildScriptPopUp (short itemToSelect);   // fills scriptSelectionPopUp from the script list
    // Marks each running script in scriptSelectionPopUp; called when a script starts or ends and on each rebuild.
    void RefreshRunningMarks ();
    GS::Ref<PopUpItemData> GetScriptOfPopUpItem (short item) const;   // nullptr for a separator and "Reload scripts..."
    GS::Ref<PopUpItemData> GetSelectedScript () const;
    ScriptList GetAllScripts () const;   // in the order of the dropdown
    GS::UniString GetScriptLabel (const PopUpItemData& popUpData) const;   // the label with the "[Shortcut N]" suffix
    bool IsPopUpContainsFile (const IO::Location& fileLocation) const;
    void SaveScriptsToPreferences ();
    bool IsValidLocation (const IO::Location& location);
    short AddScriptsFromPreferences ();
    bool AddNewScript ();
    void DeleteScriptFromPopUp ();
    bool IsSelectedScriptFromGitHub () const;
    void SetDeleteScriptButtonStatus ();
    void RefreshScriptListShortcutLabels ();   // appends "  [Shortcut N]" to scripts assigned to a slot
    void ApplyShortcutMenuItemText (short slotIndex);   // pushes scriptShortcutLabels[slotIndex] to the real Archicad menu item
    void ApplyAllShortcutMenuItemTexts ();

    template<typename... Args>
    void WriteReport (short type, const GS::UniString& format, Args&&... args);

    virtual void PanelCloseRequested (const DG::PanelCloseRequestEvent& ev, bool* accepted) override;
    virtual void PanelOpened (const DG::PanelOpenEvent& ev) override;
    virtual void PanelResized (const DG::PanelResizeEvent& ev) override;
    virtual void ButtonClicked (const DG::ButtonClickEvent& ev) override;
    virtual void PopUpChanged (const DG::PopUpChangeEvent& ev) override;
    // Overrides the ItemObserver function of both the button and the pop-up observer bases.
    virtual void ItemToolTipRequested (const DG::ItemHelpEvent& ev, GS::UniString* toolTipText) override;

    static GS::UniString GetScriptDescription (const ScriptInfo& info);   // the summary, the badge and the requirements, one per line

    static GSErrCode PaletteControlCallBack (Int32 paletteId, API_PaletteMessageID messageID, GS::IntPtr param);

    static const GS::Guid        paletteGuid;
    static GS::Ref<TapirPalette> instance;
    static std::atomic<bool>     quitting;   // set by StopRunningScriptOnQuit; the output reader then stops at once

    // The output reader's tasks posted to the message loop. A task still queued when the add-on is unloaded must
    // not be destroyed afterwards (its destructor is in this add-on), so the palette keeps a reference to each one
    // until it has run (StopRunningScriptOnQuit).
    static std::mutex                    postedTasksMutex;
    static std::vector<GS::RunnableTask> postedTasks;    // posted, not run yet
    static std::vector<GS::RunnableTask> finishedTasks;  // run; released on the next post, not inside their own Run
    static void KeepPostedTask (const GS::RunnableTask& task);
    static void ForgetPostedTask (const GS::Runnable* task);

    TapirPalette ();
};

template<typename... Args>
void TapirPalette::WriteReport (short type, const GS::UniString& format, Args&&... args)
{
    GS::UniString typeStr;
    switch (type) {
        case DG_ERROR: {
                typeStr = RSGetIndString (ID_REPORT_STRINGS, ID_REPORT_STRINGS_ERROR_PREFIX, ACAPI_GetOwnResModule ()) + " ";
                DGAlert (DG_ERROR,
                    RSGetIndString (ID_PALETTE_STRINGS, ID_PALETTE_STRINGS_EXECUTION_ERROR_TITLE, ACAPI_GetOwnResModule ()),
                    RSGetIndString (ID_PALETTE_STRINGS, ID_PALETTE_STRINGS_EXECUTION_ERROR_TEXT, ACAPI_GetOwnResModule ()),
                    GS::UniString::Printf (format, std::forward<Args> (args)...),
                    RSGetIndString (ID_PALETTE_STRINGS, ID_PALETTE_STRINGS_OK, ACAPI_GetOwnResModule ()));
            } break;
        case DG_WARNING:
            typeStr = RSGetIndString (ID_REPORT_STRINGS, ID_REPORT_STRINGS_WARNING_PREFIX, ACAPI_GetOwnResModule ()) + " ";
            break;
        default:
            break;
    }

    constexpr bool withDial = false;
    ACAPI_WriteReport (RSGetIndString (ID_REPORT_STRINGS, ID_REPORT_STRINGS_SCRIPT_REPORT_HEADER, ACAPI_GetOwnResModule ()) + "\n" + typeStr + format,
        withDial, std::forward<Args> (args)...);
}