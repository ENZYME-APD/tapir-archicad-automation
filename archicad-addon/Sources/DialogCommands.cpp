#include "DialogCommands.hpp"

namespace {

API_ToolBoxItem CreateToolBoxItem (API_ElemTypeID typeID)
{
    API_ToolBoxItem toolBoxItem = {};
#ifdef ServerMainVers_2600
    toolBoxItem.type = API_ElemType (typeID);
#else
    toolBoxItem.typeID = typeID;
    toolBoxItem.variationID = APIVarId_Generic;
#endif
    return toolBoxItem;
}

GSErrCode OpenToolSettingsDialog (API_ToolBoxItem& toolBoxItem)
{
#ifdef ServerMainVers_2700
    return ACAPI_Dialog_SettingsDialog (&toolBoxItem);
#else
    return ACAPI_Interface (APIIo_SettingsDialogID, &toolBoxItem, nullptr);
#endif
}

#ifndef ServerMainVers_2800
// Before Archicad 28 the settings dialog of the selection cannot be opened, only the one of a tool.
API_ElemTypeID GetToolBoxItemTypeID (const API_ToolBoxItem& toolBoxItem)
{
#ifdef ServerMainVers_2600
    return toolBoxItem.type.typeID;
#else
    return toolBoxItem.typeID;
#endif
}

GSErrCode GetCurrentToolBoxItem (API_ToolBoxItem& toolBoxItem)
{
#ifdef ServerMainVers_2700
    return ACAPI_Toolbox_GetToolBoxMode (&toolBoxItem);
#else
    return ACAPI_Environment (APIEnv_GetToolBoxModeID, &toolBoxItem);
#endif
}
#endif

}

OpenSettingsDialogCommand::OpenSettingsDialogCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String OpenSettingsDialogCommand::GetName () const
{
    return "OpenSettingsDialog";
}

GS::Optional<GS::UniString> OpenSettingsDialogCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementType": {
                "$ref": "#/ElementType",
                "description": "Opens the default settings dialog of this tool. Without it, Archicad 28 and later open the settings of the selected elements, or of the current tool when nothing is selected; Archicad 25 to 27 open the default settings of the current tool."
            }
        },
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> OpenSettingsDialogCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "cancelled": {
                "type": "boolean",
                "description": "True when the user closed the dialog with Cancel, false when with OK."
            }
        },
        "additionalProperties": false,
        "required": ["cancelled"]
    })";
}

GS::ObjectState OpenSettingsDialogCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GSErrCode err = NoError;

    GS::UniString elementTypeStr;
    if (parameters.Get ("elementType", elementTypeStr)) {
        const API_ElemTypeID typeID = GetElementTypeFromNonLocalizedName (elementTypeStr);
        if (typeID == API_ZombieElemID) {
            return CreateErrorResponse (APIERR_BADPARS, "Unknown elementType: " + elementTypeStr + ".");
        }
        API_ToolBoxItem toolBoxItem = CreateToolBoxItem (typeID);
        err = OpenToolSettingsDialog (toolBoxItem);
        if (err == APIERR_BADID) {
            return CreateErrorResponse (err, "Archicad has no settings dialog for elementType " + elementTypeStr + ".");
        }
    } else {
#ifdef ServerMainVers_2800
        err = ACAPI_Dialog_SettingsDialog ();
#else
        API_ToolBoxItem toolBoxItem = {};
        err = GetCurrentToolBoxItem (toolBoxItem);
        if (err != NoError) {
            return CreateErrorResponse (err, "Failed to get the current tool.");
        }
        if (GetToolBoxItemTypeID (toolBoxItem) == API_ZombieElemID) {
            return CreateErrorResponse (APIERR_BADPARS, "No tool is active (e.g. the Arrow tool); give an elementType.");
        }
        err = OpenToolSettingsDialog (toolBoxItem);
#endif
    }

    if (err == APIERR_CANCEL) {
        return GS::ObjectState ("cancelled", true);
    }
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to open the settings dialog.");
    }
    return GS::ObjectState ("cancelled", false);
}

ShowIFCExportSettingsCommand::ShowIFCExportSettingsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ShowIFCExportSettingsCommand::GetName () const
{
    return "ShowIFCExportSettings";
}

GS::Optional<GS::UniString> ShowIFCExportSettingsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "navigatorItemId": {
                "$ref": "#/NavigatorItemId",
                "description": "Optional: the view (an item of the View Map) the dialog starts with."
            },
            "translatorName": {
                "type": "string",
                "description": "Optional: the name of the IFC export translator the dialog starts with, as GetIFCExportTranslators lists them."
            }
        },
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> ShowIFCExportSettingsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "cancelled": {
                "type": "boolean",
                "description": "True when the user closed the dialog with Cancel; then no other field is given."
            },
            "navigatorItemId": {
                "$ref": "#/NavigatorItemId",
                "description": "The view the user chose."
            },
            "viewName": {
                "type": "string",
                "description": "The name of the chosen view, when Archicad finds it in the navigator."
            },
            "translatorName": {
                "type": "string",
                "description": "The name of the IFC export translator the user chose, as IFCFileOperation's translatorName takes it."
            }
        },
        "additionalProperties": false,
        "required": ["cancelled"]
    })";
}

GS::ObjectState ShowIFCExportSettingsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    API_Guid viewGuid = APINULLGuid;
    GS::ObjectState navigatorItemId;
    if (parameters.Get ("navigatorItemId", navigatorItemId)) {
        viewGuid = GetGuidFromObjectState (navigatorItemId);
        API_NavigatorItem navItem = {};
        if (viewGuid == APINULLGuid || ACAPI_Navigator_GetNavigatorItem (&viewGuid, &navItem) != NoError) {
            return CreateErrorResponse (APIERR_BADPARS, "navigatorItemId is not a navigator item of the project.");
        }
    }

    API_IFCTranslatorIdentifier translator;
    GS::UniString translatorName;
    if (parameters.Get ("translatorName", translatorName) && !translatorName.IsEmpty ()) {
        GS::Array<API_IFCTranslatorIdentifier> translators;
        const GSErrCode listErr = ACAPI_IFC_GetIFCExportTranslatorsList (translators);
        if (listErr != NoError) {
            return CreateErrorResponse (listErr, "Failed to list the IFC export translators of the project.");
        }
        bool found = false;
        for (const API_IFCTranslatorIdentifier& candidate : translators) {
            if (candidate.name == translatorName) {
                translator = candidate;
                found = true;
                break;
            }
        }
        if (!found) {
            return CreateErrorResponse (APIERR_BADPARS, "No IFC export translator is named " + translatorName + "; GetIFCExportTranslators lists them.");
        }
    }

    const GSErrCode err = ACAPI_IFC_InvokeIFCExportSettingsDlg (viewGuid, translator);
    // The DevKit documents Cancel (GSRoot) as the answer to a cancelled dialog; APIERR_CANCEL is taken too.
    if (err == Cancel || err == APIERR_CANCEL) {
        return GS::ObjectState ("cancelled", true);
    }
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to open the IFC export settings dialog.");
    }

    GS::ObjectState response ("cancelled", false);
    response.Add ("navigatorItemId", CreateGuidObjectState (viewGuid));
    API_NavigatorItem chosenView = {};
    if (ACAPI_Navigator_GetNavigatorItem (&viewGuid, &chosenView) == NoError) {
        response.Add ("viewName", GS::UniString (chosenView.uName));
    }
    response.Add ("translatorName", translator.name);
    return response;
#else
    (void) parameters;
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "ShowIFCExportSettings requires Archicad 28 or newer.");
#endif
}
