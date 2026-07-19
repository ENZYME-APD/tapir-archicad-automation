#include "ScriptUIPalette.hpp"
#include "ResourceIds.hpp"
#include "MigrationHelper.hpp"

const GS::Guid                  ScriptUIPalette::paletteGuid ("{6E3B7F0A-6C7A-4B0E-9A0D-5B6E3F2C9A11}");
GS::Ref<ScriptUIPalette>        ScriptUIPalette::instance;

// Zoom control, SetAllowSelfSignedCertificates, and the context-menu mode enum do not exist at
// all on DG::Browser before Archicad 27 (context menu can only be toggled on/off pre-27, no
// Custom mode). Archicad 27 itself is a further split: SetZoomLevel/GetZoomLevel exist there, but
// EnableZooming/DisableZooming were only added in 28 (confirmed via a real 27 build failure -
// "'EnableZooming': is not a member of 'DG::Browser'" - matching the existing precedent in
// MigrationHelper.hpp where AC27 is a known odd-one-out for other APIs too, e.g.
// ACAPI_Element_SolidLink_GetFlags). These wrappers keep ShowWithHTML version-agnostic.
#ifndef ServerMainVers_2700
static void TAPIR_Browser_SetZoomEnabled (DG::Browser&, bool) {}
static void TAPIR_Browser_SetZoomLevel (DG::Browser&, double) {}
static void TAPIR_Browser_SetAllowSelfSignedCertificates (DG::Browser&, bool) {}
static void TAPIR_Browser_SetContextMenuEnabled (DG::Browser& browser, bool enabled)
{
    if (enabled) {
        browser.EnableContextMenu ();
    } else {
        browser.DisableContextMenu ();
    }
}
#else
static void TAPIR_Browser_SetZoomLevel (DG::Browser& browser, double zoomLevel)
{
    browser.SetZoomLevel (zoomLevel);
}
static void TAPIR_Browser_SetAllowSelfSignedCertificates (DG::Browser& browser, bool allow)
{
    browser.SetAllowSelfSignedCertificates (allow);
}
static void TAPIR_Browser_SetContextMenuEnabled (DG::Browser& browser, bool enabled)
{
    browser.SetContextMenuMode (enabled ? DG::BrowserBase::ContextMenuMode::Default : DG::BrowserBase::ContextMenuMode::Disabled);
}
#if defined (ServerMainVers_2700) && !defined (ServerMainVers_2800)
static void TAPIR_Browser_SetZoomEnabled (DG::Browser&, bool) {}
#else
static void TAPIR_Browser_SetZoomEnabled (DG::Browser& browser, bool enabled)
{
    if (enabled) {
        browser.EnableZooming ();
    } else {
        browser.DisableZooming ();
    }
}
#endif
#endif

// Archicad's own browser.onContentHeightChanged event does not fire on dynamic DOM changes (only
// ever observed at initial page load, undocumented) so autoHeight instead injects a small
// ResizeObserver that reports content size changes through the same JS bridge scripts already use
// for SubmitResult, no JS authoring required from the calling script. Naively resizing on every
// observed change creates a feedback loop (our own SetClientHeight changes the browser's viewport,
// which changes layout, which re-fires the observer): a "pending" guard skips re-entrant reports
// while one is still in flight, and a last-height check skips reporting an unchanged size. That's
// enough to stop runaway growth (Archicad 25/26's older embedded browser has no loop breaker of its
// own, unlike modern Chromium), but during an active manual resize drag the observer still fires
// once per frame, each triggering a resize-and-settle round trip - visibly janky. A trailing
// debounce (only measure 150ms after the last observed change) collapses a whole flurry of drag
// frames into a single report once the user stops moving, eliminating the stutter.
static GS::UniString InjectAutoHeightScript (const GS::UniString& htmlContent)
{
    static const GS::UniString observerScript =
        "<script>(function () {"
        "var lastHeight = -1;"
        "var pending = false;"
        "var debounceTimer = null;"
        "function doReport() {"
        "  if (pending || !window.ACAPI || !ACAPI.ReportContentHeight) { return; }"
        "  var h = document.body.scrollHeight;"
        "  if (h === lastHeight) { return; }"
        "  lastHeight = h;"
        "  pending = true;"
        "  ACAPI.ReportContentHeight (h).then (function () { pending = false; });"
        "}"
        "function scheduleReport() {"
        "  if (debounceTimer) { clearTimeout (debounceTimer); }"
        "  debounceTimer = setTimeout (doReport, 150);"
        "}"
        "if (window.ResizeObserver) { new ResizeObserver (scheduleReport).observe (document.body); }"
        "window.addEventListener ('load', doReport);"
        "setTimeout (doReport, 100);"
        "})();</script>";

    // Find "</body>" case-insensitively; fall back to appending at the end when absent/malformed.
    const GS::UniString lowerContent = htmlContent.ToLowerCase ();
    if (!lowerContent.Contains (GS::UniString ("</body>"))) {
        return htmlContent + observerScript;
    }

    const UIndex insertAt = lowerContent.FindFirst (GS::UniString ("</body>"));
    GS::UniString result = htmlContent;
    result.Insert (insertAt, observerScript);
    return result;
}

ScriptUIPalette::ScriptUIPalette ()
    : DG::Palette (ACAPI_GetOwnResModule (), ID_SCRIPTUI_PALETTE, ACAPI_GetOwnResModule (), paletteGuid)
    , browser (GetReference (), 1)
{
    Attach (*this);
    BeginEventProcessing ();
    RegisterACAPIJavaScriptObject ();
}

ScriptUIPalette::~ScriptUIPalette ()
{
    EndEventProcessing ();
}

bool ScriptUIPalette::HasInstance ()
{
    return instance != nullptr;
}

ScriptUIPalette& ScriptUIPalette::Instance ()
{
    if (!HasInstance ()) {
        instance = new ScriptUIPalette ();
    }
    return *instance;
}

void ScriptUIPalette::Show ()
{
    DG::Palette::Show ();
}

void ScriptUIPalette::Hide ()
{
    DG::Palette::Hide ();
}

void ScriptUIPalette::ShowWithHTML (const GS::UniString& htmlContent, const ScriptUIOptions& options)
{
    if (options.width > 0 && options.height > 0) {
        SetClientSize (options.width, options.height);
    }
    if (!options.title.IsEmpty ()) {
        SetTitle (options.title);
    }
    SetGrowType (options.resizable ? DG::Dialog::HVGrow : DG::Dialog::NoGrow);

    TAPIR_Browser_SetZoomEnabled (browser, options.zoomEnabled);
    if (options.hasZoomLevel) {
        TAPIR_Browser_SetZoomLevel (browser, options.zoomLevel);
    }
    browser.SetScrollBarVisibility (options.scrollBarsVisible);
    TAPIR_Browser_SetContextMenuEnabled (browser, options.contextMenuEnabled);
    browser.DisableNavigation (options.navigationDisabled);
    TAPIR_Browser_SetAllowSelfSignedCertificates (browser, options.allowSelfSignedCertificates);
    if (options.clearCookies) {
        browser.DeleteAllCookies ();
    }

    autoHeightEnabled = options.autoHeight;

    hasPendingResult = false;
    pendingResult = GS::EmptyUniString;

    browser.LoadHTML (options.autoHeight ? InjectAutoHeightScript (htmlContent) : htmlContent);
    Show ();
}

bool ScriptUIPalette::ConsumeResult (GS::UniString* outResult)
{
    if (!hasPendingResult) {
        return false;
    }
    *outResult = pendingResult;
    hasPendingResult = false;
    pendingResult = GS::EmptyUniString;
    return true;
}

void ScriptUIPalette::PanelCloseRequested (const DG::PanelCloseRequestEvent&, bool* accepted)
{
    Hide ();
    *accepted = true;
}

void ScriptUIPalette::PanelResized (const DG::PanelResizeEvent& ev)
{
    BeginMoveResizeItems ();
    browser.Resize (ev.GetHorizontalChange (), ev.GetVerticalChange ());
    EndMoveResizeItems ();
}

void ScriptUIPalette::RegisterACAPIJavaScriptObject ()
{
    JS::Object* jsACAPI = new JS::Object ("ACAPI");

    jsACAPI->AddItem (new JS::Function ("SubmitResult", [] (GS::Ref<JS::Base> param) {
        GS::Ref<JS::Value> jsValue = GS::DynamicCast<JS::Value> (param);
        GS::UniString content = (jsValue != nullptr && jsValue->GetType () == JS::Value::STRING) ? jsValue->GetString () : GS::EmptyUniString;
        if (ScriptUIPalette::HasInstance ()) {
            ScriptUIPalette& palette = ScriptUIPalette::Instance ();
            palette.pendingResult = content;
            palette.hasPendingResult = true;
        }
        return GS::Ref<JS::Base> (new JS::Value (true));
    }));

    jsACAPI->AddItem (new JS::Function ("ClosePalette", [] (GS::Ref<JS::Base>) {
        if (ScriptUIPalette::HasInstance ()) {
            ScriptUIPalette::Instance ().Hide ();
        }
        return GS::Ref<JS::Base> (new JS::Value (true));
    }));

    jsACAPI->AddItem (new JS::Function ("ReportContentHeight", [] (GS::Ref<JS::Base> param) {
        GS::Ref<JS::Value> jsValue = GS::DynamicCast<JS::Value> (param);
        if (ScriptUIPalette::HasInstance () && jsValue != nullptr) {
            ScriptUIPalette& palette = ScriptUIPalette::Instance ();
            if (palette.autoHeightEnabled) {
                const Int32 pixelHeight = (jsValue->GetType () == JS::Value::DOUBLE) ? (Int32) jsValue->GetDouble () : jsValue->GetInteger ();
                palette.SetClientHeight ((short) pixelHeight);
            }
        }
        return GS::Ref<JS::Base> (new JS::Value (true));
    }));

    browser.RegisterAsynchJSObject (jsACAPI);
}

GSErrCode ScriptUIPalette::PaletteControlCallBack (Int32, API_PaletteMessageID messageID, GS::IntPtr param)
{
    switch (messageID) {
        case APIPalMsg_OpenPalette:
            Instance ().Show ();
            break;

        case APIPalMsg_ClosePalette:
            if (!HasInstance ())
                break;
            Instance ().Hide ();
            break;

        case APIPalMsg_HidePalette_Begin:
            if (HasInstance () && Instance ().IsVisible ())
                Instance ().Hide ();
            break;

        case APIPalMsg_HidePalette_End:
            if (HasInstance () && !Instance ().IsVisible ())
                Instance ().Show ();
            break;

        case APIPalMsg_IsPaletteVisible:
            *(reinterpret_cast<bool*> (param)) = HasInstance () && Instance ().IsVisible ();
            break;

        default:
            break;
    }

    return NoError;
}

GSErrCode ScriptUIPalette::RegisterPaletteControlCallBack ()
{
    return ACAPI_RegisterModelessWindow (
                    GS::CalculateHashValue (paletteGuid),
                    PaletteControlCallBack,
                    API_PalEnabled_FloorPlan + API_PalEnabled_Section + API_PalEnabled_Elevation +
                    API_PalEnabled_InteriorElevation + API_PalEnabled_3D + API_PalEnabled_Detail +
                    API_PalEnabled_Worksheet + API_PalEnabled_Layout + API_PalEnabled_DocumentFrom3D,
                    GSGuid2APIGuid (paletteGuid));
}
