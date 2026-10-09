#include "ScriptUICommands.hpp"
#include "ScriptUIPalette.hpp"

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
    return R"({
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
                "description": "Prevents the page from navigating away (e.g. clicking a link to another site). Defaults to false."
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
            }
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
        "$ref": "#/ExecutionResult"
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

    ScriptUIPalette::Instance ().ShowWithHTML (htmlContent, options);

    return CreateSuccessfulExecutionResult ();
}

GetScriptUIResultCommand::GetScriptUIResultCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetScriptUIResultCommand::GetName () const
{
    return "GetScriptUIResult";
}

GS::Optional<GS::UniString> GetScriptUIResultCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
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

GS::ObjectState GetScriptUIResultCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString result;
    const bool hasResult = ScriptUIPalette::HasInstance () && ScriptUIPalette::Instance ().ConsumeResult (&result);
    const bool isOpen = ScriptUIPalette::HasInstance () && ScriptUIPalette::Instance ().IsOpen ();

    GS::ObjectState response;
    response.Add ("hasResult", hasResult);
    if (hasResult) {
        response.Add ("result", result);
    }
    response.Add ("isOpen", isOpen);
    return response;
}
