#include "LibraryCommands.hpp"
#include "ObjectState.hpp"
#include "MigrationHelper.hpp"
#include "Folder.hpp"
#include "File.hpp"
#include "FileSystem.hpp"
#include "GSUnID.hpp"

#ifdef ServerMainVers_2800
#include "ACAPI/Result.hpp"
#include "ACAPI/Library/LibraryManager.hpp"
#include "ACAPI/Library/LibraryTreePath.hpp"
#include "ACAPI/Library/GSMObject.hpp"

#include <functional>
#include <optional>
#include <vector>

namespace ACLib = ACAPI::Library;

using ACAPI::Library::GSMObject;

struct LibraryPartScriptSection
{
    const char* key;
    std::function<ACAPI::Result<void> (GSMObject::Modifier&, const GS::UniString&)> set;
};

static const LibraryPartScriptSection libraryPartScriptSections[] = {
    { "master",     [] (GSMObject::Modifier& modifier, const GS::UniString& script) { return modifier.SetMasterScript (script); } },
    { "parameter",  [] (GSMObject::Modifier& modifier, const GS::UniString& script) { return modifier.SetParameterScript (script); } },
    { "2d",         [] (GSMObject::Modifier& modifier, const GS::UniString& script) { return modifier.Set2DScript (script); } },
    { "3d",         [] (GSMObject::Modifier& modifier, const GS::UniString& script) { return modifier.Set3DScript (script); } },
    { "properties", [] (GSMObject::Modifier& modifier, const GS::UniString& script) { return modifier.SetPropertiesScript (script); } }
};
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
        if (file.Get ("type", typeStr)) {
            if (typeStr == "Window") {
                libPart.typeID = APILib_WindowID;
            } else if (typeStr == "Door") {
                libPart.typeID = APILib_DoorID;
            } else if (typeStr == "Object") {
                libPart.typeID = APILib_ObjectID;
            } else if (typeStr == "Lamp") {
                libPart.typeID = APILib_LampID;
            } else if (typeStr == "Room") {
                libPart.typeID = APILib_RoomID;
            } else if (typeStr == "Property") {
                libPart.typeID = APILib_PropertyID;
            } else if (typeStr == "PlanSign") {
                libPart.typeID = APILib_PlanSignID;
            } else if (typeStr == "Label") {
                libPart.typeID = APILib_LabelID;
            } else if (typeStr == "Macro") {
                libPart.typeID = APILib_MacroID;
            } else if (typeStr == "Pict" || typeStr == "Picture") {
                libPart.typeID = APILib_PictID;
            } else if (typeStr == "ListScheme") {
                libPart.typeID = APILib_ListSchemeID;
            } else if (typeStr == "Skylight") {
                libPart.typeID = APILib_SkylightID;
            } else if (typeStr == "OpeningSymbol") {
                libPart.typeID = APILib_OpeningSymbolID;
            } else {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Unknown library part type."));
                continue;
            }
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

GS::ObjectState GetLibrariesCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<API_LibraryInfo> libs;

    GSErrCode err = ACAPI_LibraryManagement_GetLibraries (&libs);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to retrive libraries.");
    }

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
                        "typeId": { "$ref": "#/LibraryPartType" }
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

#ifdef ServerMainVers_2800

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
                "type": "string",
                "enum": [
                    "All",
                    "Object",
                    "Image",
                    "Text"
                ],
                "description": "Optional. Which library parts to find: all of them (default), GDL objects, images or text files, as selected by the Library Manager's own selectors."
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
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "The name of the library part."
                        },
                        "path": {
                            "type": "string",
                            "description": "The location of the library part file."
                        },
                        "index": {
                            "type": "integer",
                            "description": "The index of the library part in the loaded libraries."
                        }
                    },
                    "additionalProperties": false
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
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "The name of the loaded library part file with its extension, as returned in the fileName field of GetAvailableLibraryParts or the name field of FindLibraryParts (e.g. 'Door 26.gsm')."
                        },
                        "index": {
                            "type": "integer",
                            "description": "The index of the library part, as returned by FindLibraryParts."
                        },
                        "password": {
                            "type": "string",
                            "description": "Optional. The password of a password protected library part, used to read its scripts."
                        }
                    },
                    "additionalProperties": false,
                    "oneOf": [
                        {
                            "required": [
                                "name"
                            ]
                        },
                        {
                            "required": [
                                "index"
                            ]
                        }
                    ]
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
                            "type": "object",
                            "properties": {
                                "name": {
                                    "type": "string",
                                    "description": "The name of the library part."
                                },
                                "path": {
                                    "type": "string",
                                    "description": "The location of the library part file."
                                },
                                "index": {
                                    "type": "integer",
                                    "description": "The index of the library part in the loaded libraries."
                                },
                                "libraryTreePath": {
                                    "type": "object",
                                    "description": "The place of the library part in the library tree.",
                                    "properties": {
                                        "parts": {
                                            "type": "array",
                                            "description": "The names along the path, from the root.",
                                            "items": {
                                                "type": "string"
                                            }
                                        },
                                        "lastPart": {
                                            "type": "string",
                                            "description": "The last name of the path."
                                        },
                                        "isRoot": {
                                            "type": "boolean",
                                            "description": "True if the path is the root of the library tree."
                                        },
                                        "parentParts": {
                                            "type": "array",
                                            "description": "The names along the path of the parent, from the root. Omitted for the root.",
                                            "items": {
                                                "type": "string"
                                            }
                                        },
                                        "teamworkId": {
                                            "type": "string",
                                            "description": "The Teamwork identifier of the path. Omitted when Archicad has none for it, e.g. outside Teamwork."
                                        }
                                    },
                                    "additionalProperties": false,
                                    "required": [
                                        "parts",
                                        "lastPart",
                                        "isRoot"
                                    ]
                                },
                                "library": {
                                    "type": "object",
                                    "description": "The library containing the library part.",
                                    "properties": {
                                        "name": {
                                            "type": "string",
                                            "description": "The name of the library."
                                        },
                                        "localizedName": {
                                            "type": "string",
                                            "description": "The localized name of the library, when it has one. Archicad 29 or newer."
                                        },
                                        "path": {
                                            "type": "string",
                                            "description": "The location of the library."
                                        },
                                        "isServer": {
                                            "type": "boolean",
                                            "description": "True if it is a Teamwork server library."
                                        },
                                        "twServerPath": {
                                            "type": "string",
                                            "description": "The path of the library on the Teamwork server. Only for server libraries."
                                        },
                                        "twServerUrl": {
                                            "type": "string",
                                            "description": "The URL of the Teamwork server. Only for server libraries."
                                        }
                                    },
                                    "additionalProperties": false
                                },
                                "gsmObject": {
                                    "type": "object",
                                    "description": "The GDL object data. Omitted for a library part that is not a GDL object, e.g. an image.",
                                    "properties": {
                                        "guid": {
                                            "type": "string",
                                            "description": "The main GUID of the library part, the guid field of GetAvailableLibraryParts."
                                        },
                                        "isSubType": {
                                            "type": "boolean",
                                            "description": "True if the library part is a subtype other library parts can be derived from."
                                        },
                                        "subTypeGuid": {
                                            "type": "string",
                                            "description": "The main GUID of the subtype (parent) of the library part."
                                        },
                                        "isInSubTypeOf": {
                                            "type": "boolean",
                                            "description": "True if the library part is in the subtype given by subTypeOf. Only when subTypeOf is given."
                                        },
                                        "isUIDefault": {
                                            "type": "boolean",
                                            "description": "True if the library part is marked as the default user interface (UI default)."
                                        },
                                        "license": {
                                            "type": "string",
                                            "description": "The license of the library part."
                                        },
                                        "licenseVersion": {
                                            "type": "string",
                                            "description": "The version of the license of the library part."
                                        },
                                        "contents": {
                                            "type": "object",
                                            "description": "Which sections the library part has.",
                                            "properties": {
                                                "interfaceScript": {
                                                    "type": "boolean"
                                                },
                                                "forwardMigrationScript": {
                                                    "type": "boolean"
                                                },
                                                "backwardMigrationScript": {
                                                    "type": "boolean"
                                                },
                                                "2dVectorImage": {
                                                    "type": "boolean"
                                                },
                                                "previewPicture": {
                                                    "type": "boolean"
                                                },
                                                "gdlPictures": {
                                                    "type": "boolean"
                                                },
                                                "darkPreviewPicture": {
                                                    "type": "boolean",
                                                    "description": "Archicad 29 or newer."
                                                },
                                                "darkGdlPictures": {
                                                    "type": "boolean",
                                                    "description": "Archicad 29 or newer."
                                                }
                                            },
                                            "additionalProperties": false,
                                            "required": [
                                                "interfaceScript",
                                                "forwardMigrationScript",
                                                "backwardMigrationScript",
                                                "2dVectorImage",
                                                "previewPicture",
                                                "gdlPictures"
                                            ]
                                        },
                                        "scripts": {
                                            "type": "object",
                                            "description": "The scripts of the library part. Only when includeScripts is true. A script is omitted when the library part has none or it cannot be read, e.g. without the right password.",
                                            "properties": {
                                                "interface": {
                                                    "type": "string"
                                                },
                                                "forwardMigration": {
                                                    "type": "string"
                                                },
                                                "backwardMigration": {
                                                    "type": "string"
                                                }
                                            },
                                            "additionalProperties": false
                                        }
                                    },
                                    "additionalProperties": false,
                                    "required": [
                                        "contents"
                                    ]
                                }
                            },
                            "additionalProperties": false
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
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "The name of the library, as returned by GetLibraries."
                        },
                        "path": {
                            "type": "string",
                            "description": "The absolute path of the library, as returned by GetLibraries."
                        }
                    },
                    "additionalProperties": false,
                    "oneOf": [
                        {
                            "required": [
                                "name"
                            ]
                        },
                        {
                            "required": [
                                "path"
                            ]
                        }
                    ]
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
                "description": "The library parts to modify and the scripts to set in each.",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "The name of the loaded library part file with its extension, as returned in the fileName field of GetAvailableLibraryParts (e.g. 'My Door.gsm')."
                        },
                        "scripts": {
                            "type": "object",
                            "description": "The scripts to set. Only the scripts given here are changed, the others are kept. An empty string makes that script section empty, it does not delete it.",
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
                                }
                            },
                            "additionalProperties": false,
                            "minProperties": 1
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "scripts"
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
            if (!libraryPart.Get ("name", name) || name.IsEmpty () || scripts == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing 'name' or 'scripts' parameter."));
                continue;
            }

            bool hasScript = false;
            for (const LibraryPartScriptSection& section : libraryPartScriptSections) {
                GS::UniString script;
                hasScript = hasScript || scripts->Get (section.key, script);
            }
            if (!hasScript) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "No script is given in 'scripts'."));
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

            GS::UniString failedSection;
            GS::UniString failedText;
            ACAPI::Result<void> modifyResult = gsmObject->Modify ([&] (GSMObject::Modifier& modifier) -> GSErrCode {
                for (const LibraryPartScriptSection& section : libraryPartScriptSections) {
                    GS::UniString script;
                    if (!scripts->Get (section.key, script)) {
                        continue;
                    }
                    ACAPI::Result<void> setResult = section.set (modifier, script);
                    if (setResult.IsErr ()) {
                        failedSection = section.key;
                        failedText = setResult.UnwrapErr ().text.c_str ();
                        return setResult.UnwrapErr ().kind;
                    }
                }
                return NoError;
            });

            if (modifyResult.IsErr ()) {
                GS::UniString errorMessage = "Failed to modify the library part '" + name + "'";
                GS::UniString errorText = modifyResult.UnwrapErr ().text.c_str ();
                if (!failedSection.IsEmpty ()) {
                    errorMessage += ": setting the '" + failedSection + "' script failed";
                    errorText = failedText;
                }
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
