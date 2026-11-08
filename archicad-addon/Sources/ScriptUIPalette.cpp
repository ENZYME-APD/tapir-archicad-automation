#include "ScriptUIPalette.hpp"
#include "ResourceIds.hpp"
#include "MigrationHelper.hpp"

// The first GUID is the one of the single Script UI palette before #1819, so Archicad keeps its
// saved position for the first window. Never change or reorder them.
const GS::Guid                  ScriptUIPalette::paletteGuids[ScriptUIPalette::SlotCount] = {
    GS::Guid ("{6E3B7F0A-6C7A-4B0E-9A0D-5B6E3F2C9A11}"),
    GS::Guid ("{3A9C51D2-8E47-4F06-B1D3-7C2E94A0F512}"),
    GS::Guid ("{B47E2F90-15C6-4D8A-9E3B-60F1A7C2D813}"),
    GS::Guid ("{0D6F83A4-C2B9-4E71-8A05-D93E1B4F7C14}"),
    GS::Guid ("{E2581C7B-4A3D-4B9F-A6E0-1F7D58C9B315}"),
    GS::Guid ("{7F14B3E6-9D02-4C85-B7A1-E4C603D2A916}"),
    GS::Guid ("{51C9E0A8-6B7F-4A23-8D94-2B0E7F1C6A17}"),
    GS::Guid ("{C83D4A15-F0E6-4197-95B2-A7D1E6083F18}")
};
GS::Ref<ScriptUIPalette>        ScriptUIPalette::instances[ScriptUIPalette::SlotCount];
GS::UniString                   ScriptUIPalette::slotOwners[ScriptUIPalette::SlotCount];
GS::HashTable<GS::UniString, GS::UniString> ScriptUIPalette::scriptNames;

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

// Inserts script right before "</body>" (found case-insensitively); falls back to appending at the
// end when absent/malformed.
static GS::UniString InjectScriptBeforeBodyEnd (const GS::UniString& htmlContent, const GS::UniString& script)
{
#ifdef ServerMainVers_3000
    const GS::UniString lowerContent = htmlContent.GetLowerCased ();
#else
    const GS::UniString lowerContent = htmlContent.ToLowerCase ();
#endif
    if (!lowerContent.Contains (GS::UniString ("</body>"))) {
        return htmlContent + script;
    }

    const UIndex insertAt = lowerContent.FindFirst (GS::UniString ("</body>"));
    GS::UniString result = htmlContent;
    result.Insert (insertAt, script);
    return result;
}

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

    return InjectScriptBeforeBodyEnd (htmlContent, observerScript);
}

// DG::Browser::DisableNavigation (true) blocks every navigation, including the LoadHTML that
// follows it: with navigationDisabled the palette showed up blank and the page's scripts never ran
// (issue #768). Rather than depending on a DG::Browser load-finished event, the page is loaded with
// navigation enabled and this injected script reports back through the same JS bridge SubmitResult
// uses once the page has finished loading; the bridge then disables navigation. The
// ACAPI object is not always registered by the time the first scripts run (the autoHeight script
// above checks for it as well), hence the short polling.
static GS::UniString InjectPageLoadedScript (const GS::UniString& htmlContent)
{
    static const GS::UniString pageLoadedScript =
        "<script>(function () {"
        "var attempts = 0;"
        "function notify() {"
        "  if (window.ACAPI && ACAPI.NotifyPageLoaded) { ACAPI.NotifyPageLoaded (); }"
        "  else if (++attempts < 500) { setTimeout (notify, 20); }"
        "}"
        "if (document.readyState === 'complete') { notify (); }"
        "else { window.addEventListener ('load', notify); }"
        "})();</script>";

    return InjectScriptBeforeBodyEnd (htmlContent, pageLoadedScript);
}

ScriptUIPalette::ScriptUIPalette (Int32 _slotIndex)
    : DG::Palette (ACAPI_GetOwnResModule (), ID_SCRIPTUI_PALETTE, ACAPI_GetOwnResModule (), paletteGuids[_slotIndex])
    , slotIndex (_slotIndex)
    , browser (GetReference (), 1)
{
    if (slotIndex > 0) {
        SetTitle (GS::UniString::Printf ("%T %d", GetTitle ().ToPrintf (), (int) (slotIndex + 1)));   // "Tapir Script UI 2"
    }
    defaultTitle = GetTitle ();
    Attach (*this);
    BeginEventProcessing ();
    RegisterACAPIJavaScriptObject ();
}

ScriptUIPalette::~ScriptUIPalette ()
{
    EndEventProcessing ();
}

bool ScriptUIPalette::HasInstance (Int32 slotIndex)
{
    return slotIndex >= 0 && slotIndex < SlotCount && instances[slotIndex] != nullptr;
}

ScriptUIPalette& ScriptUIPalette::Instance (Int32 slotIndex)
{
    if (!HasInstance (slotIndex)) {
        instances[slotIndex] = new ScriptUIPalette (slotIndex);
    }
    return *instances[slotIndex];
}

Int32 ScriptUIPalette::FindSlotOfScript (const GS::UniString& scriptId)
{
    if (scriptId.IsEmpty ()) {
        return -1;
    }
    for (Int32 i = 0; i < SlotCount; ++i) {
        if (slotOwners[i] == scriptId) {
            return i;
        }
    }
    return -1;
}

Int32 ScriptUIPalette::AcquireSlotForScript (const GS::UniString& scriptId)
{
    const Int32 ownSlot = FindSlotOfScript (scriptId);
    if (ownSlot >= 0 || scriptId.IsEmpty ()) {
        return ownSlot;
    }

    // A closed window is free whoever owned it: its script already got isOpen false for it, and a
    // later ShowScriptUI of that script gets a window again. The first window comes last in each
    // round, as the callers that cannot be identified use it.
    for (Int32 round = 0; round < 2; ++round) {
        for (Int32 n = 1; n <= SlotCount; ++n) {
            const Int32 i = n % SlotCount;
            const bool isFree = (round == 0)
                ? (!HasInstance (i) || !Instance (i).IsOpen ())
                : slotOwners[i].IsEmpty ();
            if (isFree) {
                slotOwners[i] = scriptId;
                return i;
            }
        }
    }
    return -1;
}

void ScriptUIPalette::ReleaseScript (const GS::UniString& scriptId)
{
    const Int32 slot = FindSlotOfScript (scriptId);
    if (slot >= 0) {
        slotOwners[slot] = GS::EmptyUniString;
    }
    if (scriptNames.ContainsKey (scriptId)) {
        scriptNames.Delete (scriptId);
    }
}

void ScriptUIPalette::SetScriptName (const GS::UniString& scriptId, const GS::UniString& scriptName)
{
    if (!scriptId.IsEmpty ()) {
        scriptNames.Put (scriptId, scriptName);
    }
}

void ScriptUIPalette::Release ()
{
    for (Int32 i = 0; i < SlotCount; ++i) {
        instances[i] = nullptr;
        slotOwners[i] = GS::EmptyUniString;
    }
    scriptNames.Clear ();
}

void ScriptUIPalette::UpdateTitle (const GS::UniString& newPageTitle)
{
    const GS::UniString& owner = slotOwners[slotIndex];
    if (owner != titleOwner) {
        pageTitle = GS::EmptyUniString;   // the title another script gave
        titleOwner = owner;
    }
    if (!newPageTitle.IsEmpty ()) {
        pageTitle = newPageTitle;
    }

    const GS::UniString* scriptName = owner.IsEmpty () ? nullptr : scriptNames.GetPtr (owner);
    if (scriptName == nullptr || scriptName->IsEmpty ()) {
        SetTitle (pageTitle.IsEmpty () ? defaultTitle : pageTitle);
    } else if (pageTitle.IsEmpty () || pageTitle == *scriptName) {
        SetTitle (*scriptName);
    } else {
        SetTitle (pageTitle + " - " + *scriptName);   // "Choose a layer - LayerTool.py"
    }
}

Int32 ScriptUIPalette::GetSlotOfPaletteId (Int32 paletteId)
{
    for (Int32 i = 0; i < SlotCount; ++i) {
        if (paletteId == (Int32) GS::CalculateHashValue (paletteGuids[i])) {
            return i;
        }
    }
    return -1;
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
    SetGrowType (options.resizable ? DG::Dialog::HVGrow : DG::Dialog::NoGrow);
    // The GDLG size (600x450) is only meant as the starting size, but DG also keeps it as the
    // minimum client size: without this, a smaller width/height and autoHeight on a short page are
    // silently clamped to 600x450. Setting the minimum in the constructor (before
    // BeginEventProcessing) had no effect, so it is set here on every show.
    SetMinClientSize (160, 60);
    if (options.width > 0 && options.height > 0) {
        SetClientSize (options.width, options.height);
    }
    UpdateTitle (options.title);

    TAPIR_Browser_SetZoomEnabled (browser, options.zoomEnabled);
    if (options.hasZoomLevel) {
        TAPIR_Browser_SetZoomLevel (browser, options.zoomLevel);
    }
    browser.SetScrollBarVisibility (options.scrollBarsVisible);
    TAPIR_Browser_SetContextMenuEnabled (browser, options.contextMenuEnabled);
    // Navigation must be enabled while the page loads, otherwise LoadHTML itself is blocked (see
    // InjectPageLoadedScript); a previous navigationDisabled page may have left it disabled.
    browser.DisableNavigation (false);
    TAPIR_Browser_SetAllowSelfSignedCertificates (browser, options.allowSelfSignedCertificates);
    if (options.clearCookies) {
        browser.DeleteAllCookies ();
    }

    autoHeightEnabled = options.autoHeight;
    navigationDisableRequested = options.navigationDisabled;

    hasPendingResult = false;
    pendingResult = GS::EmptyUniString;

    GS::UniString content = htmlContent;
    if (options.autoHeight) {
        content = InjectAutoHeightScript (content);
    }
    if (options.navigationDisabled) {
        content = InjectPageLoadedScript (content);
    }
    browser.LoadHTML (content);
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

bool ScriptUIPalette::IsOpen ()
{
    return IsVisible () || hiddenByForcedHideBegin;
}

void ScriptUIPalette::PanelCloseRequested (const DG::PanelCloseRequestEvent&, bool* accepted)
{
    hiddenByForcedHideBegin = false;
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
    const Int32 slot = slotIndex;   // the window the page runs in

    jsACAPI->AddItem (new JS::Function ("SubmitResult", [slot] (GS::Ref<JS::Base> param) {
        GS::Ref<JS::Value> jsValue = GS::DynamicCast<JS::Value> (param);
        GS::UniString content = (jsValue != nullptr && jsValue->GetType () == JS::Value::STRING) ? jsValue->GetString () : GS::EmptyUniString;
        if (ScriptUIPalette::HasInstance (slot)) {
            ScriptUIPalette& palette = ScriptUIPalette::Instance (slot);
            palette.pendingResult = content;
            palette.hasPendingResult = true;
        }
        return GS::Ref<JS::Base> (new JS::Value (true));
    }));

    jsACAPI->AddItem (new JS::Function ("ClosePalette", [slot] (GS::Ref<JS::Base>) {
        if (ScriptUIPalette::HasInstance (slot)) {
            ScriptUIPalette& palette = ScriptUIPalette::Instance (slot);
            palette.hiddenByForcedHideBegin = false;
            palette.Hide ();
        }
        return GS::Ref<JS::Base> (new JS::Value (true));
    }));

    jsACAPI->AddItem (new JS::Function ("ReportContentHeight", [slot] (GS::Ref<JS::Base> param) {
        GS::Ref<JS::Value> jsValue = GS::DynamicCast<JS::Value> (param);
        if (ScriptUIPalette::HasInstance (slot) && jsValue != nullptr) {
            ScriptUIPalette& palette = ScriptUIPalette::Instance (slot);
            if (palette.autoHeightEnabled) {
                const Int32 pixelHeight = (jsValue->GetType () == JS::Value::DOUBLE) ? (Int32) jsValue->GetDouble () : jsValue->GetInteger ();
                palette.SetClientHeight ((short) pixelHeight);
            }
        }
        return GS::Ref<JS::Base> (new JS::Value (true));
    }));

    jsACAPI->AddItem (new JS::Function ("NotifyPageLoaded", [slot] (GS::Ref<JS::Base>) {
        if (ScriptUIPalette::HasInstance (slot)) {
            ScriptUIPalette& palette = ScriptUIPalette::Instance (slot);
            if (palette.navigationDisableRequested) {
                palette.browser.DisableNavigation (true);
            }
        }
        return GS::Ref<JS::Base> (new JS::Value (true));
    }));

    browser.RegisterAsynchJSObject (jsACAPI);
}

GSErrCode ScriptUIPalette::PaletteControlCallBack (Int32 paletteId, API_PaletteMessageID messageID, GS::IntPtr param)
{
    const Int32 slot = GetSlotOfPaletteId (paletteId);

    switch (messageID) {
        case APIPalMsg_OpenPalette:
            // Archicad restores the palette when it was open at the last quit, but the page and the
            // script that showed it are gone by then: only reopen a page shown in this session, so
            // Archicad does not start with an empty palette. The instance only exists once
            // ShowWithHTML ran.
            if (!HasInstance (slot))
                break;
            Instance (slot).Show ();
            break;

        case APIPalMsg_ClosePalette:
            if (!HasInstance (slot))
                break;
            Instance (slot).Hide ();
            break;

        case APIPalMsg_HidePalette_Begin:
            if (HasInstance (slot)) {
                ScriptUIPalette& palette = Instance (slot);
                palette.hiddenByForcedHideBegin = palette.IsVisible ();
                if (palette.hiddenByForcedHideBegin)
                    palette.Hide ();
            }
            break;

        case APIPalMsg_HidePalette_End:
            if (HasInstance (slot)) {
                ScriptUIPalette& palette = Instance (slot);
                if (palette.hiddenByForcedHideBegin)
                    palette.Show ();
                palette.hiddenByForcedHideBegin = false;
            }
            break;

        case APIPalMsg_IsPaletteVisible:
            *(reinterpret_cast<bool*> (param)) = HasInstance (slot) && Instance (slot).IsVisible ();
            break;

        default:
            break;
    }

    return NoError;
}

GSErrCode ScriptUIPalette::RegisterPaletteControlCallBack ()
{
    GSErrCode err = NoError;
    for (Int32 i = 0; i < SlotCount; ++i) {
        err |= ACAPI_RegisterModelessWindow (
                    GS::CalculateHashValue (paletteGuids[i]),
                    PaletteControlCallBack,
                    API_PalEnabled_FloorPlan + API_PalEnabled_Section + API_PalEnabled_Elevation +
                    API_PalEnabled_InteriorElevation + API_PalEnabled_3D + API_PalEnabled_Detail +
                    API_PalEnabled_Worksheet + API_PalEnabled_Layout + API_PalEnabled_DocumentFrom3D,
                    GSGuid2APIGuid (paletteGuids[i]));
    }
    return err;
}
