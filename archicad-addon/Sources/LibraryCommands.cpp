#include "LibraryCommands.hpp"
#include "ObjectState.hpp"
#include "MigrationHelper.hpp"
#include "Folder.hpp"
#include "File.hpp"
#include "FileSystem.hpp"
#include "GSUnID.hpp"

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

GS::Optional<GS::UniString> GetLibraryPartDetailsCommand::GetRawResponseSchema () const
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
                                    "type": "object",
                                    "description": "The detail flags of an Object, Lamp, Label or Room library part.",
                                    "properties": {
                                        "fixSize": { "type": "boolean" },
                                        "autoHotspot": { "type": "boolean" }
                                    },
                                    "additionalProperties": false,
                                    "required": [ "fixSize", "autoHotspot" ]
                                },
                                "doorWindowDetails": {
                                    "type": "object",
                                    "description": "The details of a Door or Window library part.",
                                    "properties": {
                                        "mirThick": { "type": "number" },
                                        "wallInset": { "type": "number" },
                                        "leftFram": { "type": "number" },
                                        "righFram": { "type": "number" },
                                        "topFram": { "type": "number" },
                                        "botFram": { "type": "number" },
                                        "orientation": { "type": "integer", "description": "The orientation setting (API_Orient value) of the door or window." },
                                        "contours3D": { "type": "boolean" }
                                    },
                                    "additionalProperties": false,
                                    "required": [ "mirThick", "wallInset", "leftFram", "righFram", "topFram", "botFram", "orientation", "contours3D" ]
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

GS::ObjectState GetLibraryPartDetailsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
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
