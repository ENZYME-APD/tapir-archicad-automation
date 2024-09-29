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
	DG::RichEdit   console;

    bool hasOwnScript;
    GS::Process process;
    GS::Ref<GS::Thread> executorThread;

    void SetMenuItemCheckedState (bool);
    bool IsPopUpContainsFile (const IO::Location& fileLocation) const;
    void ExecuteScript (const IO::Location& fileLocation);
    bool AddScriptToPopUp (const IO::Location& fileLocation, short index = DG::PopUp::TopItem);
    void SaveScriptsToPreferences ();
    short GetScriptsFromPreferences ();
    void UpdateConsol (const GS::UniString& text);

    virtual void PanelResized (const DG::PanelResizeEvent& ev) override;
    virtual void PanelCloseRequested (const DG::PanelCloseRequestEvent& ev, bool* accepted) override;
	virtual void ButtonClicked (const DG::ButtonClickEvent& ev) override;
	virtual void PopUpChanged (const DG::PopUpChangeEvent& ev) override;

    static GSErrCode PaletteControlCallBack (Int32 paletteId, API_PaletteMessageID messageID, GS::IntPtr param);
    static bool CCALL EnumFILEResIDsCallback (GSResID resID, GSResType resType, GSResModule resModule, void* userData);

    static const GS::Guid        paletteGuid;
    static GS::Ref<TapirPalette> instance;

    TapirPalette ();
};