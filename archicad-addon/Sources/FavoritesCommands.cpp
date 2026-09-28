#include "FavoritesCommands.hpp"
#include "MigrationHelper.hpp"
#include "NativeImage.hpp"
#include "MemoryOChannel32.hpp"
#include "Base64Converter.hpp"


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
                "description": "Absolute path on the AC host. If extension matches the Favorite binary format (.prefs), writes a single file; otherwise treats as folder."
            },
            "names": {
                "type": "array",
                "items": { "type": "string" },
                "description": "Optional subset of Favorites to export. Default: export all."
            }
        },
        "required": ["path"],
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> ExportFavoritesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {},
        "additionalProperties": false
    })";
}

GS::ObjectState ExportFavoritesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString path;
    parameters.Get ("path", path);
    if (path.IsEmpty ()) {
        return CreateErrorResponse (APIERR_BADPARS, "path is required");
    }

    const IO::Location location = LocationFromPath (path);

    GSErrCode err = NoError;
    GS::Array<GS::UniString> names;
    if (parameters.Get ("names", names)) {
        err = ACAPI_Favorite_Export (location, &names);
    } else {
        err = ACAPI_Favorite_Export (location, nullptr);
    }
    if (err != NoError) {
        return CreateErrorResponse (err, "ACAPI_Favorite_Export failed.");
    }

    // Empty response satisfies the schema (`additionalProperties: false`).
    // CreateSuccessfulExecutionResult() would inject `{"success": true}`
    // which the schema validator rejects for top-level command responses.
    return GS::ObjectState ();
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
                    "type": "object",
                    "description": "The identifier of the element and the name of the existing Favorite to update from it.",
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
                    "type": "object",
                    "properties": {
                        "oldName": { "type": "string" },
                        "newName": { "type": "string" }
                    },
                    "additionalProperties": false,
                    "required": ["oldName", "newName"]
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