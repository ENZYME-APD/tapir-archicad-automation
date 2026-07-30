#pragma once

#include "APIEnvir.h"
#include "ACAPinc.h"
#include "DGModule.hpp"
#include "DGBrowser.hpp"

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
// "GetScriptUIResult" Tapir command.
class ScriptUIPalette final : public DG::Palette,
    public DG::PanelObserver
{
public:
    virtual ~ScriptUIPalette ();

    static bool            HasInstance ();
    static ScriptUIPalette& Instance ();

    void ShowWithHTML (const GS::UniString& htmlContent, const ScriptUIOptions& options = ScriptUIOptions ());
    void Hide ();

    // Returns true and fills outResult if the page has called window.ACAPI.SubmitResult(...)
    // since the last call - one-shot: the pending result is cleared once read.
    bool ConsumeResult (GS::UniString* outResult);

    static GSErrCode RegisterPaletteControlCallBack ();

private:
    DG::Browser browser;

    GS::UniString pendingResult;
    bool          hasPendingResult = false;

    // Guards the ReportContentHeight JS bridge function: only resizes when the currently shown
    // page was opened with options.autoHeight = true (and therefore has the auto-height observer
    // script injected - see InjectAutoHeightScript in ScriptUIPalette.cpp).
    bool autoHeightEnabled = false;

    // Used only by PaletteControlCallBack, e.g. when Archicad restores a saved window layout that
    // had this palette open - just redisplays whatever HTML was last loaded (or a blank palette).
    void Show ();

    void RegisterACAPIJavaScriptObject ();

    virtual void PanelCloseRequested (const DG::PanelCloseRequestEvent& ev, bool* accepted) override;
    virtual void PanelResized (const DG::PanelResizeEvent& ev) override;

    static GSErrCode PaletteControlCallBack (Int32 paletteId, API_PaletteMessageID messageID, GS::IntPtr param);

    static const GS::Guid             paletteGuid;
    static GS::Ref<ScriptUIPalette>   instance;

    ScriptUIPalette ();
};
