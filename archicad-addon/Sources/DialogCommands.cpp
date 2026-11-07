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

PickLibraryFolderCommand::PickLibraryFolderCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String PickLibraryFolderCommand::GetName () const
{
    return "PickLibraryFolder";
}

GS::Optional<GS::UniString> PickLibraryFolderCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "defaultName": {
                "type": "string",
                "description": "The default name the dialog offers for a new library folder. Default: New Library."
            }
        },
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> PickLibraryFolderCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "cancelled": {
                "type": "boolean",
                "description": "True when the user cancelled the dialog."
            },
            "path": {
                "type": "string",
                "description": "The location of the chosen or created folder. It is not added to the libraries of the project; use AddLibraries for that. Only when cancelled is false."
            }
        },
        "additionalProperties": false,
        "required": ["cancelled"]
    })";
}

GS::ObjectState PickLibraryFolderCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString defaultName = "New Library";
    parameters.Get ("defaultName", defaultName);

    // The API takes the name as a modifiable C string.
    char defName[256] = {};
    CHTruncate (defaultName.ToCStr ().Get (), defName, sizeof (defName));

    IO::Location location;
#ifdef ServerMainVers_2700
    const GSErrCode err = ACAPI_LibraryManagement_NewLibDial (defName, &location);
#else
    const GSErrCode err = ACAPI_Environment (APIEnv_NewLibDialID, defName, &location);
#endif

    if (err == APIERR_CANCEL) {
        return GS::ObjectState ("cancelled", true);
    }
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to open the library folder dialog.");
    }
    return GS::ObjectState ("cancelled", false, "path", location.ToDisplayText ());
}
