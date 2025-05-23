#pragma once

#include "APIEnvir.h"
#include "ACAPinc.h"
#include "DGModule.hpp"
#include "ThreadedExecutor.hpp"
#include "Process.hpp"

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

    static GSErrCode RegisterPaletteControlCallBack ();

private:
    DG::IconButton tapirButton;
    DG::PopUp      scriptSelectionPopUp;
    DG::IconButton runScriptButton;
    DG::IconButton openScriptButton;
    DG::IconButton addScriptButton;
    DG::IconButton delScriptButton;

    GS::Process process;
    GS::ThreadedExecutor executor;
    bool hasCustomScript = false;
    bool hasAddedScript = false;

    void SetMenuItemCheckedState (bool);
    void ExecuteScript (const IO::Location& fileLocation);
    bool AddScriptToPopUp (const IO::Location& fileLocation, short index = DG::PopUp::TopItem);
    void AddBuiltInScriptsFromGithub ();
    void AddScriptsFromCustomScriptsFolder ();
    void LoadScriptsToPopUp ();
    bool IsPopUpContainsFile (const IO::Location& fileLocation) const;
    void SaveScriptsToPreferences ();
    void AddScriptsFromPreferences ();
    bool AddNewScript ();
    void DeleteScriptFromPopUp ();
    bool IsSelectedScriptFromGitHub () const;
    void SetDeleteScriptButtonStatus ();

    template<typename... Args>
    void WriteReport (short type, const GS::UniString& format, Args&&... args);

    virtual void PanelCloseRequested (const DG::PanelCloseRequestEvent& ev, bool* accepted) override;
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