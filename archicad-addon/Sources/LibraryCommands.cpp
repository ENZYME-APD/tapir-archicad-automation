#include "LibraryCommands.hpp"
#include "ObjectState.hpp"
#include "MigrationHelper.hpp"
#include "Folder.hpp"
#include "File.hpp"
#include "FileSystem.hpp"
#include "GSUnID.hpp"

// The library part type names of the LibraryPartType schema definition.
static bool LibPartTypeIdFromString (const GS::UniString& typeStr, API_LibTypeID& typeID)
{
    if (typeStr == "Window") {
        typeID = APILib_WindowID;
    } else if (typeStr == "Door") {
        typeID = APILib_DoorID;
    } else if (typeStr == "Object") {
        typeID = APILib_ObjectID;
    } else if (typeStr == "Lamp") {
        typeID = APILib_LampID;
    } else if (typeStr == "Room") {
        typeID = APILib_RoomID;
    } else if (typeStr == "Property") {
        typeID = APILib_PropertyID;
    } else if (typeStr == "PlanSign") {
        typeID = APILib_PlanSignID;
    } else if (typeStr == "Label") {
        typeID = APILib_LabelID;
    } else if (typeStr == "Macro") {
        typeID = APILib_MacroID;
    } else if (typeStr == "Pict" || typeStr == "Picture") {
        typeID = APILib_PictID;
    } else if (typeStr == "ListScheme") {
        typeID = APILib_ListSchemeID;
    } else if (typeStr == "Skylight") {
        typeID = APILib_SkylightID;
    } else if (typeStr == "OpeningSymbol") {
        typeID = APILib_OpeningSymbolID;
    } else {
        return false;
    }
    return true;
}
#ifdef ServerMainVers_2800
#include "ACAPI/Result.hpp"
#include "ACAPI/Library/LibraryManager.hpp"
#include "ACAPI/Library/LibPart.hpp"
#include "ACAPI/Library/GSMObject.hpp"

#include <functional>
#include <optional>

using ACAPI::Library::GSMObject;

struct LibraryPartScriptSection
{
    const char* key;
    std::function<ACAPI::Result<void> (GSMObject::Modifier&, const GS::UniString&)> set;
    std::function<ACAPI::Result<void> (GSMObject::Modifier&)> remove;
};

static const LibraryPartScriptSection libraryPartScriptSections[] = {
    { "master",
        [] (GSMObject::Modifier& modifier, const GS::UniString& script) { return modifier.SetMasterScript (script); },
        [] (GSMObject::Modifier& modifier) { return modifier.DeleteMasterScript (); } },
    { "parameter",
        [] (GSMObject::Modifier& modifier, const GS::UniString& script) { return modifier.SetParameterScript (script); },
        [] (GSMObject::Modifier& modifier) { return modifier.DeleteParameterScript (); } },
    { "2d",
        [] (GSMObject::Modifier& modifier, const GS::UniString& script) { return modifier.Set2DScript (script); },
        [] (GSMObject::Modifier& modifier) { return modifier.Delete2DScript (); } },
    { "3d",
        [] (GSMObject::Modifier& modifier, const GS::UniString& script) { return modifier.Set3DScript (script); },
        [] (GSMObject::Modifier& modifier) { return modifier.Delete3DScript (); } },
    { "properties",
        [] (GSMObject::Modifier& modifier, const GS::UniString& script) { return modifier.SetPropertiesScript (script); },
        [] (GSMObject::Modifier& modifier) { return modifier.DeletePropertiesScript (); } },
    { "interface",
        [] (GSMObject::Modifier& modifier, const GS::UniString& script) { return modifier.SetInterfaceScript (script); },
        [] (GSMObject::Modifier& modifier) { return modifier.DeleteInterfaceScript (); } },
    { "forwardMigration",
        [] (GSMObject::Modifier& modifier, const GS::UniString& script) { return modifier.SetForwardMigrationScript (script); },
        [] (GSMObject::Modifier& modifier) { return modifier.DeleteForwardMigrationScript (); } },
    { "backwardMigration",
        [] (GSMObject::Modifier& modifier, const GS::UniString& script) { return modifier.SetBackwardMigrationScript (script); },
        [] (GSMObject::Modifier& modifier) { return modifier.DeleteBackwardMigrationScript (); } }
};

static const LibraryPartScriptSection* FindLibraryPartScriptSection (const GS::UniString& key)
{
    for (const LibraryPartScriptSection& section : libraryPartScriptSections) {
        if (key == GS::UniString (section.key)) {
            return &section;
        }
    }
    return nullptr;
}

struct LibraryPartStringMetadata
{
    const char* key;
    std::function<ACAPI::Result<void> (GSMObject::Modifier&, const GS::UniString&)> set;
};

static const LibraryPartStringMetadata libraryPartStringMetadata[] = {
    { "author",         [] (GSMObject::Modifier& modifier, const GS::UniString& value) { return modifier.SetAuthor (value); } },
    { "keywords",       [] (GSMObject::Modifier& modifier, const GS::UniString& value) { return modifier.SetKeywords (value); } },
    { "license",        [] (GSMObject::Modifier& modifier, const GS::UniString& value) { return modifier.SetLicense (value); } },
    { "licenseVersion", [] (GSMObject::Modifier& modifier, const GS::UniString& value) { return modifier.SetLicenseVersion (value); } },
    { "password",       [] (GSMObject::Modifier& modifier, const GS::UniString& value) { return modifier.SetPassword (value); } }
};

struct LibraryPartBoolMetadata
{
    const char* key;
    std::function<ACAPI::Result<void> (GSMObject::Modifier&, bool)> set;
};

static const LibraryPartBoolMetadata libraryPartBoolMetadata[] = {
    { "placeable",             [] (GSMObject::Modifier& modifier, bool value) { return modifier.SetPlaceable (value); } },
    { "uiDefault",             [] (GSMObject::Modifier& modifier, bool value) { return modifier.SetUIDefault (value); } },
    { "isSubtype",             [] (GSMObject::Modifier& modifier, bool value) { return modifier.SetIsSubtype (value); } },
    { "hierarchicalPagesUsed", [] (GSMObject::Modifier& modifier, bool value) { return modifier.SetHierarchicalPagesUsed (value); } }
};

// Runs modify on the GSM object of the loaded library part called name, and returns the execution result.
// A part that is not editable is refused. When modify fails, it sets failedWhat (e.g. "setting the '3d' script")
// and failedText (the error text of the failed call), which go into the error message.
template <typename LibraryManagerResult>
static GS::ObjectState ModifyLibraryPart (
    LibraryManagerResult& libraryManager,
    const GS::UniString& name,
    const std::function<GSErrCode (GSMObject::Modifier&)>& modify,
    const GS::UniString& failedWhat,
    const GS::UniString& failedText)
{
    auto libPart = libraryManager->GetLibPartByName (name);
    if (libPart.IsErr ()) {
        return CreateFailedExecutionResult (libPart.UnwrapErr ().kind, "No loaded library part with the name '" + name + "'. Use the fileName (with extension) returned by GetAvailableLibraryParts.");
    }

    auto gsmObject = libraryManager->GetGSMObjectOfLibPart (libPart.Unwrap ());
    if (gsmObject.IsErr ()) {
        return CreateFailedExecutionResult (gsmObject.UnwrapErr ().kind, "Failed to get the GSM object of the library part '" + name + "'.");
    }

    if (!gsmObject->IsEditable ()) {
        return CreateFailedExecutionResult (APIERR_GENERAL, "The library part '" + name + "' is not editable, nothing was changed. Only a library part loaded from a writable file can be modified; a part of a library package or a read-only library must first be copied into a writable library.");
    }

    ACAPI::Result<void> modifyResult = gsmObject->Modify (modify);
    if (!modifyResult.IsErr ()) {
        return CreateSuccessfulExecutionResult ();
    }

    GS::UniString errorMessage = "Failed to modify the library part '" + name + "'";
    GS::UniString errorText = modifyResult.UnwrapErr ().text.c_str ();
    if (!failedWhat.IsEmpty ()) {
        errorMessage += ": " + failedWhat + " failed";
        errorText = failedText;
    }
    if (!errorText.IsEmpty ()) {
        errorMessage += " (" + errorText + ")";
    }
    errorMessage += ".";
    return CreateFailedExecutionResult (modifyResult.UnwrapErr ().kind, errorMessage);
}

struct LibraryPartScriptReader
{
    const char* key;
    std::function<bool (const GSMObject&)> has;
    std::function<ACAPI::Result<GS::UniString> (const GSMObject&, const GS::UniString&)> get;
};

static const LibraryPartScriptReader libraryPartScriptReaders[] = {
    { "master",     [] (const GSMObject& gsmObject) { return gsmObject.HasMasterScript (); },     [] (const GSMObject& gsmObject, const GS::UniString& password) { return gsmObject.GetMasterScript (password); } },
    { "parameter",  [] (const GSMObject& gsmObject) { return gsmObject.HasParameterScript (); },  [] (const GSMObject& gsmObject, const GS::UniString& password) { return gsmObject.GetParameterScript (password); } },
    { "2d",         [] (const GSMObject& gsmObject) { return gsmObject.Has2DScript (); },         [] (const GSMObject& gsmObject, const GS::UniString& password) { return gsmObject.Get2DScript (password); } },
    { "3d",         [] (const GSMObject& gsmObject) { return gsmObject.Has3DScript (); },         [] (const GSMObject& gsmObject, const GS::UniString& password) { return gsmObject.Get3DScript (password); } },
    { "properties", [] (const GSMObject& gsmObject) { return gsmObject.HasPropertiesScript (); }, [] (const GSMObject& gsmObject, const GS::UniString& password) { return gsmObject.GetPropertiesScript (password); } }
};

static Int32 GetLibIndOfElement (const API_Element& element)
{
    switch (GetElemTypeId (element.header)) {
        case API_ObjectID:                return element.object.libInd;
        case API_LampID:                  return element.lamp.libInd;
        case API_WindowID:                return element.window.openingBase.libInd;
        case API_DoorID:                  return element.door.openingBase.libInd;
        case API_SkylightID:              return element.skylight.openingBase.libInd;
        case API_ZoneID:                  return element.zone.libInd;
        case API_CurtainWallFrameID:      return element.cwFrame.libInd;
        case API_CurtainWallPanelID:      return element.cwPanel.libInd;
        case API_CurtainWallJunctionID:   return element.cwJunction.libInd;
        case API_CurtainWallAccessoryID:  return element.cwAccessory.libInd;
        default:                          return 0;
    }
}

static GS::UniString GetResultErrorText (const ACAPI::Error& error)
{
    GS::UniString text (error.text.c_str ());
    return text.IsEmpty () ? GS::UniString () : " (" + text + ")";
}

// Finds the library part given by 'libraryPartName' or by the element in 'elementId'.
static std::optional<ACAPI::Library::LibPart> FindLibPartOfItem (const ACAPI::Library::LibraryManager& libraryManager, const GS::ObjectState& item, GS::ObjectState& error)
{
    const GS::ObjectState* elementId = item.Get ("elementId");
    if (elementId == nullptr) {
        GS::UniString name;
        if (!item.Get ("libraryPartName", name) || name.IsEmpty ()) {
            error = CreateErrorResponse (APIERR_BADPARS, "Missing 'libraryPartName' or 'elementId' parameter.");
            return std::nullopt;
        }
        auto libPart = libraryManager.GetLibPartByName (name);
        if (libPart.IsErr ()) {
            error = CreateErrorResponse (libPart.UnwrapErr ().kind, "No loaded library part with the name '" + name + "'. Use the fileName (with extension) returned by GetAvailableLibraryParts.");
            return std::nullopt;
        }
        return libPart.Unwrap ();
    }

    API_Element element = {};
    element.header.guid = GetGuidFromObjectState (*elementId);
    const GSErrCode err = ACAPI_Element_Get (&element);
    if (err != NoError) {
        error = CreateErrorResponse (err, "Not found element with guid " + APIGuidToString (element.header.guid) + ".");
        return std::nullopt;
    }
    const Int32 libInd = GetLibIndOfElement (element);
    if (libInd <= 0) {
        error = CreateErrorResponse (APIERR_BADPARS, "The element with guid " + APIGuidToString (element.header.guid) + " is not placed from a library part.");
        return std::nullopt;
    }
    auto libPart = libraryManager.GetLibPartByLibInd (libInd);
    if (libPart.IsErr ()) {
        error = CreateErrorResponse (libPart.UnwrapErr ().kind, "Failed to get the library part of the element with guid " + APIGuidToString (element.header.guid) + GetResultErrorText (libPart.UnwrapErr ()) + ".");
        return std::nullopt;
    }
    return libPart.Unwrap ();
}
#endif

AddFilesToEmbeddedLibraryCommand::AddFilesToEmbeddedLibraryCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String AddFilesToEmbeddedLibraryCommand::GetName () const
{
    return "AddFilesToEmbeddedLibrary";
}

GS::Optional<GS::UniString> AddFilesToEmbeddedLibraryCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "files": {
                "$ref": "#/LibraryFileAdditions"
            },
            "overwriteExisting": {
                "type": "boolean",
                "description": "Optional. When true, an embedded library item already existing on an outputPath is deleted first, so the newly added file replaces the loaded library part. By default false: the existing loaded part stays in use. Requires Archicad 27 or newer."
            }
        },
        "additionalProperties": false,
        "required": [
            "files"
        ]
    })";
}

GS::Optional<GS::UniString> AddFilesToEmbeddedLibraryCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    })";
}

GS::ObjectState AddFilesToEmbeddedLibraryCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
	auto folderId = API_SpecFolderID::API_EmbeddedProjectLibraryFolderID;

	IO::Location embeddedLibraryFolder;

	if (ACAPI_ProjectSettings_GetSpecFolder (&folderId, &embeddedLibraryFolder) != NoError || IO::Folder (embeddedLibraryFolder).GetStatus () != NoError) {
        return CreateErrorResponse (APIERR_GENERAL, "Failed to get embedded library folder.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    bool overwriteExisting = false;
    parameters.Get ("overwriteExisting", overwriteExisting);

    GS::Array<GS::ObjectState> files;
    parameters.Get ("files", files);

    for (const GS::ObjectState& file : files) {
        GS::UniString inputPath;
        GS::UniString outputPath;
        if (!file.Get ("inputPath", inputPath) || !file.Get ("outputPath", outputPath) || inputPath.IsEmpty () || outputPath.IsEmpty ()) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing inputPath or outputPath parameters."));
            continue;
        }

        IO::File inputFile (IO::Location (inputPath), IO::File::OnNotFound::Fail);
        if (inputFile.GetStatus () != NoError) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Failed to read file on the given inputPath."));
            continue;
        }

		IO::Location outputFileLoc = embeddedLibraryFolder;
		outputFileLoc.AppendToLocal (IO::RelativeLocation (outputPath));

        if (overwriteExisting && IO::File (outputFileLoc, IO::File::OnNotFound::Fail).GetStatus () == NoError) {
#ifdef ServerMainVers_2700
            const bool keepGSMFile = false;
            const bool silentMode = true;
            GSErrCode deleteErr = ACAPI_LibraryManagement_DeleteEmbeddedLibItem (&outputFileLoc, keepGSMFile, silentMode);
            if (deleteErr != NoError) {
                executionResults (CreateFailedExecutionResult (deleteErr, "Failed to delete the existing embedded library item before overwriting."));
                continue;
            }
#else
            executionResults (CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "The overwriteExisting flag requires Archicad 27 or newer."));
            continue;
#endif
        }

        IO::Location outputFolder = outputFileLoc;
        if (outputFolder.DeleteLastLocalName () != NoError ||
            IO::fileSystem.CreateFolderTree (outputFolder) != NoError ||
            IO::fileSystem.Copy (inputFile.GetLocation (), outputFileLoc) != NoError) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "The given outputPath is not a valid relative path."));
            continue;
        }

        API_LibPart libPart = {};

        libPart.typeID = APILib_PictID;

        GS::UniString typeStr;
        if (file.Get ("type", typeStr) && !LibPartTypeIdFromString (typeStr, libPart.typeID)) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Unknown library part type."));
            continue;
        }

        libPart.location = &outputFileLoc;

        GSErrCode err = ACAPI_LibraryPart_Register (&libPart);
        if (err != NoError) {
            executionResults (CreateFailedExecutionResult (err, "Failed to add the file to the library."));
            continue;
        }

        executionResults (CreateSuccessfulExecutionResult ());
    }

    return response;
}

DeleteEmbeddedLibraryItemsCommand::DeleteEmbeddedLibraryItemsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String DeleteEmbeddedLibraryItemsCommand::GetName () const
{
    return "DeleteEmbeddedLibraryItems";
}

GS::Optional<GS::UniString> DeleteEmbeddedLibraryItemsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "embeddedLibraryItems": {
                "type": "array",
                "description": "A list of embedded library items to delete.",
                "items": {
                    "type": "object",
                    "properties": {
                        "path": {
                            "type": "string",
                            "description": "The relative path of the library item inside the embedded library, the same path AddFilesToEmbeddedLibrary takes as outputPath."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "path"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "embeddedLibraryItems"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteEmbeddedLibraryItemsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    })";
}

GS::ObjectState DeleteEmbeddedLibraryItemsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    auto folderId = API_SpecFolderID::API_EmbeddedProjectLibraryFolderID;

    IO::Location embeddedLibraryFolder;

    if (ACAPI_ProjectSettings_GetSpecFolder (&folderId, &embeddedLibraryFolder) != NoError || IO::Folder (embeddedLibraryFolder).GetStatus () != NoError) {
        return CreateErrorResponse (APIERR_GENERAL, "Failed to get embedded library folder.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    GS::Array<GS::ObjectState> items;
    parameters.Get ("embeddedLibraryItems", items);

    const auto ItemExists = [] (const IO::Location& location) {
        bool exists = false;
        return IO::fileSystem.Contains (location, &exists) == NoError && exists;
    };

    // Index of the item in the input and its location, for every item with a path.
    // The existing items are deleted with one API call, the others one by one as before.
    struct ItemToDelete {
        UIndex       inputIndex;
        IO::Location location;
    };
    GS::Array<ItemToDelete> itemsToDelete;
    GS::Array<IO::Location> locationsToDelete;
    GS::Array<GS::ObjectState> results;

    const bool keepGSMFile = false;
    const bool silentMode = true;
    const auto DeleteOneByOne = [&] (const ItemToDelete& item) {
        IO::Location itemLocation = item.location;
        GSErrCode err = ACAPI_LibraryManagement_DeleteEmbeddedLibItem (&itemLocation, keepGSMFile, silentMode);
        if (err != NoError) {
            results[item.inputIndex] = CreateFailedExecutionResult (err, "Failed to delete the embedded library item on the given path.");
        }
    };

    for (UIndex i = 0; i < items.GetSize (); ++i) {
        GS::UniString path;
        if (!items[i].Get ("path", path) || path.IsEmpty ()) {
            results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "Missing path parameter."));
            continue;
        }

        IO::Location itemLocation = embeddedLibraryFolder;
        itemLocation.AppendToLocal (IO::RelativeLocation (path));

        results.Push (CreateSuccessfulExecutionResult ());
        if (ItemExists (itemLocation)) {
            itemsToDelete.Push ({ i, itemLocation });
            locationsToDelete.Push (itemLocation);
        } else {
            DeleteOneByOne ({ i, itemLocation });
        }
    }

    if (!locationsToDelete.IsEmpty () &&
        ACAPI_LibraryManagement_DeleteEmbeddedLibItems (&locationsToDelete, keepGSMFile, silentMode) != NoError) {
        // The API reports one error for the whole list, so find out item by item which ones failed:
        // an item that is gone now was deleted, the others are tried one by one as before.
        for (const ItemToDelete& item : itemsToDelete) {
            if (ItemExists (item.location)) {
                DeleteOneByOne (item);
            }
        }
    }

    for (const GS::ObjectState& result : results) {
        executionResults (result);
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
}

// ---------------------------------------------------------------------------
// CreateLibraryParts
// ---------------------------------------------------------------------------

template<std::size_t N>
static void CopyToUCharArray (const GS::UniString& source, GS::uchar_t (&target)[N])
{
    const auto ustrObject = source.ToUStr ();
    const GS::uchar_t* sourceString = ustrObject;
    GS::ucsncpy (target, sourceString, N);
    target[N - 1] = 0;
}

// The parameter type names are the ones GetGDLParametersOfElements reports
// (ConvertAddParIDToString in ElementGDLParameterCommands.cpp).
static bool GDLParameterTypeFromString (const GS::UniString& typeName, API_AddParID& typeID)
{
    static const struct {
        const char*  name;
        API_AddParID typeID;
    } parameterTypes[] = {
        { "Integer",          APIParT_Integer },
        { "Length",           APIParT_Length },
        { "Angle",            APIParT_Angle },
        { "RealNumber",       APIParT_RealNum },
        { "LightSwitch",      APIParT_LightSw },
        { "RGBColor",         APIParT_ColRGB },
        { "Intensity",        APIParT_Intens },
        { "LineType",         APIParT_LineTyp },
        { "Material",         APIParT_Mater },
        { "FillPattern",      APIParT_FillPat },
        { "PenColor",         APIParT_PenCol },
        { "String",           APIParT_CString },
        { "Boolean",          APIParT_Boolean },
        { "Separator",        APIParT_Separator },
        { "Title",            APIParT_Title },
        { "BuildingMaterial", APIParT_BuildingMaterial },
        { "Profile",          APIParT_Profile }
    };
    for (const auto& parameterType : parameterTypes) {
        if (typeName == parameterType.name) {
            typeID = parameterType.typeID;
            return true;
        }
    }
    return false;
}

static bool GetNumberField (const GS::ObjectState& os, const char* fieldName, double& value)
{
    if (os.Get (fieldName, value)) {
        return true;
    }
    Int32 intValue = 0;
    if (os.Get (fieldName, intValue)) {
        value = intValue;
        return true;
    }
    return false;
}

// The default value of a parameter definition, from its "value" field, typed the way
// SetGDLParametersOfElements reads the same parameter types. Without a value the
// zeroed definition stays: 0, Off, false or an empty string.
static bool SetParameterDefinitionValue (API_AddParType& addPar, const GS::ObjectState& definition, GS::UniString& errorMessage)
{
    const bool hasValue = definition.Contains ("value");
    switch (addPar.typeID) {
        case APIParT_Integer:
        case APIParT_PenCol:
        case APIParT_LineTyp:
        case APIParT_Mater:
        case APIParT_FillPat:
        case APIParT_BuildingMaterial:
        case APIParT_Profile:
        case APIParT_ColRGB:
        case APIParT_Intens:
        case APIParT_Length:
        case APIParT_RealNum:
        case APIParT_Angle: {
            double value = 0.0;
            if (hasValue && !GetNumberField (definition, "value", value)) {
                errorMessage = "the value of a numeric parameter must be a number.";
                return false;
            }
            addPar.value.real = value;
            return true;
        }
        case APIParT_LightSw: {
            GS::String value;
            if (hasValue && !definition.Get ("value", value)) {
                errorMessage = "the value of a LightSwitch parameter must be \"On\" or \"Off\".";
                return false;
            }
            addPar.value.real = (value == "On" ? 1 : 0);
            return true;
        }
        case APIParT_Boolean: {
            bool value = false;
            if (hasValue && !definition.Get ("value", value)) {
                errorMessage = "the value of a Boolean parameter must be true or false.";
                return false;
            }
            addPar.value.real = (value ? 1 : 0);
            return true;
        }
        case APIParT_CString:
        case APIParT_Title: {
            GS::UniString value;
            if (hasValue && !definition.Get ("value", value)) {
                errorMessage = "the value of a String parameter must be a string.";
                return false;
            }
            GS::ucscpy (addPar.value.uStr, value.ToUStr (0, GS::Min (value.GetLength (), static_cast<USize> (API_UAddParStrLen - 1))).Get ());
            return true;
        }
        case APIParT_Separator:
            return true;
        default:
            errorMessage = "unsupported parameter type.";
            return false;
    }
}

// Fills parentUnID (and typeID, when the parent is found among the loaded library parts
// and the caller did not set it) from "parentId" - the parent subtype's unique ID
// ("{main}-{revision}") or main GUID (what GetAvailableLibraryParts reports) - or from
// "parentName", the parent subtype's document name.
static GS::Optional<GS::ObjectState> ResolveParentLibraryPart (const GS::ObjectState& libraryPartData, API_LibPart& newLibPart)
{
    GS::UniString parentId;
    GS::UniString parentName;
    libraryPartData.Get ("parentId", parentId);
    libraryPartData.Get ("parentName", parentName);

    if (!parentId.IsEmpty ()) {
        if (parentId.Contains ("}-{")) {
            CHTruncate (parentId.ToCStr ().Get (), newLibPart.parentUnID, sizeof (newLibPart.parentUnID));
            return {};
        }

        const API_Guid parentApiGuid = APIGuidFromString (parentId.ToCStr ());
        if (parentApiGuid == APINULLGuid) {
            return CreateErrorResponse (APIERR_BADPARS, "parentId is neither a unique ID ({main}-{revision}) nor a GUID.");
        }
        const GS::Guid parentGuid = APIGuid2GSGuid (parentApiGuid);

        Int32 partCount = 0;
        if (ACAPI_LibraryPart_GetNum (&partCount) != NoError) {
            return CreateErrorResponse (APIERR_GENERAL, "Failed to enumerate library parts.");
        }
        for (Int32 i = 1; i <= partCount; ++i) {
            API_LibPart libPart = {};
            libPart.index = i;
            if (ACAPI_LibraryPart_Get (&libPart) != NoError) {
                continue;
            }
            delete libPart.location;
            if (GS::UnID (libPart.ownUnID).GetMainGuid () == parentGuid) {
                CHTruncate (libPart.ownUnID, newLibPart.parentUnID, sizeof (newLibPart.parentUnID));
                if (newLibPart.typeID == API_ZombieLibID) {
                    newLibPart.typeID = libPart.typeID;
                }
                return {};
            }
        }
        return CreateErrorResponse (APIERR_BADPARS, "No loaded library part has the GUID given as parentId.");
    }

    if (!parentName.IsEmpty ()) {
        API_LibPart libPart = {};
        CopyToUCharArray (parentName, libPart.docu_UName);
        const GSErrCode err = ACAPI_LibraryPart_Search (&libPart, false, false);
        delete libPart.location;
        if (err != NoError) {
            return CreateErrorResponse (err, GS::UniString::Printf ("Not found library part with name '%T' for parentName.", parentName.ToPrintf ()));
        }
        CHTruncate (libPart.ownUnID, newLibPart.parentUnID, sizeof (newLibPart.parentUnID));
        if (newLibPart.typeID == API_ZombieLibID) {
            newLibPart.typeID = libPart.typeID;
        }
        return {};
    }

    return CreateErrorResponse (APIERR_BADPARS, "Either parentId or parentName is required.");
}

// Builds the parameter list of the new library part from its "parameters" field; the
// handle is the caller's to dispose with ACAPI_DisposeAddParHdl. Without parameters
// the list is empty: a zero-size handle, or no handle at all when the allocation
// returns none for zero bytes (A and B are not in this list, they are written apart).
static GS::Optional<GS::ObjectState> BuildParameterDefinitions (const GS::ObjectState& libraryPartData, API_AddParType**& addPars)
{
    GS::Array<GS::ObjectState> definitions;
    libraryPartData.Get ("parameters", definitions);

    addPars = reinterpret_cast<API_AddParType**> (BMAllocateHandle (definitions.GetSize () * sizeof (API_AddParType), ALLOCATE_CLEAR, 0));
    if (addPars == nullptr) {
        if (definitions.IsEmpty ()) {
            return {};
        }
        return CreateErrorResponse (APIERR_MEMFULL, "Failed to allocate the parameter list.");
    }

    for (UIndex ii = 0; ii < definitions.GetSize (); ++ii) {
        const GS::ObjectState& definition = definitions[ii];
        API_AddParType& addPar = (*addPars)[ii];

        GS::UniString parameterName;
        GS::UniString parameterType;
        if (!definition.Get ("name", parameterName) || parameterName.IsEmpty () || !definition.Get ("type", parameterType)) {
            return CreateErrorResponse (APIERR_BADPARS, "Every parameter needs a name and a type.");
        }
        if (!GDLParameterTypeFromString (parameterType, addPar.typeID)) {
            return CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("Unknown parameter type '%T'.", parameterType.ToPrintf ()));
        }

        addPar.typeMod = API_ParSimple;
        addPar.index = static_cast<short> (ii + 1);
        CHTruncate (parameterName.ToCStr ().Get (), addPar.name, API_NameLen);

        GS::UniString description;
        if (definition.Get ("description", description)) {
            CopyToUCharArray (description, addPar.uDescname);
        }

        bool isHidden = false;
        if (definition.Get ("isHidden", isHidden) && isHidden) {
            addPar.flags |= API_ParFlg_Hidden;
        }

        GS::UniString errorMessage;
        if (!SetParameterDefinitionValue (addPar, definition, errorMessage)) {
            return CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("Parameter '%T': %T", parameterName.ToPrintf (), errorMessage.ToPrintf ()));
        }
    }

    return {};
}

// The script sections of a library part: the input field of each and its section type.
// The master script is the 1D script section, the properties script the PR script section.
using LibPartSectionType = decltype (API_LibPartSection::sectType);

struct ScriptSectionField {
    const char*        fieldName;
    LibPartSectionType sectType;
};

static const ScriptSectionField scriptSectionFields[] = {
    { "masterScript",     API_Sect1DScript },
    { "script2D",         API_Sect2DScript },
    { "script3D",         API_Sect3DScript },
    { "parameterScript",  API_SectVLScript },
    { "uiScript",         API_SectUIScript },
    { "propertiesScript", API_SectPRScript }
};

struct ScriptSection {
    LibPartSectionType sectType;
    GS::UniString      script;
};

// Creates one library part in the embedded library; on success fills result with its
// identifiers, otherwise returns the error.
static GS::Optional<GS::ObjectState> CreateLibraryPart (const GS::ObjectState& libraryPartData,
                                                        const IO::Location&    embeddedLibraryFolder,
                                                        bool                   overwriteExisting,
                                                        GS::ObjectState&       result)
{
    GS::UniString name;
    if (!libraryPartData.Get ("name", name) || name.IsEmpty ()) {
        return CreateErrorResponse (APIERR_BADPARS, "Missing name parameter.");
    }

    API_LibPart libPart = {};
    libPart.typeID = API_ZombieLibID;
    libPart.isTemplate = false;
    libPart.isPlaceable = true;
#ifndef ServerMainVers_2800
    // Archicad 28 removed the platform signature from API_LibPart.
    libPart.platformSign = GS::Act_Platform_Sign;
#endif
    CopyToUCharArray (name, libPart.docu_UName);

    GS::UniString typeStr;
    if (libraryPartData.Get ("type", typeStr) && !LibPartTypeIdFromString (typeStr, libPart.typeID)) {
        return CreateErrorResponse (APIERR_BADPARS, "Unknown library part type.");
    }

    GS::Optional<GS::ObjectState> error = ResolveParentLibraryPart (libraryPartData, libPart);
    if (error.HasValue ()) {
        return error;
    }
    if (libPart.typeID == API_ZombieLibID) {
        libPart.typeID = APILib_ObjectID;
    }

    // Everything the input can get wrong is checked before ACAPI_LibraryPart_Create:
    // from that call on the library part is under creation, and only
    // ACAPI_LibraryPart_Save ends that.
    API_AddParType** addPars = nullptr;
    const GS::OnExit disposeAddPars ([&] () {
        if (addPars != nullptr) {
            ACAPI_DisposeAddParHdl (&addPars);
        }
    });
    error = BuildParameterDefinitions (libraryPartData, addPars);
    if (error.HasValue ()) {
        return error;
    }

    double a = 1.0;
    double b = 1.0;
    GetNumberField (libraryPartData, "a", a);
    GetNumberField (libraryPartData, "b", b);

    GS::Array<ScriptSection> scripts;
    for (const ScriptSectionField& scriptSectionField : scriptSectionFields) {
        GS::UniString script;
        if (libraryPartData.Get (scriptSectionField.fieldName, script) && !script.IsEmpty ()) {
            scripts.Push (ScriptSection { scriptSectionField.sectType, script });
        }
    }

    GS::UniString relativePath;
    if (!libraryPartData.Get ("path", relativePath) || relativePath.IsEmpty ()) {
        relativePath = name + ".gsm";
    }
    IO::Location outputFileLoc = embeddedLibraryFolder;
    outputFileLoc.AppendToLocal (IO::RelativeLocation (relativePath));

    if (IO::File (outputFileLoc, IO::File::OnNotFound::Fail).GetStatus () == NoError) {
        if (!overwriteExisting) {
            return CreateErrorResponse (APIERR_NAMEALREADYUSED, "A library part file already exists on this path in the embedded library; set overwriteExisting to replace it.");
        }
#ifdef ServerMainVers_2700
        const bool keepGSMFile = false;
        const bool silentMode = true;
        const GSErrCode deleteErr = ACAPI_LibraryManagement_DeleteEmbeddedLibItem (&outputFileLoc, keepGSMFile, silentMode);
        if (deleteErr != NoError) {
            return CreateErrorResponse (deleteErr, "Failed to delete the existing embedded library item before overwriting.");
        }
#else
        return CreateErrorResponse (APIERR_NOTSUPPORTED, "The overwriteExisting flag requires Archicad 27 or newer.");
#endif
    }

    IO::Location outputFolder = outputFileLoc;
    if (outputFolder.DeleteLastLocalName () != NoError || IO::fileSystem.CreateFolderTree (outputFolder) != NoError) {
        return CreateErrorResponse (APIERR_BADPARS, "The given path is not a valid relative path.");
    }

    // The location is on the heap, like the one ACAPI_LibraryPart_Get returns: Save
    // may replace it with its own allocation (freeing this one or not), and after
    // Save the location it holds is deleted whichever it is.
    libPart.location = new IO::Location (outputFileLoc);
    const GS::OnExit deleteLocation ([&] () {
        delete libPart.location;
        libPart.location = nullptr;
    });

    GSErrCode err = ACAPI_LibraryPart_Create (&libPart);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to create the library part.");
    }

    GS::UniString failedStep;
    for (const ScriptSection& script : scripts) {
        API_LibPartSection section = {};
        section.sectType = script.sectType;
        GS::UniString scriptText = script.script;
        err = ACAPI_LibraryPart_AddSection (&section, nullptr, &scriptText);
        if (err != NoError) {
            failedStep = "Failed to write a script section of the library part.";
            break;
        }
    }

    if (err == NoError) {
        GSHandle parameterSection = nullptr;
        err = ACAPI_LibraryPart_GetSect_ParamDef (&libPart, addPars, &a, &b, nullptr, &parameterSection);
        if (err == NoError) {
            API_LibPartSection section = {};
            section.sectType = API_SectParamDef;
            err = ACAPI_LibraryPart_AddSection (&section, parameterSection, nullptr);
        }
        BMKillHandle (&parameterSection);
        if (err != NoError) {
            failedStep = "Failed to write the parameter section of the library part.";
        }
    }

    // Save ends the creation even after a failed section, so a later call does not
    // find a library part still under creation. The part is then in the library
    // without the failed section; the error item says so.
    const GSErrCode saveErr = ACAPI_LibraryPart_Save (&libPart);

    if (err != NoError) {
        if (saveErr == NoError) {
            failedStep += GS::UniString::Printf (" The library part was still saved to '%T' in the embedded library without that section: create it again with overwriteExisting to replace it, or delete it with DeleteEmbeddedLibraryItems.", relativePath.ToPrintf ());
        } else {
            failedStep += " Saving the incomplete library part failed too.";
        }
        return CreateErrorResponse (err, failedStep);
    }
    if (saveErr != NoError) {
        return CreateErrorResponse (saveErr, "Failed to save the library part.");
    }

    // The registered library part, for its unique ID: by the index Save reports, or by
    // name when it reports none.
    API_LibPart savedLibPart = {};
    savedLibPart.index = libPart.index;
    if (libPart.index > 0) {
        err = ACAPI_LibraryPart_Get (&savedLibPart);
    } else {
        CopyToUCharArray (name, savedLibPart.docu_UName);
        err = ACAPI_LibraryPart_Search (&savedLibPart, false, false);
    }
    if (err != NoError) {
        return CreateErrorResponse (err, "The library part was saved but could not be found among the loaded library parts.");
    }
    delete savedLibPart.location;

    result.Add ("name", GS::UniString (savedLibPart.docu_UName));
    result.Add ("unId", GS::UniString (savedLibPart.ownUnID));
    result.Add ("guid", GS::UnID (savedLibPart.ownUnID).GetMainGuid ().ToUniString ());
    result.Add ("index", static_cast<Int32> (savedLibPart.index));
    result.Add ("path", relativePath);
    return {};
}

CreateLibraryPartsCommand::CreateLibraryPartsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String CreateLibraryPartsCommand::GetName () const
{
    return "CreateLibraryParts";
}

GS::Optional<GS::UniString> CreateLibraryPartsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraryParts": {
                "type": "array",
                "description": "The library parts to create in the embedded library.",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "The name of the new library part, as shown in the library and used by CreateObjects as libraryPartName."
                        },
                        "parentId": {
                            "type": "string",
                            "description": "The parent subtype of the new library part: its unique ID ('{main}-{revision}') or its main GUID, as GetAvailableLibraryParts reports it. Either parentId or parentName is required."
                        },
                        "parentName": {
                            "type": "string",
                            "description": "The document name of the parent subtype, e.g. 'General Object' or 'General Label' from the Archicad Library; used when parentId is not given."
                        },
                        "type": {
                            "$ref": "#/LibraryPartType",
                            "description": "Optional. The type of the new library part. By default the type of the parent found by parentName or by a parentId GUID, and Object otherwise."
                        },
                        "path": {
                            "type": "string",
                            "description": "Optional. The relative path of the .gsm file inside the embedded library, the same kind of path AddFilesToEmbeddedLibrary takes as outputPath. By default '<name>.gsm'."
                        },
                        "a": {
                            "type": "number",
                            "description": "Optional. The default value of the A (X size) parameter. 1.0 by default."
                        },
                        "b": {
                            "type": "number",
                            "description": "Optional. The default value of the B (Y size) parameter. 1.0 by default."
                        },
                        "masterScript": {
                            "type": "string",
                            "description": "Optional. The GDL master script."
                        },
                        "script2D": {
                            "type": "string",
                            "description": "Optional. The GDL 2D script."
                        },
                        "script3D": {
                            "type": "string",
                            "description": "Optional. The GDL 3D script."
                        },
                        "parameterScript": {
                            "type": "string",
                            "description": "Optional. The GDL parameter script (value lists, locks, hiding)."
                        },
                        "uiScript": {
                            "type": "string",
                            "description": "Optional. The GDL user interface script."
                        },
                        "propertiesScript": {
                            "type": "string",
                            "description": "Optional. The GDL properties (component listing) script."
                        },
                        "parameters": {
                            "type": "array",
                            "description": "Optional. The parameters of the library part besides A and B, in the order they appear in the settings dialog. Only simple (non-array) parameters can be defined.",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "name": {
                                        "type": "string",
                                        "description": "The GDL variable name of the parameter (at most 31 characters)."
                                    },
                                    "type": {
                                        "type": "string",
                                        "description": "The type of the parameter, named as GetGDLParametersOfElements reports it.",
                                        "enum": [
                                            "Integer",
                                            "Length",
                                            "Angle",
                                            "RealNumber",
                                            "LightSwitch",
                                            "RGBColor",
                                            "Intensity",
                                            "LineType",
                                            "Material",
                                            "FillPattern",
                                            "PenColor",
                                            "String",
                                            "Boolean",
                                            "Separator",
                                            "Title",
                                            "BuildingMaterial",
                                            "Profile"
                                        ]
                                    },
                                    "value": {
                                        "description": "Optional. The default value: a number for the numeric and attribute index types, a string for String and Title, true or false for Boolean, 'On' or 'Off' for LightSwitch.",
                                        "oneOf": [
                                            { "type": "number" },
                                            { "type": "string" },
                                            { "type": "boolean" }
                                        ]
                                    },
                                    "description": {
                                        "type": "string",
                                        "description": "Optional. The display name of the parameter in the settings dialog."
                                    },
                                    "isHidden": {
                                        "type": "boolean",
                                        "description": "Optional. When true, the parameter is hidden from the settings dialog. False by default."
                                    }
                                },
                                "additionalProperties": false,
                                "required": [
                                    "name",
                                    "type"
                                ]
                            }
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name"
                    ]
                }
            },
            "overwriteExisting": {
                "type": "boolean",
                "description": "Optional. When true, an embedded library item already existing on the path of a new library part is deleted first, so the new one replaces the loaded library part. By default false: creating on an existing path fails. Requires Archicad 27 or newer."
            }
        },
        "additionalProperties": false,
        "required": [
            "libraryParts"
        ]
    })";
}

GS::Optional<GS::UniString> CreateLibraryPartsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraryParts": {
                "type": "array",
                "description": "The created library parts, in the order of the input, or the error of each one that could not be created.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "name": {
                                    "type": "string",
                                    "description": "The name of the created library part."
                                },
                                "unId": {
                                    "type": "string",
                                    "description": "The unique ID of the created library part ('{main}-{revision}'), usable as parentId of further library parts."
                                },
                                "guid": {
                                    "type": "string",
                                    "description": "The main GUID of the created library part, as GetAvailableLibraryParts reports it."
                                },
                                "index": {
                                    "type": "integer",
                                    "description": "The index of the created library part in the loaded libraries."
                                },
                                "path": {
                                    "type": "string",
                                    "description": "The relative path of the .gsm file inside the embedded library; DeleteEmbeddedLibraryItems takes it as path."
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "name",
                                "unId",
                                "guid",
                                "index",
                                "path"
                            ]
                        },
                        {
                            "$ref": "#/ErrorItem"
                        }
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "libraryParts"
        ]
    })";
}

GS::ObjectState CreateLibraryPartsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    auto folderId = API_SpecFolderID::API_EmbeddedProjectLibraryFolderID;

    IO::Location embeddedLibraryFolder;

    if (ACAPI_ProjectSettings_GetSpecFolder (&folderId, &embeddedLibraryFolder) != NoError || IO::Folder (embeddedLibraryFolder).GetStatus () != NoError) {
        return CreateErrorResponse (APIERR_GENERAL, "Failed to get embedded library folder.");
    }

    bool overwriteExisting = false;
    parameters.Get ("overwriteExisting", overwriteExisting);

    GS::Array<GS::ObjectState> libraryParts;
    parameters.Get ("libraryParts", libraryParts);

    GS::ObjectState response;
    const auto& libraryPartAdder = response.AddList<GS::ObjectState> ("libraryParts");

    for (const GS::ObjectState& libraryPartData : libraryParts) {
        GS::ObjectState result;
        const GS::Optional<GS::ObjectState> error = CreateLibraryPart (libraryPartData, embeddedLibraryFolder, overwriteExisting, result);
        libraryPartAdder (error.HasValue () ? *error : result);
    }

    return response;
}

static GSErrCode GetEmbeddedLibraryFolder (IO::Location& embeddedLibraryFolder)
{
    auto folderId = API_SpecFolderID::API_EmbeddedProjectLibraryFolderID;
    if (ACAPI_ProjectSettings_GetSpecFolder (&folderId, &embeddedLibraryFolder) != NoError || IO::Folder (embeddedLibraryFolder).GetStatus () != NoError) {
        return APIERR_GENERAL;
    }
    return NoError;
}

// Splits a folder path relative to the embedded library into its folder names.
// Absolute paths and '.'/'..' parts are refused, so the result stays inside the
// embedded library. An empty path means the embedded library itself.
static bool SplitEmbeddedLibraryFolderPath (const GS::UniString& folderPath, GS::Array<GS::UniString>& folderNames)
{
    GS::UniString normalized = folderPath;
    normalized.ReplaceAll ("\\", "/");
    if (normalized.Contains (":")) {
        return false;
    }

    folderNames.Clear ();
    normalized.Split ("/", GS::UniString::SkipEmptyParts, &folderNames);
    for (const GS::UniString& folderName : folderNames) {
        if (folderName == "." || folderName == "..") {
            return false;
        }
    }
    return true;
}

// Creates the missing folders of the given path one level at a time with
// ACAPI_LibraryManagement_CreateFolderInLibrary, so Archicad knows about each.
// Returns the failed execution result, or no value when the folder is ready.
static GS::Optional<GS::ObjectState> CreateEmbeddedLibraryFolderTree (const GS::UniString& folderPath, IO::Location& folderLocation)
{
    GS::Array<GS::UniString> folderNames;
    if (!SplitEmbeddedLibraryFolderPath (folderPath, folderNames)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "The folder path must be relative to the embedded library and must not contain '.' or '..' parts.");
    }

    if (GetEmbeddedLibraryFolder (folderLocation) != NoError) {
        return CreateFailedExecutionResult (APIERR_GENERAL, "Failed to get embedded library folder.");
    }

    for (const GS::UniString& folderName : folderNames) {
        folderLocation.AppendToLocal (IO::Name (folderName));
        if (IO::Folder (folderLocation).GetStatus () == NoError) {
            continue;
        }

        const GSErrCode err = ACAPI_LibraryManagement_CreateFolderInLibrary (&folderLocation);
        if (err != NoError) {
            return CreateFailedExecutionResult (err, "Failed to create the folder '" + folderName + "' in the embedded library.");
        }
    }

    return GS::NoValue;
}

CreateFolderInEmbeddedLibraryCommand::CreateFolderInEmbeddedLibraryCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String CreateFolderInEmbeddedLibraryCommand::GetName () const
{
    return "CreateFolderInEmbeddedLibrary";
}

GS::Optional<GS::UniString> CreateFolderInEmbeddedLibraryCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "folderPath": {
                "type": "string",
                "description": "The path of the folder relative to the embedded library, for example 'Office/Doors'. Missing parent folders are created too; folders that already exist are kept.",
                "minLength": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "folderPath"
        ]
    })";
}

GS::Optional<GS::UniString> CreateFolderInEmbeddedLibraryCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState CreateFolderInEmbeddedLibraryCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString folderPath;
    if (!parameters.Get ("folderPath", folderPath) || folderPath.IsEmpty ()) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Missing folderPath parameter.");
    }

    IO::Location folderLocation;
    const GS::Optional<GS::ObjectState> folderError = CreateEmbeddedLibraryFolderTree (folderPath, folderLocation);
    if (folderError.HasValue ()) {
        return *folderError;
    }

    return CreateSuccessfulExecutionResult ();
}

CopyFilesIntoEmbeddedLibraryCommand::CopyFilesIntoEmbeddedLibraryCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String CopyFilesIntoEmbeddedLibraryCommand::GetName () const
{
    return "CopyFilesIntoEmbeddedLibrary";
}

GS::Optional<GS::UniString> CopyFilesIntoEmbeddedLibraryCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "files": {
                "type": "array",
                "description": "The files to copy. Each keeps its file name inside the target folder.",
                "items": {
                    "type": "object",
                    "properties": {
                        "path": {
                            "type": "string",
                            "description": "The absolute path of the file to copy."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "path"
                    ]
                },
                "minItems": 1
            },
            "targetFolder": {
                "type": "string",
                "description": "Optional. The folder relative to the embedded library to copy the files into, for example 'Office/Doors'. Missing folders are created. By default the root of the embedded library."
            },
            "overwriteIfExists": {
                "type": "boolean",
                "description": "Optional. When true, an item with the same name in the target folder is replaced. By default false."
            }
        },
        "additionalProperties": false,
        "required": [
            "files"
        ]
    })";
}

GS::Optional<GS::UniString> CopyFilesIntoEmbeddedLibraryCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState CopyFilesIntoEmbeddedLibraryCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> files;
    parameters.Get ("files", files);

    GS::Array<IO::Location> filesToCopy;
    for (const GS::ObjectState& file : files) {
        GS::UniString path;
        if (!file.Get ("path", path) || path.IsEmpty ()) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "Missing path parameter.");
        }

        IO::File inputFile (IO::Location (path), IO::File::OnNotFound::Fail);
        if (inputFile.GetStatus () != NoError) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "Failed to read the file on the path '" + path + "'.");
        }
        filesToCopy.Push (inputFile.GetLocation ());
    }

    if (filesToCopy.IsEmpty ()) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "No files given.");
    }

    GS::UniString targetFolder;
    parameters.Get ("targetFolder", targetFolder);

    bool overwriteIfExists = false;
    parameters.Get ("overwriteIfExists", overwriteIfExists);

    IO::Location targetFolderLocation;
    const GS::Optional<GS::ObjectState> folderError = CreateEmbeddedLibraryFolderTree (targetFolder, targetFolderLocation);
    if (folderError.HasValue ()) {
        return *folderError;
    }

    const GSErrCode err = ACAPI_LibraryManagement_CopyFilesIntoLibrary (&targetFolderLocation, &filesToCopy, &overwriteIfExists);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to copy the files into the embedded library.");
    }

    return CreateSuccessfulExecutionResult ();
}

GS::Optional<GS::UniString> GetLibrariesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraries": {
                "type": "array",
                "description": "A list of project libraries.",
                "items": {
                    "$ref": "#/Library"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "libraries"
        ]
    })";
}

// ---------------------------------------------------------------------------
// SetLibraries / AddLibraries
// ---------------------------------------------------------------------------

static const char* LibraryPathsSchema = R"({
        "type": "object",
        "properties": {
            "libraries": {
                "type": "array",
                "description": "Local library folders or container files, by absolute path.",
                "items": {
                    "$ref": "#/LibraryLocation"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "libraries"
        ]
    })";

static GS::Array<API_LibraryInfo> LocalLibrariesFromParameters (const GS::ObjectState& parameters)
{
    GS::Array<GS::ObjectState> items;
    parameters.Get ("libraries", items);
    GS::Array<API_LibraryInfo> out;
    for (const GS::ObjectState& item : items) {
        GS::UniString path;
        if (!item.Get ("path", path) || path.IsEmpty ()) {
            continue;
        }
        API_LibraryInfo info = {};
        info.location = IO::Location (path);
        info.libraryType = API_LibraryTypeID::API_LocalLibrary;
        IO::Name name;
        if (info.location.GetLastLocalName (&name) == NoError) {
            info.name = name.ToString ();
        }
        out.Push (info);
    }
    return out;
}

static bool SameLocation (const IO::Location& a, const IO::Location& b)
{
    // IO::Location's own equality (InputOutput/Location.hpp, present in every
    // kit): it knows the platform's rules for separators and case, where a
    // display-text comparison guessed.
    return a == b;
}

static GS::ObjectState ApplyLibraries (const GS::Array<API_LibraryInfo>& libs)
{
    const GSErrCode err = ACAPI_LibraryManagement_SetLibraries (&libs);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to set the libraries.");
    }
    return CreateSuccessfulExecutionResult ();
}

SetLibrariesCommand::SetLibrariesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetLibrariesCommand::GetName () const
{
    return "SetLibraries";
}

GS::Optional<GS::UniString> SetLibrariesCommand::GetInputParametersSchema () const
{
    return GS::UniString (LibraryPathsSchema);
}

GS::Optional<GS::UniString> SetLibrariesCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState SetLibrariesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    // The local libraries become exactly the given ones; built-in, embedded, server
    // and web libraries stay as they are.
    GS::Array<API_LibraryInfo> current;
    if (ACAPI_LibraryManagement_GetLibraries (&current) != NoError) {
        return CreateFailedExecutionResult (APIERR_COMMANDFAILED, "Failed to read the libraries.");
    }
    GS::Array<API_LibraryInfo> libs;
    for (const API_LibraryInfo& lib : current) {
        if (lib.libraryType != API_LibraryTypeID::API_LocalLibrary) {
            libs.Push (lib);
        }
    }
    for (const API_LibraryInfo& lib : LocalLibrariesFromParameters (parameters)) {
        libs.Push (lib);
    }
    return ApplyLibraries (libs);
}

AddLibrariesCommand::AddLibrariesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String AddLibrariesCommand::GetName () const
{
    return "AddLibraries";
}

GS::Optional<GS::UniString> AddLibrariesCommand::GetInputParametersSchema () const
{
    return GS::UniString (LibraryPathsSchema);
}

GS::Optional<GS::UniString> AddLibrariesCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState AddLibrariesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<API_LibraryInfo> libs;
    if (ACAPI_LibraryManagement_GetLibraries (&libs) != NoError) {
        return CreateFailedExecutionResult (APIERR_COMMANDFAILED, "Failed to read the libraries.");
    }
    USize added = 0;
    for (const API_LibraryInfo& lib : LocalLibrariesFromParameters (parameters)) {
        bool present = false;
        for (const API_LibraryInfo& existing : libs) {
            if (SameLocation (existing.location, lib.location)) {
                present = true;
                break;
            }
        }
        if (!present) {
            libs.Push (lib);
            ++added;
        }
    }
    if (added == 0) {
        // Every folder is already loaded: SetLibraries would only force a reload.
        return CreateSuccessfulExecutionResult ();
    }
    return ApplyLibraries (libs);
}

GetLibrariesCommand::GetLibrariesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetLibrariesCommand::GetName () const
{
    return "GetLibraries";
}

#ifdef ServerMainVers_2800
// The Library Manager's library at the location of an API_LibraryInfo. Several
// built-in libraries share one location, so the name decides between them; a
// location that stays ambiguous gives nothing.
static const ACAPI::Library::Library* FindManagerLibrary (const std::vector<ACAPI::Library::Library>& managerLibraries, const API_LibraryInfo& lib)
{
    const ACAPI::Library::Library* found = nullptr;
    bool ambiguous = false;
    for (const ACAPI::Library::Library& library : managerLibraries) {
        auto location = library.GetLocation ();
        if (location.IsErr () || !SameLocation (location.Unwrap (), lib.location)) {
            continue;
        }
        auto name = library.GetName ();
        if (name.IsOk () && name.Unwrap () == lib.name) {
            return &library;
        }
        ambiguous = ambiguous || found != nullptr;
        found = &library;
    }
    return ambiguous ? nullptr : found;
}

// The package information of a library, or nothing when it is not a package.
// A value the API cannot read is left out.
static GS::Optional<GS::ObjectState> GetLibraryPackage (const ACAPI::Library::Library& library)
{
    auto isPackage = library.IsPackage ();
    if (isPackage.IsErr () || !isPackage.Unwrap ()) {
        return {};
    }

    GS::ObjectState package;
    auto packageId = library.GetPackageID ();
    if (packageId.IsOk ()) {
        package.Add ("packageId", packageId.Unwrap ().ToUniString ());
    }
    auto mainVersion = library.GetPackageMainVersion ();
    if (mainVersion.IsOk ()) {
        package.Add ("mainVersion", mainVersion.Unwrap ());
    }
    auto subVersion = library.GetPackageSubVersion ();
    if (subVersion.IsOk ()) {
        package.Add ("subVersion", subVersion.Unwrap ());
    }
    auto minArchicadVersion = library.GetPackageMinACVersion ();
    if (minArchicadVersion.IsOk ()) {
        package.Add ("minArchicadVersion", minArchicadVersion.Unwrap ());
    }
    auto minArchicadBuildNumber = library.GetPackageMinACBuildNumber ();
    if (minArchicadBuildNumber.IsOk ()) {
        package.Add ("minArchicadBuildNumber", minArchicadBuildNumber.Unwrap ());
    }
    auto dependencies = library.GetPackageDependencies ();
    if (dependencies.IsOk ()) {
        const auto& dependencyAdder = package.AddList<GS::ObjectState> ("dependencies");
        for (const auto& dependency : dependencies.Unwrap ()) {
            GS::ObjectState dependencyData;
            dependencyData.Add ("name", std::get<0> (dependency));
            dependencyData.Add ("packageId", std::get<1> (dependency).ToUniString ());
            dependencyData.Add ("version", std::get<2> (dependency));
            dependencyAdder (dependencyData);
        }
    }
    return package;
}
#endif

GS::ObjectState GetLibrariesCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<API_LibraryInfo> libs;

    GSErrCode err = ACAPI_LibraryManagement_GetLibraries (&libs);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to retrive libraries.");
    }

#ifdef ServerMainVers_2800
    // Package information is optional: without the Library Manager the list is
    // returned as before.
    std::vector<ACAPI::Library::Library> managerLibraries;
    auto libraryManager = ACAPI::Library::GetLibraryManager ();
    if (libraryManager.IsOk ()) {
        managerLibraries = libraryManager->FindLibraries (ACAPI::Library::SelectAllLibraries);
    }
#endif

    GS::ObjectState response;
    const auto& listAdder = response.AddList<GS::ObjectState> ("libraries");

    for (const API_LibraryInfo& lib : libs) {
        GS::ObjectState libraryData;
        GS::UniString   type;
        GS::UniString   twServerUrl;
        GS::UniString   urlWebLibrary;
        switch (lib.libraryType) {
            case API_LibraryTypeID::API_Undefined:
                type = "Undefined";
                break;
            case API_LibraryTypeID::API_LocalLibrary:
                type = "LocalLibrary";
                break;
            case API_LibraryTypeID::API_UrlLibrary:
                type = "UrlLibrary";
                urlWebLibrary = lib.twServerUrl;
                break;
            case API_LibraryTypeID::API_BuiltInLibrary:
                type = "BuiltInLibrary";
                break;
            case API_LibraryTypeID::API_EmbeddedLibrary:
                type = "EmbeddedLibrary";
                break;
            case API_LibraryTypeID::API_OtherObject:
                type = "OtherObject";
                break;
            case API_LibraryTypeID::API_UrlOtherObject:
                type = "UrlOtherObject";
                urlWebLibrary = lib.twServerUrl;
                break;
            case API_LibraryTypeID::API_ServerLibrary:
                type = "ServerLibrary";
                twServerUrl = lib.twServerUrl;
                break;
        }

        libraryData.Add ("name", lib.name);
        libraryData.Add ("path", lib.location.ToDisplayText ());
        libraryData.Add ("type", type);
        libraryData.Add ("available", lib.available);
        libraryData.Add ("readOnly", lib.readOnly);
        libraryData.Add ("twServerUrl", twServerUrl);
        libraryData.Add ("urlWebLibrary", urlWebLibrary);
#ifdef ServerMainVers_2800
        const ACAPI::Library::Library* managerLibrary = FindManagerLibrary (managerLibraries, lib);
        if (managerLibrary != nullptr) {
            const GS::Optional<GS::ObjectState> package = GetLibraryPackage (*managerLibrary);
            if (package.HasValue ()) {
                libraryData.Add ("package", *package);
            }
        }
#endif
        listAdder (libraryData);
    }

    return response;
}

ReloadLibrariesCommand::ReloadLibrariesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ReloadLibrariesCommand::GetName () const
{
    return "ReloadLibraries";
}

GS::Optional<GS::UniString> ReloadLibrariesCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState ReloadLibrariesCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GSErrCode err = ACAPI_ProjectOperation_ReloadLibraries ();
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to reload libraries.");
    }

    return CreateSuccessfulExecutionResult ();
}

GetAvailableLibraryPartsCommand::GetAvailableLibraryPartsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAvailableLibraryPartsCommand::GetName () const
{
    return "GetAvailableLibraryParts";
}

GS::Optional<GS::UniString> GetAvailableLibraryPartsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "filterByTypeId": {
                "$ref": "#/LibraryPartType",
                "description": "Optional. Filter by libpart type (matches the value returned by LibPartTypeIdToString)."
            }
        },
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> GetAvailableLibraryPartsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraryParts": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "guid": { "type": "string" },
                        "index": { "type": "integer" },
                        "documentName": { "type": "string" },
                        "fileName": { "type": "string" },
                        "typeId": { "$ref": "#/LibraryPartType" },
                        "type": {
                            "type": "string",
                            "enum": ["Object", "Image", "Text", "BinaryListCrit", "Other"],
                            "description": "The kind of the library part file: a GDL object, an image, a text file, a binary list criteria file or other. Available from Archicad 28; missing when it cannot be read."
                        }
                    }
                }
            },
            "skippedCount": {
                "type": "integer",
                "description": "Library parts that ACAPI_LibraryPart_Get failed to read. Non-zero means the inventory is partial."
            },
            "skippedSample": {
                "type": "array",
                "description": "First five failed indices with their ACAPI error code, for diagnostic.",
                "items": {
                    "type": "object",
                    "properties": {
                        "index": { "type": "integer" },
                        "code": { "type": "integer" }
                    }
                }
            }
        },
        "additionalProperties": false,
        "required": ["libraryParts", "skippedCount"]
    })";
}

static GS::UniString LibPartTypeIdToString (Int32 typeID)
{
    switch (typeID) {
        case APILib_SpecID:          return "Spec";
        case APILib_WindowID:        return "Window";
        case APILib_DoorID:          return "Door";
        case APILib_ObjectID:        return "Object";
        case APILib_LampID:          return "Lamp";
        case APILib_RoomID:          return "Room";
        case APILib_PropertyID:      return "Property";
        case APILib_PlanSignID:      return "PlanSign";
        case APILib_LabelID:         return "Label";
        case APILib_MacroID:         return "Macro";
        case APILib_PictID:          return "Picture";
        case APILib_ListSchemeID:    return "ListScheme";
        case APILib_SkylightID:      return "Skylight";
        case APILib_OpeningSymbolID: return "OpeningSymbol";
        // API_ZombieLibID (ACAPI's uninitialized / General-Object
        // sentinel) and any future ACAPI subtype fall through here.
        // We return the schema-valid catch-all "Unknown" rather than a
        // dynamic "TypeN" value so callers can validate the response
        // against the LibraryPartType enum without surprises.
        default:                     return "Unknown";
    }
}

#ifdef ServerMainVers_2800
static GS::UniString LibPartTypeToString (ACAPI::Library::LibPartType type)
{
    switch (type) {
        case ACAPI::Library::LibPartType::Object:         return "Object";
        case ACAPI::Library::LibPartType::Image:          return "Image";
        case ACAPI::Library::LibPartType::Text:           return "Text";
        case ACAPI::Library::LibPartType::BinaryListCrit: return "BinaryListCrit";
        default:                                          return "Other";
    }
}
#endif

GS::ObjectState GetAvailableLibraryPartsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString filterTypeId;
    parameters.Get ("filterByTypeId", filterTypeId);
    // `LibPartTypeIdToString` emits "Picture" for APILib_PictID, but the
    // shared LibraryPartType enum keeps both "Pict" (legacy
    // AddFilesToEmbeddedLibrary input) and "Picture" (this output).
    // Normalise so callers can filter by either spelling.
    if (filterTypeId == "Pict") {
        filterTypeId = "Picture";
    }

    Int32 partCount = 0;
    GSErrCode err = ACAPI_LibraryPart_GetNum (&partCount);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to enumerate library parts.");
    }

    GS::ObjectState response;
    const auto& libpartAdder = response.AddList<GS::ObjectState> ("libraryParts");
    const auto& skippedAdder = response.AddList<GS::ObjectState> ("skippedSample");

    Int32 skippedCount = 0;
    constexpr Int32 maxSkippedSample = 5;

#ifdef ServerMainVers_2800
    auto libraryManager = ACAPI::Library::GetLibraryManager ();
#endif

    for (Int32 i = 1; i <= partCount; ++i) {
        API_LibPart libPart = {};
        libPart.index = i;
        err = ACAPI_LibraryPart_Get (&libPart);
        if (err != NoError) {
            // libPart.location is not allocated on failure.
            if (skippedCount < maxSkippedSample) {
                GS::ObjectState skipEntry;
                skipEntry.Add ("index", i);
                skipEntry.Add ("code", static_cast<Int32> (err));
                skippedAdder (skipEntry);
            }
            ++skippedCount;
            continue;
        }

        const GS::UniString typeId = LibPartTypeIdToString (libPart.typeID);
        const bool matchesFilter = filterTypeId.IsEmpty () || typeId == filterTypeId;

        if (matchesFilter) {
            GS::ObjectState entry;
            entry.Add ("guid", GS::UnID (libPart.ownUnID).GetMainGuid ().ToUniString ());
            entry.Add ("index", static_cast<Int32> (libPart.index));
            entry.Add ("documentName", GS::UniString (libPart.docu_UName));
            entry.Add ("fileName", GS::UniString (libPart.file_UName));
            entry.Add ("typeId", typeId);
#ifdef ServerMainVers_2800
            if (libraryManager.IsOk ()) {
                auto acLibPart = libraryManager->GetLibPartByName (GS::UniString (libPart.file_UName));
                if (acLibPart.IsOk ()) {
                    auto libPartType = acLibPart->GetType ();
                    if (libPartType.IsOk ()) {
                        entry.Add ("type", LibPartTypeToString (libPartType.Unwrap ()));
                    }
                }
            }
#endif
            libpartAdder (entry);
        }

        // ACAPI_LibraryPart_Get allocates libPart.location; free it on
        // every successful Get to avoid leaking ~one IO::Location per
        // libpart (thousands per call).
        delete libPart.location;
        libPart.location = nullptr;
    }

    response.Add ("skippedCount", skippedCount);

    return response;
}

IsUsableGDLStringCommand::IsUsableGDLStringCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String IsUsableGDLStringCommand::GetName () const
{
    return "IsUsableGDLString";
}

GS::Optional<GS::UniString> IsUsableGDLStringCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "strings": {
                "type": "array",
                "description": "The strings to check.",
                "items": {
                    "type": "string"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "strings"
        ]
    })";
}

GS::Optional<GS::UniString> IsUsableGDLStringCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "results": {
                "type": "array",
                "description": "One item for each given string, in the same order: true if Archicad accepts the string for use in GDL, false otherwise.",
                "items": {
                    "type": "boolean"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "results"
        ]
    })";
}

GS::ObjectState IsUsableGDLStringCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::UniString> strings;
    parameters.Get ("strings", strings);

    GS::ObjectState response;
    const auto& results = response.AddList<bool> ("results");

    // ACAPI_LibraryManagement_UsableGDLString returns NoError when the string is usable
    // in GDL and an error code when it is not.
    // It takes a GS::UniString from Archicad 30 and a C string before.
    for (const GS::UniString& str : strings) {
#ifdef ServerMainVers_3000
        results (ACAPI_LibraryManagement_UsableGDLString (str) == NoError);
#else
        results (ACAPI_LibraryManagement_UsableGDLString (str.ToCStr ().Get ()) == NoError);
#endif
    }

    return response;
}

SetLibraryPartScriptsCommand::SetLibraryPartScriptsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetLibraryPartScriptsCommand::GetName () const
{
    return "SetLibraryPartScripts";
}

GS::Optional<GS::UniString> SetLibraryPartScriptsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraryParts": {
                "type": "array",
                "description": "The library parts to modify, with the scripts to set and to delete in each.",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "The name of the loaded library part file with its extension, as returned in the fileName field of GetAvailableLibraryParts (e.g. 'My Door.gsm')."
                        },
                        "scripts": {
                            "type": "object",
                            "description": "Optional. The scripts to set. Only the scripts given here are changed, the others are kept. An empty string makes that script section empty, it does not delete it; use deleteScripts for that.",
                            "properties": {
                                "master": {
                                    "type": "string",
                                    "description": "The master script."
                                },
                                "parameter": {
                                    "type": "string",
                                    "description": "The parameter script."
                                },
                                "2d": {
                                    "type": "string",
                                    "description": "The 2D script."
                                },
                                "3d": {
                                    "type": "string",
                                    "description": "The 3D script."
                                },
                                "properties": {
                                    "type": "string",
                                    "description": "The properties script."
                                },
                                "interface": {
                                    "type": "string",
                                    "description": "The interface (user interface) script."
                                },
                                "forwardMigration": {
                                    "type": "string",
                                    "description": "The forward migration script."
                                },
                                "backwardMigration": {
                                    "type": "string",
                                    "description": "The backward migration script."
                                }
                            },
                            "additionalProperties": false,
                            "minProperties": 1
                        },
                        "deleteScripts": {
                            "type": "array",
                            "description": "Optional. The script sections to delete from the library part. A script cannot be both set in scripts and deleted here.",
                            "items": {
                                "type": "string",
                                "enum": [
                                    "master",
                                    "parameter",
                                    "2d",
                                    "3d",
                                    "properties",
                                    "interface",
                                    "forwardMigration",
                                    "backwardMigration"
                                ]
                            },
                            "minItems": 1,
                            "uniqueItems": true
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name"
                    ],
                    "anyOf": [
                        { "required": ["scripts"] },
                        { "required": ["deleteScripts"] }
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "libraryParts"
        ]
    })";
}

GS::Optional<GS::UniString> SetLibraryPartScriptsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    })";
}

GS::ObjectState SetLibraryPartScriptsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::Array<GS::ObjectState> libraryParts;
    if (!parameters.Get ("libraryParts", libraryParts)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'libraryParts' parameter.");
    }

    auto libraryManager = ACAPI::Library::GetLibraryManager ();
    if (libraryManager.IsErr ()) {
        return CreateErrorResponse (libraryManager.UnwrapErr ().kind, "Failed to get the Library Manager.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Set Library Part Scripts", [&]() -> GSErrCode {
        for (const GS::ObjectState& libraryPart : libraryParts) {
            GS::UniString name;
            const GS::ObjectState* scripts = libraryPart.Get ("scripts");
            GS::Array<GS::UniString> deleteScripts;
            libraryPart.Get ("deleteScripts", deleteScripts);
            if (!libraryPart.Get ("name", name) || name.IsEmpty () || (scripts == nullptr && deleteScripts.IsEmpty ())) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing 'name' parameter, or neither 'scripts' nor 'deleteScripts' is given."));
                continue;
            }

            bool hasScript = false;
            if (scripts != nullptr) {
                for (const LibraryPartScriptSection& section : libraryPartScriptSections) {
                    GS::UniString script;
                    hasScript = hasScript || scripts->Get (section.key, script);
                }
            }
            if (!hasScript && deleteScripts.IsEmpty ()) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "No script is given in 'scripts'."));
                continue;
            }

            GS::Array<const LibraryPartScriptSection*> sectionsToDelete;
            GS::UniString badDeleteScript;
            for (const GS::UniString& key : deleteScripts) {
                const LibraryPartScriptSection* section = FindLibraryPartScriptSection (key);
                GS::UniString script;
                if (section == nullptr || (scripts != nullptr && scripts->Get (section->key, script))) {
                    badDeleteScript = key;
                    break;
                }
                sectionsToDelete.Push (section);
            }
            if (!badDeleteScript.IsEmpty ()) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "The script '" + badDeleteScript + "' in 'deleteScripts' is unknown or is also given in 'scripts'. Nothing was changed."));
                continue;
            }

            GS::UniString failedWhat;
            GS::UniString failedText;
            executionResults (ModifyLibraryPart (libraryManager, name, [&] (GSMObject::Modifier& modifier) -> GSErrCode {
                for (const LibraryPartScriptSection* section : sectionsToDelete) {
                    ACAPI::Result<void> deleteResult = section->remove (modifier);
                    if (deleteResult.IsErr ()) {
                        failedWhat = GS::UniString ("deleting the '") + section->key + "' script";
                        failedText = deleteResult.UnwrapErr ().text.c_str ();
                        return deleteResult.UnwrapErr ().kind;
                    }
                }
                if (scripts == nullptr) {
                    return NoError;
                }
                for (const LibraryPartScriptSection& section : libraryPartScriptSections) {
                    GS::UniString script;
                    if (!scripts->Get (section.key, script)) {
                        continue;
                    }
                    ACAPI::Result<void> setResult = section.set (modifier, script);
                    if (setResult.IsErr ()) {
                        failedWhat = GS::UniString ("setting the '") + section.key + "' script";
                        failedText = setResult.UnwrapErr ().text.c_str ();
                        return setResult.UnwrapErr ().kind;
                    }
                }
                return NoError;
            }, failedWhat, failedText));
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

SetLibraryPartMetadataCommand::SetLibraryPartMetadataCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetLibraryPartMetadataCommand::GetName () const
{
    return "SetLibraryPartMetadata";
}

GS::Optional<GS::UniString> SetLibraryPartMetadataCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraryParts": {
                "type": "array",
                "description": "The library parts to modify and the metadata to set in each.",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "The name of the loaded library part file with its extension, as returned in the fileName field of GetAvailableLibraryParts (e.g. 'My Door.gsm')."
                        },
                        "metadata": {
                            "type": "object",
                            "description": "The metadata to set. Only the fields given here are changed, the others are kept.",
                            "properties": {
                                "author": {
                                    "type": "string",
                                    "description": "The author of the library part."
                                },
                                "keywords": {
                                    "type": "string",
                                    "description": "The keywords of the library part."
                                },
                                "license": {
                                    "type": "string",
                                    "description": "The license of the library part."
                                },
                                "licenseVersion": {
                                    "type": "string",
                                    "description": "The version of the license."
                                },
                                "password": {
                                    "type": "string",
                                    "description": "The new password of the library part."
                                },
                                "placeable": {
                                    "type": "boolean",
                                    "description": "Whether the library part can be placed in the project."
                                },
                                "uiDefault": {
                                    "type": "boolean",
                                    "description": "The UI Default flag of the library part."
                                },
                                "isSubtype": {
                                    "type": "boolean",
                                    "description": "Whether the library part is a subtype template."
                                },
                                "hierarchicalPagesUsed": {
                                    "type": "boolean",
                                    "description": "Whether the library part uses hierarchical pages in its user interface."
                                }
                            },
                            "additionalProperties": false,
                            "minProperties": 1
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "metadata"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "libraryParts"
        ]
    })";
}

GS::Optional<GS::UniString> SetLibraryPartMetadataCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    })";
}

GS::ObjectState SetLibraryPartMetadataCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::Array<GS::ObjectState> libraryParts;
    if (!parameters.Get ("libraryParts", libraryParts)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'libraryParts' parameter.");
    }

    auto libraryManager = ACAPI::Library::GetLibraryManager ();
    if (libraryManager.IsErr ()) {
        return CreateErrorResponse (libraryManager.UnwrapErr ().kind, "Failed to get the Library Manager.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Set Library Part Metadata", [&]() -> GSErrCode {
        for (const GS::ObjectState& libraryPart : libraryParts) {
            GS::UniString name;
            const GS::ObjectState* metadata = libraryPart.Get ("metadata");
            if (!libraryPart.Get ("name", name) || name.IsEmpty () || metadata == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing 'name' or 'metadata' parameter."));
                continue;
            }

            bool hasField = false;
            for (const LibraryPartStringMetadata& field : libraryPartStringMetadata) {
                GS::UniString value;
                hasField = hasField || metadata->Get (field.key, value);
            }
            for (const LibraryPartBoolMetadata& field : libraryPartBoolMetadata) {
                bool value = false;
                hasField = hasField || metadata->Get (field.key, value);
            }
            if (!hasField) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "No field is given in 'metadata'."));
                continue;
            }

            GS::UniString failedWhat;
            GS::UniString failedText;
            executionResults (ModifyLibraryPart (libraryManager, name, [&] (GSMObject::Modifier& modifier) -> GSErrCode {
                for (const LibraryPartStringMetadata& field : libraryPartStringMetadata) {
                    GS::UniString value;
                    if (!metadata->Get (field.key, value)) {
                        continue;
                    }
                    ACAPI::Result<void> setResult = field.set (modifier, value);
                    if (setResult.IsErr ()) {
                        failedWhat = GS::UniString ("setting '") + field.key + "'";
                        failedText = setResult.UnwrapErr ().text.c_str ();
                        return setResult.UnwrapErr ().kind;
                    }
                }
                for (const LibraryPartBoolMetadata& field : libraryPartBoolMetadata) {
                    bool value = false;
                    if (!metadata->Get (field.key, value)) {
                        continue;
                    }
                    ACAPI::Result<void> setResult = field.set (modifier, value);
                    if (setResult.IsErr ()) {
                        failedWhat = GS::UniString ("setting '") + field.key + "'";
                        failedText = setResult.UnwrapErr ().text.c_str ();
                        return setResult.UnwrapErr ().kind;
                    }
                }
                return NoError;
            }, failedWhat, failedText));
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

GetLibraryPartScriptsCommand::GetLibraryPartScriptsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetLibraryPartScriptsCommand::GetName () const
{
    return "GetLibraryPartScripts";
}

GS::Optional<GS::UniString> GetLibraryPartScriptsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraryParts": {
                "type": "array",
                "description": "The library parts to read, each given by its name or by an element placed from it.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "libraryPartName": {
                                    "type": "string",
                                    "description": "The name of the loaded library part file with its extension, as returned in the fileName field of GetAvailableLibraryParts (e.g. 'My Door.gsm')."
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "libraryPartName"
                            ]
                        },
                        {
                            "type": "object",
                            "properties": {
                                "elementId": {
                                    "$ref": "#/ElementId",
                                    "description": "An object, lamp, window, door, skylight, zone or curtain wall frame, panel, junction or accessory; the library part it is placed from is read."
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "elementId"
                            ]
                        }
                    ]
                }
            },
            "scripts": {
                "type": "array",
                "description": "The scripts to read. All five by default.",
                "items": {
                    "type": "string",
                    "enum": [
                        "master",
                        "parameter",
                        "2d",
                        "3d",
                        "properties"
                    ]
                },
                "minItems": 1
            },
            "password": {
                "type": "string",
                "description": "The password of password protected library parts. Reading a script of a protected part without the right password is an error for that part."
            }
        },
        "additionalProperties": false,
        "required": [
            "libraryParts"
        ]
    })";
}

GS::Optional<GS::UniString> GetLibraryPartScriptsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraryParts": {
                "type": "array",
                "description": "The scripts of the library parts in the order of the input, or an error for a library part that cannot be read.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "libraryPartName": {
                                    "type": "string",
                                    "description": "The name of the library part file with its extension."
                                },
                                "unID": {
                                    "type": "string",
                                    "description": "The unique ID of the library part, its main and revision GUID."
                                },
                                "scripts": {
                                    "type": "object",
                                    "description": "The requested scripts, by the keys used in the input.",
                                    "additionalProperties": {
                                        "type": "object",
                                        "properties": {
                                            "present": {
                                                "type": "boolean",
                                                "description": "Whether the library part has this script section."
                                            },
                                            "script": {
                                                "type": "string",
                                                "description": "The script text. Left out when the section is not present."
                                            }
                                        },
                                        "additionalProperties": false,
                                        "required": [
                                            "present"
                                        ]
                                    }
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "libraryPartName",
                                "scripts"
                            ]
                        },
                        {
                            "$ref": "#/ErrorItem"
                        }
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "libraryParts"
        ]
    })";
}

GS::ObjectState GetLibraryPartScriptsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::Array<GS::ObjectState> libraryParts;
    if (!parameters.Get ("libraryParts", libraryParts)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'libraryParts' parameter.");
    }

    GS::Array<const LibraryPartScriptReader*> readers;
    GS::Array<GS::UniString> scriptKeys;
    if (parameters.Get ("scripts", scriptKeys)) {
        for (const GS::UniString& scriptKey : scriptKeys) {
            const LibraryPartScriptReader* found = nullptr;
            for (const LibraryPartScriptReader& reader : libraryPartScriptReaders) {
                if (scriptKey == GS::UniString (reader.key)) {
                    found = &reader;
                    break;
                }
            }
            if (found == nullptr) {
                return CreateErrorResponse (APIERR_BADPARS, "Unknown script '" + scriptKey + "'. Use master, parameter, 2d, 3d or properties.");
            }
            if (!readers.Contains (found)) {
                readers.Push (found);
            }
        }
    } else {
        for (const LibraryPartScriptReader& reader : libraryPartScriptReaders) {
            readers.Push (&reader);
        }
    }

    GS::UniString password;
    parameters.Get ("password", password);

    auto libraryManager = ACAPI::Library::GetLibraryManager ();
    if (libraryManager.IsErr ()) {
        return CreateErrorResponse (libraryManager.UnwrapErr ().kind, "Failed to get the Library Manager.");
    }

    GS::ObjectState response;
    const auto& libraryPartsOut = response.AddList<GS::ObjectState> ("libraryParts");

    for (const GS::ObjectState& libraryPart : libraryParts) {
        GS::ObjectState error;
        const std::optional<ACAPI::Library::LibPart> libPart = FindLibPartOfItem (libraryManager.Unwrap (), libraryPart, error);
        if (!libPart.has_value ()) {
            libraryPartsOut (error);
            continue;
        }

        GS::UniString name;
        auto libPartName = libPart->GetName ();
        if (!libPartName.IsErr ()) {
            name = libPartName.Unwrap ();
        }

        auto gsmObject = libraryManager->GetGSMObjectOfLibPart (*libPart);
        if (gsmObject.IsErr ()) {
            libraryPartsOut (CreateErrorResponse (gsmObject.UnwrapErr ().kind, "Failed to get the GSM object of the library part '" + name + "', it may not be a GSM object" + GetResultErrorText (gsmObject.UnwrapErr ()) + "."));
            continue;
        }
        const GSMObject& object = gsmObject.Unwrap ();

        GS::ObjectState scriptsOut;
        bool scriptFailed = false;
        for (const LibraryPartScriptReader* reader : readers) {
            GS::ObjectState scriptOut;
            const bool present = reader->has (object);
            scriptOut.Add ("present", present);
            if (present) {
                ACAPI::Result<GS::UniString> script = reader->get (object, password);
                if (script.IsErr ()) {
                    error = CreateErrorResponse (script.UnwrapErr ().kind, "Failed to read the '" + GS::UniString (reader->key) + "' script of the library part '" + name + "', check the password" + GetResultErrorText (script.UnwrapErr ()) + ".");
                    scriptFailed = true;
                    break;
                }
                scriptOut.Add ("script", script.Unwrap ());
            }
            scriptsOut.Add (reader->key, scriptOut);
        }
        if (scriptFailed) {
            libraryPartsOut (error);
            continue;
        }

        GS::ObjectState libraryPartOut;
        libraryPartOut.Add ("libraryPartName", name);
        auto unID = object.GetUnID ();
        if (!unID.IsErr ()) {
            libraryPartOut.Add ("unID", unID.Unwrap ().ToUniString ());
        }
        libraryPartOut.Add ("scripts", scriptsOut);
        libraryPartsOut (libraryPartOut);
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

CopyLibraryPartsCommand::CopyLibraryPartsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CopyLibraryPartsCommand::GetName () const
{
    return "CopyLibraryParts";
}

GS::Optional<GS::UniString> CopyLibraryPartsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraryParts": {
                "type": "array",
                "description": "The library parts to copy, each given by its name or by an element placed from it, with the name of the copy.",
                "items": {
                    "type": "object",
                    "properties": {
                        "libraryPartName": {
                            "type": "string",
                            "description": "The name of the loaded library part file with its extension, as returned in the fileName field of GetAvailableLibraryParts (e.g. 'My Door.gsm'). Give this or elementId."
                        },
                        "elementId": {
                            "$ref": "#/ElementId",
                            "description": "An object, lamp, window, door, skylight, zone or curtain wall frame, panel, junction or accessory; the library part it is placed from is copied. Give this or libraryPartName."
                        },
                        "newName": {
                            "type": "string",
                            "description": "The file name of the copy with its extension (e.g. 'My Door copy.gsm'). It must not contain a folder.",
                            "minLength": 1
                        },
                        "targetFolder": {
                            "type": "string",
                            "description": "Optional. The absolute path of an existing folder to put the copy into. By default the folder of the embedded library. The copy can only be found by name after the reload when the folder belongs to a loaded library."
                        },
                        "keepMainGuid": {
                            "type": "boolean",
                            "description": "Optional. When true, the copy keeps the main GUID of the original. By default false: the copy gets a new unique ID."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "newName"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "libraryParts"
        ]
    })";
}

GS::Optional<GS::UniString> CopyLibraryPartsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraryParts": {
                "type": "array",
                "description": "The copies in the order of the input, or an error for a library part that was not copied.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "libraryPartName": {
                                    "type": "string",
                                    "description": "The file name of the copy with its extension."
                                },
                                "unID": {
                                    "type": "string",
                                    "description": "The unique ID of the copy, its main and revision GUID."
                                },
                                "path": {
                                    "type": "string",
                                    "description": "The absolute path of the copy."
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "libraryPartName",
                                "path"
                            ]
                        },
                        {
                            "$ref": "#/ErrorItem"
                        }
                    ]
                }
            },
            "librariesReloaded": {
                "type": "boolean",
                "description": "Whether the libraries were reloaded after copying. False when nothing was copied or the reload failed; then the copies are not found by name until the libraries are reloaded."
            }
        },
        "additionalProperties": false,
        "required": [
            "libraryParts",
            "librariesReloaded"
        ]
    })";
}

GS::ObjectState CopyLibraryPartsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::Array<GS::ObjectState> libraryParts;
    if (!parameters.Get ("libraryParts", libraryParts)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'libraryParts' parameter.");
    }

    auto libraryManager = ACAPI::Library::GetLibraryManager ();
    if (libraryManager.IsErr ()) {
        return CreateErrorResponse (libraryManager.UnwrapErr ().kind, "Failed to get the Library Manager.");
    }

    // The folder of the embedded library, read once when an item has no targetFolder.
    std::optional<IO::Location> embeddedLibraryFolder;
    GS::ObjectState embeddedLibraryError;
    bool embeddedLibraryFailed = false;
    const auto GetEmbeddedLibraryLocation = [&] () -> bool {
        if (embeddedLibraryFolder.has_value ()) {
            return true;
        }
        if (embeddedLibraryFailed) {
            return false;
        }
        embeddedLibraryFailed = true;
        auto embeddedLibrary = libraryManager->GetEmbeddedLibrary ();
        if (embeddedLibrary.IsErr ()) {
            embeddedLibraryError = CreateErrorResponse (embeddedLibrary.UnwrapErr ().kind, "Failed to get the embedded library" + GetResultErrorText (embeddedLibrary.UnwrapErr ()) + ".");
            return false;
        }
        auto location = embeddedLibrary->GetLocation ();
        if (location.IsErr ()) {
            embeddedLibraryError = CreateErrorResponse (location.UnwrapErr ().kind, "Failed to get the folder of the embedded library" + GetResultErrorText (location.UnwrapErr ()) + ".");
            return false;
        }
        embeddedLibraryFolder = location.Unwrap ();
        embeddedLibraryFailed = false;
        return true;
    };

    GS::ObjectState response;
    const auto& libraryPartsOut = response.AddList<GS::ObjectState> ("libraryParts");

    bool anyCopied = false;
    for (const GS::ObjectState& libraryPart : libraryParts) {
        GS::UniString newName;
        if (!libraryPart.Get ("newName", newName) || newName.IsEmpty ()) {
            libraryPartsOut (CreateErrorResponse (APIERR_BADPARS, "Missing 'newName' parameter."));
            continue;
        }
        if (newName.Contains ("/") || newName.Contains ("\\") || newName.Contains (":") || newName == "." || newName == "..") {
            libraryPartsOut (CreateErrorResponse (APIERR_BADPARS, "The newName '" + newName + "' must be a file name without a folder."));
            continue;
        }

        GS::ObjectState error;
        const std::optional<ACAPI::Library::LibPart> libPart = FindLibPartOfItem (libraryManager.Unwrap (), libraryPart, error);
        if (!libPart.has_value ()) {
            libraryPartsOut (error);
            continue;
        }

        GS::UniString name;
        auto libPartName = libPart->GetName ();
        if (!libPartName.IsErr ()) {
            name = libPartName.Unwrap ();
        }

        IO::Location targetLocation;
        GS::UniString targetFolder;
        if (libraryPart.Get ("targetFolder", targetFolder) && !targetFolder.IsEmpty ()) {
            targetLocation = IO::Location (targetFolder);
        } else if (GetEmbeddedLibraryLocation ()) {
            targetLocation = *embeddedLibraryFolder;
        } else {
            libraryPartsOut (embeddedLibraryError);
            continue;
        }
        if (IO::Folder (targetLocation).GetStatus () != NoError) {
            libraryPartsOut (CreateErrorResponse (APIERR_BADPARS, "The target folder '" + targetLocation.ToDisplayText () + "' does not exist or is not a folder."));
            continue;
        }

        targetLocation.AppendToLocal (IO::Name (newName));
        bool exists = false;
        if (IO::fileSystem.Contains (targetLocation, &exists) != NoError || exists) {
            libraryPartsOut (CreateErrorResponse (APIERR_BADPARS, "The file '" + targetLocation.ToDisplayText () + "' already exists, nothing was copied."));
            continue;
        }

        auto gsmObject = libraryManager->GetGSMObjectOfLibPart (*libPart);
        if (gsmObject.IsErr ()) {
            libraryPartsOut (CreateErrorResponse (gsmObject.UnwrapErr ().kind, "Failed to get the GSM object of the library part '" + name + "', it may not be a GSM object" + GetResultErrorText (gsmObject.UnwrapErr ()) + "."));
            continue;
        }

        bool keepMainGuid = false;
        libraryPart.Get ("keepMainGuid", keepMainGuid);
        const GSMObject::CopyMethod copyMethod = keepMainGuid ? GSMObject::CopyMethod::KeepMainGuid : GSMObject::CopyMethod::GenerateNewUnID;

        ACAPI::Result<void> copyResult = gsmObject->CopyTo (targetLocation, copyMethod);
        if (copyResult.IsErr ()) {
            libraryPartsOut (CreateErrorResponse (copyResult.UnwrapErr ().kind, "Failed to copy the library part '" + name + "' to '" + targetLocation.ToDisplayText () + "'" + GetResultErrorText (copyResult.UnwrapErr ()) + "."));
            continue;
        }
        anyCopied = true;

        GS::ObjectState libraryPartOut;
        libraryPartOut.Add ("libraryPartName", newName);
        auto copy = libraryManager->LoadGSMObjectFromFile (targetLocation);
        if (!copy.IsErr ()) {
            auto unID = copy->GetUnID ();
            if (!unID.IsErr ()) {
                libraryPartOut.Add ("unID", unID.Unwrap ().ToUniString ());
            }
        }
        libraryPartOut.Add ("path", targetLocation.ToDisplayText ());
        libraryPartsOut (libraryPartOut);
    }

    // One reload for the whole call, so the copies can be found by name.
    response.Add ("librariesReloaded", anyCopied && ACAPI_ProjectOperation_ReloadLibraries () == NoError);

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}
