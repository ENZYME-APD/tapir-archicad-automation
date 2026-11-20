#pragma once

#include "APIEnvir.h"
#include "ACAPinc.h"
#include "DGModule.hpp"
#include "ScriptsPanel.hpp"

// The Tapir Scripts palette (#2407): the scripts panel in a palette of its own. It opens from the button at the right
// end of the Tapir Palette's bar and from the Tapir menu, and it is resizable. The bar keeps its size, so that it
// stays where the user docked it. See ScriptsPanel.hpp for what the panel shows.
class ScriptsPalette final : public DG::Palette,
    public DG::PanelObserver,
    public DG::ButtonItemObserver,
    public DG::CompoundItemObserver
{
public:
    virtual ~ScriptsPalette ();

    static bool            HasInstance ();
    // Creates the palette when it does not exist yet, with the Tapir Palette's script list and run and stop functions.
    static ScriptsPalette& Instance ();

    // Opens the palette, or brings it to the front when it is open.
    void Show ();
    void Hide ();
    // Opens the palette, or closes it when it is open.
    void Toggle ();

    ScriptsPanel& GetPanel ();

    static GSErrCode RegisterPaletteControlCallBack ();

    // Tears the palette down. Called from FreeData.
    static void Release ();

private:
    ScriptsPalette ();

    virtual void PanelCloseRequested (const DG::PanelCloseRequestEvent& ev, bool* accepted) override;
    virtual void PanelResized (const DG::PanelResizeEvent& ev) override;
    virtual void ButtonClicked (const DG::ButtonClickEvent& ev) override;

    static GSErrCode PaletteControlCallBack (Int32 referenceID, API_PaletteMessageID messageID, GS::IntPtr param);

    ScriptsPanel  panel;
    bool          hiddenByArchicad = false;

    static const GS::Guid                paletteGuid;
    static GS::Ref<ScriptsPalette>       instance;
};
