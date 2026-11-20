#include "ScriptsPalette.hpp"
#include "TapirPalette.hpp"
#include "ResourceIds.hpp"

const GS::Guid          ScriptsPalette::paletteGuid ("{2EB0D1D7-FA1A-421B-9BC0-C70D2A46B97A}");
GS::Ref<ScriptsPalette> ScriptsPalette::instance;

ScriptsPalette::ScriptsPalette ()
    : DG::Palette (ACAPI_GetOwnResModule (), ID_SCRIPTS_PALETTE, ACAPI_GetOwnResModule (), paletteGuid)
    , panel (*this, TapirPalette::Instance ().GetPanelActions ())
{
    Attach (*this);
    AttachToAllItems (*this);

    panel.RestoreState ();
    panel.ScriptListChanged ();

    BeginEventProcessing ();
}

ScriptsPalette::~ScriptsPalette ()
{
    DetachFromAllItems (*this);
    EndEventProcessing ();
}

bool ScriptsPalette::HasInstance ()
{
    return instance != nullptr;
}

ScriptsPalette& ScriptsPalette::Instance ()
{
    if (!HasInstance ()) {
        instance = new ScriptsPalette ();
    }
    return *instance;
}

void ScriptsPalette::Release ()
{
    instance = nullptr;
}

ScriptsPanel& ScriptsPalette::GetPanel ()
{
    return panel;
}

void ScriptsPalette::Show ()
{
    hiddenByArchicad = false;
    if (IsVisible ()) {
        BringToFront ();
        return;
    }
    DG::Palette::Show ();
    // The controls were placed while the palette was hidden; place and repaint them now that it shows.
    panel.Layout ();
}

void ScriptsPalette::Hide ()
{
    hiddenByArchicad = false;
    DG::Palette::Hide ();
}

void ScriptsPalette::Toggle ()
{
    if (IsVisible ()) {
        Hide ();
    } else {
        Show ();
    }
}

void ScriptsPalette::PanelCloseRequested (const DG::PanelCloseRequestEvent&, bool* accepted)
{
    Hide ();
    *accepted = true;
}

void ScriptsPalette::PanelResized (const DG::PanelResizeEvent&)
{
    panel.Resized ();
}

void ScriptsPalette::ButtonClicked (const DG::ButtonClickEvent& ev)
{
    panel.ButtonClicked (ev);
}

GSErrCode ScriptsPalette::PaletteControlCallBack (Int32, API_PaletteMessageID messageID, GS::IntPtr param)
{
    switch (messageID) {
        case APIPalMsg_OpenPalette:
            Instance ().Show ();
            break;

        case APIPalMsg_ClosePalette:
            if (HasInstance ()) {
                Instance ().Hide ();
            }
            break;

        // Archicad hides the palettes for a while (e.g. on a tab switch); only a palette that was open comes back.
        case APIPalMsg_HidePalette_Begin:
            if (HasInstance () && Instance ().IsVisible ()) {
                Instance ().DG::Palette::Hide ();
                Instance ().hiddenByArchicad = true;
            }
            break;

        case APIPalMsg_HidePalette_End:
            if (HasInstance () && Instance ().hiddenByArchicad) {
                Instance ().hiddenByArchicad = false;
                if (!Instance ().IsVisible ()) {
                    Instance ().DG::Palette::Show ();
                }
            }
            break;

        case APIPalMsg_IsPaletteVisible:
            *(reinterpret_cast<bool*> (param)) = HasInstance () && Instance ().IsVisible ();
            break;

        default:
            break;
    }

    return NoError;
}

GSErrCode ScriptsPalette::RegisterPaletteControlCallBack ()
{
    return ACAPI_RegisterModelessWindow (
                    GS::CalculateHashValue (paletteGuid),
                    PaletteControlCallBack,
                    API_PalEnabled_FloorPlan + API_PalEnabled_Section + API_PalEnabled_Elevation +
                    API_PalEnabled_InteriorElevation + API_PalEnabled_3D + API_PalEnabled_Detail +
                    API_PalEnabled_Worksheet + API_PalEnabled_Layout + API_PalEnabled_DocumentFrom3D,
                    GSGuid2APIGuid (paletteGuid));
}
