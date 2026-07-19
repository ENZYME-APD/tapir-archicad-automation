#pragma once

#include "APIEnvir.h"
#include "ACAPinc.h"
#include "DGModule.hpp"
#include "ThreadedExecutor.hpp"
#include "Process.hpp"
#include "Config.hpp"
#include "UvManager.hpp"

#include <utility>

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

    static constexpr short ScriptShortcutSlotCount = 6;
    void RunShortcutSlot (short slotIndex);   // 0-based, 0..ScriptShortcutSlotCount-1

    void GetAvailableScripts (GS::Array<std::pair<GS::UniString, IO::Location>>& outScripts);
    IO::Location GetShortcutSlot (short slotIndex) const;   // 0-based; empty location = unassigned
    void SetShortcutSlot (short slotIndex, const IO::Location& location);   // empty location clears it
    GS::UniString GetShortcutLabel (short slotIndex) const;   // 0-based; empty = using the default "Run Script Shortcut N" text
    void SetShortcutLabel (short slotIndex, const GS::UniString& label);   // empty resets to the default text
    void OpenShortcutsDialog ();

    static GSErrCode RegisterPaletteControlCallBack ();

private:
    struct PopUpItemData : public GS::Object
    {
        IO::Location fileLocation;
        const GS::UniString repoRelLoc;
        const Config::Repository* repo = nullptr;
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
    void SetRunButtonIcon ();
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