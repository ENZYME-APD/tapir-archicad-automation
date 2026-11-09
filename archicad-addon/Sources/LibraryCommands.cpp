// GDLDefs.h MUST be included before LibraryCommands.hpp/ACAPinc.h: included afterward it fails to
// parse ("no member named 'GS' in namespace 'GS'"), as ExtendedElementCommands.cpp notes for MeshBody.hpp.
// The standard headers come first: GSRoot.hpp, pulled in by GDLDefs.h, redefines realloc as GS_realloc,
// which breaks libc++'s <locale> (reached through <functional>) when it is parsed afterward (AC27 Mac).
#include <functional>
#include <locale>
#include <vector>

#include "GDLDefs.h"

#include "LibraryCommands.hpp"
#include "LibraryPartCreationCommands.hpp"
#include "ElementGDLParameterCommands.hpp"
#include "ObjectState.hpp"
#include "MigrationHelper.hpp"
#include "Folder.hpp"
#include "File.hpp"
#include "FileSystem.hpp"
#include "GSUnID.hpp"

#include <string>

#ifdef ServerMainVers_2900
#include "ACAPI/Result.hpp"
#include "ACAPI/Library/LibraryManager.hpp"
#include "ACAPI/Library/LibPart.hpp"
#include "ACAPI/Library/Library.hpp"
#include "ACAPI/Library/LibraryTreePath.hpp"
#endif

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
#include "ACAPI/Library/LibraryTreePath.hpp"
#include "ACAPI/Library/LibPart.hpp"
#include "ACAPI/Library/GSMObject.hpp"
// GSMObject.hpp declares the picture setters with GX::ImageRef; included here to construct the images.
#if __has_include ("GXImage.hpp")
#include "GXImage.hpp"
#elif __has_include ("GXImage/GXImage.hpp")
#include "GXImage/GXImage.hpp"
#endif
#include "NativeImage.hpp"
#include "Base64Converter.hpp"
#include "GDLParamInterface.hpp"

#include <functional>
#include <optional>

#include <optional>
#include <vector>

namespace ACLib = ACAPI::Library;

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

// Decodes a base64 encoded PNG into an image, or returns false when it is not a valid PNG.
static bool DecodeBase64PngImage (const GS::UniString& base64, GX::ImageRef& image)
{
    GS::Array<char> bytes = Base64Converter::Decode (base64);
    if (bytes.IsEmpty ()) {
        return false;
    }

    NewDisplay::NativeImage nativeImage (bytes.GetContent (), bytes.GetSize (), NewDisplay::NativeImage::Encoding::PNG);
    if (nativeImage.GetWidth () == 0 || nativeImage.GetHeight () == 0) {
        return false;
    }

    image = GX::ImageRef (new GX::Image (nativeImage));
    return true;
}

// Decodes the picture given under key, or the list of pictures when isList is true.
// Returns false and sets errorMessage when a picture cannot be decoded; a missing key is not an error.
static bool GetPictureImages (const GS::ObjectState& parameters, const char* key, bool isList, bool& found, std::vector<GX::ImageRef>& images, GS::UniString& errorMessage)
{
    GS::Array<GS::UniString> pictures;
    if (isList) {
        found = parameters.Get (key, pictures);
    } else {
        GS::UniString picture;
        found = parameters.Get (key, picture);
        if (found) {
            pictures.Push (picture);
        }
    }
    if (!found) {
        return true;
    }

    for (UIndex i = 0; i < pictures.GetSize (); ++i) {
        GX::ImageRef image;
        if (!DecodeBase64PngImage (pictures[i], image)) {
            errorMessage = "'" + GS::UniString (key) + "'";
            if (isList) {
                errorMessage += GS::UniString::Printf (" item %u", i);
            }
            errorMessage += " is not a valid base64 encoded PNG image.";
            return false;
        }
        images.push_back (image);
    }
    return true;
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

// Collects the readers of the scripts listed in the 'scripts' parameter, all five when it is missing.
static bool GetRequestedScriptReaders (const GS::ObjectState& parameters, GS::Array<const LibraryPartScriptReader*>& readers, GS::ObjectState& error)
{
    GS::Array<GS::UniString> scriptKeys;
    if (!parameters.Get ("scripts", scriptKeys)) {
        for (const LibraryPartScriptReader& reader : libraryPartScriptReaders) {
            readers.Push (&reader);
        }
        return true;
    }

    for (const GS::UniString& scriptKey : scriptKeys) {
        const LibraryPartScriptReader* found = nullptr;
        for (const LibraryPartScriptReader& reader : libraryPartScriptReaders) {
            if (scriptKey == GS::UniString (reader.key)) {
                found = &reader;
                break;
            }
        }
        if (found == nullptr) {
            error = CreateErrorResponse (APIERR_BADPARS, "Unknown script '" + scriptKey + "'. Use master, parameter, 2d, 3d or properties.");
            return false;
        }
        if (!readers.Contains (found)) {
            readers.Push (found);
        }
    }
    return true;
}

// Reads the given scripts of a GSM object into {key: {present, script}}; 'name' names the part in the error.
static bool ReadLibraryPartScripts (const GSMObject& object, const GS::Array<const LibraryPartScriptReader*>& readers, const GS::UniString& password, const GS::UniString& name, GS::ObjectState& scriptsOut, GS::ObjectState& error)
{
    for (const LibraryPartScriptReader* reader : readers) {
        GS::ObjectState scriptOut;
        const bool present = reader->has (object);
        scriptOut.Add ("present", present);
        if (present) {
            ACAPI::Result<GS::UniString> script = reader->get (object, password);
            if (script.IsErr ()) {
                error = CreateErrorResponse (script.UnwrapErr ().kind, "Failed to read the '" + GS::UniString (reader->key) + "' script of the library part '" + name + "', check the password" + GetResultErrorText (script.UnwrapErr ()) + ".");
                return false;
            }
            scriptOut.Add ("script", script.Unwrap ());
        }
        scriptsOut.Add (reader->key, scriptOut);
    }
    return true;
}

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
                    "$ref": "#/EmbeddedLibraryItem"
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

    // Index of the item in the input and its location, for every existing item.
    // They are deleted with one API call; an item that does not exist is a failed result,
    // because the API reports no error for a location that is not there.
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
        } else if (ItemExists (item.location)) {
            results[item.inputIndex] = CreateFailedExecutionResult (APIERR_GENERAL, "The embedded library item is still there after the deletion.");
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

        if (!ItemExists (itemLocation)) {
            results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "No item at the given path in the embedded library."));
            continue;
        }

        results.Push (CreateSuccessfulExecutionResult ());
        itemsToDelete.Push ({ i, itemLocation });
        locationsToDelete.Push (itemLocation);
    }

    if (!locationsToDelete.IsEmpty ()) {
        ACAPI_LibraryManagement_DeleteEmbeddedLibItems (&locationsToDelete, keepGSMFile, silentMode);
        // The API reports at most one error for the whole list, and none for an item it left in place,
        // so check item by item: an item that is gone now was deleted, the others are tried one by one.
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

// Writes an empty section of the library part under creation, streamed the way the
// DevKit's LibPart_Test example writes its empty master script.
static GSErrCode WriteEmptyLibPartSection (const API_LibPartSection& section)
{
    const char emptyText[] = "";
    GSErrCode err = NewLibPartSection (&section);
    if (err != NoError) {
        return err;
    }
    err = WriteLibPartSection (0, emptyText);
    const GSErrCode endErr = EndLibPartSection ();
    return err != NoError ? err : endErr;
}

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

    // The master script section is always written, empty when not given, as the DevKit's
    // LibPart_Test example and CreateLibraryPartFromElements do; the others only when given.
    GS::Array<ScriptSection> scripts;
    for (const ScriptSectionField& scriptSectionField : scriptSectionFields) {
        GS::UniString script;
        const bool hasScript = libraryPartData.Get (scriptSectionField.fieldName, script) && !script.IsEmpty ();
        if (hasScript || scriptSectionField.sectType == API_Sect1DScript) {
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
        if (script.script.IsEmpty ()) {
            err = WriteEmptyLibPartSection (section);
        } else {
            GS::UniString scriptText = script.script;
            err = ACAPI_LibraryPart_AddSection (&section, nullptr, &scriptText);
        }
        if (err != NoError) {
            failedStep = "Failed to write a script section of the library part.";
            break;
        }
    }

    if (err == NoError) {
        // Without parameters of its own the list is passed as nullptr, as the DevKit's
        // LibPart_Test example does, not as an empty handle.
        API_AddParType** ownParameters = (addPars != nullptr && BMGetHandleSize (reinterpret_cast<GSConstHandle> (addPars)) > 0) ? addPars : nullptr;
        GSHandle parameterSection = nullptr;
        err = ACAPI_LibraryPart_GetSect_ParamDef (&libPart, ownParameters, &a, &b, nullptr, &parameterSection);
        if (err != NoError) {
            failedStep = "Failed to prepare the parameter section of the library part (ACAPI_LibraryPart_GetSect_ParamDef).";
        } else if (parameterSection == nullptr) {
            err = APIERR_GENERAL;
            failedStep = "Failed to prepare the parameter section of the library part: the API returned no section.";
        } else {
            // The details are set on the parameter section before it is added, as the
            // DevKit's LibPart_Test example and CreateLibraryPartFromElements do.
            API_LibPartDetails details = {};
            details.object.autoHotspot = false;
            err = SetLibPartDetails (&libPart, parameterSection, &details);
            if (err != NoError) {
                failedStep = "Failed to set the details of the parameter section of the library part (ACAPI_LibraryPart_SetDetails_ParamDef).";
            } else {
                API_LibPartSection section = {};
                section.sectType = API_SectParamDef;
                err = ACAPI_LibraryPart_AddSection (&section, parameterSection, nullptr);
                if (err != NoError) {
                    failedStep = "Failed to write the parameter section of the library part (ACAPI_LibraryPart_AddSection).";
                }
            }
        }
        if (parameterSection != nullptr) {
            BMKillHandle (&parameterSection);
        }
    }

    // Save ends the creation even after a failed section, so a later call does not
    // find a library part still under creation.
    const GSErrCode saveErr = ACAPI_LibraryPart_Save (&libPart);

    if (err != NoError) {
        if (saveErr != NoError) {
            failedStep += " Saving the incomplete library part failed too.";
            return CreateErrorResponse (err, failedStep);
        }
#ifdef ServerMainVers_2700
        // The saved part lacks the failed section and is unusable (a part without its
        // parameter section has no parameters at all), so it is deleted again.
        const bool keepGSMFile = false;
        const bool silentMode = true;
        if (ACAPI_LibraryManagement_DeleteEmbeddedLibItem (&outputFileLoc, keepGSMFile, silentMode) == NoError) {
            failedStep += " The incomplete library part was not kept.";
        } else {
            failedStep += GS::UniString::Printf (" The library part was still saved to '%T' in the embedded library without that section, and deleting it failed: delete it with DeleteEmbeddedLibraryItems.", relativePath.ToPrintf ());
        }
#else
        failedStep += GS::UniString::Printf (" The library part was still saved to '%T' in the embedded library without that section.", relativePath.ToPrintf ());
#endif
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
                    "$ref": "#/LibraryPartData"
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
                            "$ref": "#/LibraryPartCreationDetails"
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
                    "$ref": "#/EmbeddedLibraryFile"
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

// ACAPI_LibraryManagement_GetGDLDate and ChangeGDLDate read and write an Int32 array of six:
// year, month, day, hour, minute and second.
static const char* GDLDateComponentNames[] = { "year", "month", "day", "hour", "minute", "second" };
static const USize GDLDateComponentCount = 6;

static GS::UniString GetGDLDateSchema (const char* description)
{
    return GS::UniString (R"({
        "type": "object",
        "description": ")") + description + R"(",
        "properties": {
            "year": {
                "type": "integer"
            },
            "month": {
                "type": "integer",
                "minimum": 1,
                "maximum": 12
            },
            "day": {
                "type": "integer",
                "minimum": 1,
                "maximum": 31
            },
            "hour": {
                "type": "integer",
                "minimum": 0,
                "maximum": 23
            },
            "minute": {
                "type": "integer",
                "minimum": 0,
                "maximum": 59
            },
            "second": {
                "type": "integer",
                "minimum": 0,
                "maximum": 59
            }
        },
        "additionalProperties": false,
        "required": [
            "year",
            "month",
            "day",
            "hour",
            "minute",
            "second"
        ]
    })";
}

GetGDLDateCommand::GetGDLDateCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetGDLDateCommand::GetName () const
{
    return "GetGDLDate";
}

GS::Optional<GS::UniString> GetGDLDateCommand::GetRawResponseSchema () const
{
    return GS::UniString (R"({
        "type": "object",
        "properties": {
            "date": )") + GetGDLDateSchema ("The date GDL scripts see.") + R"(
        },
        "additionalProperties": false,
        "required": [
            "date"
        ]
    })";
}

GS::ObjectState GetGDLDateCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    Int32 date[GDLDateComponentCount] = {};
    GSErrCode err = ACAPI_LibraryManagement_GetGDLDate (date);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the GDL date.");
    }

    GS::ObjectState dateOs;
    for (USize i = 0; i < GDLDateComponentCount; ++i) {
        dateOs.Add (GDLDateComponentNames[i], date[i]);
    }

    return GS::ObjectState ("date", dateOs);
}

SetGDLDateCommand::SetGDLDateCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetGDLDateCommand::GetName () const
{
    return "SetGDLDate";
}

GS::Optional<GS::UniString> SetGDLDateCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
        "type": "object",
        "properties": {
            "date": )") + GetGDLDateSchema ("The date GDL scripts should see, in the format GetGDLDate returns.") + R"(
        },
        "additionalProperties": false,
        "required": [
            "date"
        ]
    })";
}

GS::Optional<GS::UniString> SetGDLDateCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState SetGDLDateCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::ObjectState dateOs;
    if (!parameters.Get ("date", dateOs)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "The date parameter is missing.");
    }

    Int32 date[GDLDateComponentCount] = {};
    for (USize i = 0; i < GDLDateComponentCount; ++i) {
        if (!dateOs.Get (GDLDateComponentNames[i], date[i])) {
            return CreateFailedExecutionResult (APIERR_BADPARS, GS::UniString ("The date has no ") + GDLDateComponentNames[i] + ".");
        }
    }

    GSErrCode err = ACAPI_LibraryManagement_ChangeGDLDate (date);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to set the GDL date.");
    }

    return CreateSuccessfulExecutionResult ();
}

CheckLibrariesCommand::CheckLibrariesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CheckLibrariesCommand::GetName () const
{
    return "CheckLibraries";
}

GS::Optional<GS::UniString> CheckLibrariesCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState CheckLibrariesCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GSErrCode err = ACAPI_LibraryManagement_CheckLibraries ();
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to check libraries.");
    }

    return CreateSuccessfulExecutionResult ();
}

ResetLibrariesCommand::ResetLibrariesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ResetLibrariesCommand::GetName () const
{
    return "ResetLibraries";
}

GS::Optional<GS::UniString> ResetLibrariesCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState ResetLibrariesCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GSErrCode err = ACAPI_LibraryManagement_ResetLibraries ();
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to reset libraries.");
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
                    "$ref": "#/AvailableLibraryPart"
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
                    "$ref": "#/SkippedLibraryPart"
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

GetLibrariesOfLibPartsCommand::GetLibrariesOfLibPartsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetLibrariesOfLibPartsCommand::GetName () const
{
    return "GetLibrariesOfLibParts";
}

GS::Optional<GS::UniString> GetLibrariesOfLibPartsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraryParts": {
                "type": "array",
                "description": "Library parts to look up, each given by exactly one of name and guid.",
                "items": {
                    "$ref": "#/LibraryPartNameOrGuid"
                }
            }
        },
        "additionalProperties": false,
        "required": ["libraryParts"]
    })";
}

GS::Optional<GS::UniString> GetLibrariesOfLibPartsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraries": {
                "type": "array",
                "description": "One entry per input library part, in the same order.",
                "items": {
                    "$ref": "#/LibraryOfLibraryPart"
                }
            }
        },
        "additionalProperties": false,
        "required": ["libraries"]
    })";
}

#ifdef ServerMainVers_2900
// The index is only used internally: it can change when the libraries are reloaded.
static GSErrCode FindLibPartIndexByGuid (const GS::Guid& guid, Int32& foundIndex)
{
    Int32 partCount = 0;
    GSErrCode err = ACAPI_LibraryPart_GetNum (&partCount);
    if (err != NoError) {
        return err;
    }

    for (Int32 i = 1; i <= partCount; ++i) {
        API_LibPart libPart = {};
        libPart.index = i;
        if (ACAPI_LibraryPart_Get (&libPart) != NoError) {
            continue;
        }
        const bool matches = GS::UnID (libPart.ownUnID).GetMainGuid () == guid;
        delete libPart.location;
        libPart.location = nullptr;
        if (matches) {
            foundIndex = i;
            return NoError;
        }
    }
    return APIERR_BADID;
}

static GS::ObjectState CreateLibraryErrorEntry (GSErrCode code, const GS::UniString& message)
{
    GS::ObjectState error;
    error.Add ("code", static_cast<Int32> (code));
    error.Add ("message", message);
    GS::ObjectState entry;
    entry.Add ("error", error);
    return entry;
}
#endif

GS::ObjectState GetLibrariesOfLibPartsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    GS::Array<GS::ObjectState> inputs;
    parameters.Get ("libraryParts", inputs);

    ACAPI::Result<ACAPI::Library::LibraryManager> manager = ACAPI::Library::GetLibraryManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Unable to get the library manager.");
    }

    GS::ObjectState response;
    const auto& librariesAdder = response.AddList<GS::ObjectState> ("libraries");

    for (const GS::ObjectState& input : inputs) {
        GS::UniString name;
        GS::UniString guidString;
        const bool hasName = input.Get ("name", name);
        const bool hasGuid = input.Get ("guid", guidString);
        if (hasName == hasGuid) {
            librariesAdder (CreateLibraryErrorEntry (APIERR_BADPARS, "Give exactly one of name and guid."));
            continue;
        }

        Int32 libPartIndex = 0;
        if (hasGuid) {
            const GSErrCode findErr = FindLibPartIndexByGuid (GS::Guid (guidString), libPartIndex);
            if (findErr != NoError) {
                librariesAdder (CreateLibraryErrorEntry (findErr, "Library part not found."));
                continue;
            }
        }

        // Result and LibPart have no default constructor and no copy assignment.
        const ACAPI::Result<ACAPI::Library::LibPart> libPart = hasName
            ? manager->GetLibPartByName (name)
            : manager->GetLibPartByLibInd (libPartIndex);
        if (libPart.IsErr ()) {
            librariesAdder (CreateLibraryErrorEntry (libPart.UnwrapErr ().kind, "Library part not found."));
            continue;
        }

        const auto library = manager->GetLibraryOfLibPart (libPart.Unwrap ());
        if (library.IsErr ()) {
            librariesAdder (CreateLibraryErrorEntry (library.UnwrapErr ().kind, "Unable to get the library of the library part."));
            continue;
        }

        GS::ObjectState libraryData;
        const auto displayName = library->GetDisplayName ();
        if (displayName.IsOk ()) {
            libraryData.Add ("displayName", displayName.Unwrap ());
        }
        const auto location = library->GetLocation ();
        if (location.IsOk ()) {
            libraryData.Add ("path", location.Unwrap ().ToDisplayText ());
        }
        const auto isEmbedded = library->IsEmbedded ();
        if (isEmbedded.IsOk ()) {
            libraryData.Add ("isEmbedded", isEmbedded.Unwrap ());
        }
        const auto isBuiltIn = library->IsBuiltIn ();
        if (isBuiltIn.IsOk ()) {
            libraryData.Add ("isBuiltIn", isBuiltIn.Unwrap ());
        }
        const auto isPackage = library->IsPackage ();
        if (isPackage.IsOk ()) {
            libraryData.Add ("isPackage", isPackage.Unwrap ());
        }
        const auto isLocal = library->IsLocal ();
        if (isLocal.IsOk ()) {
            libraryData.Add ("isLocal", isLocal.Unwrap ());
        }
        const auto isMissing = library->IsMissing ();
        if (isMissing.IsOk ()) {
            libraryData.Add ("isMissing", isMissing.Unwrap ());
        }

        GS::ObjectState entry;
        entry.Add ("library", libraryData);

        const auto treePath = manager->GetLibraryTreePathOfLibPart (libPart.Unwrap ());
        if (treePath.IsOk ()) {
            const auto& partsAdder = entry.AddList<GS::UniString> ("libraryTreePath");
            for (const GS::UniString& part : treePath->GetParts ()) {
                partsAdder (part);
            }
        }
        librariesAdder (entry);
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}

// Resolves a LibraryPartIdentifier to a loaded library part: its index, ownUnID and names
// are filled in, the location ACAPI allocates is freed.
static GSErrCode ResolveLibraryPart (const GS::ObjectState& identifier, API_LibPart& libPart, GS::UniString& errorMessage)
{
    libPart = {};

    Int32 index = 0;
    GS::UniString name;
    GS::UniString unId;
    GSErrCode err = NoError;
    if (identifier.Get ("index", index)) {
        libPart.index = index;
        err = ACAPI_LibraryPart_Get (&libPart);
        errorMessage = GS::UniString::Printf ("Failed to get the library part with index %d.", index);
    } else if (identifier.Get ("name", name)) {
        GS::ucscpy (libPart.docu_UName, name.ToUStr (0, GS::Min (name.GetLength (), (USize) API_UniLongNameLen - 1)).Get ());
        err = ACAPI_LibraryPart_Search (&libPart, false, false);
        errorMessage = GS::UniString::Printf ("Not found library part with name '%T'.", name.ToPrintf ());
    } else if (identifier.Get ("unId", unId)) {
        CHTruncate (unId.ToCStr ().Get (), libPart.ownUnID, sizeof (libPart.ownUnID));
        err = ACAPI_LibraryPart_Search (&libPart, false, false);
        errorMessage = GS::UniString::Printf ("Not found library part with unique ID '%T'.", unId.ToPrintf ());
    } else {
        err = APIERR_BADPARS;
        errorMessage = "A library part must be identified by one of index, name or unId.";
    }

    delete libPart.location;
    libPart.location = nullptr;
    return err;
}

static GS::UniString VariationIdToString (API_ElemVariationID variationID)
{
    const UInt32 v = static_cast<UInt32> (variationID);
    char s[5] = {
        (char) ((v >> 24) & 0xFF),
        (char) ((v >> 16) & 0xFF),
        (char) ((v >> 8) & 0xFF),
        (char) (v & 0xFF),
        0
    };
    return GS::UniString (s);
}

// Templates, because before Archicad 27 the members of API_LibPartDetails have unnamed struct
// types (API_LibPartObjectDetails and API_LibPartDoorWindowDetails do not exist there).
template <typename ObjectDetails>
static GS::ObjectState CreateObjectDetails (const ObjectDetails& details)
{
    return GS::ObjectState (
        "fixSize", details.fixSize,
        "autoHotspot", details.autoHotspot);
}

template <typename DoorWindowDetails>
static GS::ObjectState CreateDoorWindowDetails (const DoorWindowDetails& details)
{
    GS::ObjectState os;
    os.Add ("mirThick", static_cast<double> (details.mirThick));
    os.Add ("wallInset", static_cast<double> (details.wallInset));
    os.Add ("leftFram", static_cast<double> (details.leftFram));
    os.Add ("righFram", static_cast<double> (details.righFram));
    os.Add ("topFram", static_cast<double> (details.topFram));
    os.Add ("botFram", static_cast<double> (details.botFram));
    os.Add ("orientation", static_cast<Int32> (details.orientation));
    os.Add ("contours3D", details.contours_3D);
    return os;
}

GetLibraryPartTypeDetailsCommand::GetLibraryPartTypeDetailsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetLibraryPartTypeDetailsCommand::GetName () const
{
    return "GetLibraryPartTypeDetails";
}

GS::Optional<GS::UniString> GetLibraryPartTypeDetailsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraryParts": {
                "type": "array",
                "description": "The library parts to get the details of.",
                "items": {
                    "$ref": "#/LibraryPartIdentifier"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "libraryParts"
        ]
    })";
}

GS::Optional<GS::UniString> GetLibraryPartTypeDetailsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraryParts": {
                "type": "array",
                "description": "The details of the library parts, in the order of the input, or an error for a library part that was not found.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "properties": {
                                "index": { "type": "integer", "description": "The current index of the library part. It may change when the libraries are reloaded." },
                                "name": { "type": "string", "description": "The name of the library part." },
                                "fileName": { "type": "string", "description": "The file name of the library part." },
                                "unId": { "type": "string", "description": "The full unique ID (main and revision GUID) of the library part." },
                                "parentUnId": { "type": "string", "description": "The full unique ID of the direct parent subtype of the library part." },
                                "typeId": { "$ref": "#/LibraryPartType" },
                                "isPlaceable": { "type": "boolean" },
                                "isTemplate": { "type": "boolean" },
                                "objectDetails": {
                                    "$ref": "#/LibraryPartObjectDetails"
                                },
                                "doorWindowDetails": {
                                    "$ref": "#/LibraryPartDoorWindowDetails"
                                },
                                "creatorElementType": { "$ref": "#/ElementType", "description": "The type of the element the library part is placed as (its creator tool). Missing when Archicad does not report one." },
                                "creatorVariation": { "type": "string", "description": "The four character variation ID of the creator tool, when it is not the generic variation." }
                            },
                            "additionalProperties": false,
                            "required": [ "index", "name", "fileName", "unId", "parentUnId", "typeId", "isPlaceable", "isTemplate" ]
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

GS::ObjectState GetLibraryPartTypeDetailsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> identifiers;
    parameters.Get ("libraryParts", identifiers);

    GS::ObjectState response;
    const auto& libraryParts = response.AddList<GS::ObjectState> ("libraryParts");

    for (const GS::ObjectState& identifier : identifiers) {
        API_LibPart libPart = {};
        GS::UniString errorMessage;
        GSErrCode err = ResolveLibraryPart (identifier, libPart, errorMessage);
        if (err != NoError) {
            libraryParts (CreateErrorResponse (err, errorMessage));
            continue;
        }

        GS::ObjectState details;
        details.Add ("index", static_cast<Int32> (libPart.index));
        details.Add ("name", GS::UniString (libPart.docu_UName));
        details.Add ("fileName", GS::UniString (libPart.file_UName));
        details.Add ("unId", GS::UniString (libPart.ownUnID));
        details.Add ("parentUnId", GS::UniString (libPart.parentUnID));
        details.Add ("typeId", LibPartTypeIdToString (libPart.typeID));
        details.Add ("isPlaceable", libPart.isPlaceable);
        details.Add ("isTemplate", libPart.isTemplate);

        // ACAPI_LibraryPart_GetDetails supports only these library part types; it returns
        // APIERR_BADPARS for the others (macros, pictures etc.).
        API_LibPartDetails libPartDetails = {};
        switch (libPart.typeID) {
            case APILib_WindowID:
                if (ACAPI_LibraryPart_GetDetails (libPart.index, &libPartDetails) == NoError) {
                    details.Add ("doorWindowDetails", CreateDoorWindowDetails (libPartDetails.wind));
                }
                break;
            case APILib_DoorID:
                if (ACAPI_LibraryPart_GetDetails (libPart.index, &libPartDetails) == NoError) {
                    details.Add ("doorWindowDetails", CreateDoorWindowDetails (libPartDetails.door));
                }
                break;
            case APILib_ObjectID:
                if (ACAPI_LibraryPart_GetDetails (libPart.index, &libPartDetails) == NoError) {
                    details.Add ("objectDetails", CreateObjectDetails (libPartDetails.object));
                }
                break;
            case APILib_LampID:
                if (ACAPI_LibraryPart_GetDetails (libPart.index, &libPartDetails) == NoError) {
                    details.Add ("objectDetails", CreateObjectDetails (libPartDetails.light));
                }
                break;
            case APILib_RoomID:
                if (ACAPI_LibraryPart_GetDetails (libPart.index, &libPartDetails) == NoError) {
                    details.Add ("objectDetails", CreateObjectDetails (libPartDetails.zone));
                }
                break;
            case APILib_LabelID:
                if (ACAPI_LibraryPart_GetDetails (libPart.index, &libPartDetails) == NoError) {
                    details.Add ("objectDetails", CreateObjectDetails (libPartDetails.label));
                }
                break;
            default:
                break;
        }

        // GetLibPartToolVariation searches the library part itself, so it gets a copy without
        // the location, and whatever location it allocates is freed afterwards.
        API_LibPart searchedLibPart = libPart;
        searchedLibPart.location = nullptr;
        API_ToolBoxItem toolboxItem = {};
        err = ACAPI_LibraryPart_GetLibPartToolVariation (&searchedLibPart, &toolboxItem);
        delete searchedLibPart.location;
        if (err == NoError) {
#ifdef ServerMainVers_2600
            const API_ElemTypeID creatorTypeID = toolboxItem.type.typeID;
            const API_ElemVariationID creatorVariationID = toolboxItem.type.variationID;
#else
            const API_ElemTypeID creatorTypeID = toolboxItem.typeID;
            const API_ElemVariationID creatorVariationID = toolboxItem.variationID;
#endif
            if (creatorTypeID != API_ZombieElemID) {
                details.Add ("creatorElementType", GetElementTypeNonLocalizedName (creatorTypeID));
                if (creatorVariationID != APIVarId_Generic) {
                    details.Add ("creatorVariation", VariationIdToString (creatorVariationID));
                }
            }
        }

        libraryParts (details);
    }

    return response;
}

IsLibraryPartSubtypeOfCommand::IsLibraryPartSubtypeOfCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String IsLibraryPartSubtypeOfCommand::GetName () const
{
    return "IsLibraryPartSubtypeOf";
}

GS::Optional<GS::UniString> IsLibraryPartSubtypeOfCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraryParts": {
                "type": "array",
                "description": "The library parts to check.",
                "items": {
                    "$ref": "#/LibraryPartIdentifier"
                }
            },
            "ancestor": {
                "$ref": "#/LibraryPartIdentifier",
                "description": "The subtype to check against. Given by unId, it is used as it is, without checking that such a library part is loaded."
            },
            "matchAncestorByMainId": {
                "type": "boolean",
                "description": "Optional, false by default. When true, only the main GUID of the ancestor's unique ID is compared, so any revision of the ancestor matches; when false, the full unique ID (main and revision GUID) must match."
            }
        },
        "additionalProperties": false,
        "required": [
            "libraryParts",
            "ancestor"
        ]
    })";
}

GS::Optional<GS::UniString> IsLibraryPartSubtypeOfCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "ancestorUnId": {
                "type": "string",
                "description": "The full unique ID of the ancestor the library parts were checked against."
            },
            "results": {
                "type": "array",
                "description": "One result for each library part, in the order of the input.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "properties": {
                                "unId": { "type": "string", "description": "The full unique ID of the checked library part." },
                                "isSubtypeOf": { "type": "boolean", "description": "True if the ancestor is anywhere up the subtype hierarchy of the library part. False for the ancestor itself: a library part is not a subtype of itself." }
                            },
                            "additionalProperties": false,
                            "required": [ "unId", "isSubtypeOf" ]
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
            "ancestorUnId",
            "results"
        ]
    })";
}

GS::ObjectState IsLibraryPartSubtypeOfCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> identifiers;
    parameters.Get ("libraryParts", identifiers);

    GS::ObjectState ancestorIdentifier;
    parameters.Get ("ancestor", ancestorIdentifier);

    bool matchAncestorByMainId = false;
    parameters.Get ("matchAncestorByMainId", matchAncestorByMainId);

    GS::UniString ancestorUnId;
    if (!ancestorIdentifier.Get ("unId", ancestorUnId)) {
        API_LibPart ancestor = {};
        GS::UniString errorMessage;
        const GSErrCode err = ResolveLibraryPart (ancestorIdentifier, ancestor, errorMessage);
        if (err != NoError) {
            return CreateErrorResponse (err, GS::UniString ("Ancestor: ") + errorMessage);
        }
        ancestorUnId = GS::UniString (ancestor.ownUnID);
    }

    GS::ObjectState response;
    response.Add ("ancestorUnId", ancestorUnId);
    const auto& results = response.AddList<GS::ObjectState> ("results");

    for (const GS::ObjectState& identifier : identifiers) {
        API_LibPart libPart = {};
        GS::UniString errorMessage;
        GSErrCode err = ResolveLibraryPart (identifier, libPart, errorMessage);
        if (err != NoError) {
            results (CreateErrorResponse (err, errorMessage));
            continue;
        }

        // NoError means the ancestor is a strict ancestor, APIERR_NOTSUBTYPEOF that it is not
        // (also when the two unique IDs are equal).
        if (matchAncestorByMainId) {
            err = ACAPI_LibraryPart_CheckLibPartSubtypeOfbyMain (libPart.ownUnID, ancestorUnId.ToCStr ().Get ());
        } else {
            err = ACAPI_LibraryPart_CheckLibPartSubtypeOf (libPart.ownUnID, ancestorUnId.ToCStr ().Get ());
        }
        if (err != NoError && err != APIERR_NOTSUBTYPEOF) {
            results (CreateErrorResponse (err, "Failed to check the subtype of the library part."));
            continue;
        }

        results (GS::ObjectState (
            "unId", GS::UniString (libPart.ownUnID),
            "isSubtypeOf", err == NoError));
    }

    return response;
}

GetMissingLibPartNamesCommand::GetMissingLibPartNamesCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String GetMissingLibPartNamesCommand::GetName () const
{
    return "GetMissingLibPartNames";
}

GS::Optional<GS::UniString> GetMissingLibPartNamesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> GetMissingLibPartNamesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libPartNames": {
                "type": "array",
                "description": "The library part name of each given element, in the order of the input, or an error.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "$ref": "#/MissingLibraryPartName"
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
            "libPartNames"
        ]
    })";
}

GS::ObjectState GetMissingLibPartNamesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& libPartNames = response.AddList<GS::ObjectState> ("libPartNames");

    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            libPartNames (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        const API_Guid elemGuid = GetGuidFromObjectState (*elementId);
        API_Elem_Head elemHeader;
        if (!LoadElementHeaderByGuid (elemGuid, elemHeader)) {
            libPartNames (CreateErrorResponse (APIERR_BADID, "The given element was not found"));
            continue;
        }

        switch (GetElemTypeId (elemHeader)) {
            case API_ObjectID:
            case API_LampID:
            case API_WindowID:
            case API_DoorID:
                break;
            default:
                libPartNames (CreateErrorResponse (APIERR_BADPARS, "The given element is not an Object, Lamp, Window or Door"));
                continue;
        }

        GS::UniString libPartName;
        const GSErrCode err = ACAPI_LibraryPart_GetUnloadedLibpartName (&elemGuid, &libPartName);
        if (err != NoError) {
            libPartNames (CreateErrorResponse (err, "Failed to get the library part name of the element"));
            continue;
        }

        GS::ObjectState libPartNameOfElement = CreateElementIdObjectState (elemGuid);
        libPartNameOfElement.Add ("libPartName", libPartName);
        libPartNames (libPartNameOfElement);
    }

    return response;
}

GetUnusedLibraryPartsCommand::GetUnusedLibraryPartsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String GetUnusedLibraryPartsCommand::GetName () const
{
    return "GetUnusedLibraryParts";
}

GS::Optional<GS::UniString> GetUnusedLibraryPartsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "filterByTypeId": {
                "$ref": "#/LibraryPartType",
                "description": "Optional. Only list the unused library parts of this type (the typeId of GetAvailableLibraryParts)."
            },
            "includeNotPlaceable": {
                "type": "boolean",
                "description": "Also list the library parts that cannot be placed (macros, pictures, property objects, list schemes and the like), which are never placed on their own. Default: false."
            }
        },
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> GetUnusedLibraryPartsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "unusedLibraryParts": {
                "type": "array",
                "description": "The loaded library parts that no element of the project is placed from, in index order.",
                "items": {
                    "$ref": "#/UnusedLibraryPart"
                }
            },
            "usedLibraryPartCount": {
                "type": "integer",
                "description": "The number of different loaded library parts that are placed in the project."
            },
            "scannedElementCount": {
                "type": "integer",
                "description": "The number of elements checked, in the floor plan and every view and layout database."
            },
            "complete": {
                "type": "boolean",
                "description": "False when a view or layout database or a library part could not be read (see warnings): then a listed library part may still be used."
            },
            "warnings": {
                "type": "array",
                "items": { "type": "string" }
            }
        },
        "additionalProperties": false,
        "required": [
            "unusedLibraryParts",
            "usedLibraryPartCount",
            "scannedElementCount",
            "complete",
            "warnings"
        ]
    })";
}

// The library part the element itself is placed from, 0 when it has none: objects, lamps, windows, doors,
// skylights, zone stamps, symbol labels, drawing titles and curtain wall parts.
static Int32 GetPlacedLibIndOfElement (const API_Element& element)
{
    switch (GetElemTypeId (element.header)) {
        case API_ObjectID:                return element.object.libInd;
        case API_LampID:                  return element.lamp.libInd;
        case API_WindowID:                return element.window.openingBase.libInd;
        case API_DoorID:                  return element.door.openingBase.libInd;
        case API_SkylightID:              return element.skylight.openingBase.libInd;
        case API_ZoneID:                  return element.zone.libInd;
        // u.symbol shares its storage with u.text, so libInd is only meaningful for a symbol label.
        case API_LabelID:                 return element.label.labelClass == APILblClass_Symbol ? element.label.u.symbol.libInd : 0;
        case API_DrawingID:               return element.drawing.title.libInd;
        case API_CurtainWallFrameID:      return element.cwFrame.libInd;
        case API_CurtainWallPanelID:      return element.cwPanel.libInd;
        case API_CurtainWallJunctionID:   return element.cwJunction.libInd;
        case API_CurtainWallAccessoryID:  return element.cwAccessory.libInd;
        default:                          return 0;
    }
}

// The marker elements of the element (openings, sections, elevations, details and worksheets): each is an object
// element placed from the marker's library part.
static void GetMarkerGuidsOfElement (const API_Element& element, GS::Array<API_Guid>& markerGuids)
{
    switch (GetElemTypeId (element.header)) {
        case API_WindowID:      markerGuids.Push (element.window.openingBase.markGuid); break;
        case API_DoorID:        markerGuids.Push (element.door.openingBase.markGuid); break;
        case API_SkylightID:    markerGuids.Push (element.skylight.openingBase.markGuid); break;
        case API_CutPlaneID:
            markerGuids.Push (element.cutPlane.segment.begMarkerId);
            markerGuids.Push (element.cutPlane.segment.midMarkerId);
            markerGuids.Push (element.cutPlane.segment.endMarkerId);
            break;
        case API_ElevationID:
            markerGuids.Push (element.elevation.segment.begMarkerId);
            markerGuids.Push (element.elevation.segment.midMarkerId);
            markerGuids.Push (element.elevation.segment.endMarkerId);
            break;
        case API_DetailID:      markerGuids.Push (element.detail.markId); break;
        case API_WorksheetID:   markerGuids.Push (element.worksheet.markId); break;
        default:                break;
    }
}

static bool MayUseLibraryPart (API_ElemTypeID typeId)
{
    switch (typeId) {
        case API_ObjectID:
        case API_LampID:
        case API_WindowID:
        case API_DoorID:
        case API_SkylightID:
        case API_ZoneID:
        case API_LabelID:
        case API_DrawingID:
        case API_CurtainWallFrameID:
        case API_CurtainWallPanelID:
        case API_CurtainWallJunctionID:
        case API_CurtainWallAccessoryID:
        case API_CutPlaneID:
        case API_ElevationID:
        case API_DetailID:
        case API_WorksheetID:
            return true;
        default:
            return false;
    }
}

// The floor plan and the databases of every view and layout, which together hold every placed element.
static void GetDatabasesOfPlacedElements (GS::Array<API_DatabaseInfo>& databases, GS::Array<GS::UniString>& warnings)
{
    API_DatabaseInfo floorPlan = {};
    floorPlan.typeID = APIWind_FloorPlanID;
    if (ACAPI_Window_GetDatabaseInfo (&floorPlan) == NoError) {
        databases.Push (floorPlan);
    } else {
        warnings.Push ("The floor plan database could not be read.");
    }

    const struct {
        API_WindowTypeID typeID;
        const char*      name;
    } databaseTypes[] = {
        { APIWind_SectionID,           "section" },
        { APIWind_ElevationID,         "elevation" },
        { APIWind_InteriorElevationID, "interior elevation" },
        { APIWind_DetailID,            "detail" },
        { APIWind_WorksheetID,         "worksheet" },
        { APIWind_DocumentFrom3DID,    "3D document" },
        { APIWind_LayoutID,            "layout" },
        { APIWind_MasterLayoutID,      "master layout" }
    };
    for (const auto& databaseType : databaseTypes) {
        GS::Array<API_DatabaseUnId> databaseUnIds;
        GSErrCode err = NoError;
        switch (databaseType.typeID) {
            case APIWind_SectionID:           err = ACAPI_Database_GetSectionDatabases (nullptr, &databaseUnIds); break;
            case APIWind_ElevationID:         err = ACAPI_Database_GetElevationDatabases (nullptr, &databaseUnIds); break;
            case APIWind_InteriorElevationID: err = ACAPI_Database_GetInteriorElevationDatabases (nullptr, &databaseUnIds); break;
            case APIWind_DetailID:            err = ACAPI_Database_GetDetailDatabases (nullptr, &databaseUnIds); break;
            case APIWind_WorksheetID:         err = ACAPI_Database_GetWorksheetDatabases (nullptr, &databaseUnIds); break;
            case APIWind_DocumentFrom3DID:    err = ACAPI_Database_GetDocumentFrom3DDatabases (nullptr, &databaseUnIds); break;
            case APIWind_LayoutID:            err = ACAPI_Database_GetLayoutDatabases (nullptr, &databaseUnIds); break;
            case APIWind_MasterLayoutID:      err = ACAPI_Database_GetMasterLayoutDatabases (nullptr, &databaseUnIds); break;
            default:                          break;
        }
        if (err != NoError) {
            warnings.Push (GS::UniString::Printf ("The %s databases could not be listed (error %d).", databaseType.name, (int) err));
            continue;
        }
        for (const API_DatabaseUnId& databaseUnId : databaseUnIds) {
            API_DatabaseInfo database = {};
            database.typeID = databaseType.typeID;
            database.databaseUnId = databaseUnId;
            if (ACAPI_Window_GetDatabaseInfo (&database) != NoError) {
                warnings.Push (GS::UniString::Printf ("A %s database could not be read.", databaseType.name));
                continue;
            }
            databases.Push (database);
        }
    }
}

GS::ObjectState GetUnusedLibraryPartsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    API_LibTypeID filterTypeId = API_ZombieLibID;
    GS::UniString filterTypeIdStr;
    if (parameters.Get ("filterByTypeId", filterTypeIdStr) && !LibPartTypeIdFromString (filterTypeIdStr, filterTypeId)) {
        return CreateErrorResponse (APIERR_BADPARS, "Library part type '" + filterTypeIdStr + "' cannot be filtered by.");
    }
    bool includeNotPlaceable = false;
    parameters.Get ("includeNotPlaceable", includeNotPlaceable);

    Int32 partCount = 0;
    GSErrCode err = ACAPI_LibraryPart_GetNum (&partCount);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to enumerate library parts.");
    }

    API_DatabaseInfo startingDatabase = {};
    err = ACAPI_Database_GetCurrentDatabase (&startingDatabase);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the current database.");
    }

    GS::Array<GS::UniString> warnings;
    GS::Array<API_DatabaseInfo> databases;
    GetDatabasesOfPlacedElements (databases, warnings);

    GS::HashSet<Int32> usedLibInds;
    Int32 scannedElementCount = 0;
    for (const API_DatabaseInfo& database : databases) {
        API_DatabaseInfo current = database;
        if (ACAPI_Database_ChangeCurrentDatabase (&current) != NoError) {
            warnings.Push ("The view or layout database '" + GS::UniString (database.name) + "' could not be opened, so its elements were not checked.");
            continue;
        }
        GS::Array<API_Guid> guids;
        if (ACAPI_Element_GetElemList (API_ZombieElemID, &guids) != NoError) {
            warnings.Push ("The elements of the database '" + GS::UniString (database.name) + "' could not be listed.");
            continue;
        }
        for (const API_Guid& guid : guids) {
            ++scannedElementCount;
            API_Elem_Head head = {};
            head.guid = guid;
            if (ACAPI_Element_GetHeader (&head) != NoError || !MayUseLibraryPart (GetElemTypeId (head))) {
                continue;
            }
            API_Element element = {};
            element.header.guid = guid;
            if (ACAPI_Element_Get (&element) != NoError) {
                continue;
            }
            const Int32 libInd = GetPlacedLibIndOfElement (element);
            if (libInd > 0) {
                usedLibInds.Add (libInd);
            }
            GS::Array<API_Guid> markerGuids;
            GetMarkerGuidsOfElement (element, markerGuids);
            for (const API_Guid& markerGuid : markerGuids) {
                if (markerGuid == APINULLGuid) {
                    continue;
                }
                API_Element marker = {};
                marker.header.guid = markerGuid;
                if (ACAPI_Element_Get (&marker) == NoError) {
                    const Int32 markerLibInd = GetPlacedLibIndOfElement (marker);
                    if (markerLibInd > 0) {
                        usedLibInds.Add (markerLibInd);
                    }
                }
            }
        }
    }

    err = ACAPI_Database_ChangeCurrentDatabase (&startingDatabase);
    if (err != NoError) {
        warnings.Push ("The database that was current before the command could not be made current again.");
    }

    GS::ObjectState response;
    const auto& unusedAdder = response.AddList<GS::ObjectState> ("unusedLibraryParts");
    Int32 usedLibraryPartCount = 0;
    Int32 unreadCount = 0;
    for (Int32 i = 1; i <= partCount; ++i) {
        API_LibPart libPart = {};
        libPart.index = i;
        if (ACAPI_LibraryPart_Get (&libPart) != NoError) {
            // libPart.location is not allocated on failure.
            ++unreadCount;
            continue;
        }
        const bool isPlaceable = libPart.isPlaceable;
        const bool isMissing = libPart.missingDef;
        GS::ObjectState entry;
        entry.Add ("guid", GS::UnID (libPart.ownUnID).GetMainGuid ().ToUniString ());
        entry.Add ("index", static_cast<Int32> (libPart.index));
        entry.Add ("documentName", GS::UniString (libPart.docu_UName));
        entry.Add ("fileName", GS::UniString (libPart.file_UName));
        entry.Add ("typeId", LibPartTypeIdToString (libPart.typeID));
        entry.Add ("isPlaceable", isPlaceable);
        const API_LibTypeID typeId = libPart.typeID;
        // ACAPI_LibraryPart_Get allocates libPart.location.
        delete libPart.location;
        libPart.location = nullptr;

        if (usedLibInds.Contains (i)) {
            ++usedLibraryPartCount;
            continue;
        }
        if (isMissing || (!includeNotPlaceable && !isPlaceable) || (filterTypeId != API_ZombieLibID && typeId != filterTypeId)) {
            continue;
        }
        unusedAdder (entry);
    }
    if (unreadCount > 0) {
        warnings.Push (GS::UniString::Printf ("%d library parts could not be read, so they were not checked.", (int) unreadCount));
    }

    response.Add ("usedLibraryPartCount", usedLibraryPartCount);
    response.Add ("scannedElementCount", scannedElementCount);
    response.Add ("complete", warnings.IsEmpty ());
    const auto& warningAdder = response.AddList<GS::UniString> ("warnings");
    for (const GS::UniString& warning : warnings) {
        warningAdder (warning);
    }
    return response;
}

#ifdef ServerMainVers_2800

namespace {

// Which getter of GDL::Parameters reads a value of the type, and its JSON form.
enum class LibPartParameterValueKind {
    None,
    Integer,
    Number,
    OnOff,
    Boolean,
    String
};

}

// The type name is the one GetGDLParametersOfElements uses.
static LibPartParameterValueKind GetLibPartParameterValueKind (short type, const char** typeName)
{
    // GetVarType returns the ParT_ codes of GDLDefs.h, which differ from API_AddParID.
    switch (type) {
        case ParT_Integer:          *typeName = "Integer";          return LibPartParameterValueKind::Integer;
        case ParT_Length:           *typeName = "Length";           return LibPartParameterValueKind::Number;
        case ParT_Angle:            *typeName = "Angle";            return LibPartParameterValueKind::Number;
        case ParT_RealNum:          *typeName = "RealNumber";       return LibPartParameterValueKind::Number;
        case ParT_LightSw:          *typeName = "LightSwitch";      return LibPartParameterValueKind::OnOff;
        case ParT_ColRGB:           *typeName = "RGBColor";         return LibPartParameterValueKind::Number;
        case ParT_Intens:           *typeName = "Intensity";        return LibPartParameterValueKind::Number;
        case ParT_LineTyp:          *typeName = "LineType";         return LibPartParameterValueKind::Integer;
        case ParT_Mater:            *typeName = "Material";         return LibPartParameterValueKind::Integer;
        case ParT_FillPat:          *typeName = "FillPattern";      return LibPartParameterValueKind::Integer;
        case ParT_PenCol:           *typeName = "PenColor";         return LibPartParameterValueKind::Integer;
        case ParT_CString:          *typeName = "String";           return LibPartParameterValueKind::String;
        case ParT_Boolean:          *typeName = "Boolean";          return LibPartParameterValueKind::Boolean;
        case ParT_BuildingMaterial: *typeName = "BuildingMaterial"; return LibPartParameterValueKind::Integer;
        case ParT_Profile:          *typeName = "Profile";          return LibPartParameterValueKind::Integer;
        default:                    *typeName = nullptr;            return LibPartParameterValueKind::None;
    }
}

// The parameters of the list in the format of GetGDLParametersOfElements: name, type, value and,
// for an array, dimension1 and dimension2 with its items as a flat list, the first index outer.
// Separators, titles and dictionaries have no value read this way, so they are left out.
// The description is the one of the same index in `descriptions`, when there is one.
static GS::Array<GS::ObjectState> LibPartParametersToObjectStates (const GDL::Parameters& gdlParameters, const std::vector<GS::UniString>& descriptions)
{
    GS::Array<GS::ObjectState> result;
    const Int32 varNumber = gdlParameters.GetVarNumber ();
    for (Int32 index = 0; index < varNumber; ++index) {
        bool isArray = false;
        Int32 dim1 = 0;
        Int32 dim2 = 0;
        const short type = gdlParameters.GetVarType (index, &isArray, &dim1, &dim2);
        const char* typeName = nullptr;
        const LibPartParameterValueKind kind = GetLibPartParameterValueKind (type, &typeName);
        const char* name = gdlParameters.GetVarNameRef (index);
        if (kind == LibPartParameterValueKind::None || name == nullptr) {
            continue;
        }

        GS::ObjectState parameter;
        parameter.Add ("name", GS::String (name));
        parameter.Add ("type", GS::String (typeName));

        if (!isArray) {
            switch (kind) {
                case LibPartParameterValueKind::Integer:
                    parameter.Add ("value", gdlParameters.GetIntVar (index));
                    break;
                case LibPartParameterValueKind::Number:
                    parameter.Add ("value", gdlParameters.GetNumVar (index));
                    break;
                case LibPartParameterValueKind::OnOff:
                    parameter.Add ("value", GS::String (gdlParameters.GetNumVar (index) == 0.0 ? "Off" : "On"));
                    break;
                case LibPartParameterValueKind::Boolean:
                    parameter.Add ("value", gdlParameters.GetBoolVar (index));
                    break;
                case LibPartParameterValueKind::String: {
                    GS::UniString value;
                    gdlParameters.GetUStrVar (index, value);
                    parameter.Add ("value", value);
                    break;
                }
                default:
                    break;
            }
        } else {
            // A one-dimensional array has no second dimension; its items are read as the first column.
            const Int32 columns = dim2 > 0 ? dim2 : 1;
            parameter.Add ("dimension1", dim1);
            parameter.Add ("dimension2", columns);
            switch (kind) {
                case LibPartParameterValueKind::Integer: {
                    const auto& itemAdder = parameter.AddList<Int32> ("value");
                    for (Int32 i1 = 1; i1 <= dim1; ++i1) {
                        for (Int32 i2 = 1; i2 <= columns; ++i2) {
                            itemAdder (gdlParameters.GetIntArrayVal (index, i1, i2));
                        }
                    }
                    break;
                }
                case LibPartParameterValueKind::Number: {
                    const auto& itemAdder = parameter.AddList<double> ("value");
                    for (Int32 i1 = 1; i1 <= dim1; ++i1) {
                        for (Int32 i2 = 1; i2 <= columns; ++i2) {
                            itemAdder (gdlParameters.GetNumArrayVal (index, i1, i2));
                        }
                    }
                    break;
                }
                case LibPartParameterValueKind::OnOff: {
                    const auto& itemAdder = parameter.AddList<GS::String> ("value");
                    for (Int32 i1 = 1; i1 <= dim1; ++i1) {
                        for (Int32 i2 = 1; i2 <= columns; ++i2) {
                            itemAdder (GS::String (gdlParameters.GetNumArrayVal (index, i1, i2) == 0.0 ? "Off" : "On"));
                        }
                    }
                    break;
                }
                case LibPartParameterValueKind::Boolean: {
                    const auto& itemAdder = parameter.AddList<bool> ("value");
                    for (Int32 i1 = 1; i1 <= dim1; ++i1) {
                        for (Int32 i2 = 1; i2 <= columns; ++i2) {
                            itemAdder (gdlParameters.GetBoolArrayVal (index, i1, i2));
                        }
                    }
                    break;
                }
                case LibPartParameterValueKind::String: {
                    const auto& itemAdder = parameter.AddList<GS::UniString> ("value");
                    for (Int32 i1 = 1; i1 <= dim1; ++i1) {
                        for (Int32 i2 = 1; i2 <= columns; ++i2) {
                            GS::UniString value;
                            gdlParameters.GetUStrArrayVal (index, i1, i2, value);
                            itemAdder (value);
                        }
                    }
                    break;
                }
                default:
                    break;
            }
        }

        if (static_cast<size_t> (index) < descriptions.size ()) {
            parameter.Add ("description", descriptions[static_cast<size_t> (index)]);
        }
        result.Push (parameter);
    }
    return result;
}

static GS::Array<GS::UniString> ToUniStringArray (const std::vector<GS::UniString>& strings)
{
    GS::Array<GS::UniString> result;
    for (const GS::UniString& str : strings) {
        result.Push (str);
    }
    return result;
}

static GS::ObjectState LibPartToObjectState (const ACLib::LibraryManager& libraryManager, const ACLib::LibPart& libPart)
{
    GS::ObjectState libPartData;
    auto name = libPart.GetName ();
    if (name.IsOk ()) {
        libPartData.Add ("name", *name);
    }
    auto location = libPart.GetLocation ();
    if (location.IsOk ()) {
        libPartData.Add ("path", location->ToDisplayText ());
    }
    auto index = libraryManager.GetLibIndOfLibPart (libPart);
    if (index.IsOk ()) {
        libPartData.Add ("index", *index);
    }
    return libPartData;
}

static GS::ObjectState LibraryTreePathToObjectState (const ACLib::LibraryManager& libraryManager, const ACLib::LibraryTreePath& treePath)
{
    GS::ObjectState treePathData;
    treePathData.Add ("parts", ToUniStringArray (treePath.GetParts ()));
    treePathData.Add ("lastPart", treePath.GetLastPart ());
    treePathData.Add ("isRoot", treePath.IsRoot ());
    if (!treePath.IsRoot ()) {
        treePathData.Add ("parentParts", ToUniStringArray (treePath.GetParent ().GetParts ()));
    }
    auto teamworkId = libraryManager.GetTWID (treePath);
    if (teamworkId.IsOk ()) {
        treePathData.Add ("teamworkId", teamworkId->ToUniString ());
    }
    return treePathData;
}

static GS::ObjectState LibraryToObjectState (const ACLib::Library& library)
{
    GS::ObjectState libraryData;
    auto name = library.GetName ();
    if (name.IsOk ()) {
        libraryData.Add ("name", *name);
    }
#ifdef ServerMainVers_2900
    auto localizedName = library.GetLocalizedName ();
    if (localizedName.IsOk () && localizedName->HasValue ()) {
        libraryData.Add ("localizedName", localizedName->Get ());
    }
#endif
    auto location = library.GetLocation ();
    if (location.IsOk ()) {
        libraryData.Add ("path", location->ToDisplayText ());
    }
    auto isServer = library.IsServer ();
    if (isServer.IsOk ()) {
        libraryData.Add ("isServer", *isServer);
        if (*isServer) {
            auto twServerPath = library.GetTWServerPath ();
            if (twServerPath.IsOk ()) {
                libraryData.Add ("twServerPath", *twServerPath);
            }
            auto twServerUrl = library.GetTWServerUrl ();
            if (twServerUrl.IsOk ()) {
                libraryData.Add ("twServerUrl", *twServerUrl);
            }
        }
    }
    return libraryData;
}

static GS::ObjectState GSMObjectToObjectState (const ACLib::GSMObject& gsmObject, const std::optional<GS::UnID>& subTypeToCheck, bool includeScripts, const GS::UniString& password)
{
    GS::ObjectState gsmObjectData;

    auto unId = gsmObject.GetUnID ();
    if (unId.IsOk ()) {
        gsmObjectData.Add ("guid", unId->GetMainGuid ().ToUniString ());
    }
    auto isSubType = gsmObject.IsSubType ();
    if (isSubType.IsOk ()) {
        gsmObjectData.Add ("isSubType", *isSubType);
    }
    auto subTypeUnId = gsmObject.GetSubTypeUnID ();
    if (subTypeUnId.IsOk ()) {
        gsmObjectData.Add ("subTypeGuid", subTypeUnId->GetMainGuid ().ToUniString ());
    }
    if (subTypeToCheck.has_value ()) {
        auto isInSubType = gsmObject.IsInSubTypeOf (subTypeToCheck.value ());
        if (isInSubType.IsOk ()) {
            gsmObjectData.Add ("isInSubTypeOf", *isInSubType);
        }
    }
    auto isUIDefault = gsmObject.IsUIDefault ();
    if (isUIDefault.IsOk ()) {
        gsmObjectData.Add ("isUIDefault", *isUIDefault);
    }
    auto license = gsmObject.GetLicense ();
    if (license.IsOk ()) {
        gsmObjectData.Add ("license", *license);
    }
    auto licenseVersion = gsmObject.GetLicenseVersion ();
    if (licenseVersion.IsOk ()) {
        gsmObjectData.Add ("licenseVersion", *licenseVersion);
    }

    GS::ObjectState contents;
    contents.Add ("interfaceScript", gsmObject.HasInterfaceScript ());
    contents.Add ("forwardMigrationScript", gsmObject.HasForwardMigrationScript ());
    contents.Add ("backwardMigrationScript", gsmObject.HasBackwardMigrationScript ());
    contents.Add ("2dVectorImage", gsmObject.Has2DVectorImage ());
    contents.Add ("previewPicture", gsmObject.HasPreviewPicture ());
    contents.Add ("gdlPictures", gsmObject.HasGDLPictures ());
#ifdef ServerMainVers_2900
    contents.Add ("darkPreviewPicture", gsmObject.HasDarkPreviewPicture ());
    contents.Add ("darkGdlPictures", gsmObject.HasDarkGDLPictures ());
#endif
    gsmObjectData.Add ("contents", contents);

    if (includeScripts) {
        GS::ObjectState scripts;
        if (gsmObject.HasInterfaceScript ()) {
            auto script = gsmObject.GetInterfaceScript (password);
            if (script.IsOk ()) {
                scripts.Add ("interface", *script);
            }
        }
        if (gsmObject.HasForwardMigrationScript ()) {
            auto script = gsmObject.GetForwardMigrationScript (password);
            if (script.IsOk ()) {
                scripts.Add ("forwardMigration", *script);
            }
        }
        if (gsmObject.HasBackwardMigrationScript ()) {
            auto script = gsmObject.GetBackwardMigrationScript (password);
            if (script.IsOk ()) {
                scripts.Add ("backwardMigration", *script);
            }
        }
        gsmObjectData.Add ("scripts", scripts);
    }

    return gsmObjectData;
}

#endif

FindLibraryPartsCommand::FindLibraryPartsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String FindLibraryPartsCommand::GetName () const
{
    return "FindLibraryParts";
}

GS::Optional<GS::UniString> FindLibraryPartsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "kind": {
                "$ref": "#/LibraryPartKind"
            },
            "nameContains": {
                "type": "string",
                "description": "Optional. Only library parts whose name contains this text, ignoring case."
            },
            "firstOnly": {
                "type": "boolean",
                "description": "Optional. When true, only the first matching library part is returned. By default false."
            }
        },
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> FindLibraryPartsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraryParts": {
                "type": "array",
                "description": "The matching library parts.",
                "items": {
                    "$ref": "#/FoundLibraryPart"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "libraryParts"
        ]
    })";
}

GS::ObjectState FindLibraryPartsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::UniString kind ("All");
    parameters.Get ("kind", kind);

    std::function<bool (const ACLib::LibPart&)> kindSelector;
    if (kind == "All") {
        kindSelector = ACLib::SelectAllLibParts;
    } else if (kind == "Object") {
        kindSelector = ACLib::SelectObjectLibParts;
    } else if (kind == "Image") {
        kindSelector = ACLib::SelectImageLibParts;
    } else if (kind == "Text") {
        kindSelector = ACLib::SelectTextLibParts;
    } else {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid 'kind' parameter: " + kind);
    }

    GS::UniString nameContains;
    parameters.Get ("nameContains", nameContains);
#ifdef ServerMainVers_3000
    const GS::UniString nameFilter = nameContains.GetLowerCased ();
#else
    const GS::UniString nameFilter = nameContains.ToLowerCase ();
#endif

    bool firstOnly = false;
    parameters.Get ("firstOnly", firstOnly);

    auto libraryManager = ACLib::GetLibraryManager ();
    if (libraryManager.IsErr ()) {
        return CreateErrorResponse (libraryManager.UnwrapErr ().kind, "Failed to get the Library Manager.");
    }

    const std::function<bool (const ACLib::LibPart&)> selector = [&] (const ACLib::LibPart& libPart) -> bool {
        if (!kindSelector (libPart)) {
            return false;
        }
        if (nameFilter.IsEmpty ()) {
            return true;
        }
        auto name = libPart.GetName ();
#ifdef ServerMainVers_3000
        return name.IsOk () && name->GetLowerCased ().Contains (nameFilter);
#else
        return name.IsOk () && name->ToLowerCase ().Contains (nameFilter);
#endif
    };

    GS::ObjectState response;
    const auto& libraryParts = response.AddList<GS::ObjectState> ("libraryParts");

    if (firstOnly) {
        auto libPart = libraryManager->FindLibPart (selector);
        if (libPart.IsOk ()) {
            libraryParts (LibPartToObjectState (*libraryManager, *libPart));
        }
    } else {
        for (const ACLib::LibPart& libPart : libraryManager->FindLibParts (selector)) {
            libraryParts (LibPartToObjectState (*libraryManager, libPart));
        }
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

GetLibraryPartDetailsCommand::GetLibraryPartDetailsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetLibraryPartDetailsCommand::GetName () const
{
    return "GetLibraryPartDetails";
}

GS::Optional<GS::UniString> GetLibraryPartDetailsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraryParts": {
                "type": "array",
                "description": "The library parts to read, each by its name or by its index.",
                "items": {
                    "$ref": "#/LibraryPartNameOrIndex"
                }
            },
            "subTypeOf": {
                "type": "string",
                "description": "Optional. The name of another loaded library part, e.g. a subtype template. When given, the isInSubTypeOf field of each library part tells whether it is in that subtype."
            },
            "includeScripts": {
                "type": "boolean",
                "description": "Optional. When true, the interface script and the forward and backward migration scripts are returned too. By default false."
            }
        },
        "additionalProperties": false,
        "required": [
            "libraryParts"
        ]
    })";
}

GS::Optional<GS::UniString> GetLibraryPartDetailsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraryParts": {
                "type": "array",
                "description": "The details of each given library part, or an error, in the order of the input. A field is omitted when Archicad cannot return it for that library part.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "$ref": "#/LibraryPartDetails"
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

GS::ObjectState GetLibraryPartDetailsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::Array<GS::ObjectState> libraryParts;
    if (!parameters.Get ("libraryParts", libraryParts)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'libraryParts' parameter.");
    }

    bool includeScripts = false;
    parameters.Get ("includeScripts", includeScripts);

    auto libraryManager = ACLib::GetLibraryManager ();
    if (libraryManager.IsErr ()) {
        return CreateErrorResponse (libraryManager.UnwrapErr ().kind, "Failed to get the Library Manager.");
    }

    std::optional<GS::UnID> subTypeToCheck;
    GS::UniString subTypeOf;
    if (parameters.Get ("subTypeOf", subTypeOf)) {
        auto subTypeLibPart = libraryManager->GetLibPartByName (subTypeOf);
        if (subTypeLibPart.IsErr ()) {
            return CreateErrorResponse (subTypeLibPart.UnwrapErr ().kind, "No loaded library part with the name '" + subTypeOf + "' given in subTypeOf.");
        }
        auto subTypeGSMObject = libraryManager->GetGSMObjectOfLibPart (*subTypeLibPart);
        if (subTypeGSMObject.IsErr ()) {
            return CreateErrorResponse (subTypeGSMObject.UnwrapErr ().kind, "Failed to get the GSM object of the library part '" + subTypeOf + "' given in subTypeOf.");
        }
        auto subTypeUnId = subTypeGSMObject->GetUnID ();
        if (subTypeUnId.IsErr ()) {
            return CreateErrorResponse (subTypeUnId.UnwrapErr ().kind, "Failed to get the identifier of the library part '" + subTypeOf + "' given in subTypeOf.");
        }
        subTypeToCheck.emplace (*subTypeUnId);
    }

    GS::ObjectState response;
    const auto& detailsAdder = response.AddList<GS::ObjectState> ("libraryParts");

    for (const GS::ObjectState& libraryPart : libraryParts) {
        GS::UniString name;
        Int32 index = 0;
        const bool hasName = libraryPart.Get ("name", name) && !name.IsEmpty ();
        if (!hasName && !libraryPart.Get ("index", index)) {
            detailsAdder (CreateErrorResponse (APIERR_BADPARS, "Missing 'name' or 'index' parameter."));
            continue;
        }

        auto libPart = [&] () {
            if (hasName) {
                return libraryManager->GetLibPartByName (name);
            }
            return libraryManager->GetLibPartByLibInd (index);
        } ();
        if (libPart.IsErr ()) {
            detailsAdder (CreateErrorResponse (libPart.UnwrapErr ().kind, hasName ?
                "No loaded library part with the name '" + name + "'. Use the fileName (with extension) returned by GetAvailableLibraryParts." :
                "No loaded library part with the index " + GS::ValueToUniString (index) + "."));
            continue;
        }

        GS::ObjectState details = LibPartToObjectState (*libraryManager, *libPart);

        auto treePath = libraryManager->GetLibraryTreePathOfLibPart (*libPart);
        if (treePath.IsOk ()) {
            details.Add ("libraryTreePath", LibraryTreePathToObjectState (*libraryManager, *treePath));
        }

        auto library = libraryManager->GetLibraryOfLibPart (*libPart);
        if (library.IsOk ()) {
            details.Add ("library", LibraryToObjectState (*library));
        }

        auto gsmObject = libraryManager->GetGSMObjectOfLibPart (*libPart);
        if (gsmObject.IsOk ()) {
            GS::UniString password;
            libraryPart.Get ("password", password);
            details.Add ("gsmObject", GSMObjectToObjectState (*gsmObject, subTypeToCheck, includeScripts, password));
        }

        detailsAdder (details);
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

ModifyLibraryListCommand::ModifyLibraryListCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyLibraryListCommand::GetName () const
{
    return "ModifyLibraryList";
}

GS::Optional<GS::UniString> ModifyLibraryListCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "add": {
                "type": "array",
                "description": "Library folders or container files to add, by absolute path.",
                "items": {
                    "$ref": "#/LibraryLocation"
                }
            },
            "remove": {
                "type": "array",
                "description": "Loaded libraries to remove, each by its name or by its path.",
                "items": {
                    "$ref": "#/LibraryToRemove"
                }
            }
        },
        "additionalProperties": false,
        "minProperties": 1
    })";
}

GS::Optional<GS::UniString> ModifyLibraryListCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "addResults": {
                "$ref": "#/ExecutionResults"
            },
            "removeResults": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "addResults",
            "removeResults"
        ]
    })";
}

GS::ObjectState ModifyLibraryListCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::Array<GS::ObjectState> additions;
    GS::Array<GS::ObjectState> removals;
    parameters.Get ("add", additions);
    parameters.Get ("remove", removals);
    if (additions.IsEmpty () && removals.IsEmpty ()) {
        return CreateErrorResponse (APIERR_BADPARS, "Nothing to add or remove.");
    }

    auto libraryManager = ACLib::GetLibraryManager ();
    if (libraryManager.IsErr ()) {
        return CreateErrorResponse (libraryManager.UnwrapErr ().kind, "Failed to get the Library Manager.");
    }

    // Every library to remove is looked up before the list changes; then one
    // modification applies all additions and removals.
    std::vector<std::optional<GS::UniString>> pathsToAdd;
    std::vector<GS::ObjectState> addResults (additions.GetSize ());
    for (UIndex i = 0; i < additions.GetSize (); ++i) {
        GS::UniString path;
        if (!additions[i].Get ("path", path) || path.IsEmpty ()) {
            pathsToAdd.push_back (std::nullopt);
            addResults[i] = CreateFailedExecutionResult (APIERR_BADPARS, "Missing 'path' parameter.");
            continue;
        }
        pathsToAdd.push_back (path);
    }

    std::vector<std::optional<ACLib::Library>> librariesToRemove;
    std::vector<GS::ObjectState> removeResults (removals.GetSize ());
    for (UIndex i = 0; i < removals.GetSize (); ++i) {
        GS::UniString name;
        GS::UniString path;
        if (removals[i].Get ("name", name) && !name.IsEmpty ()) {
            auto library = libraryManager->GetLibraryByName (name);
            if (library.IsErr ()) {
                librariesToRemove.push_back (std::nullopt);
                removeResults[i] = CreateFailedExecutionResult (library.UnwrapErr ().kind, "No loaded library with the name '" + name + "'.");
                continue;
            }
            librariesToRemove.push_back (*library);
        } else if (removals[i].Get ("path", path) && !path.IsEmpty ()) {
            const IO::Location location (path);
            auto library = libraryManager->FindLibrary ([&] (const ACLib::Library& lib) -> bool {
                auto libraryLocation = lib.GetLocation ();
                return libraryLocation.IsOk () && SameLocation (*libraryLocation, location);
            });
            if (library.IsErr ()) {
                librariesToRemove.push_back (std::nullopt);
                removeResults[i] = CreateFailedExecutionResult (library.UnwrapErr ().kind, "No loaded library on the path '" + path + "'.");
                continue;
            }
            librariesToRemove.push_back (*library);
        } else {
            librariesToRemove.push_back (std::nullopt);
            removeResults[i] = CreateFailedExecutionResult (APIERR_BADPARS, "Missing 'name' or 'path' parameter.");
        }
    }

    std::vector<GSErrCode> addErrors (pathsToAdd.size (), NoError);
    std::vector<GSErrCode> removeErrors (librariesToRemove.size (), NoError);
    auto modifyResult = libraryManager->ModifyLibraryList ([&] (ACLib::LibraryManager::Modifier& modifier) -> GSErrCode {
        for (size_t i = 0; i < pathsToAdd.size (); ++i) {
            if (pathsToAdd[i].has_value ()) {
                auto result = modifier.AddLibrary (IO::Location (pathsToAdd[i].value ()));
                addErrors[i] = result.IsOk () ? NoError : result.UnwrapErr ().kind;
            }
        }
        for (size_t i = 0; i < librariesToRemove.size (); ++i) {
            if (librariesToRemove[i].has_value ()) {
                auto result = modifier.RemoveLibrary (librariesToRemove[i].value ());
                removeErrors[i] = result.IsOk () ? NoError : result.UnwrapErr ().kind;
            }
        }
        return NoError;
    });
    const GSErrCode modifyError = modifyResult.IsOk () ? NoError : modifyResult.UnwrapErr ().kind;

    GS::ObjectState response;
    const auto& addResultsAdder = response.AddList<GS::ObjectState> ("addResults");
    for (size_t i = 0; i < pathsToAdd.size (); ++i) {
        if (!pathsToAdd[i].has_value ()) {
            addResultsAdder (addResults[i]);
        } else if (addErrors[i] != NoError) {
            addResultsAdder (CreateFailedExecutionResult (addErrors[i], "Failed to add the library on the path '" + pathsToAdd[i].value () + "'."));
        } else if (modifyError != NoError) {
            addResultsAdder (CreateFailedExecutionResult (modifyError, "Failed to modify the library list."));
        } else {
            addResultsAdder (CreateSuccessfulExecutionResult ());
        }
    }
    const auto& removeResultsAdder = response.AddList<GS::ObjectState> ("removeResults");
    for (size_t i = 0; i < librariesToRemove.size (); ++i) {
        if (!librariesToRemove[i].has_value ()) {
            removeResultsAdder (removeResults[i]);
        } else if (removeErrors[i] != NoError) {
            removeResultsAdder (CreateFailedExecutionResult (removeErrors[i], "Failed to remove the library."));
        } else if (modifyError != NoError) {
            removeResultsAdder (CreateFailedExecutionResult (modifyError, "Failed to modify the library list."));
        } else {
            removeResultsAdder (CreateSuccessfulExecutionResult ());
        }
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

GetLibraryPartMetadataCommand::GetLibraryPartMetadataCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetLibraryPartMetadataCommand::GetName () const
{
    return "GetLibraryPartMetadata";
}

GS::Optional<GS::UniString> GetLibraryPartMetadataCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraryParts": {
                "type": "array",
                "description": "The library parts to read the metadata of.",
                "items": {
                    "$ref": "#/LibraryPartName"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "libraryParts"
        ]
    })";
}

GS::Optional<GS::UniString> GetLibraryPartMetadataCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraryParts": {
                "type": "array",
                "description": "The metadata of each given library part, or an error, in the order of the input.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "$ref": "#/LibraryPartMetadataDetails"
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

#ifdef ServerMainVers_2800
template <typename T>
static bool AddLibraryPartMetadataField (GS::ObjectState& metadata, const char* key, const ACAPI::Result<T>& result, const GS::UniString& name, GS::ObjectState& error)
{
    if (result.IsErr ()) {
        GS::UniString errorMessage = "Failed to read the " + GS::UniString (key) + " of the library part '" + name + "'";
        const GS::UniString errorText (result.UnwrapErr ().text.c_str ());
        if (!errorText.IsEmpty ()) {
            errorMessage += " (" + errorText + ")";
        }
        errorMessage += ".";
        error = CreateErrorResponse (result.UnwrapErr ().kind, errorMessage);
        return false;
    }
    metadata.Add (key, result.Unwrap ());
    return true;
}
#endif

GS::ObjectState GetLibraryPartMetadataCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
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
    const auto& metadataAdder = response.AddList<GS::ObjectState> ("libraryParts");

    for (const GS::ObjectState& libraryPart : libraryParts) {
        GS::UniString name;
        if (!libraryPart.Get ("name", name) || name.IsEmpty ()) {
            metadataAdder (CreateErrorResponse (APIERR_BADPARS, "Missing 'name' parameter."));
            continue;
        }

        auto libPart = libraryManager->GetLibPartByName (name);
        if (libPart.IsErr ()) {
            metadataAdder (CreateErrorResponse (libPart.UnwrapErr ().kind, "No loaded library part with the name '" + name + "'. Use the fileName (with extension) returned by GetAvailableLibraryParts."));
            continue;
        }

        auto gsmObject = libraryManager->GetGSMObjectOfLibPart (libPart.Unwrap ());
        if (gsmObject.IsErr ()) {
            metadataAdder (CreateErrorResponse (gsmObject.UnwrapErr ().kind, "Failed to get the GSM object of the library part '" + name + "'."));
            continue;
        }

        GS::ObjectState metadata;
        GS::ObjectState error;
        if (AddLibraryPartMetadataField (metadata, "description", gsmObject->GetDescription (), name, error) &&
            AddLibraryPartMetadataField (metadata, "keywords", gsmObject->GetKeywords (), name, error) &&
            AddLibraryPartMetadataField (metadata, "author", gsmObject->GetAuthor (), name, error) &&
            AddLibraryPartMetadataField (metadata, "isPlaceable", gsmObject->IsPlaceable (), name, error) &&
            AddLibraryPartMetadataField (metadata, "isPasswordProtected", gsmObject->IsPasswordProtected (), name, error) &&
            AddLibraryPartMetadataField (metadata, "isHierarchicalPagesUsed", gsmObject->IsHierarchicalPagesUsed (), name, error)) {
            metadataAdder (metadata);
        } else {
            metadataAdder (error);
        }
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

GetLibraryPartOriginalLocationsCommand::GetLibraryPartOriginalLocationsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetLibraryPartOriginalLocationsCommand::GetName () const
{
    return "GetLibraryPartOriginalLocations";
}

GS::Optional<GS::UniString> GetLibraryPartOriginalLocationsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraryParts": {
                "type": "array",
                "description": "The library parts to look up.",
                "items": {
                    "$ref": "#/LibraryPartByIndex"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "libraryParts"
        ]
    })";
}

GS::Optional<GS::UniString> GetLibraryPartOriginalLocationsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "locations": {
                "type": "array",
                "description": "The original location of each given library part, in the order of the input.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "$ref": "#/LibraryPartOriginalLocation"
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
            "locations"
        ]
    })";
}

GS::ObjectState GetLibraryPartOriginalLocationsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    Int32 partCount = 0;
    GSErrCode err = ACAPI_LibraryPart_GetNum (&partCount);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to enumerate library parts.");
    }

    GS::Array<GS::ObjectState> libraryParts;
    parameters.Get ("libraryParts", libraryParts);

    GS::ObjectState response;
    const auto& locationAdder = response.AddList<GS::ObjectState> ("locations");

    for (const GS::ObjectState& libraryPart : libraryParts) {
        Int32 index = 0;
        if (!libraryPart.Get ("index", index)) {
            locationAdder (CreateErrorResponse (APIERR_BADPARS, "index is missing."));
            continue;
        }
        if (index < 1 || index > partCount) {
            locationAdder (CreateErrorResponse (APIERR_BADINDEX, "There is no library part with the given index."));
            continue;
        }

        IO::Location origLocation;
        err = ACAPI_LibraryManagement_GetLibPartOrigLocation (&index, &origLocation);
        if (err != NoError) {
            locationAdder (CreateErrorResponse (err, "Failed to get the original location of the library part."));
            continue;
        }

        GS::ObjectState location;
        location.Add ("path", origLocation.ToDisplayText ());
        locationAdder (location);
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
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

// A section type is a four-character code such as 'TEXT'; it is given and returned as that string.
static GS::UniString SectionTypeToString (GSType sectType)
{
    char code[5] = {};
    for (int i = 0; i < 4; ++i) {
        code[i] = static_cast<char> ((sectType >> (8 * (3 - i))) & 0xFF);
    }
    return GS::UniString (code);
}

// A code shorter than four characters is padded with spaces, as the API's own codes are.
static bool SectionTypeFromString (const GS::UniString& str, GSType& sectType)
{
    const std::string code (str.ToCStr ().Get ());
    if (code.empty () || code.size () > 4) {
        return false;
    }
    const std::string padded = code + std::string (4 - code.size (), ' ');
    sectType = 0;
    for (const char c : padded) {
        sectType = (sectType << 8) | static_cast<unsigned char> (c);
    }
    return true;
}

static GSErrCode CheckLibraryPartIndex (Int32 index, GS::UniString& errorMessage)
{
    Int32 partCount = 0;
    const GSErrCode err = ACAPI_LibraryPart_GetNum (&partCount);
    if (err != NoError) {
        errorMessage = "Failed to enumerate library parts.";
        return err;
    }
    if (index < 1 || index > partCount) {
        errorMessage = GS::UniString::Printf ("No library part with index %d.", static_cast<int> (index));
        return APIERR_BADINDEX;
    }
    return NoError;
}

GetLibraryPartSectionsCommand::GetLibraryPartSectionsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetLibraryPartSectionsCommand::GetName () const
{
    return "GetLibraryPartSections";
}

GS::Optional<GS::UniString> GetLibraryPartSectionsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraryParts": {
                "type": "array",
                "description": "The library parts to list the sections of.",
                "items": {
                    "$ref": "#/LibraryPartIndexAndPassword"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "libraryParts"
        ]
    })";
}

GS::Optional<GS::UniString> GetLibraryPartSectionsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraryParts": {
                "type": "array",
                "description": "One item for each given library part, in the same order.",
                "items": {
                    "oneOf": [
                        {
                            "$ref": "#/LibraryPartSections"
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

GS::ObjectState GetLibraryPartSectionsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> libraryParts;
    parameters.Get ("libraryParts", libraryParts);

    GS::ObjectState response;
    const auto& libraryPartsAdder = response.AddList<GS::ObjectState> ("libraryParts");

    for (const GS::ObjectState& libraryPart : libraryParts) {
        Int32 index = 0;
        libraryPart.Get ("index", index);
        GS::UniString password;
        const bool hasPassword = libraryPart.Get ("password", password);

        GS::UniString errorMessage;
        GSErrCode err = CheckLibraryPartIndex (index, errorMessage);
        if (err != NoError) {
            libraryPartsAdder (CreateErrorResponse (err, errorMessage));
            continue;
        }

        Int32 sectionCount = 0;
        API_LibPartSection** sectionList = nullptr;
        err = ACAPI_LibraryPart_GetSectionList (index, &sectionCount, &sectionList);
        if (err != NoError) {
            BMKillHandle (reinterpret_cast<GSHandle*> (&sectionList));
            libraryPartsAdder (CreateErrorResponse (err, GS::UniString::Printf ("Failed to list the sections of the library part with index %d.", static_cast<int> (index))));
            continue;
        }

        GS::ObjectState entry;
        entry.Add ("index", index);
        const auto& sectionsAdder = entry.AddList<GS::ObjectState> ("sections");
        for (Int32 i = 0; i < sectionCount && sectionList != nullptr; ++i) {
            API_LibPartSection section = (*sectionList)[i];

            GS::ObjectState sectionEntry;
            sectionEntry.Add ("sectionType", SectionTypeToString (section.sectType));
            sectionEntry.Add ("subIdent", static_cast<Int32> (section.subIdent));

            // Archicad fills sectionStr only for text sections; the handle holds the raw content of every section.
            GSHandle sectionHdl = nullptr;
            GS::UniString sectionStr;
            if (ACAPI_LibraryPart_GetSection (index, &section, &sectionHdl, &sectionStr, hasPassword ? &password : nullptr) == NoError) {
                const Int32 size = sectionHdl != nullptr ? static_cast<Int32> (BMGetHandleSize (sectionHdl)) : 0;
                sectionEntry.Add ("size", size);
                sectionEntry.Add ("isText", !sectionStr.IsEmpty () || size == 0);
            }
            BMKillHandle (&sectionHdl);

            sectionsAdder (sectionEntry);
        }
        BMKillHandle (reinterpret_cast<GSHandle*> (&sectionList));

        libraryPartsAdder (entry);
    }

    return response;
}

GetLibraryPartSectionCommand::GetLibraryPartSectionCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetLibraryPartSectionCommand::GetName () const
{
    return "GetLibraryPartSection";
}

GS::Optional<GS::UniString> GetLibraryPartSectionCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "index": {
                "type": "integer",
                "description": "The index of a loaded library part, as GetAvailableLibraryParts returns it."
            },
            "sectionType": {
                "type": "string",
                "description": "The four-character code of the section type, as GetLibraryPartSections returns it. A shorter code is padded with spaces.",
                "minLength": 1,
                "maxLength": 4
            },
            "subIdent": {
                "type": "integer",
                "description": "The sub-identifier of the section, as GetLibraryPartSections returns it. 0 when not given.",
                "minimum": 0,
                "maximum": 65535
            },
            "password": {
                "type": "string",
                "description": "The password of a protected library part."
            }
        },
        "additionalProperties": false,
        "required": [
            "index",
            "sectionType"
        ]
    })";
}

GS::Optional<GS::UniString> GetLibraryPartSectionCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "content": {
                "type": "string",
                "description": "The text of the section."
            }
        },
        "additionalProperties": false,
        "required": [
            "content"
        ]
    })";
}

GS::ObjectState GetLibraryPartSectionCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    Int32 index = 0;
    parameters.Get ("index", index);
    GS::UniString sectionTypeStr;
    parameters.Get ("sectionType", sectionTypeStr);
    Int32 subIdent = 0;
    parameters.Get ("subIdent", subIdent);
    GS::UniString password;
    const bool hasPassword = parameters.Get ("password", password);

    API_LibPartSection section = {};
    if (!SectionTypeFromString (sectionTypeStr, section.sectType)) {
        return CreateErrorResponse (APIERR_BADPARS, "The section type is a code of one to four characters, as GetLibraryPartSections returns it.");
    }
    section.subIdent = static_cast<unsigned short> (subIdent);

    GS::UniString errorMessage;
    GSErrCode err = CheckLibraryPartIndex (index, errorMessage);
    if (err != NoError) {
        return CreateErrorResponse (err, errorMessage);
    }

    GSHandle sectionHdl = nullptr;
    GS::UniString sectionStr;
    err = ACAPI_LibraryPart_GetSection (index, &section, &sectionHdl, &sectionStr, hasPassword ? &password : nullptr);
    const Int32 size = (err == NoError && sectionHdl != nullptr) ? static_cast<Int32> (BMGetHandleSize (sectionHdl)) : 0;
    BMKillHandle (&sectionHdl);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to read the section '" + sectionTypeStr + "' of the library part. Check the section type, the sub-identifier and, for a protected library part, the password.");
    }
    if (sectionStr.IsEmpty () && size > 0) {
        return CreateErrorResponse (APIERR_BADPARS, "The section '" + sectionTypeStr + "' is binary, not text. GetLibraryPartSections lists its size.");
    }

    GS::ObjectState response;
    response.Add ("content", sectionStr);
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
                    "$ref": "#/LibraryPartScriptsUpdate"
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
                    "$ref": "#/LibraryPartMetadataUpdate"
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

// A library part unique ID string is "{main GUID}-{revision GUID}".
static bool SplitLibPartUnId (const char* unId, std::string& mainGuid, std::string& revisionGuid)
{
    const std::string str (unId);
    const size_t separator = str.find ("}-{");
    if (str.size () < 6 || str.front () != '{' || str.back () != '}' || separator == std::string::npos) {
        return false;
    }
    mainGuid = str.substr (1, separator - 1);
    revisionGuid = str.substr (separator + 3, str.size () - separator - 4);
    // APIGuidFromString returns APINULLGuid for a string that is not a GUID.
    return APIGuidFromString (mainGuid.c_str ()) != APINULLGuid && APIGuidFromString (revisionGuid.c_str ()) != APINULLGuid;
}

// The file of a library part given by its index (as GetAvailableLibraryParts returns it) or by its path.
static GSErrCode GetLibraryPartLocation (const GS::ObjectState& libraryPart, IO::Location& location, GS::UniString& errorMessage)
{
    Int32 index = 0;
    if (libraryPart.Get ("index", index)) {
        API_LibPart libPart = {};
        libPart.index = index;
        const GSErrCode err = ACAPI_LibraryPart_Get (&libPart);
        if (err != NoError) {
            // libPart.location is not allocated on failure.
            errorMessage = GS::UniString::Printf ("No library part with index %d.", static_cast<int> (index));
            return err;
        }
        if (libPart.location == nullptr) {
            errorMessage = GS::UniString::Printf ("The library part with index %d has no file.", static_cast<int> (index));
            return APIERR_GENERAL;
        }
        location = *libPart.location;
        delete libPart.location;
        return NoError;
    }

    GS::UniString path;
    if (libraryPart.Get ("path", path) && !path.IsEmpty ()) {
        location = IO::Location (path);
        return NoError;
    }

    errorMessage = "Give either the index or the path of the library part.";
    return APIERR_BADPARS;
}

SetLibraryPartParametersCommand::SetLibraryPartParametersCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetLibraryPartParametersCommand::GetName () const
{
    return "SetLibraryPartParameters";
}

GS::Optional<GS::UniString> SetLibraryPartParametersCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
        "type": "object",
        "properties": {
            "libraryParts": {
                "type": "array",
                "description": "The library parts to modify and the parameter values to set in each.",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "The name of the loaded library part file with its extension, as returned in the fileName field of GetAvailableLibraryParts (e.g. 'My Door.gsm')."
                        },
                        "parameters": )") + SetGDLParameterValuesSchema + R"(,
                        "descriptions": {
                            "type": "array",
                            "description": "The new descriptions of all parameters of the library part, one for each parameter in the order of its parameter list. Defaults to the current descriptions.",
                            "items": {
                                "type": "string"
                            }
                        },
                        "merge": {
                            "type": "boolean",
                            "description": "When true, the changed parameter list is merged into the parameters of the library part (MergeParameters of the Archicad API) instead of replacing them (SetParameters). Defaults to false."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "parameters"
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

GS::Optional<GS::UniString> SetLibraryPartParametersCommand::GetRawResponseSchema () const
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

GS::ObjectState SetLibraryPartParametersCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
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

    ACAPI_CallUndoableCommand ("Set Library Part Parameters", [&]() -> GSErrCode {
        for (const GS::ObjectState& libraryPart : libraryParts) {
            GS::UniString name;
            GS::Array<GS::ObjectState> parameterChanges;
            if (!libraryPart.Get ("name", name) || name.IsEmpty () || !libraryPart.Get ("parameters", parameterChanges)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing 'name' or 'parameters' parameter."));
                continue;
            }

            GS::Array<GS::UniString> givenDescriptions;
            const bool hasDescriptions = libraryPart.Contains ("descriptions");
            if (hasDescriptions && !libraryPart.Get ("descriptions", givenDescriptions)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Invalid 'descriptions' parameter, it must be a list of strings."));
                continue;
            }

            bool merge = false;
            if (libraryPart.Contains ("merge") && !libraryPart.Get ("merge", merge)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Invalid 'merge' parameter, it must be a boolean."));
                continue;
            }

            auto libPart = libraryManager->GetLibPartByName (name);
            if (libPart.IsErr ()) {
                executionResults (CreateFailedExecutionResult (libPart.UnwrapErr ().kind, "No loaded library part with the name '" + name + "'. Use the fileName (with extension) returned by GetAvailableLibraryParts."));
                continue;
            }

            auto gsmObject = libraryManager->GetGSMObjectOfLibPart (libPart.Unwrap ());
            if (gsmObject.IsErr ()) {
                executionResults (CreateFailedExecutionResult (gsmObject.UnwrapErr ().kind, "Failed to get the GSM object of the library part '" + name + "'."));
                continue;
            }

            if (!gsmObject->IsEditable ()) {
                executionResults (CreateFailedExecutionResult (APIERR_GENERAL, "The library part '" + name + "' is not editable, nothing was changed. Only a library part loaded from a writable file can be modified; a part of a library package or a read-only library must first be copied into a writable library."));
                continue;
            }

            auto currentParameters = gsmObject->GetParameters ();
            if (currentParameters.IsErr ()) {
                executionResults (CreateFailedExecutionResult (currentParameters.UnwrapErr ().kind, "Failed to get the parameters of the library part '" + name + "'."));
                continue;
            }

            // The changes are made in the parameters read above, a copy that is only written when
            // all of them succeed. They are used by reference only: the DevKit has no library or
            // framework of the GDL module, so the constructors, destructor and assignment of
            // GDL::Parameters do not link, only its virtual functions (the Result destroys them
            // through the virtual destructor).
            GDL::Parameters& gdlParameters = *currentParameters;
            GS::UniString errMessage;
            const GSErrCode setErr = SetGDLParametersFromObjectStates (gdlParameters, parameterChanges, errMessage);
            if (setErr != NoError) {
                executionResults (CreateFailedExecutionResult (setErr, errMessage + " Nothing was changed in the library part '" + name + "'."));
                continue;
            }

            std::vector<GS::UniString> descriptions;
            if (hasDescriptions) {
                const Int32 varNumber = gdlParameters.GetVarNumber ();
                if (givenDescriptions.GetSize () != static_cast<USize> (varNumber)) {
                    executionResults (CreateFailedExecutionResult (APIERR_BADPARS, GS::UniString::Printf ("Invalid 'descriptions' parameter: the library part has %d parameters, give exactly one description for each. Nothing was changed.", varNumber)));
                    continue;
                }
                for (const GS::UniString& description : givenDescriptions) {
                    descriptions.push_back (description);
                }
            } else {
                auto currentDescriptions = gsmObject->GetParameterDescriptions ();
                if (currentDescriptions.IsErr ()) {
                    executionResults (CreateFailedExecutionResult (currentDescriptions.UnwrapErr ().kind, "Failed to get the parameter descriptions of the library part '" + name + "'."));
                    continue;
                }
                descriptions = currentDescriptions.Unwrap ();
            }

            GS::UniString failedText;
            ACAPI::Result<void> modifyResult = gsmObject->Modify ([&] (GSMObject::Modifier& modifier) -> GSErrCode {
                ACAPI::Result<void> setResult = merge ? modifier.MergeParameters (gdlParameters, descriptions) : modifier.SetParameters (gdlParameters, descriptions);
                if (setResult.IsErr ()) {
                    failedText = setResult.UnwrapErr ().text.c_str ();
                    return setResult.UnwrapErr ().kind;
                }
                return NoError;
            });

            if (modifyResult.IsErr ()) {
                GS::UniString errorMessage = "Failed to modify the parameters of the library part '" + name + "'";
                GS::UniString errorText = failedText.IsEmpty () ? GS::UniString (modifyResult.UnwrapErr ().text.c_str ()) : failedText;
                if (!errorText.IsEmpty ()) {
                    errorMessage += " (" + errorText + ")";
                }
                errorMessage += ".";
                executionResults (CreateFailedExecutionResult (modifyResult.UnwrapErr ().kind, errorMessage));
                continue;
            }

            executionResults (CreateSuccessfulExecutionResult ());
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
                    "$ref": "#/LibraryPartReadableScriptType"
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
                            "$ref": "#/LibraryPartScriptsDetails"
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
    GS::ObjectState readersError;
    if (!GetRequestedScriptReaders (parameters, readers, readersError)) {
        return readersError;
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
        if (!ReadLibraryPartScripts (object, readers, password, name, scriptsOut, error)) {
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

GetLibraryPartFileInfoCommand::GetLibraryPartFileInfoCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetLibraryPartFileInfoCommand::GetName () const
{
    return "GetLibraryPartFileInfo";
}

GS::Optional<GS::UniString> GetLibraryPartFileInfoCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "files": {
                "type": "array",
                "description": "The absolute paths of the library part files (.gsm) to read. The files are read without loading them into the libraries of the project.",
                "items": {
                    "type": "string"
                }
            },
            "scripts": {
                "type": "array",
                "description": "The scripts to read. All five by default.",
                "items": {
                    "$ref": "#/LibraryPartReadableScriptType"
                },
                "minItems": 1
            },
            "password": {
                "type": "string",
                "description": "The password of password protected library parts. Reading a script of a protected part without the right password is an error for that file."
            }
        },
        "additionalProperties": false,
        "required": [
            "files"
        ]
    })";
}

GS::Optional<GS::UniString> GetLibraryPartFileInfoCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "files": {
                "type": "array",
                "description": "The information of the files in the order of the input, or an error for a file that cannot be read.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "$ref": "#/LibraryPartFileInfo"
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
            "files"
        ]
    })";
}

GS::ObjectState GetLibraryPartFileInfoCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::Array<GS::UniString> files;
    if (!parameters.Get ("files", files)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'files' parameter.");
    }

    GS::Array<const LibraryPartScriptReader*> readers;
    GS::ObjectState readersError;
    if (!GetRequestedScriptReaders (parameters, readers, readersError)) {
        return readersError;
    }

    GS::UniString password;
    parameters.Get ("password", password);

    auto libraryManager = ACAPI::Library::GetLibraryManager ();
    if (libraryManager.IsErr ()) {
        return CreateErrorResponse (libraryManager.UnwrapErr ().kind, "Failed to get the Library Manager.");
    }

    GS::ObjectState response;
    const auto& filesOut = response.AddList<GS::ObjectState> ("files");

    for (const GS::UniString& path : files) {
        if (path.IsEmpty ()) {
            filesOut (CreateErrorResponse (APIERR_BADPARS, "Empty file path."));
            continue;
        }
        const IO::Location location (path);
        bool exists = false;
        if (IO::fileSystem.Contains (location, &exists) != NoError || !exists) {
            filesOut (CreateErrorResponse (APIERR_BADPARS, "The file '" + path + "' does not exist."));
            continue;
        }

        auto gsmObject = libraryManager->LoadGSMObjectFromFile (location);
        if (gsmObject.IsErr ()) {
            filesOut (CreateErrorResponse (gsmObject.UnwrapErr ().kind, "Failed to read the file '" + path + "', it may not be a GSM object" + GetResultErrorText (gsmObject.UnwrapErr ()) + "."));
            continue;
        }
        const GSMObject& object = gsmObject.Unwrap ();

        GS::ObjectState error;
        GS::ObjectState scriptsOut;
        if (!ReadLibraryPartScripts (object, readers, password, path, scriptsOut, error)) {
            filesOut (error);
            continue;
        }

        GS::ObjectState fileOut;
        fileOut.Add ("path", path);
        auto unID = object.GetUnID ();
        if (!unID.IsErr ()) {
            fileOut.Add ("unID", unID.Unwrap ().ToUniString ());
        }
        auto subTypeUnID = object.GetSubTypeUnID ();
        if (!subTypeUnID.IsErr ()) {
            fileOut.Add ("subTypeUnID", subTypeUnID.Unwrap ().ToUniString ());
        }
        fileOut.Add ("scripts", scriptsOut);
        filesOut (fileOut);
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
                    "$ref": "#/LibraryPartCopyData"
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
                            "$ref": "#/CopiedLibraryPart"
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

GetLibraryPartIdentitiesCommand::GetLibraryPartIdentitiesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetLibraryPartIdentitiesCommand::GetName () const
{
    return "GetLibraryPartIdentities";
}

GS::Optional<GS::UniString> GetLibraryPartIdentitiesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraryParts": {
                "type": "array",
                "description": "The library parts, each given by its index or by the path of its file.",
                "items": {
                    "$ref": "#/LibraryPartIndexOrPath"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "libraryParts"
        ]
    })";
}

GS::Optional<GS::UniString> GetLibraryPartIdentitiesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "identities": {
                "type": "array",
                "description": "One item for each given library part, in the same order.",
                "items": {
                    "oneOf": [
                        {
                            "$ref": "#/LibraryPartIdentity"
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
            "identities"
        ]
    })";
}

GetLibraryPartParameterValuesCommand::GetLibraryPartParameterValuesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetLibraryPartParameterValuesCommand::GetName () const
{
    return "GetLibraryPartParameterValues";
}

GS::Optional<GS::UniString> GetLibraryPartParameterValuesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraryParts": {
                "type": "array",
                "description": "The library parts to read.",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "The name of the loaded library part file with its extension, as returned in the fileName field of GetAvailableLibraryParts (e.g. 'My Door.gsm')."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name"
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

GS::Optional<GS::UniString> GetLibraryPartParameterValuesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraryParts": {
                "type": "array",
                "description": "The default parameters of each given library part, in the order of the input, or an error for a library part that could not be read.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "$ref": "#/LibraryPartParameterValues"
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

GS::ObjectState GetLibraryPartParameterValuesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
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
    const auto& libraryPartsAdder = response.AddList<GS::ObjectState> ("libraryParts");
    for (const GS::ObjectState& libraryPart : libraryParts) {
        GS::UniString name;
        if (!libraryPart.Get ("name", name) || name.IsEmpty ()) {
            libraryPartsAdder (CreateErrorResponse (APIERR_BADPARS, "Missing 'name' parameter."));
            continue;
        }

        auto libPart = libraryManager->GetLibPartByName (name);
        if (libPart.IsErr ()) {
            libraryPartsAdder (CreateErrorResponse (libPart.UnwrapErr ().kind, "No loaded library part with the name '" + name + "'. Use the fileName (with extension) returned by GetAvailableLibraryParts."));
            continue;
        }

        auto gsmObject = libraryManager->GetGSMObjectOfLibPart (libPart.Unwrap ());
        if (gsmObject.IsErr ()) {
            libraryPartsAdder (CreateErrorResponse (gsmObject.UnwrapErr ().kind, "Failed to get the GSM object of the library part '" + name + "'."));
            continue;
        }

        auto gdlParameters = gsmObject->GetParameters ();
        if (gdlParameters.IsErr ()) {
            libraryPartsAdder (CreateErrorResponse (gdlParameters.UnwrapErr ().kind, "Failed to get the parameters of the library part '" + name + "'."));
            continue;
        }

        // The parameters are still returned when their descriptions cannot be read, only without them.
        std::vector<GS::UniString> descriptions;
        auto parameterDescriptions = gsmObject->GetParameterDescriptions ();
        if (parameterDescriptions.IsOk ()) {
            descriptions = parameterDescriptions.Unwrap ();
        }

        GS::ObjectState libraryPartOs ("name", name);
        const auto& parametersAdder = libraryPartOs.AddList<GS::ObjectState> ("parameters");
        for (const GS::ObjectState& parameter : LibPartParametersToObjectStates (gdlParameters.Unwrap (), descriptions)) {
            parametersAdder (parameter);
        }
        libraryPartsAdder (libraryPartOs);
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

GS::ObjectState GetLibraryPartIdentitiesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> libraryParts;
    parameters.Get ("libraryParts", libraryParts);

    GS::ObjectState response;
    const auto& identities = response.AddList<GS::ObjectState> ("identities");

    for (const GS::ObjectState& libraryPart : libraryParts) {
        IO::Location location;
        GS::UniString errorMessage;
        GSErrCode err = GetLibraryPartLocation (libraryPart, location, errorMessage);
        if (err != NoError) {
            identities (CreateErrorResponse (err, errorMessage));
            continue;
        }

        // A unique ID string is 77 characters: two braced GUIDs and a hyphen.
        char unId[128] = {};
        err = ACAPI_LibraryPart_GetLibPartUnIDStr (&location, unId);
        if (err != NoError) {
            identities (CreateErrorResponse (err, "Failed to get the unique ID of the library part " + location.ToDisplayText () + "."));
            continue;
        }

        GS::ObjectState identity;
        identity.Add ("path", location.ToDisplayText ());
        identity.Add ("unId", GS::UniString (unId));

        std::string mainGuid;
        std::string revisionGuid;
        if (SplitLibPartUnId (unId, mainGuid, revisionGuid)) {
            identity.Add ("mainGuid", GS::UniString (mainGuid.c_str ()));
            identity.Add ("revisionGuid", GS::UniString (revisionGuid.c_str ()));
        }

        API_Guid refGuid = APINULLGuid;
        if (ACAPI_LibraryPart_GetLibPartRefGuid (&location, &refGuid) == NoError) {
            identity.Add ("refGuid", APIGuidToString (refGuid));
        }

        API_LockableStatus lockStatus = APILockableStatus_NotExist;
        if (ACAPI_LibraryPart_GetLibPartLockStatus (&location, &lockStatus) == NoError) {
            identity.Add ("lockStatus", GS::UniString (LockableStatusToString (lockStatus)));
        }

        identity.Add ("isServerLibraryPart", ACAPI_Teamwork_IsServerLibPart (location));
        identities (identity);
    }

    return response;
}

CompareLibraryPartUnIdsCommand::CompareLibraryPartUnIdsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CompareLibraryPartUnIdsCommand::GetName () const
{
    return "CompareLibraryPartUnIds";
}

GS::Optional<GS::UniString> CompareLibraryPartUnIdsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "unId1": {
                "type": "string",
                "description": "A library part unique ID, '{main GUID}-{revision GUID}', as GetLibraryPartIdentities returns it."
            },
            "unId2": {
                "type": "string",
                "description": "The library part unique ID to compare unId1 with."
            }
        },
        "additionalProperties": false,
        "required": [
            "unId1",
            "unId2"
        ]
    })";
}

GS::Optional<GS::UniString> CompareLibraryPartUnIdsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "apiErrorCode": {
                "type": "integer",
                "description": "The code ACAPI_LibraryPart_CompareLibPartUnIds returned: 0 (NoError) when Archicad reports the two unique IDs as matching, otherwise the API error code that tells how they differ."
            },
            "sameMainGuid": {
                "type": "boolean",
                "description": "True when the two unique IDs have the same main GUID, that is they are revisions of the same library part."
            },
            "sameRevisionGuid": {
                "type": "boolean",
                "description": "True when the two unique IDs have the same revision GUID."
            }
        },
        "additionalProperties": false,
        "required": [
            "apiErrorCode",
            "sameMainGuid",
            "sameRevisionGuid"
        ]
    })";
}

GS::ObjectState CompareLibraryPartUnIdsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString unId1;
    GS::UniString unId2;
    parameters.Get ("unId1", unId1);
    parameters.Get ("unId2", unId2);

    const std::string unIdStr1 (unId1.ToCStr ().Get ());
    const std::string unIdStr2 (unId2.ToCStr ().Get ());

    std::string mainGuid1;
    std::string revisionGuid1;
    std::string mainGuid2;
    std::string revisionGuid2;
    if (!SplitLibPartUnId (unIdStr1.c_str (), mainGuid1, revisionGuid1) || !SplitLibPartUnId (unIdStr2.c_str (), mainGuid2, revisionGuid2)) {
        return CreateErrorResponse (APIERR_BADPARS, "A library part unique ID has the form '{main GUID}-{revision GUID}'.");
    }

    const GSErrCode err = ACAPI_LibraryPart_CompareLibPartUnIds (unIdStr1.c_str (), unIdStr2.c_str ());

    GS::ObjectState response;
    response.Add ("apiErrorCode", static_cast<Int32> (err));
    response.Add ("sameMainGuid", APIGuidFromString (mainGuid1.c_str ()) == APIGuidFromString (mainGuid2.c_str ()));
    response.Add ("sameRevisionGuid", APIGuidFromString (revisionGuid1.c_str ()) == APIGuidFromString (revisionGuid2.c_str ()));
    return response;
}

GetLastValidEmbeddedLibraryPartNameCommand::GetLastValidEmbeddedLibraryPartNameCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetLastValidEmbeddedLibraryPartNameCommand::GetName () const
{
    return "GetLastValidEmbeddedLibraryPartName";
}

GS::Optional<GS::UniString> GetLastValidEmbeddedLibraryPartNameCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "name": {
                "type": "string",
                "description": "The name of the last valid library part of the embedded library."
            },
            "path": {
                "type": "string",
                "description": "The path of the file of that library part."
            }
        },
        "additionalProperties": false,
        "required": [
            "name",
            "path"
        ]
    })";
}

GS::ObjectState GetLastValidEmbeddedLibraryPartNameCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString name;
    IO::Location location;
    const GSErrCode err = ACAPI_LibraryPart_GetLastValidEmbeddedLPName (&name, &location);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the last valid library part of the embedded library.");
    }

    GS::ObjectState response;
    response.Add ("name", name);
    response.Add ("path", location.ToDisplayText ());
    return response;
}

SearchLibraryPartsCommand::SearchLibraryPartsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SearchLibraryPartsCommand::GetName () const
{
    return "SearchLibraryParts";
}

GS::Optional<GS::UniString> SearchLibraryPartsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "ancestorUnId": {
                "type": "string",
                "description": "The unique ID of the ancestor library part, '{main GUID}-{revision GUID}', as GetLibraryPartIdentities returns it. Only the library parts derived from it are found."
            },
            "pattern": {
                "type": "string",
                "description": "The name pattern to match, with * as wildcard, for example 'Chair*'. Use '*' to find every library part derived from the ancestor."
            }
        },
        "additionalProperties": false,
        "required": [
            "ancestorUnId",
            "pattern"
        ]
    })";
}

GS::Optional<GS::UniString> SearchLibraryPartsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "libraryParts": {
                "type": "array",
                "description": "The library parts found. Archicad returns at most 50.",
                "items": {
                    "$ref": "#/LibraryPartSearchResult"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "libraryParts"
        ]
    })";
}

GS::ObjectState SearchLibraryPartsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString ancestorUnId;
    GS::UniString pattern;
    parameters.Get ("ancestorUnId", ancestorUnId);
    parameters.Get ("pattern", pattern);

    std::string mainGuid;
    std::string revisionGuid;
    if (!SplitLibPartUnId (ancestorUnId.ToCStr ().Get (), mainGuid, revisionGuid)) {
        return CreateErrorResponse (APIERR_BADPARS, "A library part unique ID has the form '{main GUID}-{revision GUID}'.");
    }

    API_LibPart ancestor = {};
    CHTruncate (ancestorUnId.ToCStr ().Get (), ancestor.ownUnID, sizeof (ancestor.ownUnID));

    // The API fills at most 50 results.
    constexpr Int32 maxResults = 50;
    API_LibPart results[maxResults] = {};
    Int32 numFound = 0;
    const GSErrCode err = ACAPI_LibraryPart_PatternSearch (&ancestor, pattern, results, &numFound);

    GS::ObjectState response;
    const auto& libraryParts = response.AddList<GS::ObjectState> ("libraryParts");
    if (err == NoError) {
        for (Int32 i = 0; i < numFound && i < maxResults; ++i) {
            const API_LibPart& libPart = results[i];
            GS::ObjectState libraryPart;
            libraryPart.Add ("index", static_cast<Int32> (libPart.index));
            libraryPart.Add ("name", GS::UniString (libPart.docu_UName));
            libraryPart.Add ("fileName", GS::UniString (libPart.file_UName));
            libraryPart.Add ("unId", GS::UniString (libPart.ownUnID));
            libraryPart.Add ("parentUnId", GS::UniString (libPart.parentUnID));
            libraryPart.Add ("typeId", LibPartTypeIdToString (libPart.typeID));
            libraryParts (libraryPart);
        }
    }

    // The results were zero-initialised, so a location the API did not allocate is nullptr.
    for (API_LibPart& libPart : results) {
        delete libPart.location;
        libPart.location = nullptr;
    }

    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to search the library parts derived from " + ancestorUnId + ".");
    }

    return response;
}

SetLibraryPartPictureImagesCommand::SetLibraryPartPictureImagesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetLibraryPartPictureImagesCommand::GetName () const
{
    return "SetLibraryPartPictureImages";
}

GS::Optional<GS::UniString> SetLibraryPartPictureImagesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "name": {
                "type": "string",
                "description": "The name of the loaded library part file with its extension, as returned in the fileName field of GetAvailableLibraryParts (e.g. 'My Door.gsm')."
            },
            "previewPicture": {
                "type": "string",
                "description": "Optional. The new preview picture as a base64 encoded PNG."
            },
            "gdlPictures": {
                "type": "array",
                "description": "Optional. The new GDL pictures, each a base64 encoded PNG. They replace all GDL pictures of the library part.",
                "items": {
                    "type": "string"
                },
                "minItems": 1
            },
            "darkPreviewPicture": {
                "type": "string",
                "description": "Optional. The new preview picture for the dark user interface as a base64 encoded PNG. Requires Archicad 29 or newer."
            },
            "darkGdlPictures": {
                "type": "array",
                "description": "Optional. The new GDL pictures for the dark user interface, each a base64 encoded PNG. They replace all dark GDL pictures of the library part. Requires Archicad 29 or newer.",
                "items": {
                    "type": "string"
                },
                "minItems": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "name"
        ]
    })";
}

GS::Optional<GS::UniString> SetLibraryPartPictureImagesCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState SetLibraryPartPictureImagesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::UniString name;
    if (!parameters.Get ("name", name) || name.IsEmpty ()) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid or missing 'name' parameter.");
    }

    // Every picture is decoded before anything is changed, so an invalid picture changes nothing.
    GS::UniString errorMessage;
    bool hasPreviewPicture = false;
    bool hasGdlPictures = false;
    bool hasDarkPreviewPicture = false;
    bool hasDarkGdlPictures = false;
    std::vector<GX::ImageRef> previewPicture;
    std::vector<GX::ImageRef> gdlPictures;
    std::vector<GX::ImageRef> darkPreviewPicture;
    std::vector<GX::ImageRef> darkGdlPictures;
    if (!GetPictureImages (parameters, "previewPicture", false, hasPreviewPicture, previewPicture, errorMessage) ||
        !GetPictureImages (parameters, "gdlPictures", true, hasGdlPictures, gdlPictures, errorMessage) ||
        !GetPictureImages (parameters, "darkPreviewPicture", false, hasDarkPreviewPicture, darkPreviewPicture, errorMessage) ||
        !GetPictureImages (parameters, "darkGdlPictures", true, hasDarkGdlPictures, darkGdlPictures, errorMessage)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, errorMessage + " Nothing was changed.");
    }

#ifndef ServerMainVers_2900
    if (hasDarkPreviewPicture || hasDarkGdlPictures) {
        return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "'darkPreviewPicture' and 'darkGdlPictures' require Archicad 29 or newer. Nothing was changed.");
    }
#endif

    if (!hasPreviewPicture && !hasGdlPictures && !hasDarkPreviewPicture && !hasDarkGdlPictures) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Nothing to change: give at least one of 'previewPicture', 'gdlPictures', 'darkPreviewPicture' or 'darkGdlPictures'.");
    }

    auto libraryManager = ACAPI::Library::GetLibraryManager ();
    if (libraryManager.IsErr ()) {
        return CreateFailedExecutionResult (libraryManager.UnwrapErr ().kind, "Failed to get the Library Manager.");
    }

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

    GS::ObjectState result = CreateSuccessfulExecutionResult ();
    ACAPI_CallUndoableCommand ("Set Library Part Pictures", [&]() -> GSErrCode {
        GS::UniString failedPicture;
        GS::UniString failedText;
        const auto Check = [&] (const char* key, ACAPI::Result<void> setResult) -> GSErrCode {
            if (setResult.IsErr ()) {
                failedPicture = key;
                failedText = setResult.UnwrapErr ().text.c_str ();
                return setResult.UnwrapErr ().kind;
            }
            return NoError;
        };

        ACAPI::Result<void> modifyResult = gsmObject->Modify ([&] (GSMObject::Modifier& modifier) -> GSErrCode {
            GSErrCode err = NoError;
            if (hasPreviewPicture && err == NoError) {
                err = Check ("previewPicture", modifier.SetPreviewPicture (previewPicture[0]));
            }
            if (hasGdlPictures && err == NoError) {
                err = Check ("gdlPictures", modifier.SetGDLPictures (gdlPictures));
            }
#ifdef ServerMainVers_2900
            if (hasDarkPreviewPicture && err == NoError) {
                err = Check ("darkPreviewPicture", modifier.SetDarkPreviewPicture (darkPreviewPicture[0]));
            }
            if (hasDarkGdlPictures && err == NoError) {
                err = Check ("darkGdlPictures", modifier.SetDarkGDLPictures (darkGdlPictures));
            }
#endif
            return err;
        });

        if (modifyResult.IsErr ()) {
            GS::UniString modifyErrorMessage = "Failed to modify the library part '" + name + "'";
            GS::UniString errorText = modifyResult.UnwrapErr ().text.c_str ();
            if (!failedPicture.IsEmpty ()) {
                modifyErrorMessage += ": setting '" + failedPicture + "' failed";
                errorText = failedText;
            }
            if (!errorText.IsEmpty ()) {
                modifyErrorMessage += " (" + errorText + ")";
            }
            modifyErrorMessage += ".";
            result = CreateFailedExecutionResult (modifyResult.UnwrapErr ().kind, modifyErrorMessage);
        }
        return NoError;
    });

    return result;
#else
    UNUSED_PARAMETER (parameters);
    return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

OpenLibraryPartInEditorCommand::OpenLibraryPartInEditorCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String OpenLibraryPartInEditorCommand::GetName () const
{
    return "OpenLibraryPartInEditor";
}

GS::Optional<GS::UniString> OpenLibraryPartInEditorCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "name": {
                "type": "string",
                "description": "The name of the loaded library part to open: its fileName (with extension, e.g. 'My Object.gsm') or its documentName, as returned by GetAvailableLibraryParts.",
                "minLength": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "name"
        ]
    })";
}

GS::Optional<GS::UniString> OpenLibraryPartInEditorCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState OpenLibraryPartInEditorCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::UniString name;
    if (!parameters.Get ("name", name) || name.IsEmpty ()) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Missing 'name' parameter.");
    }

    Int32 partCount = 0;
    GSErrCode err = ACAPI_LibraryPart_GetNum (&partCount);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to enumerate library parts.");
    }

    // The library part is looked up among the parts GetAvailableLibraryParts lists,
    // by its fileName or its documentName.
    API_LibPart libPart = {};
    bool found = false;
    for (Int32 i = 1; i <= partCount && !found; ++i) {
        libPart = {};
        libPart.index = i;
        if (ACAPI_LibraryPart_Get (&libPart) != NoError) {
            // libPart.location is not allocated on failure.
            continue;
        }

        found = GS::UniString (libPart.file_UName) == name || GS::UniString (libPart.docu_UName) == name;
        if (!found) {
            delete libPart.location;
            libPart.location = nullptr;
        }
    }

    if (!found) {
        return CreateFailedExecutionResult (APIERR_BADPARS,"No loaded library part with the name '" + name + "'. Use the fileName or documentName returned by GetAvailableLibraryParts.");
    }

    // Archicad tries the document name first, then the file at the location.
    API_OpenLibPartInfo openLibPartInfo = {};
    openLibPartInfo.libPartLoc = libPart.location;
    openLibPartInfo.docName = libPart.docu_UName;
    openLibPartInfo.debugGDL = false;

    err = ACAPI_LibraryManagement_OpenLibraryPart (&openLibPartInfo);

    delete libPart.location;
    libPart.location = nullptr;

    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to open the library part '" + name + "' in the library part editor.");
    }

    return CreateSuccessfulExecutionResult ();
#else
    UNUSED_PARAMETER (parameters);
    return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
}
