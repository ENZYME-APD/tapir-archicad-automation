#include "DialogCommands.hpp"
#include "ResourceIds.hpp"

#include "DGModule.hpp"

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

#ifdef ServerMainVers_2600

// The attribute types PickAttribute and PickAttributeFolder offer, with the picker Archicad shows for each.
// Only pickers that list one attribute type are used, so the chosen index always belongs to attrTypeID.
struct AttributePickerType {
    const char*         name;           // the AttributeType name in the schema
    const char*         label;          // the label in front of the picker
    API_UserControlType controlType;
    API_AttrTypeID      attrTypeID;
    bool                needsArchicad27; // the DevKit lists the fill and line pickers from Archicad 27
};

const AttributePickerType AttributePickerTypes[] = {
    { "Layer",            "Layer:",             APIUserControlType_Layer,             API_LayerID,            false },
    { "BuildingMaterial", "Building Material:", APIUserControlType_BuildingMaterial,  API_BuildingMaterialID, false },
    { "Composite",        "Composite:",         APIUserControlType_OnlyCompFill,      API_CompWallID,         false },
    { "Profile",          "Profile:",           APIUserControlType_AllProfile,        API_ProfileID,          false },
    { "Surface",          "Surface:",           APIUserControlType_Material,          API_MaterialID,         false },
    { "Fill",             "Fill:",              APIUserControlType_AllFillGradNoComp, API_FilltypeID,         true },
    { "Line",             "Line Type:",         APIUserControlType_SymbolLine,        API_LinetypeID,         true }
};

const AttributePickerType* FindAttributePickerType (const GS::UniString& name)
{
    for (const AttributePickerType& pickerType : AttributePickerTypes) {
        if (name == pickerType.name) {
            return &pickerType;
        }
    }
    return nullptr;
}

// Reads attributeType; returns the error response for an unknown or (in Archicad 26) unsupported one.
GS::Optional<GS::ObjectState> GetAttributePickerType (const GS::ObjectState& parameters, const AttributePickerType*& pickerType)
{
    GS::UniString typeStr;
    parameters.Get ("attributeType", typeStr);
    pickerType = FindAttributePickerType (typeStr);
    if (pickerType == nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, "Unknown attributeType: " + typeStr + ".");
    }
#ifndef ServerMainVers_2700
    if (pickerType->needsArchicad27) {
        return CreateErrorResponse (APIERR_NOTSUPPORTED, "The " + typeStr + " picker requires Archicad 27 or newer.");
    }
#endif
    return GS::NoValue;
}

bool IsPositiveIndex (const API_AttributeIndex& index)
{
#ifdef ServerMainVers_2700
    return index.IsPositive ();
#else
    return index > 0;
#endif
}

// A small dialog around Archicad's attribute picker or attribute folder picker: the picker opens when the
// push check is clicked, and the dialog's OK or Cancel ends the command. The pickers are members, so they are
// destroyed before the dialog they are attached to.
class AttributePickerDialog :
    public DG::ModalDialog,
    public DG::ButtonItemObserver,
    public DG::CheckItemObserver,
    public DG::CompoundItemObserver
{
public:
    AttributePickerDialog (const GS::UniString& title, const GS::UniString& label) :
        DG::ModalDialog (ACAPI_GetOwnResModule (), ID_ATTRIBUTE_PICKER_DIALOG, ACAPI_GetOwnResModule ()),
        okButton (GetReference (), ID_ATTRIBUTE_PICKER_DIALOG_OK_BUTTON),
        cancelButton (GetReference (), ID_ATTRIBUTE_PICKER_DIALOG_CANCEL_BUTTON),
        pickerLabel (GetReference (), ID_ATTRIBUTE_PICKER_DIALOG_LABEL),
        pickerPushCheck (GetReference (), ID_ATTRIBUTE_PICKER_DIALOG_PICKER)
    {
        SetTitle (title);
        pickerLabel.SetText (label);
        AttachToAllItems (*this);
    }

    GSErrCode CreateAttributePicker (API_UserControlType controlType)
    {
        API_AttributePickerParams params;
        params.type = controlType;
        params.dialogID = ID_ATTRIBUTE_PICKER_DIALOG;
        params.itemID = ID_ATTRIBUTE_PICKER_DIALOG_PICKER;
        params.pushCheckAppearance = API_AttributePickerParams::PushCheckAppearance::ArrowIconAndText;
        params.emptySelectionBehaviour = API_AttributePickerParams::EmptySelectionBehaviour::SelectFirst;
#ifdef ServerMainVers_2700
        return ACAPI_Dialog_CreateAttributePicker (params, attributePicker);
#else
        return ACAPI_Interface_CreateAttributePicker (params, attributePicker);
#endif
    }

    GSErrCode CreateAttributeFolderPicker (API_UserControlType controlType)
    {
        API_AttributeFolderPickerParams params;
        params.type = controlType;
        params.dialogID = ID_ATTRIBUTE_PICKER_DIALOG;
        params.itemID = ID_ATTRIBUTE_PICKER_DIALOG_PICKER;
        params.pushCheckAppearance = API_AttributeFolderPickerParams::PushCheckAppearance::ArrowIconAndText;
        return ACAPI_Attribute_CreateAttributeFolderPicker (params, attributeFolderPicker);
    }

    // OK is only enabled while the attribute picker holds an attribute; the folder picker always holds a folder.
    void UpdateOkButton ()
    {
        if (attributePicker != nullptr && !IsPositiveIndex (attributePicker->GetSelectedAttributeIndex ())) {
            okButton.Disable ();
        } else {
            okButton.Enable ();
        }
    }

    GS::Owner<API_AttributePicker>       attributePicker;
    GS::Owner<API_AttributeFolderPicker> attributeFolderPicker;

private:
    virtual void ButtonClicked (const DG::ButtonClickEvent& ev) override
    {
        if (ev.GetSource () == &okButton) {
            PostCloseRequest (DG::ModalDialog::Accept);
        } else if (ev.GetSource () == &cancelButton) {
            PostCloseRequest (DG::ModalDialog::Cancel);
        }
    }

    virtual void CheckItemChanged (const DG::CheckItemChangeEvent& ev) override
    {
        if (ev.GetSource () != &pickerPushCheck) {
            return;
        }
        if (attributePicker != nullptr) {
            attributePicker->Invoke ();
        } else if (attributeFolderPicker != nullptr) {
            attributeFolderPicker->Invoke ();
        }
        UpdateOkButton ();
    }

    DG::Button    okButton;
    DG::Button    cancelButton;
    DG::LeftText  pickerLabel;
    DG::PushCheck pickerPushCheck;
};

GS::UniString GetDialogTitle (const GS::ObjectState& parameters, const GS::UniString& defaultTitle)
{
    GS::UniString title;
    if (parameters.Get ("title", title) && !title.IsEmpty ()) {
        return title;
    }
    return defaultTitle;
}

// The path of a folder from its root folder, the root folder left out, as in the AttributeFolder schema.
// Archicad puts the localized name of the root folder ("Layers", "Ebenen") in front of the path it returns,
// so the folder is read again by its guid, which gives its full path, and that first name is dropped.
GS::Optional<GS::Array<GS::UniString>> GetPathFromRootFolder (const API_AttributeFolder& selectedFolder)
{
    API_AttributeFolder rootFolder;
    rootFolder.typeID = selectedFolder.typeID;
    rootFolder.guid = GS::NULLGuid;
    if (ACAPI_Attribute_GetFolder (rootFolder) != NoError) {
        return GS::NoValue;
    }
    if (selectedFolder.guid.IsNull () || selectedFolder.guid == rootFolder.guid) {
        return GS::Array<GS::UniString> ();
    }

    API_AttributeFolder folder;
    folder.typeID = selectedFolder.typeID;
    folder.guid = selectedFolder.guid;
    if (ACAPI_Attribute_GetFolder (folder) != NoError) {
        return GS::NoValue;
    }
    GS::Array<GS::UniString> path = folder.path;
    if (!path.IsEmpty () && rootFolder.path.GetSize () == 1 && path[0] == rootFolder.path[0]) {
        path.Delete (0);
    }
    return path;
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

PickAttributeCommand::PickAttributeCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String PickAttributeCommand::GetName () const
{
    return "PickAttribute";
}

GS::Optional<GS::UniString> PickAttributeCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributeType": {
                "type": "string",
                "description": "The type of the attribute to pick. Fill and Line need Archicad 27 or newer. Composite lists the composites only.",
                "enum": ["Layer", "BuildingMaterial", "Composite", "Profile", "Surface", "Fill", "Line"]
            },
            "attributeId": {
                "$ref": "#/AttributeId",
                "description": "The attribute selected when the dialog opens. Without it the picker starts empty and OK is enabled once the user has picked an attribute."
            },
            "title": {
                "type": "string",
                "description": "The title of the dialog. Default: Pick Attribute."
            }
        },
        "additionalProperties": false,
        "required": ["attributeType"]
    })";
}

GS::Optional<GS::UniString> PickAttributeCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "cancelled": {
                "type": "boolean",
                "description": "True when the user closed the dialog with Cancel, false when with OK."
            },
            "attributeType": {
                "$ref": "#/AttributeType",
                "description": "The type of the chosen attribute, the attributeType of the input. Only when not cancelled."
            },
            "attributeId": {
                "$ref": "#/AttributeId",
                "description": "The chosen attribute. Only when not cancelled."
            }
        },
        "additionalProperties": false,
        "required": ["cancelled"]
    })";
}

GS::ObjectState PickAttributeCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2600
    const AttributePickerType* pickerType = nullptr;
    const GS::Optional<GS::ObjectState> invalidType = GetAttributePickerType (parameters, pickerType);
    if (invalidType.HasValue ()) {
        return *invalidType;
    }

    API_AttributeIndex initialIndex = {};
    GS::ObjectState attributeId;
    const bool hasInitialAttribute = parameters.Get ("attributeId", attributeId);
    if (hasInitialAttribute) {
        initialIndex = GetAttributeIndexFromGuid (pickerType->attrTypeID, GetGuidFromObjectState (attributeId));
        if (!IsPositiveIndex (initialIndex)) {
            return CreateErrorResponse (APIERR_BADPARS, "attributeId is not a " + GS::UniString (pickerType->name) + " of the project.");
        }
    }

    AttributePickerDialog dialog (GetDialogTitle (parameters, "Pick Attribute"), pickerType->label);
    const GSErrCode err = dialog.CreateAttributePicker (pickerType->controlType);
    if (err != NoError || dialog.attributePicker == nullptr) {
        return CreateErrorResponse (err != NoError ? err : APIERR_GENERAL, "Failed to create the attribute picker.");
    }
    if (hasInitialAttribute) {
        dialog.attributePicker->SetSelectedAttributeIndex (initialIndex);
        dialog.attributePicker->Refresh ();
    }
    dialog.UpdateOkButton ();

    if (!dialog.Invoke ()) {
        return GS::ObjectState ("cancelled", true);
    }

    const API_Guid guid = GetAttributeGuidFromIndex (pickerType->attrTypeID, dialog.attributePicker->GetSelectedAttributeIndex ());
    if (guid == APINULLGuid) {
        return CreateErrorResponse (APIERR_GENERAL, "Failed to read the chosen attribute.");
    }

    GS::ObjectState response ("cancelled", false);
    response.Add ("attributeType", GS::UniString (pickerType->name));
    response.Add ("attributeId", CreateGuidObjectState (guid));
    return response;
#else
    (void) parameters;
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "PickAttribute requires Archicad 26 or newer.");
#endif
}

PickAttributeFolderCommand::PickAttributeFolderCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String PickAttributeFolderCommand::GetName () const
{
    return "PickAttributeFolder";
}

GS::Optional<GS::UniString> PickAttributeFolderCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributeType": {
                "type": "string",
                "description": "The type of the attributes whose folders the picker lists. Fill and Line need Archicad 27 or newer.",
                "enum": ["Layer", "BuildingMaterial", "Composite", "Profile", "Surface", "Fill", "Line"]
            },
            "title": {
                "type": "string",
                "description": "The title of the dialog. Default: Pick Attribute Folder."
            }
        },
        "additionalProperties": false,
        "required": ["attributeType"]
    })";
}

GS::Optional<GS::UniString> PickAttributeFolderCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "cancelled": {
                "type": "boolean",
                "description": "True when the user closed the dialog with Cancel, false when with OK."
            },
            "attributeFolder": {
                "$ref": "#/AttributeFolder",
                "description": "The chosen folder; its path works as the targetFolder of MoveAttributesAndFolders. Only when not cancelled."
            }
        },
        "additionalProperties": false,
        "required": ["cancelled"]
    })";
}

GS::ObjectState PickAttributeFolderCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2600
    const AttributePickerType* pickerType = nullptr;
    const GS::Optional<GS::ObjectState> invalidType = GetAttributePickerType (parameters, pickerType);
    if (invalidType.HasValue ()) {
        return *invalidType;
    }

    AttributePickerDialog dialog (GetDialogTitle (parameters, "Pick Attribute Folder"), "Folder:");
    const GSErrCode err = dialog.CreateAttributeFolderPicker (pickerType->controlType);
    if (err != NoError || dialog.attributeFolderPicker == nullptr) {
        return CreateErrorResponse (err != NoError ? err : APIERR_GENERAL, "Failed to create the attribute folder picker.");
    }

    // The picker starts on the root folder, so OK always returns a folder.
    API_AttributeFolder rootFolder;
    rootFolder.typeID = pickerType->attrTypeID;
    rootFolder.guid = GS::NULLGuid;
    if (ACAPI_Attribute_GetFolder (rootFolder) == NoError) {
        dialog.attributeFolderPicker->SetSelectedAttributeFolder (rootFolder);
    }
    dialog.UpdateOkButton ();

    if (!dialog.Invoke ()) {
        return GS::ObjectState ("cancelled", true);
    }

    API_AttributeFolder selectedFolder = dialog.attributeFolderPicker->GetSelectedAttributeFolder ();
    selectedFolder.typeID = pickerType->attrTypeID;
    const GS::Optional<GS::Array<GS::UniString>> path = GetPathFromRootFolder (selectedFolder);
    if (!path.HasValue ()) {
        return CreateErrorResponse (APIERR_GENERAL, "Failed to read the chosen attribute folder.");
    }

    GS::ObjectState attributeFolder ("attributeType", GS::UniString (pickerType->name));
    attributeFolder.Add ("path", *path);

    GS::ObjectState response ("cancelled", false);
    response.Add ("attributeFolder", attributeFolder);
    return response;
#else
    (void) parameters;
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "PickAttributeFolder requires Archicad 26 or newer.");
#endif
}
