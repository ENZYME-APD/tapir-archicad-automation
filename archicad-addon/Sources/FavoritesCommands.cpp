#include "FavoritesCommands.hpp"
#include "ElementGDLParameterCommands.hpp"
#include "ElementCategoryCommands.hpp"
#include "MigrationHelper.hpp"
#include "NativeImage.hpp"
#include "MemoryOChannel32.hpp"
#include "Base64Converter.hpp"

#ifdef ServerMainVers_2900
#include "ACAPI/Result.hpp"
#include "ACAPI/Favorite.hpp"
#include <optional>
#include <vector>
#endif


GetFavoritesByTypeCommand::GetFavoritesByTypeCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String GetFavoritesByTypeCommand::GetName () const
{
    return "GetFavoritesByType";
}

GS::Optional<GS::UniString> GetFavoritesByTypeCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementType": {
                "$ref": "#/ElementType"
            }
        },
        "additionalProperties": false,
        "required": [
            "elementType"
        ]
    })";
}

GS::Optional<GS::UniString> GetFavoritesByTypeCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/FavoritesOrError"
    })";
}

GS::ObjectState GetFavoritesByTypeCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{   
    API_ElemTypeID elemType = API_ZombieElemID;
    GS::UniString elementTypeStr;
    if (parameters.Get ("elementType", elementTypeStr)) {
        elemType = GetElementTypeFromNonLocalizedName (elementTypeStr);
        if (elemType == API_ZombieElemID) {
            return CreateErrorResponse (APIERR_BADPARS,
                GS::UniString::Printf ("Invalid elementType '%T'.", elementTypeStr.ToPrintf ()));
        }
    }

    GS::Array< GS::UniString > names;
#ifdef ServerMainVers_2600
    GSErrCode err = ACAPI_Favorite_GetNum (elemType, nullptr, nullptr, &names);
#else
    API_ElemVariationID variation = APIVarId_Generic;
    GSErrCode err = ACAPI_Favorite_GetNum (elemType, variation, nullptr, nullptr, &names);
#endif
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to retrieve favorites of the given type.");
    }

    GS::ObjectState response;
    const auto& favorites = response.AddList<GS::UniString> ("favorites");
    for (const GS::UniString& favoriteName : names) {
        favorites (favoriteName);
    }
    return response;
}

GetAllFavoritesCommand::GetAllFavoritesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAllFavoritesCommand::GetName () const
{
    return "GetAllFavorites";
}

GS::Optional<GS::UniString> GetAllFavoritesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementType": {
                "$ref": "#/ElementType",
                "description": "Only the Favorites of this element type are returned. Omit it to get every Favorite."
            }
        },
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> GetAllFavoritesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "favorites": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "The name of the Favorite."
                        },
                        "elementType": {
                            "$ref": "#/ElementType",
                            "description": "The element type of the Favorite. Missing when Archicad cannot read the Favorite's element settings."
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
            "favorites"
        ]
    })";
}

GS::ObjectState GetAllFavoritesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    API_ElemTypeID elemTypeId = API_ZombieElemID;
    GS::UniString elementTypeStr;
    const bool filterByType = parameters.Get ("elementType", elementTypeStr);
    if (filterByType) {
        elemTypeId = GetElementTypeFromNonLocalizedName (elementTypeStr);
        if (elemTypeId == API_ZombieElemID) {
            return CreateErrorResponse (APIERR_BADPARS,
                GS::UniString::Printf ("Invalid elementType '%T'.", elementTypeStr.ToPrintf ()));
        }
    }

    const ACAPI::Favorite::FavoriteManager manager = ACAPI::Favorite::GetFavoriteManager ();
    const std::vector<ACAPI::Favorite::Favorite> favorites = filterByType
        ? manager.GetFavoritesByElementType (API_ElemType (elemTypeId))
        : manager.GetAllFavorites ();

    GS::ObjectState response;
    const auto& favoritesList = response.AddList<GS::ObjectState> ("favorites");
    for (const ACAPI::Favorite::Favorite& favorite : favorites) {
        if (!favorite.IsValid ()) {
            continue;
        }

        const auto favoriteName = favorite.GetName ();
        if (favoriteName.IsErr ()) {
            return CreateErrorResponse (favoriteName.UnwrapErr ().kind, "Failed to get the name of a Favorite.");
        }

        GS::ObjectState favoriteOs ("name", favoriteName.Unwrap ());
        const auto element = favorite.GetOldElementDefault ();
        if (element.IsOk ()) {
            favoriteOs.Add ("elementType", GetElementTypeNonLocalizedName (GetElemTypeId (element.Unwrap ().header)));
        }
        favoritesList (favoriteOs);
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}

GetFavoritePreviewImageCommand::GetFavoritePreviewImageCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetFavoritePreviewImageCommand::GetName () const
{
    return "GetFavoritePreviewImage";
}

GS::Optional<GS::UniString> GetFavoritePreviewImageCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "favorite": {
                "type": "string",
                "description": "The name of the favorite."
            },
            "imageType": {
                "type": "string",
                "description": "The type of the preview image. Default is 3D.",
                "enum": ["2D", "Section", "3D"]
            },
            "format": {
                "type": "string",
                "description": "The image format. Default is png.",
                "enum": ["png", "jpg"]
            },
            "width": {
                "type": "integer",
                "description": "The width of the preview image in pixels. Default is 128."
            },
            "height": {
                "type": "integer",
                "description": "The height of the preview image in pixels. Default is 128."
            }
        },
        "additionalProperties": false,
        "required": [
            "favorite"
        ]
    })";
}

GS::Optional<GS::UniString> GetFavoritePreviewImageCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "previewImage": {
                "type": "string",
                "description": "The base64 encoded preview image."
            }
        },
        "additionalProperties": false,
        "required": [
            "previewImage"
        ]
    })";
}

GS::ObjectState GetFavoritePreviewImageCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString favorite;
    if (!parameters.Get ("favorite", favorite)) {
        return CreateErrorResponse (APIERR_BADPARS, "Missing favorite parameter.");
    }

    API_ImageViewID viewType = APIImage_Model3D;
    GS::UniString imageTypeStr;
    if (parameters.Get ("imageType", imageTypeStr)) {
        if (imageTypeStr == "2D") {
            viewType = APIImage_Model2D;
        } else if (imageTypeStr == "Section") {
            viewType = APIImage_Section;
        } else if (imageTypeStr == "3D") {
            viewType = APIImage_Model3D;
        } else {
            return CreateErrorResponse (APIERR_BADPARS, "Invalid imageType parameter.");
        }
    }

    NewDisplay::NativeImage::Encoding encoding = NewDisplay::NativeImage::Encoding::PNG;
    GS::UniString formatStr;
    if (parameters.Get ("format", formatStr)) {
        if (formatStr == "png") {
            encoding = NewDisplay::NativeImage::Encoding::PNG;
        } else if (formatStr == "jpg") {
            encoding = NewDisplay::NativeImage::Encoding::JPEG;
        } else {
            return CreateErrorResponse (APIERR_BADPARS, "Invalid format parameter.");
        }
    }

    UInt32 width = 128;
    UInt32 height = 128;
    parameters.Get ("width", width);
    parameters.Get ("height", height);

    NewDisplay::NativeImage nativeImage (width, height, 32, nullptr);
    GSErrCode err = ACAPI_Favorite_GetPreviewImage (favorite, viewType, &nativeImage);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get favorite preview image.");
    }

    GS::MemoryOChannel32 memChannel (GS::MemoryOChannel32::BMAllocation);
    if (!nativeImage.Encode (memChannel, encoding)) {
        return CreateErrorResponse (APIERR_GENERAL, "Failed to encode favorite preview image.");
    }

    auto str = Base64Converter::Encode (memChannel.GetDestination (), memChannel.GetDataSize ());
    str.DeleteAll (GS::UniChar(char('\n')));
    return GS::ObjectState ("previewImage", str);
}

ApplyFavoritesToElementDefaultsCommand::ApplyFavoritesToElementDefaultsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ApplyFavoritesToElementDefaultsCommand::GetName () const
{
    return "ApplyFavoritesToElementDefaults";
}

GS::Optional<GS::UniString> ApplyFavoritesToElementDefaultsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "favorites": {
              "$ref": "#/Favorites"
            }
        },
        "additionalProperties": false,
        "required": [
            "favorites"
        ]
    })";
}

GS::Optional<GS::UniString> ApplyFavoritesToElementDefaultsCommand::GetRawResponseSchema () const
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

GS::ObjectState ApplyFavoritesToElementDefaultsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::UniString> favorites;
    parameters.Get ("favorites", favorites);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    API_Element mask;
    ACAPI_ELEMENT_MASK_SETFULL (mask);

    API_Favorite favorite;
    favorite.memo.New ();
    favorite.properties.New ();
    favorite.classifications.New ();
    favorite.elemCategoryValues.New ();

    ACAPI_CallUndoableCommand ("ApplyFavoritesToElementDefaults", [&]() -> GSErrCode {
        for (const GS::UniString& favoriteName : favorites) {
            favorite.name = favoriteName;

            GSErrCode err = ACAPI_Favorite_Get (&favorite);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to get favorite"));
                ACAPI_DisposeElemMemoHdls (&favorite.memo.Get ());
                continue;
            }

            err = ACAPI_Element_ChangeDefaults (&favorite.element, favorite.memo.GetPtr (), &mask);
            ACAPI_DisposeElemMemoHdls (&favorite.memo.Get ());
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to set element defaults"));
                continue;
            }

            for (const GS::Pair<API_Guid, API_Guid>& pair : *favorite.classifications) {
                TAPIR_Element_AddClassificationItemDefault (favorite.element.header, pair.second);
            }

            for (const API_ElemCategoryValue& categoryValue : *favorite.elemCategoryValues) {
                TAPIR_Element_SetCategoryValueDefault (favorite.element.header, categoryValue);
            }

            TAPIR_Element_SetPropertiesOfDefaultElem (favorite.element.header, *favorite.properties);

            executionResults (CreateSuccessfulExecutionResult ());
        }

        return NoError;
    });

    return response;
}

CreateFavoritesFromElementsCommand::CreateFavoritesFromElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateFavoritesFromElementsCommand::GetName () const
{
    return "CreateFavoritesFromElements";
}

GS::Optional<GS::UniString> CreateFavoritesFromElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "favoritesFromElements": {
                "type": "array",
                "items": {
                    "type": "object",
                    "description": "The identifier of the element and the name of the new favorite.",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "favorite": {
                            "type": "string"
                        },
                        "folder": {
                            "type": "array",
                            "description": "Optional folder hierarchy in the Favorites palette to place the new favorite under. Empty/omitted = root.",
                            "items": { "type": "string" }
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "favorite"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "favoritesFromElements"
        ]
    })";
}

GS::Optional<GS::UniString> CreateFavoritesFromElementsCommand::GetRawResponseSchema () const
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

// GDL parameter names are case insensitive, but API_AddParType::name keeps the spelling the
// library part declared, so fold to lower case before comparing. `lowerCaseName` must already
// be lower case.
static bool IsGDLParameterNamed (const char* paramName, const char* lowerCaseName)
{
    UIndex ii = 0;
    for (; paramName[ii] != '\0' && lowerCaseName[ii] != '\0'; ++ii) {
        const char ch = (paramName[ii] >= 'A' && paramName[ii] <= 'Z')
            ? char (paramName[ii] - 'A' + 'a')
            : paramName[ii];
        if (ch != lowerCaseName[ii]) {
            return false;
        }
    }
    return paramName[ii] == '\0' && lowerCaseName[ii] == '\0';
}

// Some standard library window parts - the Hungarian "ablak" family (parentUnID A7D46BBD)
// among them - are dual-use GDL parts that both the Window tool and the Corner Window tool
// place. For those, ACAPI_Element_Get reports header.variationID as the corner window
// variation even for an element that was placed as an ordinary window, and handing that header
// to ACAPI_Favorite_Create/_Change files the new Favorite under the Corner Window section of
// the Favorites palette instead of the Window section.
// The part's own AC_CW_Function GDL parameter is the reliable signal: it stays 0 while the
// element is not acting as a corner window. Only then is the variation reset, and only to
// APIVarId_Generic - which is what a window placed with the Window tool carries anyway, so
// this is a no-op for every window whose header was already correct, and for every part that
// does not have the parameter at all.
static void ResetCornerWindowVariationIfNotCornerWindow (API_Elem_Head& header, const API_ElementMemo& memo)
{
    if (GetElemTypeId (header) != API_WindowID || memo.params == nullptr) {
        return;
    }

    const GSSize nParams = BMGetHandleSize ((GSHandle) memo.params) / sizeof (API_AddParType);
    for (GSIndex ii = 0; ii < nParams; ++ii) {
        const API_AddParType& actParam = (*memo.params)[ii];
        if (actParam.typeMod != API_ParSimple || !IsGDLParameterNamed (actParam.name, "ac_cw_function")) {
            continue;
        }
        if (actParam.value.real == 0.0) {
            SetElemVariationId (header, APIVarId_Generic);
        }
        return;
    }
}

// Fills in favorite.element/classifications/properties/memo by reading them off an existing
// element - shared by CreateFavoritesFromElements (-> ACAPI_Favorite_Create, a new entry) and
// UpdateFavoritesFromElements (-> ACAPI_Favorite_Change, re-capturing an existing entry in place).
// favorite.name is intentionally left untouched here - the two callers use it differently (new
// name to create vs. existing name to update).
static GS::Optional<GS::ObjectState> BuildFavoriteFromElement (const API_Guid& elemGuid, API_Favorite& favorite)
{
    favorite.element.header.guid = elemGuid;
    GSErrCode err = ACAPI_Element_Get (&favorite.element);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to find element");
    }

    err = ACAPI_Element_GetClassificationItems (favorite.element.header.guid, favorite.classifications.Get ());
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to get the classifications of the element");
    }

    GS::Array<API_PropertyDefinition> definitions;
    err = ACAPI_Element_GetPropertyDefinitions (favorite.element.header.guid, API_PropertyDefinitionFilter_UserDefined, definitions);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to get the properties of the element");
    }

    err = ACAPI_Element_GetPropertyValues (favorite.element.header.guid, definitions, favorite.properties.Get ());
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to get the property values of the element");
    }
    for (UIndex ii = favorite.properties->GetSize (); ii >= 1; --ii) {
        const API_Property& p = favorite.properties->Get (ii - 1);
        if (p.isDefault || p.definition.canValueBeEditable == false || p.status != API_Property_HasValue)
            favorite.properties->Delete (ii - 1);
    }

    *favorite.memo = {};

    err = ACAPI_Element_GetMemo (favorite.element.header.guid, favorite.memo.GetPtr ());
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to get details of the element");
    }

    ResetCornerWindowVariationIfNotCornerWindow (favorite.element.header, favorite.memo.Get ());

    return {};
}

GS::ObjectState CreateFavoritesFromElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> favoritesFromElements;
    parameters.Get ("favoritesFromElements", favoritesFromElements);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    API_Favorite favorite;
    favorite.memo.New ();
    favorite.properties.New ();
    favorite.classifications.New ();
    favorite.elemCategoryValues.New ();
    favorite.subElements.New ();
    favorite.folder.New ();

    ACAPI_CallUndoableCommand ("CreateFavoritesFromElements", [&]() -> GSErrCode {
        for (const GS::ObjectState& favoriteFromElement : favoritesFromElements) {
            favoriteFromElement.Get ("favorite", favorite.name);

            favorite.folder->Clear ();
            GS::Array<GS::UniString> folderParts;
            if (favoriteFromElement.Get ("folder", folderParts)) {
                for (const GS::UniString& part : folderParts) {
                    favorite.folder->Push (part);
                }
            }

            const API_Guid elemGuid = GetGuidFromElementsArrayItem (favoriteFromElement);
            auto buildErr = BuildFavoriteFromElement (elemGuid, favorite);
            if (buildErr.HasValue ()) {
                executionResults (*buildErr);
                continue;
            }

            GSErrCode err = ACAPI_Favorite_Create (favorite);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to create the favorite"));
                continue;
            }

            executionResults (CreateSuccessfulExecutionResult ());
        }

        return NoError;
    });

    return response;
}


// ============================================================================
// ImportFavorites / ExportFavorites — wrap ACAPI_Favorite_Import / _Export
// so a caller can load a .prefs file into the project (replicating known-
// good GDL defaults across installs) or back the current Favorites database
// out to a file, both without going through AC's UI.
// ============================================================================

static IO::Location LocationFromPath (const GS::UniString& path)
{
    return IO::Location (path);
}

static API_FavoriteNameConflictResolutionPolicy ParseConflictPolicy (
    const GS::UniString& s, API_FavoriteNameConflictResolutionPolicy fallback)
{
    if (s == "Error")     return API_FavoriteError;
    if (s == "Skip")      return API_FavoriteSkip;
    if (s == "Overwrite") return API_FavoriteOverwrite;
    if (s == "Append")    return API_FavoriteAppend;
    return fallback;
}

ImportFavoritesCommand::ImportFavoritesCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String ImportFavoritesCommand::GetName () const
{
    return "ImportFavorites";
}

GS::Optional<GS::UniString> ImportFavoritesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "path": {
                "type": "string",
                "description": "Absolute path on the AC host to a Favorites file (.prefs) or folder."
            },
            "targetFolder": {
                "type": "array",
                "items": { "type": "string" },
                "description": "Folder hierarchy under which to import. Empty = root."
            },
            "importFolders": {
                "type": "boolean",
                "description": "If true and `path` is a folder, the folder structure is preserved."
            },
            "conflictPolicy": {
                "type": "string",
                "enum": ["Error", "Skip", "Overwrite", "Append"],
                "description": "How to resolve name conflicts. Default Overwrite."
            }
        },
        "required": ["path"],
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> ImportFavoritesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "firstConflictName": {
                "type": "string",
                "description": "Set when conflictPolicy=Error and a name collided; absent otherwise."
            }
        },
        "additionalProperties": false
    })";
}

GS::ObjectState ImportFavoritesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString path;
    parameters.Get ("path", path);
    if (path.IsEmpty ()) {
        return CreateErrorResponse (APIERR_BADPARS, "path is required");
    }

    API_FavoriteFolderHierarchy targetFolder;
    GS::Array<GS::UniString> folders;
    parameters.Get ("targetFolder", folders);
    for (const GS::UniString& f : folders) {
        targetFolder.Push (f);
    }

    bool importFolders = false;
    parameters.Get ("importFolders", importFolders);

    GS::UniString policyStr = "Overwrite";
    parameters.Get ("conflictPolicy", policyStr);
    const auto policy = ParseConflictPolicy (policyStr, API_FavoriteOverwrite);

    const IO::Location location = LocationFromPath (path);
    GS::UniString firstConflictName;
    const GSErrCode err = ACAPI_Favorite_Import (
        location, targetFolder, importFolders, policy, &firstConflictName);

    // On conflictPolicy=Error + name collision, ACAPI returns
    // APIERR_NAMEALREADYUSED AND fills firstConflictName. We must NOT
    // collapse this into a generic error response, because the caller
    // (the MCP wrapper) special-cases firstConflictName to surface
    // ok=False with the name. Translate it to a structured success-
    // shape response so the field reaches the caller.
    if (err == APIERR_NAMEALREADYUSED && !firstConflictName.IsEmpty ()) {
        GS::ObjectState response;
        response.Add ("firstConflictName", firstConflictName);
        return response;
    }
    if (err != NoError) {
        return CreateErrorResponse (err, "ACAPI_Favorite_Import failed.");
    }

    GS::ObjectState response;
    if (!firstConflictName.IsEmpty ()) {
        response.Add ("firstConflictName", firstConflictName);
    }
    return response;
}

ExportFavoritesCommand::ExportFavoritesCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String ExportFavoritesCommand::GetName () const
{
    return "ExportFavorites";
}

GS::Optional<GS::UniString> ExportFavoritesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "path": {
                "type": "string",
                "description": "Absolute path on the AC host. With the prefs format: if extension matches the Favorite binary format (.prefs), writes a single file; otherwise treats as folder. With the xml format: the folder to write the Favorite files to. With the legacyPRF format: the .prf file to write."
            },
            "names": {
                "type": "array",
                "items": { "type": "string" },
                "description": "Optional subset of Favorites to export. Default: export all."
            },
            "format": {
                "type": "string",
                "enum": ["prefs", "xml", "legacyPRF"],
                "description": "The format to export to. prefs (the default) works in every Archicad version. xml exports each Favorite to its own file in the folder, and legacyPRF exports to a Favorites file of earlier Archicad versions; these two require Archicad 29 or newer."
            }
        },
        "required": ["path"],
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> ExportFavoritesCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState ExportFavoritesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString path;
    parameters.Get ("path", path);
    if (path.IsEmpty ()) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "path is required");
    }

    GS::UniString format = "prefs";
    parameters.Get ("format", format);
    if (format != "prefs" && format != "xml" && format != "legacyPRF") {
        return CreateFailedExecutionResult (APIERR_BADPARS, GS::UniString::Printf ("Invalid format '%T'.", format.ToPrintf ()));
    }

    const IO::Location location = LocationFromPath (path);

    GSErrCode err = NoError;
    GS::Array<GS::UniString> names;
    const bool hasNames = parameters.Get ("names", names);
    if (format != "prefs") {
#ifdef ServerMainVers_2900
        const ACAPI::Favorite::FavoriteManager manager = ACAPI::Favorite::GetFavoriteManager ();
        std::vector<ACAPI::Favorite::Favorite> favorites;
        if (hasNames) {
            for (const GS::UniString& name : names) {
                const auto favorite = manager.GetFavoriteByName (name);
                if (favorite.IsErr ()) {
                    return CreateFailedExecutionResult (favorite.UnwrapErr ().kind, GS::UniString::Printf ("Favorite '%T' not found.", name.ToPrintf ()));
                }
                favorites.push_back (favorite.Unwrap ());
            }
        } else {
            favorites = manager.GetAllFavorites ();
        }

        if (format == "xml") {
            const auto exportResult = manager.ExportFavorites (favorites, location);
            if (exportResult.IsErr ()) {
                return CreateFailedExecutionResult (exportResult.UnwrapErr ().kind, "Failed to export favorites.");
            }
        } else {
            // ExportFavoritesToLegacyPRF takes the target file by non-const reference.
            IO::Location targetFile = location;
            const auto exportResult = manager.ExportFavoritesToLegacyPRF (favorites, targetFile);
            if (exportResult.IsErr ()) {
                return CreateFailedExecutionResult (exportResult.UnwrapErr ().kind, "Failed to export favorites.");
            }
        }
        return CreateSuccessfulExecutionResult ();
#else
        return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, GS::UniString::Printf ("The %T format requires Archicad 29 or newer.", format.ToPrintf ()));
#endif
    }

    if (hasNames) {
        err = ACAPI_Favorite_Export (location, &names);
    } else {
        err = ACAPI_Favorite_Export (location, nullptr);
    }
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to export favorites.");
    }

    return CreateSuccessfulExecutionResult ();
}

// ============================================================================
// ApplyFavoritesToElements — the real-element counterpart of
// ApplyFavoritesToElementDefaults (see above): same four calls, each swapped for
// its real-element equivalent (ACAPI_Element_ChangeParameters instead of
// ChangeDefaults - settings-only, keeps the target's own geometry (see
// MemoCarriesGeometry below) and its guid,
// ACAPI_Element_AddClassificationItem instead of the TAPIR_..._Default helper,
// TAPIR_Element_SetCategoryValue instead of TAPIR_Element_SetCategoryValueDefault,
// ACAPI_Element_SetProperties instead of TAPIR_Element_SetPropertiesOfDefaultElem).
// ============================================================================

// A Favorite's memo is captured from a real, placed element - see
// BuildFavoriteFromElement above, which fills it with ACAPI_Element_GetMemo, and the
// same holds for the Favorites that ship in Archicad's own templates. For the
// hierarchical element types that memo therefore also carries the source element's
// geometry: API_ElementMemo::stairBaseLine for a Stair - the very handle
// CreateStairsCommand::SetTypeSpecificParameters fills in to place one - and the
// sub-element arrays for a Railing or a Curtain Wall. Handing that memo to
// ACAPI_Element_ChangeParameters re-places the target element on the Favorite's own
// baseline instead of leaving it where it was, which is what made applying a stair
// Favorite scatter the placed stairs across the project (#576).
// For these types the memo is withheld, so only the settings that live in the
// API_Element struct are applied. The settings that live in the memo's sub-elements (a
// Stair's structure, treads and risers, a Railing's posts) are left untouched too -
// that is the price of the fix, and it is the safe half of the trade-off: an element
// that keeps some of its own settings is recoverable, one that has moved is not.
static bool MemoCarriesGeometry (const API_Elem_Head& header)
{
    switch (GetElemTypeId (header)) {
        case API_StairID:
        case API_RailingID:
        case API_CurtainWallID: return true;
        default:                return false;
    }
}

// The fields of the target element that `keepFields` can copy back after the Favorite has been
// applied (#903). A Favorite captured from a placed element carries that element's size, sill
// height, reference line, top link and Element ID, and ACAPI_Element_ChangeParameters applies
// them all - so swapping only the library part or the wall type of an element whose size,
// position and ID are already right took two or three extra calls per element. The listed
// fields are read off the target before the Favorite is applied and written back afterwards,
// with the same calls ModifyWindows/ModifyDoors/ModifyWalls/SetDetailsOfElements use for them.
// A field the target's type does not have is ignored.
struct KeptFields
{
    bool id = false;
    bool layerIndex = false;
    bool width = false;
    bool height = false;
    bool sillHeight = false;
    bool referenceLineLocation = false;
    bool topOffset = false;
    bool relativeTopStory = false;

    bool Any () const
    {
        return id || layerIndex || width || height || sillHeight || referenceLineLocation || topOffset || relativeTopStory;
    }
};

static GS::Optional<GS::UniString> ParseKeptFields (const GS::Array<GS::UniString>& names, KeptFields& kept)
{
    for (const GS::UniString& name : names) {
        if (name == "id") {
            kept.id = true;
        } else if (name == "layerIndex") {
            kept.layerIndex = true;
        } else if (name == "width") {
            kept.width = true;
        } else if (name == "height") {
            kept.height = true;
        } else if (name == "sillHeight") {
            kept.sillHeight = true;
        } else if (name == "referenceLineLocation") {
            kept.referenceLineLocation = true;
        } else if (name == "topOffset") {
            kept.topOffset = true;
        } else if (name == "relativeTopStory") {
            kept.relativeTopStory = true;
        } else {
            return GS::UniString::Printf ("Unknown keepFields value '%T'.", name.ToPrintf ());
        }
    }
    return {};
}

// Writes the kept fields back from `elementBefore`, the target as ACAPI_Element_Get returned it
// before the Favorite was applied, and from `memoBefore`, its memo read with
// APIMemoMask_ElemInfoString - the Element ID lives there (see GetDetailsOfElements), not in the
// API_Element struct. The element is read again after the apply and only the kept fields are
// copied into it and masked, the way ModifyWindows/ModifyWalls change them, so nothing else of
// the now stale `elementBefore` is written. The ID is restored last, with the same memo mask it
// was read with, so the Favorite's own properties cannot overwrite it afterwards.
static GSErrCode RestoreKeptFields (const API_Element& elementBefore, API_ElementMemo& memoBefore, const KeptFields& kept)
{
    API_Element element = {};
    element.header.guid = elementBefore.header.guid;
    GSErrCode err = ACAPI_Element_Get (&element);
    if (err != NoError) {
        return err;
    }

    API_Element mask = {};
    ACAPI_ELEMENT_MASK_CLEAR (mask);
    bool hasMaskedField = false;

    if (kept.layerIndex) {
        element.header.layer = elementBefore.header.layer;
        ACAPI_ELEMENT_MASK_SET (mask, API_Elem_Head, layer);
        hasMaskedField = true;
    }

    switch (GetElemTypeId (element.header)) {
        case API_WindowID:
        case API_DoorID:
            if (kept.width) {
                element.window.openingBase.width = elementBefore.window.openingBase.width;
                ACAPI_ELEMENT_MASK_SET (mask, API_WindowType, openingBase.width);
                hasMaskedField = true;
            }
            if (kept.height) {
                element.window.openingBase.height = elementBefore.window.openingBase.height;
                ACAPI_ELEMENT_MASK_SET (mask, API_WindowType, openingBase.height);
                hasMaskedField = true;
            }
            if (kept.sillHeight) {
                element.window.lower = elementBefore.window.lower;
                ACAPI_ELEMENT_MASK_SET (mask, API_WindowType, lower);
                hasMaskedField = true;
            }
            break;
        case API_SkylightID:
            if (kept.width) {
                element.skylight.openingBase.width = elementBefore.skylight.openingBase.width;
                ACAPI_ELEMENT_MASK_SET (mask, API_SkylightType, openingBase.width);
                hasMaskedField = true;
            }
            if (kept.height) {
                element.skylight.openingBase.height = elementBefore.skylight.openingBase.height;
                ACAPI_ELEMENT_MASK_SET (mask, API_SkylightType, openingBase.height);
                hasMaskedField = true;
            }
            break;
        case API_WallID:
            if (kept.height) {
                element.wall.height = elementBefore.wall.height;
                ACAPI_ELEMENT_MASK_SET (mask, API_WallType, height);
                hasMaskedField = true;
            }
            if (kept.referenceLineLocation) {
                element.wall.referenceLineLocation = elementBefore.wall.referenceLineLocation;
                ACAPI_ELEMENT_MASK_SET (mask, API_WallType, referenceLineLocation);
                hasMaskedField = true;
            }
            if (kept.topOffset) {
                element.wall.topOffset = elementBefore.wall.topOffset;
                ACAPI_ELEMENT_MASK_SET (mask, API_WallType, topOffset);
                hasMaskedField = true;
            }
            if (kept.relativeTopStory) {
                element.wall.relativeTopStory = elementBefore.wall.relativeTopStory;
                ACAPI_ELEMENT_MASK_SET (mask, API_WallType, relativeTopStory);
                hasMaskedField = true;
            }
            break;
        case API_ColumnID:
            if (kept.height) {
                element.column.height = elementBefore.column.height;
                ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, height);
                hasMaskedField = true;
            }
            if (kept.topOffset) {
                element.column.topOffset = elementBefore.column.topOffset;
                ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, topOffset);
                hasMaskedField = true;
            }
            if (kept.relativeTopStory) {
                element.column.relativeTopStory = elementBefore.column.relativeTopStory;
                ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, relativeTopStory);
                hasMaskedField = true;
            }
            break;
        default:
            break;
    }

    if (hasMaskedField) {
        err = ACAPI_Element_Change (&element, &mask, nullptr, 0, true);
        if (err != NoError) {
            return err;
        }
    }

    if (kept.id) {
        // An element whose ID is empty may come back without the string at all; the memo must
        // carry one for the write, and ACAPI_DisposeElemMemoHdls frees it with the rest.
        if (memoBefore.elemInfoString == nullptr) {
            memoBefore.elemInfoString = new GS::UniString ();
        }
        err = ACAPI_Element_ChangeMemo (element.header.guid, APIMemoMask_ElemInfoString, &memoBefore);
        if (err != NoError) {
            return err;
        }
    }

    return NoError;
}

ApplyFavoritesToElementsCommand::ApplyFavoritesToElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ApplyFavoritesToElementsCommand::GetName () const
{
    return "ApplyFavoritesToElements";
}

GS::Optional<GS::UniString> ApplyFavoritesToElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "favoritesToApply": {
                "type": "array",
                "items": {
                    "type": "object",
                    "description": "The identifier of the element and the name of the Favorite to apply to it.",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "favorite": {
                            "type": "string"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "favorite"
                    ]
                }
            },
            "applySettings": {
                "type": "boolean",
                "description": "Whether to apply the Favorite's settings-type parameters (structure, materials, pens, etc. - never geometry). For the hierarchical types (Stair, Railing, Curtain Wall) the settings of the sub-elements are not applied, because they are inseparable from the Favorite's own geometry. Default is true."
            },
            "applyClassifications": {
                "type": "boolean",
                "description": "Whether to apply the Favorite's classifications. Default is true."
            },
            "applyCategories": {
                "type": "boolean",
                "description": "Whether to apply the Favorite's element categories (e.g. IFC categories). Default is true."
            },
            "applyProperties": {
                "type": "boolean",
                "description": "Whether to apply the Favorite's property values. Default is true."
            },
            "keepFields": {
                "type": "array",
                "description": "Fields of the target element to keep: after the Favorite has been applied, they are copied back from the target as it was before, so only the Favorite's other settings (e.g. its library part or wall type) take effect. 'id' and 'layerIndex' apply to every element type; 'width', 'height' and 'sillHeight' to a Window or Door ('width' and 'height' also to a Skylight); 'height', 'topOffset' and 'relativeTopStory' to a Wall or Column; 'referenceLineLocation' to a Wall. A field the target's type does not have is ignored. To keep a Wall's or Column's top exactly as it was, keep 'height', 'relativeTopStory' and 'topOffset' together - a kept 'height' alone is recomputed by Archicad while the Favorite's relativeTopStory links the top to a story. Default: nothing is kept.",
                "items": {
                    "type": "string",
                    "enum": [
                        "id",
                        "layerIndex",
                        "width",
                        "height",
                        "sillHeight",
                        "referenceLineLocation",
                        "topOffset",
                        "relativeTopStory"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "favoritesToApply"
        ]
    })";
}

GS::Optional<GS::UniString> ApplyFavoritesToElementsCommand::GetRawResponseSchema () const
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

GS::ObjectState ApplyFavoritesToElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> favoritesToApply;
    parameters.Get ("favoritesToApply", favoritesToApply);

    bool applySettings = true;
    parameters.Get ("applySettings", applySettings);
    bool applyClassifications = true;
    parameters.Get ("applyClassifications", applyClassifications);
    bool applyCategories = true;
    parameters.Get ("applyCategories", applyCategories);
    bool applyProperties = true;
    parameters.Get ("applyProperties", applyProperties);

    KeptFields keptFields;
    GS::Array<GS::UniString> keepFieldNames;
    if (parameters.Get ("keepFields", keepFieldNames)) {
        const auto parseError = ParseKeptFields (keepFieldNames, keptFields);
        if (parseError.HasValue ()) {
            return CreateErrorResponse (APIERR_BADPARS, parseError.Get ());
        }
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("ApplyFavoritesToElements", [&]() -> GSErrCode {
        for (const GS::ObjectState& item : favoritesToApply) {
            const GS::ObjectState* elementId = item.Get ("elementId");
            if (elementId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
                continue;
            }

            const API_Guid targetGuid = GetGuidFromObjectState (*elementId);
            API_Element targetElement = {};
            targetElement.header.guid = targetGuid;
            GSErrCode err = ACAPI_Element_Get (&targetElement);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to find element"));
                continue;
            }

            // The Element ID is read off the memo before anything is applied - see RestoreKeptFields.
            API_ElementMemo targetIdMemo = {};
            const GS::OnExit disposeTargetIdMemo ([&targetIdMemo] () { ACAPI_DisposeElemMemoHdls (&targetIdMemo); });
            if (keptFields.id) {
                err = ACAPI_Element_GetMemo (targetGuid, &targetIdMemo, APIMemoMask_ElemInfoString);
                if (err != NoError) {
                    executionResults (CreateFailedExecutionResult (err, "Failed to read the ID of the element"));
                    continue;
                }
            }

            API_Favorite favorite;
            favorite.memo.New ();
            favorite.properties.New ();
            favorite.classifications.New ();
            favorite.elemCategoryValues.New ();
            item.Get ("favorite", favorite.name);

            err = ACAPI_Favorite_Get (&favorite);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to get favorite"));
                ACAPI_DisposeElemMemoHdls (&favorite.memo.Get ());
                continue;
            }

            // Compare the type ID only - never the whole API_ElemType. On AC26+ that struct
            // also carries the variationID, and a Window favorite can legitimately hold a
            // different variation than the window it is applied to (dual-use library parts
            // report APIVarId_CornerWindow even when placed as regular windows), which made
            // every Window favorite look like a type mismatch. The same guard elsewhere
            // (see ExtendedElementCommands.cpp) compares the type ID for this reason.
            const bool typeMatches = GetElemTypeId (favorite.element.header) == GetElemTypeId (targetElement.header);
            if (!typeMatches) {
                executionResults (CreateFailedExecutionResult (APIERR_BADID, "The Favorite's element type does not match the target element's type."));
                ACAPI_DisposeElemMemoHdls (&favorite.memo.Get ());
                continue;
            }

            // ACAPI_Element_ChangeParameters (unlike ACAPI_Element_Change) only ever touches
            // settings-type parameters of the API_Element itself, never its geometry, and
            // always keeps the target's own guid - exactly the "apply favorite settings"
            // semantics we want here. The memo is the one part that can carry geometry, so
            // it is withheld for the types where it does (see MemoCarriesGeometry above).
            if (applySettings) {
                API_Element mask = {};
                ACAPI_ELEMENT_MASK_SETFULL (mask);

                GS::Array<API_Guid> targetGuids;
                targetGuids.Push (targetGuid);

                const API_ElementMemo* memoToApply = MemoCarriesGeometry (targetElement.header)
                    ? nullptr
                    : favorite.memo.GetPtr ();

                err = ACAPI_Element_ChangeParameters (targetGuids, &favorite.element, memoToApply, &mask);
            }
            ACAPI_DisposeElemMemoHdls (&favorite.memo.Get ());
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to apply favorite to element"));
                continue;
            }

            if (applyClassifications) {
                for (const GS::Pair<API_Guid, API_Guid>& pair : *favorite.classifications) {
                    ACAPI_Element_AddClassificationItem (targetGuid, pair.second);
                }
            }

            if (applyCategories) {
                for (const API_ElemCategoryValue& categoryValue : *favorite.elemCategoryValues) {
                    TAPIR_Element_SetCategoryValue (targetGuid, categoryValue);
                }
            }

            if (applyProperties) {
                ACAPI_Element_SetProperties (targetGuid, *favorite.properties);
            }

            if (keptFields.Any ()) {
                err = RestoreKeptFields (targetElement, targetIdMemo, keptFields);
                if (err != NoError) {
                    executionResults (CreateFailedExecutionResult (err, "The favorite was applied, but a field listed in keepFields could not be restored"));
                    continue;
                }
            }

            executionResults (CreateSuccessfulExecutionResult ());
        }

        return NoError;
    });

    return response;
}

// ============================================================================
// UpdateFavoritesFromElements — wraps ACAPI_Favorite_Change, the "re-capture
// settings into an EXISTING Favorite" counterpart to CreateFavoritesFromElements
// (which always makes a new entry). Shares BuildFavoriteFromElement (above) with
// CreateFavoritesFromElementsCommand - only the final ACAPI_Favorite_* call differs.
// ============================================================================

UpdateFavoritesFromElementsCommand::UpdateFavoritesFromElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String UpdateFavoritesFromElementsCommand::GetName () const
{
    return "UpdateFavoritesFromElements";
}

GS::Optional<GS::UniString> UpdateFavoritesFromElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "favoritesFromElements": {
                "type": "array",
                "items": {
                    "$ref": "#/FavoritesFromElementUpdate"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "favoritesFromElements"
        ]
    })";
}

GS::Optional<GS::UniString> UpdateFavoritesFromElementsCommand::GetRawResponseSchema () const
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

GS::ObjectState UpdateFavoritesFromElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> favoritesFromElements;
    parameters.Get ("favoritesFromElements", favoritesFromElements);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    API_Favorite favorite;
    favorite.memo.New ();
    favorite.properties.New ();
    favorite.classifications.New ();
    favorite.elemCategoryValues.New ();
    favorite.subElements.New ();

    ACAPI_CallUndoableCommand ("UpdateFavoritesFromElements", [&]() -> GSErrCode {
        for (const GS::ObjectState& favoriteFromElement : favoritesFromElements) {
            favoriteFromElement.Get ("favorite", favorite.name);

            const API_Guid elemGuid = GetGuidFromElementsArrayItem (favoriteFromElement);
            auto buildErr = BuildFavoriteFromElement (elemGuid, favorite);
            if (buildErr.HasValue ()) {
                executionResults (*buildErr);
                continue;
            }

            GSErrCode err = ACAPI_Favorite_Change (favorite);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to update the favorite"));
                continue;
            }

            executionResults (CreateSuccessfulExecutionResult ());
        }

        return NoError;
    });

    return response;
}

// ============================================================================
// RenameFavorites / DeleteFavorites — trivial batch wrappers around
// ACAPI_Favorite_Rename / ACAPI_Favorite_Delete.
// ============================================================================

RenameFavoritesCommand::RenameFavoritesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String RenameFavoritesCommand::GetName () const
{
    return "RenameFavorites";
}

GS::Optional<GS::UniString> RenameFavoritesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "renames": {
                "type": "array",
                "items": {
                    "$ref": "#/FavoriteRename"
                }
            }
        },
        "additionalProperties": false,
        "required": ["renames"]
    })";
}

GS::Optional<GS::UniString> RenameFavoritesCommand::GetRawResponseSchema () const
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

GS::ObjectState RenameFavoritesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> renames;
    parameters.Get ("renames", renames);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("RenameFavorites", [&]() -> GSErrCode {
        for (const GS::ObjectState& rename : renames) {
            GS::UniString oldName;
            GS::UniString newName;
            rename.Get ("oldName", oldName);
            rename.Get ("newName", newName);

            const GSErrCode err = ACAPI_Favorite_Rename (oldName, newName);
            executionResults (err == NoError ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (err, "Failed to rename favorite"));
        }

        return NoError;
    });

    return response;
}

DeleteFavoritesCommand::DeleteFavoritesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteFavoritesCommand::GetName () const
{
    return "DeleteFavorites";
}

GS::Optional<GS::UniString> DeleteFavoritesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "favorites": {
                "$ref": "#/Favorites"
            }
        },
        "additionalProperties": false,
        "required": ["favorites"]
    })";
}

GS::Optional<GS::UniString> DeleteFavoritesCommand::GetRawResponseSchema () const
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

GS::ObjectState DeleteFavoritesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::UniString> favorites;
    parameters.Get ("favorites", favorites);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("DeleteFavorites", [&]() -> GSErrCode {
        for (const GS::UniString& favoriteName : favorites) {
            const GSErrCode err = ACAPI_Favorite_Delete (favoriteName);
            executionResults (err == NoError ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (err, "Failed to delete favorite"));
        }

        return NoError;
    });

    return response;
}

// ============================================================================
// GetFavoriteSettings — reads what ACAPI_Favorite_Get returns about a Favorite
// (element type, the GDL parameters of its library part, its marker, its
// sub-elements and its folder) without applying it to the tool defaults or
// placing an element. Nothing is changed, so there is no undoable command.
// ============================================================================

// The library part index of the library part based element types; 0 for the others.
static Int32 GetLibIndOfElement (const API_Element& element)
{
    switch (GetElemTypeId (element.header)) {
        case API_ObjectID:      return element.object.libInd;
        case API_LampID:        return element.lamp.libInd;
        case API_WindowID:      return element.window.openingBase.libInd;
        case API_DoorID:        return element.door.openingBase.libInd;
        case API_SkylightID:    return element.skylight.openingBase.libInd;
        case API_ZoneID:        return element.zone.libInd;
        default:                return 0;
    }
}

GetFavoriteSettingsCommand::GetFavoriteSettingsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetFavoriteSettingsCommand::GetName () const
{
    return "GetFavoriteSettings";
}

GS::Optional<GS::UniString> GetFavoriteSettingsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "favorites": {
                "$ref": "#/Favorites"
            }
        },
        "additionalProperties": false,
        "required": [
            "favorites"
        ]
    })";
}

GS::Optional<GS::UniString> GetFavoriteSettingsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "favoriteSettings": {
                "type": "array",
                "description": "The settings of each given Favorite, in the order of the input, or an error for a Favorite that could not be read.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "favorite": {
                                    "type": "string",
                                    "description": "The name of the Favorite."
                                },
                                "elementType": {
                                    "$ref": "#/ElementType"
                                },
                                "folder": {
                                    "type": "array",
                                    "description": "The folder hierarchy of the Favorite in the Favorites palette. Empty for the root.",
                                    "items": {
                                        "type": "string"
                                    }
                                },
                                "libPart": {
                                    "$ref": "#/LibPartDetails",
                                    "description": "The library part of the Favorite, for the library part based types (Object, Lamp, Window, Door, Skylight, Zone). Absent otherwise or when it could not be read."
                                },
                                "parameters": {
                                    "type": "array",
                                    "description": "The GDL parameters stored with the Favorite. Empty for the types without GDL parameters.",
                                    "items": {
                                        "$ref": "#/StoredGDLParameter"
                                    }
                                },
                                "marker": {
                                    "type": "object",
                                    "description": "The marker stored with a Window or Door Favorite. Absent when the Favorite has no marker - that is not an error.",
                                    "properties": {
                                        "libPart": {
                                            "$ref": "#/LibPartDetails",
                                            "description": "The library part of the marker. Absent when it could not be read."
                                        },
                                        "parameters": {
                                            "type": "array",
                                            "description": "The GDL parameters of the marker.",
                                            "items": {
                                                "$ref": "#/StoredGDLParameter"
                                            }
                                        }
                                    },
                                    "additionalProperties": false,
                                    "required": [
                                        "parameters"
                                    ]
                                },
                                "subElements": {
                                    "type": "array",
                                    "description": "The sub-elements stored with the Favorite.",
                                    "items": {
                                        "type": "object",
                                        "properties": {
                                            "subType": {
                                                "type": "string",
                                                "enum": [
                                                    "MainMarker",
                                                    "Other"
                                                ],
                                                "description": "MainMarker for the main marker of the element; Other for any other kind of sub-element."
                                            },
                                            "elementType": {
                                                "$ref": "#/ElementType"
                                            },
                                            "libPart": {
                                                "$ref": "#/LibPartDetails",
                                                "description": "The library part of the sub-element. Absent when it has none or it could not be read."
                                            },
                                            "parameters": {
                                                "type": "array",
                                                "description": "The GDL parameters of the sub-element.",
                                                "items": {
                                                    "$ref": "#/StoredGDLParameter"
                                                }
                                            }
                                        },
                                        "additionalProperties": false,
                                        "required": [
                                            "subType",
                                            "elementType",
                                            "parameters"
                                        ]
                                    }
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "favorite",
                                "elementType",
                                "parameters",
                                "subElements"
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
            "favoriteSettings"
        ]
    })";
}

GS::ObjectState GetFavoriteSettingsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::UniString> favorites;
    parameters.Get ("favorites", favorites);

    GS::ObjectState response;
    const auto& favoriteSettingsAdder = response.AddList<GS::ObjectState> ("favoriteSettings");

    for (const GS::UniString& favoriteName : favorites) {
        API_Favorite favorite;
        favorite.name = favoriteName;
        favorite.memo.New ();
        favorite.elementMarker.New ();
        favorite.memoMarker.New ();
        favorite.subElements.New ();
        favorite.folder.New ();

        // ACAPI_Favorite_Get may have allocated some of the handles even when it fails.
        const GS::OnExit cleanup ([&]() {
            if (favorite.memo.HasValue ()) {
                ACAPI_DisposeElemMemoHdls (&favorite.memo.Get ());
            }
            if (favorite.memoMarker.HasValue ()) {
                ACAPI_DisposeElemMemoHdls (&favorite.memoMarker.Get ());
            }
            if (favorite.subElements.HasValue ()) {
                for (API_SubElement& subElement : *favorite.subElements) {
                    ACAPI_DisposeElemMemoHdls (&subElement.memo);
                }
            }
        });

        const GSErrCode err = ACAPI_Favorite_Get (&favorite);
        if (err != NoError) {
            favoriteSettingsAdder (CreateErrorResponse (err, GS::UniString::Printf ("Failed to get favorite '%T'.", favoriteName.ToPrintf ())));
            continue;
        }

        const API_ElemTypeID elemTypeId = GetElemTypeId (favorite.element.header);

        GS::ObjectState favoriteSettings;
        favoriteSettings.Add ("favorite", favoriteName);
        favoriteSettings.Add ("elementType", GetElementTypeNonLocalizedName (elemTypeId));

        if (favorite.folder.HasValue ()) {
            const auto& folderAdder = favoriteSettings.AddList<GS::UniString> ("folder");
            for (const GS::UniString& folderName : *favorite.folder) {
                folderAdder (folderName);
            }
        }

        AddLibPartAndGDLParameters (favoriteSettings,
            GetLibIndOfElement (favorite.element),
            favorite.memo.HasValue () ? favorite.memo->params : nullptr);

        // Only windows and doors have a marker; a Favorite of theirs without one has no marker
        // library part and no marker parameters, which is reported by leaving "marker" out.
        if ((elemTypeId == API_WindowID || elemTypeId == API_DoorID) &&
            favorite.elementMarker.HasValue () && favorite.memoMarker.HasValue ())
        {
            const Int32 markerLibInd = favorite.elementMarker->object.libInd;
            API_AddParType** markerParams = favorite.memoMarker->params;
            if (markerLibInd > 0 || markerParams != nullptr) {
                GS::ObjectState marker;
                AddLibPartAndGDLParameters (marker, markerLibInd, markerParams);
                favoriteSettings.Add ("marker", marker);
            }
        }

        const auto& subElementsAdder = favoriteSettings.AddList<GS::ObjectState> ("subElements");
        if (favorite.subElements.HasValue ()) {
            for (const API_SubElement& subElement : *favorite.subElements) {
                const API_ElemTypeID subElemTypeId = GetElemTypeId (subElement.subElem.header);
                GS::ObjectState subElementSettings;
                subElementSettings.Add ("subType", GS::UniString ((subElement.subType & APISubElement_MainMarker) != 0 ? "MainMarker" : "Other"));
                subElementSettings.Add ("elementType", GetElementTypeNonLocalizedName (subElemTypeId));
                AddLibPartAndGDLParameters (subElementSettings, GetLibIndOfElement (subElement.subElem), subElement.memo.params);
                subElementsAdder (subElementSettings);
            }
        }

        favoriteSettingsAdder (favoriteSettings);
    }

    return response;
}

// ============================================================================
// GetFavoriteContents / DeleteFavoritesWithManager — the Favorites and their
// folders through ACAPI::Favorite::FavoriteManager (Archicad 29 and newer).
// ============================================================================

#ifdef ServerMainVers_2900

// Finds the folder at the given path, the folder names from the root down, by
// walking down from the root folder. The first subfolder with the matching name
// is taken at each level. An empty path is the root folder itself.
static GSErrCode FindFavoriteFolderAtPath (const ACAPI::Favorite::FavoriteManager& manager, const GS::Array<GS::UniString>& path, std::optional<ACAPI::Favorite::FavoriteFolder>& folder, GS::UniString& errorMessage)
{
    const auto rootFolder = manager.GetRootFolder ();
    if (rootFolder.IsErr ()) {
        errorMessage = "Failed to get the root folder of the Favorites.";
        return rootFolder.UnwrapErr ().kind;
    }
    folder.reset ();
    folder.emplace (rootFolder.Unwrap ());

    for (const GS::UniString& name : path) {
        std::optional<ACAPI::Favorite::FavoriteFolder> found;
        for (const ACAPI::Favorite::FavoriteFolder& subFolder : folder->GetSubFolders ()) {
            const auto subFolderName = subFolder.GetName ();
            if (subFolderName.IsErr ()) {
                errorMessage = "Failed to get the name of a Favorite folder.";
                return subFolderName.UnwrapErr ().kind;
            }
            if (subFolderName.Unwrap () == name) {
                found.emplace (subFolder);
                break;
            }
        }
        if (!found.has_value ()) {
            errorMessage = GS::UniString::Printf ("Favorite folder '%T' not found.", name.ToPrintf ());
            return APIERR_BADNAME;
        }
        folder.reset ();
        folder.emplace (*found);
    }

    return NoError;
}

// The name, folder path, classifications, category values and properties of the
// Favorite, or an error item when one of them cannot be read.
static GS::ObjectState GetFavoriteContentObjectState (const ACAPI::Favorite::Favorite& favorite, const std::vector<GS::UniString>& folderPath)
{
    const auto favoriteName = favorite.GetName ();
    if (favoriteName.IsErr ()) {
        return CreateErrorResponse (favoriteName.UnwrapErr ().kind, "Failed to get the name of a Favorite.");
    }
    const GS::UniString name = favoriteName.Unwrap ();

    const auto classifications = favorite.GetClassifications ();
    if (classifications.IsErr ()) {
        return CreateErrorResponse (classifications.UnwrapErr ().kind, GS::UniString::Printf ("Failed to get the classifications of the Favorite '%T'.", name.ToPrintf ()));
    }
    const auto categoryValues = favorite.GetElemCategoryValues ();
    if (categoryValues.IsErr ()) {
        return CreateErrorResponse (categoryValues.UnwrapErr ().kind, GS::UniString::Printf ("Failed to get the category values of the Favorite '%T'.", name.ToPrintf ()));
    }
    const auto properties = favorite.GetProperties ();
    if (properties.IsErr ()) {
        return CreateErrorResponse (properties.UnwrapErr ().kind, GS::UniString::Printf ("Failed to get the properties of the Favorite '%T'.", name.ToPrintf ()));
    }

    GS::ObjectState favoriteOs ("name", name);

    const auto& folderPathAdder = favoriteOs.AddList<GS::UniString> ("folderPath");
    for (const GS::UniString& folderName : folderPath) {
        folderPathAdder (folderName);
    }

    // The pairs are (classification system, classification item), as ACAPI_Element_GetClassificationItems gives them.
    const auto& classificationsAdder = favoriteOs.AddList<GS::ObjectState> ("classifications");
    for (const GS::Pair<API_Guid, API_Guid>& pair : classifications.Unwrap ()) {
        GS::ObjectState classificationId ("classificationSystemId", CreateGuidObjectState (pair.first));
        if (pair.second != APINULLGuid) {
            classificationId.Add ("classificationItemId", CreateGuidObjectState (pair.second));
        }
        classificationsAdder (GS::ObjectState ("classificationId", classificationId));
    }

    const auto& categoryValuesAdder = favoriteOs.AddList<GS::ObjectState> ("categoryValues");
    for (const API_ElemCategoryValue& value : categoryValues.Unwrap ()) {
        const char* type = CategoryTypeToString (value.category.categoryID);
        if (type == nullptr) {
            continue;
        }
        categoryValuesAdder (GS::ObjectState (
            "type", GS::UniString (type),
            "categoryId", CreateGuidObjectState (value.category.guid),
            "categoryValueId", CreateGuidObjectState (value.guid),
            "name", GS::UniString (value.name)));
    }

    const auto& propertiesAdder = favoriteOs.AddList<GS::ObjectState> ("properties");
    for (const API_Property& property : properties.Unwrap ()) {
        GS::ObjectState propertyOs ("propertyId", CreateGuidObjectState (property.definition.guid));
        if (property.status == API_Property_NotAvailable || property.status == API_Property_NotEvaluated) {
            propertyOs.Add ("error", GS::ObjectState ("code", APIERR_BADPROPERTY, "message", GS::UniString ("Not available or not evaluated property.")));
            propertiesAdder (propertyOs);
            continue;
        }
        GS::UniString valueString;
        const GSErrCode err = ACAPI_Property_GetPropertyValueString (property, &valueString);
        if (err == NoError) {
            propertyOs.Add ("propertyValue", GS::ObjectState ("value", valueString));
        } else {
            propertyOs.Add ("error", GS::ObjectState ("code", err, "message", GS::UniString ("Failed to get property value as string.")));
        }
        propertiesAdder (propertyOs);
    }

    return favoriteOs;
}

// Collects the Favorites of the folder and of its subfolders, recursively.
static void CollectFavoriteContentsOfFolder (const ACAPI::Favorite::FavoriteFolder& folder, GS::Array<GS::ObjectState>& favorites)
{
    const std::vector<GS::UniString> folderPath = folder.GetPath ();
    for (const ACAPI::Favorite::Favorite& favorite : folder.GetFavorites ()) {
        if (!favorite.IsValid ()) {
            continue;
        }
        favorites.Push (GetFavoriteContentObjectState (favorite, folderPath));
    }
    for (const ACAPI::Favorite::FavoriteFolder& subFolder : folder.GetSubFolders ()) {
        CollectFavoriteContentsOfFolder (subFolder, favorites);
    }
}

#endif

GetFavoriteContentsCommand::GetFavoriteContentsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetFavoriteContentsCommand::GetName () const
{
    return "GetFavoriteContents";
}

GS::Optional<GS::UniString> GetFavoriteContentsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "folderPath": {
                "type": "array",
                "description": "The folder to read, the folder names from the root of the Favorites palette down. The Favorites of its subfolders are returned too. Omit it or leave it empty for every Favorite.",
                "items": {
                    "type": "string"
                }
            }
        },
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> GetFavoriteContentsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "favorites": {
                "type": "array",
                "description": "The Favorites of the folder and of its subfolders, or an error for a Favorite that could not be read.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "name": {
                                    "type": "string",
                                    "description": "The name of the Favorite."
                                },
                                "folderPath": {
                                    "type": "array",
                                    "description": "The path of the folder the Favorite is in, as Archicad gives it. Empty for the root folder.",
                                    "items": {
                                        "type": "string"
                                    }
                                },
                                "classifications": {
                                    "$ref": "#/ClassificationIdsOrErrors",
                                    "description": "The classifications stored with the Favorite, in the format of GetClassificationsOfElements."
                                },
                                "categoryValues": {
                                    "type": "array",
                                    "description": "The element category values stored with the Favorite, in the format of GetCategoryValuesOfElements.",
                                    "items": {
                                        "type": "object",
                                        "properties": {
                                            "type": {
                                                "$ref": "#/ElementCategoryType"
                                            },
                                            "categoryId": {
                                                "$ref": "#/ElementCategoryId"
                                            },
                                            "categoryValueId": {
                                                "$ref": "#/ElementCategoryValueId"
                                            },
                                            "name": {
                                                "type": "string",
                                                "description": "The localized name of the value, for display only."
                                            }
                                        },
                                        "additionalProperties": false,
                                        "required": [
                                            "type",
                                            "categoryId",
                                            "categoryValueId",
                                            "name"
                                        ]
                                    }
                                },
                                "properties": {
                                    "type": "array",
                                    "description": "The property values stored with the Favorite. Each has propertyValue, in the format of GetPropertyValuesOfElements, or an error when its value cannot be read.",
                                    "items": {
                                        "type": "object",
                                        "properties": {
                                            "propertyId": {
                                                "$ref": "#/PropertyId"
                                            },
                                            "propertyValue": {
                                                "$ref": "#/PropertyValue"
                                            },
                                            "error": {
                                                "$ref": "#/Error"
                                            }
                                        },
                                        "additionalProperties": false,
                                        "required": [
                                            "propertyId"
                                        ]
                                    }
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "name",
                                "folderPath",
                                "classifications",
                                "categoryValues",
                                "properties"
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
            "favorites"
        ]
    })";
}

GS::ObjectState GetFavoriteContentsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    GS::Array<GS::UniString> folderPath;
    parameters.Get ("folderPath", folderPath);

    const ACAPI::Favorite::FavoriteManager manager = ACAPI::Favorite::GetFavoriteManager ();
    std::optional<ACAPI::Favorite::FavoriteFolder> folder;
    GS::UniString errorMessage;
    const GSErrCode err = FindFavoriteFolderAtPath (manager, folderPath, folder, errorMessage);
    if (err != NoError) {
        return CreateErrorResponse (err, errorMessage);
    }

    GS::Array<GS::ObjectState> favorites;
    CollectFavoriteContentsOfFolder (*folder, favorites);

    GS::ObjectState response;
    const auto& favoritesAdder = response.AddList<GS::ObjectState> ("favorites");
    for (const GS::ObjectState& favoriteOs : favorites) {
        favoritesAdder (favoriteOs);
    }
    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}

#ifdef ServerMainVers_2900

// Finds the Favorite by name, in the folder at folderPath when it is given
// (only directly in that folder, not in its subfolders), anywhere otherwise.
static GSErrCode FindFavoriteToDelete (const ACAPI::Favorite::FavoriteManager& manager, const GS::UniString& name, const GS::Array<GS::UniString>* folderPath, std::optional<ACAPI::Favorite::Favorite>& favorite, GS::UniString& errorMessage)
{
    if (folderPath == nullptr) {
        const auto found = manager.GetFavoriteByName (name);
        if (found.IsErr ()) {
            errorMessage = GS::UniString::Printf ("Favorite '%T' not found.", name.ToPrintf ());
            return found.UnwrapErr ().kind;
        }
        favorite.emplace (found.Unwrap ());
        return NoError;
    }

    std::optional<ACAPI::Favorite::FavoriteFolder> folder;
    const GSErrCode err = FindFavoriteFolderAtPath (manager, *folderPath, folder, errorMessage);
    if (err != NoError) {
        return err;
    }
    for (const ACAPI::Favorite::Favorite& candidate : folder->GetFavorites ()) {
        const auto candidateName = candidate.GetName ();
        if (candidateName.IsOk () && candidateName.Unwrap () == name) {
            favorite.emplace (candidate);
            return NoError;
        }
    }
    errorMessage = GS::UniString::Printf ("Favorite '%T' not found in the given folder.", name.ToPrintf ());
    return APIERR_BADNAME;
}

#endif

DeleteFavoritesWithManagerCommand::DeleteFavoritesWithManagerCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteFavoritesWithManagerCommand::GetName () const
{
    return "DeleteFavoritesWithManager";
}

GS::Optional<GS::UniString> DeleteFavoritesWithManagerCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "favorites": {
                "type": "array",
                "description": "The Favorites to delete.",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "The name of the Favorite."
                        },
                        "folderPath": {
                            "type": "array",
                            "description": "The folder the Favorite is in, the folder names from the root of the Favorites palette down, in the format GetFavoriteContents returns. When given, the Favorite is only looked for directly in that folder. Omit it to find the Favorite by name anywhere.",
                            "items": {
                                "type": "string"
                            }
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
            "favorites"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteFavoritesWithManagerCommand::GetRawResponseSchema () const
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

GS::ObjectState DeleteFavoritesWithManagerCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    GS::Array<GS::ObjectState> favorites;
    parameters.Get ("favorites", favorites);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    // One undoable command for the whole call, as DeleteFavorites does.
    ACAPI_CallUndoableCommand ("DeleteFavoritesWithManager", [&]() -> GSErrCode {
        const ACAPI::Favorite::FavoriteManager manager = ACAPI::Favorite::GetFavoriteManager ();
        for (const GS::ObjectState& item : favorites) {
            GS::UniString name;
            if (!item.Get ("name", name) || name.IsEmpty ()) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "The name of the Favorite is missing."));
                continue;
            }

            GS::Array<GS::UniString> folderPath;
            const bool hasFolderPath = item.Get ("folderPath", folderPath);

            std::optional<ACAPI::Favorite::Favorite> favorite;
            GS::UniString errorMessage;
            const GSErrCode err = FindFavoriteToDelete (manager, name, hasFolderPath ? &folderPath : nullptr, favorite, errorMessage);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, errorMessage));
                continue;
            }

            const auto deleteResult = manager.DeleteFavorite (*favorite);
            if (deleteResult.IsErr ()) {
                executionResults (CreateFailedExecutionResult (deleteResult.UnwrapErr ().kind, GS::UniString::Printf ("Failed to delete the Favorite '%T'.", name.ToPrintf ())));
                continue;
            }

            executionResults (CreateSuccessfulExecutionResult ());
        }

        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}