#pragma once

#include "APIEnvir.h"
#include "ACAPinc.h"
#include "DGModule.hpp"
#include "ThreadedExecutor.hpp"
#include "Process.hpp"
#include "Config.hpp"
#include "UvManager.hpp"

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
        GS::UniString baseDisplayText;   // label as computed by AddScriptToPopUp, before any shortcut-slot suffix

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

    GS::Process process;
    GS::ThreadedExecutor executor;
    bool hasCustomScript = false;
    bool hasAddedScript = false;
    UvManager uvManager;
    IO::Location scriptShortcutSlots[ScriptShortcutSlotCount];    // empty = unassigned
    GS::UniString scriptShortcutLabels[ScriptShortcutSlotCount];  // empty = using the default menu text

    void SetMenuItemCheckedState (bool);
    void ExecuteScript (const PopUpItemData& popUpItemData);
    bool AddScriptToPopUp (GS::Ref<PopUpItemData> popUpData, short index = DG::PopUp::TopItem);
    void AddScriptsFromRepositories ();
    void AddScriptsFromLocalFolders ();
    void AddScriptsFromCustomScriptsFolder ();
    void LoadScriptsToPopUp ();
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
                typeStr = "ERROR: ";
                DGAlert (DG_ERROR, "Tapir Script Execution", "Error", GS::UniString::Printf (format, std::forward<Args> (args)...), "OK");
            } break;
        case DG_WARNING:
            typeStr = "WARNING: ";
            break;
        default:
            break;
    }

    constexpr bool withDial = false;
    ACAPI_WriteReport ("----- Tapir Script Execution Report -----\n" + typeStr + format, withDial, std::forward<Args> (args)...);
}