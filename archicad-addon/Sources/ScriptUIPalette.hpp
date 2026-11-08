#pragma once

#include "APIEnvir.h"
#include "ACAPinc.h"
#include "DGModule.hpp"
#include "DGBrowser.hpp"
#include "HashTable.hpp"

// Archicad < 27's browser JS bridge used DG::JSObject/JSFunction/JSValue/JSBase/JSArray (same
// shape as JS::Object/Function/Value/Base/Array from the JavascriptEngine module introduced in
// 27, just a different namespace/names, declared in a header that 27+ dropped entirely). Aliasing
// them lets ScriptUIPalette.cpp use JS::* uniformly across all supported Archicad versions.
#ifndef ServerMainVers_2700
#include "DGBrowserJSHandler.hpp"
namespace JS {
    using Base = DG::JSBase;
    using Function = DG::JSFunction;
    using Object = DG::JSObject;
    using Array = DG::JSArray;
    using Value = DG::JSValue;
}
#endif

// Every DG::Browser setting exposed to scripts through ShowScriptUI - see Sources/ScriptUICommands.cpp
// for the JSON schema/defaults. width/height/title <= 0 / empty mean "leave unchanged".
struct ScriptUIOptions
{
    short         width = 0;
    short         height = 0;
    GS::UniString title;
    bool          resizable = true;
    bool          zoomEnabled = true;
    bool          hasZoomLevel = false;
    double        zoomLevel = 1.0;
    bool          scrollBarsVisible = true;
    bool          contextMenuEnabled = true;
    bool          navigationDisabled = false;
    bool          allowSelfSignedCertificates = false;
    bool          clearCookies = false;
    bool          autoHeight = false;
};

// Hosts a native Chromium-based DG::Browser control that Python scripts can use as a tkinter
// replacement: a script calls the "ShowScriptUI" Tapir command with an HTML string, the user
// interacts with it, and the page's JS hands data back via window.ACAPI.SubmitResult(...) (a
// RegisterAsynchJSObject bridge, not HTTP/fetch - proven reliable in the original proof-of-concept
// on branch experiment/native-browser-poc). The script itself retrieves that data by polling the
// "GetScriptUIResult" Tapir command, which also tells it whether the palette is still open (see
// IsOpen), so it can stop waiting when the user closes the palette instead.
//
// There is one window per script (issue #1819): SlotCount palettes, each registered with its own
// GUID, so Archicad remembers the position of each. A window is addressed by its uiId (1-based
// slot index, returned by ShowScriptUI) or by the scriptId of the script that owns it; a call with
// neither uses the first window, as before there were several.
class ScriptUIPalette final : public DG::Palette,
    public DG::PanelObserver
{
public:
    static constexpr Int32 SlotCount = 8;

    virtual ~ScriptUIPalette ();

    static bool             HasInstance (Int32 slotIndex);   // 0-based, 0..SlotCount-1
    static ScriptUIPalette& Instance (Int32 slotIndex);

    // The window of the script: the slot it owns, -1 when it owns none.
    static Int32 FindSlotOfScript (const GS::UniString& scriptId);
    // The window of the script, given to it when it owns none yet: first a window that is closed,
    // then one whose script has ended (no owner), the first window last, as it is also the window
    // of the callers that cannot be identified. -1 when every window is open for another script.
    static Int32 AcquireSlotForScript (const GS::UniString& scriptId);
    // Frees the window of the script once it ended or was stopped, and forgets its name. Its page
    // stays shown until another script takes the window.
    static void  ReleaseScript (const GS::UniString& scriptId);
    // The name shown in the title of the script's window, given by the Tapir palette for the
    // scripts it starts (their scriptId is the TAPIR_SCRIPT_ID environment variable it sets).
    static void  SetScriptName (const GS::UniString& scriptId, const GS::UniString& scriptName);

    // Explicitly tears down the browser controls while Archicad's own environment (including its
    // CEF/browser subsystem) is still fully alive - must be called from FreeData, not left to run
    // via the static GS::Ref's destructor at DLL/dylib unload time (confirmed via a real macOS
    // crash report: EXC_BAD_ACCESS in the DG::Browser destructor when it ran during add-on
    // finalization, after Archicad had already begun tearing down its own browser subsystem).
    static void             Release ();

    void ShowWithHTML (const GS::UniString& htmlContent, const ScriptUIOptions& options = ScriptUIOptions ());
    void Hide ();

    // Returns true and fills outResult if the page has called window.ACAPI.SubmitResult(...)
    // since the last call - one-shot: the pending result is cleared once read.
    bool ConsumeResult (GS::UniString* outResult);

    // Whether the palette is open from the script's point of view: false once the user closed it
    // with its close box, the page called window.ACAPI.ClosePalette or Archicad closed it, but
    // still true while Archicad only temporarily hides every palette (APIPalMsg_HidePalette_Begin,
    // e.g. during a modal dialog), as that hide is undone by APIPalMsg_HidePalette_End. The page
    // itself gets no event when the palette is hidden (no visibilitychange/pagehide/blur, and
    // requestAnimationFrame keeps firing), so this is the only way a polling script can notice
    // that the user closed the palette.
    bool IsOpen ();

    static GSErrCode RegisterPaletteControlCallBack ();

private:
    const Int32 slotIndex;
    DG::Browser browser;

    GS::UniString pendingResult;
    bool          hasPendingResult = false;

    // The title: defaultTitle ("Tapir Script UI 2") until a script gives one (pageTitle), followed
    // by the name of the script that owns the window, when the Tapir palette started it. pageTitle
    // belongs to titleOwner, the owner of the window when it was given, and is dropped when
    // another script takes the window.
    GS::UniString defaultTitle;
    GS::UniString pageTitle;
    GS::UniString titleOwner;
    void UpdateTitle (const GS::UniString& newPageTitle);

    // Guards the ReportContentHeight JS bridge function: only resizes when the currently shown
    // page was opened with options.autoHeight = true (and therefore has the auto-height observer
    // script injected - see InjectAutoHeightScript in ScriptUIPalette.cpp).
    bool autoHeightEnabled = false;

    // Guards the NotifyPageLoaded JS bridge function: DG::Browser::DisableNavigation (true) blocks
    // LoadHTML itself when it is already set at load time (the palette stays blank, see issue
    // #768), so ShowWithHTML loads the page with navigation enabled and only disables it from the
    // injected page-loaded script when the currently shown page asked for options.navigationDisabled.
    bool navigationDisableRequested = false;

    // Set only when APIPalMsg_HidePalette_Begin itself hid a visible palette. Guards
    // APIPalMsg_HidePalette_End so it only restores visibility it took away - not a palette the
    // user (or the script, via ClosePalette) had already explicitly closed beforehand.
    bool hiddenByForcedHideBegin = false;

    // Used only by PaletteControlCallBack, e.g. when Archicad restores a saved window layout that
    // had this palette open - just redisplays whatever HTML was last loaded in this session (a
    // restore before any ShowWithHTML call is ignored, so the palette never opens blank).
    void Show ();

    void RegisterACAPIJavaScriptObject ();

    virtual void PanelCloseRequested (const DG::PanelCloseRequestEvent& ev, bool* accepted) override;
    virtual void PanelResized (const DG::PanelResizeEvent& ev) override;

    static GSErrCode PaletteControlCallBack (Int32 paletteId, API_PaletteMessageID messageID, GS::IntPtr param);

    static Int32 GetSlotOfPaletteId (Int32 paletteId);   // -1 for an unknown id

    static const GS::Guid             paletteGuids[SlotCount];
    static GS::Ref<ScriptUIPalette>   instances[SlotCount];
    static GS::UniString              slotOwners[SlotCount];   // the scriptId owning each window; empty: none
    static GS::HashTable<GS::UniString, GS::UniString> scriptNames;   // scriptId -> name, see SetScriptName

    explicit ScriptUIPalette (Int32 slotIndex);
};
