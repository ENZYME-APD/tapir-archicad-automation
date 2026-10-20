#include "LibraryCommands.hpp"
#include "ObjectState.hpp"
#include "MigrationHelper.hpp"
#include "Folder.hpp"
#include "File.hpp"
#include "FileSystem.hpp"
#include "GSUnID.hpp"

#include <string>

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

    for (const GS::ObjectState& item : items) {
        GS::UniString path;
        if (!item.Get ("path", path) || path.IsEmpty ()) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing path parameter."));
            continue;
        }

        IO::Location itemLocation = embeddedLibraryFolder;
        itemLocation.AppendToLocal (IO::RelativeLocation (path));

        const bool keepGSMFile = false;
        const bool silentMode = true;
        GSErrCode err = ACAPI_LibraryManagement_DeleteEmbeddedLibItem (&itemLocation, keepGSMFile, silentMode);
        if (err != NoError) {
            executionResults (CreateFailedExecutionResult (err, "Failed to delete the embedded library item on the given path."));
            continue;
        }

        executionResults (CreateSuccessfulExecutionResult ());
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
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
    return !mainGuid.empty () && !revisionGuid.empty ();
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
                    "type": "object",
                    "properties": {
                        "index": {
                            "type": "integer",
                            "description": "The index of a loaded library part, as GetAvailableLibraryParts returns it."
                        },
                        "path": {
                            "type": "string",
                            "description": "The path of a library part file, for example a .gsm file."
                        }
                    },
                    "additionalProperties": false,
                    "oneOf": [
                        { "required": ["index"] },
                        { "required": ["path"] }
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
                            "type": "object",
                            "properties": {
                                "path": {
                                    "type": "string",
                                    "description": "The path of the library part file."
                                },
                                "unId": {
                                    "type": "string",
                                    "description": "The unique ID of the library part, '{main GUID}-{revision GUID}'."
                                },
                                "mainGuid": {
                                    "$ref": "#/Guid",
                                    "description": "The main GUID of the unique ID. It stays the same across the revisions of a library part."
                                },
                                "revisionGuid": {
                                    "$ref": "#/Guid",
                                    "description": "The revision GUID of the unique ID. It changes whenever the library part is saved."
                                },
                                "refGuid": {
                                    "$ref": "#/Guid",
                                    "description": "The reference GUID of the library part file. Missing when Archicad could not tell it."
                                },
                                "lockStatus": {
                                    "type": "string",
                                    "enum": ["Free", "Editable", "Locked", "NotAvailable", "NotExist"],
                                    "description": "The Teamwork lock status of the library part file. Missing when Archicad could not tell it."
                                },
                                "isServerLibraryPart": {
                                    "type": "boolean",
                                    "description": "True when the library part is in a BIMcloud or Teamwork server library."
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "path",
                                "unId",
                                "isServerLibraryPart"
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
            "identities"
        ]
    })";
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
                    "type": "object",
                    "properties": {
                        "index": {
                            "type": "integer",
                            "description": "The index of the library part, as GetAvailableLibraryParts returns it."
                        },
                        "name": {
                            "type": "string",
                            "description": "The name of the library part."
                        },
                        "fileName": {
                            "type": "string",
                            "description": "The file name of the library part."
                        },
                        "unId": {
                            "type": "string",
                            "description": "The unique ID of the library part, '{main GUID}-{revision GUID}'."
                        },
                        "parentUnId": {
                            "type": "string",
                            "description": "The unique ID of the parent of the library part."
                        },
                        "typeId": {
                            "$ref": "#/LibraryPartType"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "index",
                        "name",
                        "unId"
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
