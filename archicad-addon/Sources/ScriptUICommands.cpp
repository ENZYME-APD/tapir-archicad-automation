#include "ScriptUICommands.hpp"
#include "ScriptUIPalette.hpp"

// The input keys that pick the Script UI window, shared by the Script UI commands (issue #1819).
static const char* ScriptUIWindowInputProperties = R"(
            "uiId": {
                "type": "integer",
                "description": "The window to use, as returned by ShowScriptUI (1 to 8). Takes precedence over scriptId.",
                "minimum": 1,
                "maximum": 8
            },
            "scriptId": {
                "type": "string",
                "description": "Any text that identifies the calling script, so it uses its own window: ShowScriptUI gives a script without a window a free one, preferring the closed windows. The Tapir palette gives each script it starts its own id in the TAPIR_SCRIPT_ID environment variable, which aclib passes here automatically; that script's window is titled with its name and is freed when it ends. Without uiId and scriptId the first window is used.",
                "minLength": 1
            })";

// The 0-based window of the call: uiId, else the window of scriptId, else the first window. With
// acquire, a scriptId that has no window yet gets a free one. -1: scriptId has no window (and, with
// acquire, every window is open for another script).
static Int32 GetScriptUIWindow (const GS::ObjectState& parameters, bool acquire)
{
    Int32 uiId = 0;
    if (parameters.Get ("uiId", uiId) && uiId >= 1 && uiId <= ScriptUIPalette::SlotCount) {
        return uiId - 1;
    }
    GS::UniString scriptId;
    if (parameters.Get ("scriptId", scriptId) && !scriptId.IsEmpty ()) {
        return acquire ? ScriptUIPalette::AcquireSlotForScript (scriptId) : ScriptUIPalette::FindSlotOfScript (scriptId);
    }
    return 0;
}

ShowScriptUICommand::ShowScriptUICommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ShowScriptUICommand::GetName () const
{
    return "ShowScriptUI";
}

GS::Optional<GS::UniString> ShowScriptUICommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
        "type": "object",
        "properties": {
            "htmlContent": {
                "type": "string",
                "description": "The full HTML document to display in the Script UI palette.",
                "minLength": 1
            },
            "width": {
                "type": "integer",
                "description": "Optional palette client width in pixels. Leave unset to keep the current size.",
                "minimum": 1
            },
            "height": {
                "type": "integer",
                "description": "Optional palette client height in pixels. Leave unset to keep the current size.",
                "minimum": 1
            },
            "title": {
                "type": "string",
                "description": "Optional palette window title. Leave unset to keep the current title.",
                "minLength": 1
            },
            "resizable": {
                "type": "boolean",
                "description": "Whether the user can drag-resize the palette. Defaults to true."
            },
            "zoomEnabled": {
                "type": "boolean",
                "description": "Whether Ctrl+scroll/pinch zooming is allowed in the page. Defaults to true."
            },
            "zoomLevel": {
                "type": "number",
                "description": "Optional initial zoom level (1.0 = 100%). Leave unset to keep the current zoom."
            },
            "scrollBarsVisible": {
                "type": "boolean",
                "description": "Whether the browser's own scrollbars are shown. Defaults to true."
            },
            "contextMenuEnabled": {
                "type": "boolean",
                "description": "Whether right-click shows the browser's context menu (e.g. Inspect Element). Defaults to true."
            },
            "navigationDisabled": {
                "type": "boolean",
                "description": "Prevents the page from navigating away (e.g. clicking a link to another site) once it has loaded. Defaults to false."
            },
            "allowSelfSignedCertificates": {
                "type": "boolean",
                "description": "Only relevant if htmlContent links to a remote https:// resource with a self-signed certificate. Defaults to false."
            },
            "clearCookies": {
                "type": "boolean",
                "description": "Deletes all browser cookies before loading htmlContent. Defaults to false."
            },
            "autoHeight": {
                "type": "boolean",
                "description": "Automatically resizes the palette's height to fit the page content as it changes. Defaults to false."
            },)") + ScriptUIWindowInputProperties + R"(
        },
        "additionalProperties": false,
        "required": [
            "htmlContent"
        ]
    })";
}

GS::Optional<GS::UniString> ShowScriptUICommand::GetRawResponseSchema () const
{
    return R"({
        "oneOf": [
            {
                "type": "object",
                "properties": {
                    "success": {
                        "type": "boolean",
                        "enum": [ true ]
                    },
                    "uiId": {
                        "type": "integer",
                        "description": "The window the page is shown in (1 to 8). Pass it to GetScriptUIResult, or to a later ShowScriptUI to replace the page."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "success",
                    "uiId"
                ]
            },
            {
                "$ref": "#/FailedExecutionResult"
            }
        ]
    })";
}

GS::ObjectState ShowScriptUICommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString htmlContent;
    if (!parameters.Get ("htmlContent", htmlContent)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Missing parameter: htmlContent.");
    }

    ScriptUIOptions options;

    Int32 width = 0;
    Int32 height = 0;
    parameters.Get ("width", width);
    parameters.Get ("height", height);
    options.width = (short) width;
    options.height = (short) height;

    parameters.Get ("title", options.title);
    parameters.Get ("resizable", options.resizable);
    parameters.Get ("zoomEnabled", options.zoomEnabled);
    options.hasZoomLevel = parameters.Get ("zoomLevel", options.zoomLevel);
    parameters.Get ("scrollBarsVisible", options.scrollBarsVisible);
    parameters.Get ("contextMenuEnabled", options.contextMenuEnabled);
    parameters.Get ("navigationDisabled", options.navigationDisabled);
    parameters.Get ("allowSelfSignedCertificates", options.allowSelfSignedCertificates);
    parameters.Get ("clearCookies", options.clearCookies);
    parameters.Get ("autoHeight", options.autoHeight);

    const Int32 window = GetScriptUIWindow (parameters, true);
    if (window < 0) {
        return CreateFailedExecutionResult (APIERR_GENERAL, GS::UniString::Printf (
            "Every Script UI window (%d) is open for another script. Close one of them, or pass the uiId of a window to replace its page.",
            (int) ScriptUIPalette::SlotCount));
    }

    ScriptUIPalette::Instance (window).ShowWithHTML (htmlContent, options);

    GS::ObjectState response = CreateSuccessfulExecutionResult ();
    response.Add ("uiId", window + 1);
    return response;
}

GetScriptUIResultCommand::GetScriptUIResultCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetScriptUIResultCommand::GetName () const
{
    return "GetScriptUIResult";
}

GS::Optional<GS::UniString> GetScriptUIResultCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
        "type": "object",
        "properties": {)") + ScriptUIWindowInputProperties + R"(
        },
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> GetScriptUIResultCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "uiId": {
                "type": "integer",
                "description": "The window the result was read from (1 to 8). Missing when scriptId was given and that script has no window."
            },
            "hasResult": {
                "type": "boolean",
                "description": "True if the Script UI page has submitted a result since the last call."
            },
            "result": {
                "type": "string",
                "description": "The submitted content (as passed to window.ACAPI.SubmitResult). Only present when hasResult is true."
            },
            "isOpen": {
                "type": "boolean",
                "description": "True while the Script UI palette is open. False before the first ShowScriptUI call and once the user closed the palette with its close box, the page called window.ACAPI.ClosePalette or Archicad closed it; stays true while Archicad only temporarily hides all palettes (e.g. during a modal dialog). Lets a waiting script stop polling when the user closes the palette."
            }
        },
        "additionalProperties": false,
        "required": [
            "hasResult",
            "isOpen"
        ]
    })";
}

GS::ObjectState GetScriptUIResultCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    const Int32 window = GetScriptUIWindow (parameters, false);

    GS::UniString result;
    const bool hasResult = ScriptUIPalette::HasInstance (window) && ScriptUIPalette::Instance (window).ConsumeResult (&result);
    const bool isOpen = ScriptUIPalette::HasInstance (window) && ScriptUIPalette::Instance (window).IsOpen ();

    GS::ObjectState response;
    if (window >= 0) {
        response.Add ("uiId", window + 1);
    }
    response.Add ("hasResult", hasResult);
    if (hasResult) {
        response.Add ("result", result);
    }
    response.Add ("isOpen", isOpen);
    return response;
}
