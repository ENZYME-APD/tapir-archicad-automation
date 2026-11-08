#include "DialogCommands.hpp"
#include "ElementGDLParameterCommands.hpp"
#include "MigrationHelper.hpp"
#include "Folder.hpp"

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

GSErrCode OpenObjectSettingsDialog (API_ObjectSettingsPars& objectSettingsPars)
{
#ifdef ServerMainVers_2700
    return ACAPI_Dialog_ObjectSettings (&objectSettingsPars);
#else
    return ACAPI_Interface (APIIo_ObjectSettingsID, &objectSettingsPars, nullptr);
#endif
}

Int32 GetLibIndOfLibPartElement (const API_Element& element, API_ElemTypeID typeID)
{
    switch (typeID) {
        case API_ObjectID:  return element.object.libInd;
        case API_LampID:    return element.lamp.libInd;
        case API_WindowID:  return element.window.openingBase.libInd;
        case API_DoorID:    return element.door.openingBase.libInd;
        default:            return 0;
    }
}

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

PickLibraryPartCommand::PickLibraryPartCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String PickLibraryPartCommand::GetName () const
{
    return "PickLibraryPart";
}

GS::Optional<GS::UniString> PickLibraryPartCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementType": {
                "type": "string",
                "enum": ["Object", "Lamp", "Window", "Door"],
                "description": "Whose settings dialog to open: the Object, Lamp, Window or Door tool's. Without a subtypeUnID the dialog lists the library parts of that tool. Object by default."
            },
            "subtypeUnID": {
                "type": "string",
                "description": "Limits the dialog to the library parts of this subtype: the unique ID of the subtype library part, its main and revision GUID in the form {main}-{revision}."
            }
        },
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> PickLibraryPartCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "cancelled": {
                "type": "boolean",
                "description": "True when the user closed the dialog with Cancel; then nothing else is returned."
            },
            "libPart": {
                "$ref": "#/LibPartDetails",
                "description": "The library part the user chose. Absent when it could not be read."
            },
            "subtypeUnID": {
                "type": "string",
                "description": "The unique ID of the chosen library part's subtype, as Archicad returned it."
            },
            "parameters": {
                "type": "array",
                "description": "The GDL parameters of the chosen library part, with the values set in the dialog. Present when the user pressed OK.",
                "items": {
                    "$ref": "#/StoredGDLParameter"
                }
            }
        },
        "additionalProperties": false,
        "required": ["cancelled"]
    })";
}

GS::ObjectState PickLibraryPartCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    API_ElemTypeID typeID = API_ObjectID;
    GS::UniString elementTypeStr;
    if (parameters.Get ("elementType", elementTypeStr)) {
        typeID = GetElementTypeFromNonLocalizedName (elementTypeStr);
        if (typeID != API_ObjectID && typeID != API_LampID && typeID != API_WindowID && typeID != API_DoorID) {
            return CreateErrorResponse (APIERR_BADPARS, "Unsupported elementType: " + elementTypeStr + ". Use Object, Lamp, Window or Door.");
        }
    }

    API_ObjectSettingsPars objectSettingsPars = {};
#ifdef ServerMainVers_2600
    objectSettingsPars.elemDef.header.type = API_ElemType (typeID);
#else
    objectSettingsPars.elemDef.header.typeID = typeID;
#endif

    GS::UniString subtypeUnIDStr;
    if (parameters.Get ("subtypeUnID", subtypeUnIDStr)) {
        if (subtypeUnIDStr.GetLength () >= sizeof (objectSettingsPars.subtypeID)) {
            return CreateErrorResponse (APIERR_BADPARS, "subtypeUnID is too long: " + subtypeUnIDStr + ".");
        }
        CHTruncate (subtypeUnIDStr.ToCStr ().Get (), objectSettingsPars.subtypeID, sizeof (objectSettingsPars.subtypeID));
    }

    const GSErrCode err = OpenObjectSettingsDialog (objectSettingsPars);
    if (err != NoError) {
        if (objectSettingsPars.addPars != nullptr) {
            ACAPI_DisposeAddParHdl (&objectSettingsPars.addPars);
        }
        if (err == APIERR_CANCEL) {
            return GS::ObjectState ("cancelled", true);
        }
        if (err == APIERR_BADID) {
            return CreateErrorResponse (err, "Archicad could not open the settings dialog with these parameters (e.g. an unknown subtypeUnID).");
        }
        return CreateErrorResponse (err, "Failed to open the library part settings dialog.");
    }

    GS::ObjectState response ("cancelled", false);
    if (objectSettingsPars.subtypeID[0] != '\0') {
        response.Add ("subtypeUnID", GS::UniString (objectSettingsPars.subtypeID));
    }
    AddLibPartAndGDLParameters (response, GetLibIndOfLibPartElement (objectSettingsPars.elemDef, typeID), objectSettingsPars.addPars);
    if (objectSettingsPars.addPars != nullptr) {
        ACAPI_DisposeAddParHdl (&objectSettingsPars.addPars);
    }
    return response;
}

namespace {

// Archicad 25 and 26 open these dialogs through ACAPI_Interface with an APIIo_ ID.

GSErrCode OpenLibPartFileDialog (bool& chosen, IO::Location& location, bool& isGSM, bool& wasExternalEditorInvoked)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Dialog_OpenLibPartFileDialog (&chosen, &location, &isGSM, &wasExternalEditorInvoked);
#elif defined (ServerMainVers_2600)
    return ACAPI_Interface (APIIo_OpenLibPartFileDialogID, &chosen, &location, &isGSM, &wasExternalEditorInvoked);
#else
    wasExternalEditorInvoked = false;
    return ACAPI_Interface (APIIo_OpenLibPartFileDialogID, &chosen, &location, &isGSM);
#endif
}

GSErrCode OpenPictureDialog (bool& chosen, IO::Location& location)
{
#ifdef ServerMainVers_2700
    return ACAPI_Dialog_OpenPictureDialog (&chosen, &location);
#else
    return ACAPI_Interface (APIIo_OpenPictureDialogID, &chosen, &location);
#endif
}

GSErrCode OpenOtherObjectDialog (bool& chosen, IO::Location& location, bool& isZoneStamp)
{
#ifdef ServerMainVers_2700
    return ACAPI_Dialog_OpenOtherObjectDialog (&chosen, &location, &isZoneStamp);
#else
    return ACAPI_Interface (APIIo_OpenOtherObjectDialogID, &chosen, &location, &isZoneStamp);
#endif
}

GSErrCode SaveLibPartFileDialog (bool& chosen, IO::Location& location, bool& browseForFolder, GS::UniString* dialogTitle, GS::UniString& dialogType)
{
#ifdef ServerMainVers_2700
    return ACAPI_Dialog_SaveLibPartFileDialog (&chosen, &location, &browseForFolder, dialogTitle, &dialogType);
#else
    return ACAPI_Interface (APIIo_SaveLibPartFileDialogID, &chosen, &location, &browseForFolder, dialogTitle, &dialogType);
#endif
}

// The dialogs report Cancel through their bool, not through the error code. An APIERR_CANCEL is a
// Cancel too.
GS::ObjectState CreatePickedFileResponse (GSErrCode err, bool chosen, const IO::Location& location, const GS::UniString& errorMessage)
{
    if (err == APIERR_CANCEL || (err == NoError && !chosen)) {
        return GS::ObjectState ("cancelled", true);
    }
    if (err != NoError) {
        return CreateErrorResponse (err, errorMessage);
    }
    return GS::ObjectState ("cancelled", false, "path", location.ToDisplayText ());
}

}

PickLibraryPartFileCommand::PickLibraryPartFileCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String PickLibraryPartFileCommand::GetName () const
{
    return "PickLibraryPartFile";
}

GS::Optional<GS::UniString> PickLibraryPartFileCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "generalGDLObjectsOnly": {
                "type": "boolean",
                "description": "Sets the dialog's file filter to General GDL Objects. Default: false."
            }
        },
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> PickLibraryPartFileCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "cancelled": {
                "type": "boolean",
                "description": "True when the user cancelled the dialog or chose no valid library part."
            },
            "path": {
                "type": "string",
                "description": "The location of the chosen library part. Only when cancelled is false."
            },
            "openedInExternalEditor": {
                "type": "boolean",
                "description": "True when Archicad opened the chosen file in an external editor (Archicad 26 and later; always false in Archicad 25). Only when cancelled is false."
            }
        },
        "additionalProperties": false,
        "required": ["cancelled"]
    })";
}

GS::ObjectState PickLibraryPartFileCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    bool isGSM = false;
    parameters.Get ("generalGDLObjectsOnly", isGSM);

    bool chosen = false;
    bool wasExternalEditorInvoked = false;
    IO::Location location;
    const GSErrCode err = OpenLibPartFileDialog (chosen, location, isGSM, wasExternalEditorInvoked);

    GS::ObjectState response = CreatePickedFileResponse (err, chosen, location, "Failed to open the Open Library Part dialog.");
    if (err == NoError && chosen) {
        response.Add ("openedInExternalEditor", wasExternalEditorInvoked);
    }
    return response;
}

PickPictureFileCommand::PickPictureFileCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String PickPictureFileCommand::GetName () const
{
    return "PickPictureFile";
}

GS::Optional<GS::UniString> PickPictureFileCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "cancelled": {
                "type": "boolean",
                "description": "True when the user cancelled the dialog or chose no valid picture."
            },
            "path": {
                "type": "string",
                "description": "The location of the chosen picture. Archicad adds the picture to the embedded library when it is not in it yet. Only when cancelled is false."
            }
        },
        "additionalProperties": false,
        "required": ["cancelled"]
    })";
}

GS::ObjectState PickPictureFileCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    bool chosen = false;
    IO::Location location;
    const GSErrCode err = OpenPictureDialog (chosen, location);
    return CreatePickedFileResponse (err, chosen, location, "Failed to open the Open Picture dialog.");
}

PickZoneStampOrLabelFileCommand::PickZoneStampOrLabelFileCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String PickZoneStampOrLabelFileCommand::GetName () const
{
    return "PickZoneStampOrLabelFile";
}

GS::Optional<GS::UniString> PickZoneStampOrLabelFileCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "objectType": {
                "type": "string",
                "enum": ["Label", "ZoneStamp"],
                "description": "The type of library part the dialog lets the user open. Default: Label."
            }
        },
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> PickZoneStampOrLabelFileCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "cancelled": {
                "type": "boolean",
                "description": "True when the user cancelled the dialog or chose no valid zone stamp or label."
            },
            "path": {
                "type": "string",
                "description": "The location of the chosen zone stamp or label. Only when cancelled is false."
            }
        },
        "additionalProperties": false,
        "required": ["cancelled"]
    })";
}

GS::ObjectState PickZoneStampOrLabelFileCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString objectType = "Label";
    parameters.Get ("objectType", objectType);
    if (objectType != "Label" && objectType != "ZoneStamp") {
        return CreateErrorResponse (APIERR_BADPARS, "Unknown objectType: " + objectType + ". Use Label or ZoneStamp.");
    }
    bool isZoneStamp = (objectType == "ZoneStamp");

    bool chosen = false;
    IO::Location location;
    const GSErrCode err = OpenOtherObjectDialog (chosen, location, isZoneStamp);
    return CreatePickedFileResponse (err, chosen, location, "Failed to open the Open Library dialog.");
}

PickLibraryPartSaveLocationCommand::PickLibraryPartSaveLocationCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String PickLibraryPartSaveLocationCommand::GetName () const
{
    return "PickLibraryPartSaveLocation";
}

GS::Optional<GS::UniString> PickLibraryPartSaveLocationCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "defaultPath": {
                "type": "string",
                "description": "The location (library folder and file name) the dialog starts from. Default: <libraryPartType>.gsm in the embedded library of the project."
            },
            "libraryPartType": {
                "type": "string",
                "enum": ["Object", "Door", "Window"],
                "description": "Which Save Library Part dialog to open. Default: Object."
            },
            "browseForFolder": {
                "type": "boolean",
                "description": "Lets the user save outside the embedded library. Default: false."
            },
            "title": {
                "type": "string",
                "description": "The title of the dialog. Default: Archicad's own title."
            }
        },
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> PickLibraryPartSaveLocationCommand::GetRawResponseSchema () const
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
                "description": "The location the user chose to save the library part to. Nothing is saved there. Only when cancelled is false."
            }
        },
        "additionalProperties": false,
        "required": ["cancelled"]
    })";
}

GS::ObjectState PickLibraryPartSaveLocationCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString libraryPartType = "Object";
    parameters.Get ("libraryPartType", libraryPartType);
    GS::UniString dialogType;
    if (libraryPartType == "Object") {
        dialogType = "ExportObjectDialog";
    } else if (libraryPartType == "Door") {
        dialogType = "ExportDoorDialog";
    } else if (libraryPartType == "Window") {
        dialogType = "ExportWindowDialog";
    } else {
        return CreateErrorResponse (APIERR_BADPARS, "Unknown libraryPartType: " + libraryPartType + ". Use Object, Door or Window.");
    }

    IO::Location location;
    GS::UniString defaultPath;
    if (parameters.Get ("defaultPath", defaultPath)) {
        if (defaultPath.IsEmpty ()) {
            return CreateErrorResponse (APIERR_BADPARS, "defaultPath is empty.");
        }
        location = IO::Location (defaultPath);
    } else {
        API_SpecFolderID folderId = API_SpecFolderID::API_EmbeddedProjectLibraryFolderID;
        if (ACAPI_ProjectSettings_GetSpecFolder (&folderId, &location) != NoError || IO::Folder (location).GetStatus () != NoError) {
            return CreateErrorResponse (APIERR_GENERAL, "Failed to get the embedded library folder; give a defaultPath.");
        }
        location.AppendToLocal (IO::Name (libraryPartType + ".gsm"));
    }

    bool browseForFolder = false;
    parameters.Get ("browseForFolder", browseForFolder);

    GS::UniString title;
    const bool hasTitle = parameters.Get ("title", title);

    bool chosen = false;
    const GSErrCode err = SaveLibPartFileDialog (chosen, location, browseForFolder, hasTitle ? &title : nullptr, dialogType);
    return CreatePickedFileResponse (err, chosen, location, "Failed to open the Save Library Part dialog.");
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
