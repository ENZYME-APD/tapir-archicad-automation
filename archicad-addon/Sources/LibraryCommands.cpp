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
