#pragma once

#include "APIEnvir.h"
#include "ACAPinc.h"
#include "DGModule.hpp"
#include "Thread.hpp"
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
    DG::IconItem   tapirIcon;
    DG::PopUp      scriptSelectionPopUp;
    DG::IconButton runScriptButton;

    bool hasCustomScript = false;
    GS::Process process;
    IO::Location tmpACLibInitPyLoc;

    void SetMenuItemCheckedState (bool);
    bool IsPopUpContainsFile (const IO::Location& fileLocation) const;
    void ExecuteScript (const IO::Location& fileLocation);
    bool AddScriptToPopUp (const IO::Location& fileLocation, short index = DG::PopUp::TopItem);
    void AddBuiltInScripts ();
    void AddScriptsFromCustomScriptsFolder ();

    template<typename... Args>
    void WriteReport (short type, const GS::UniString& format, Args&&... args);

    virtual void PanelCloseRequested (const DG::PanelCloseRequestEvent& ev, bool* accepted) override;
	virtual void PanelIdle (const DG::PanelIdleEvent& ev) override;
	virtual void ButtonClicked (const DG::ButtonClickEvent& ev) override;
	virtual void PopUpChanged (const DG::PopUpChangeEvent& ev) override;

    static GSErrCode PaletteControlCallBack (Int32 paletteId, API_PaletteMessageID messageID, GS::IntPtr param);
    static bool CCALL EnumFILEResBuiltInScriptsCallback (GSResID resID, GSResType resType, GSResModule resModule, void* userData);

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
    ACAPI_WriteReport ("Tapir Script Execution Report:\n" + typeStr + format, withDial, std::forward<Args> (args)...);
}