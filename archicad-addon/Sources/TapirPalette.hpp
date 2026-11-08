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

#include <atomic>
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

    bool UpdateAddOn ();
    void SetRunButtonIcon ();   // also called once the VersionChecker's lookup completes
    // The "Check for Updates..." menu item and the palette's update button: updates when a newer version exists,
    // otherwise tells why there is nothing to update to. Also offered when a script needs a newer Tapir.
    void CheckForUpdates ();
    void SetUpdateButtonText ();   // also called once the VersionChecker's lookup completes
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

    // Called from FreeData: kills a script that is still running and waits for its output reader thread, so that
    // the thread does not outlive the add-on (Archicad crashed on quit while a palette script was running).
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
    DG::Button     checkForUpdatesButton;

    GS::Process process;
    GS::UniString runningCommand;   // the command process was started with, to find the process among the child processes of Archicad
    GS::Array<GS::UInt64> childProcessIdsBeforeStart;   // the child processes of Archicad that ran before process was started, so are not it
    GS::UInt32 startedScriptCount = 0;   // numbers the TAPIR_SCRIPT_ID of each started script
    GS::ThreadedExecutor executor;
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
    void KillRunningProcess ();   // ends the running process together with the processes it started
    bool PrepareScript (PopUpItemData& popUpData);   // sets its label and reads its info; false: the file cannot be listed
    void AddScriptsFromRepositories ();
    void AddScriptsFromLocalFolders ();
    void AddScriptsFromCustomScriptsFolder ();
    void LoadScriptsToPopUp ();
    void RebuildScriptPopUp (short itemToSelect);   // fills scriptSelectionPopUp from the script list
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
    bool IsProcessRunning () {
        return process.IsValid ();
    }


    template<typename... Args>
    void WriteReport (short type, const GS::UniString& format, Args&&... args);

    virtual void PanelCloseRequested (const DG::PanelCloseRequestEvent& ev, bool* accepted) override;
    virtual void PanelOpened (const DG::PanelOpenEvent& ev) override;
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