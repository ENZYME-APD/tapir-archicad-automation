#include "FavoritesCommands.hpp"
#include "PropertyCommands.hpp"
#include "ElementGDLParameterCommands.hpp"
#include "MigrationHelper.hpp"
#include "NativeImage.hpp"
#include "MemoryOChannel32.hpp"
#include "Base64Converter.hpp"

#ifdef ServerMainVers_2900
#include "ACAPI/Result.hpp"
#include "ACAPI/Favorite.hpp"
#include "ACAPI/Element/Opening/OpeningDefault.hpp"
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

// ============================================================================
// GetFavoriteDetails — read-only audit of what a Favorite would apply: its
// classifications, property values and element category values, as
// ACAPI_Favorite_Get fills them in (the same data ApplyFavoritesToElements and
// ApplyFavoritesToElementDefaults apply). The memo and the sub-elements are not
// requested, so no handle is allocated.
// ============================================================================

GetFavoriteDetailsCommand::GetFavoriteDetailsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetFavoriteDetailsCommand::GetName () const
{
    return "GetFavoriteDetails";
}

GS::Optional<GS::UniString> GetFavoriteDetailsCommand::GetInputParametersSchema () const
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

GS::Optional<GS::UniString> GetFavoriteDetailsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "favoriteDetails": {
                "type": "array",
                "description": "The details of the given favorites, in the order of the input, or an error for a favorite that could not be read.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "favorite": {
                                    "type": "string",
                                    "description": "The name of the favorite."
                                },
                                "elementType": {
                                    "type": "string",
                                    "description": "The element type of the favorite, as in GetFavoritesByType."
                                },
                                "classifications": {
                                    "type": "array",
                                    "description": "The classifications the favorite applies, one per classification system.",
                                    "items": {
                                        "$ref": "#/ClassificationIdArrayItem"
                                    }
                                },
                                "properties": {
                                    "type": "array",
                                    "description": "The property values stored in the favorite.",
                                    "items": {
                                        "type": "object",
                                        "properties": {
                                            "propertyId": {
                                                "$ref": "#/PropertyId"
                                            },
                                            "propertyName": {
                                                "type": "string"
                                            },
                                            "status": {
                                                "type": "string",
                                                "enum": [
                                                    "HasValue",
                                                    "NotEvaluated",
                                                    "NotAvailable"
                                                ],
                                                "description": "The evaluation status of the value. propertyValue is only present for HasValue."
                                            },
                                            "isDefault": {
                                                "type": "boolean",
                                                "description": "True if the value is the default value of the property."
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
                                            "propertyId",
                                            "propertyName",
                                            "status",
                                            "isDefault"
                                        ]
                                    }
                                },
                                "categoryValues": {
                                    "type": "array",
                                    "description": "The element category values (e.g. Structural Function, Position, Renovation Status) stored in the favorite.",
                                    "items": {
                                        "type": "object",
                                        "properties": {
                                            "categoryId": {
                                                "$ref": "#/GuidId"
                                            },
                                            "categoryName": {
                                                "type": "string"
                                            },
                                            "categoryValueId": {
                                                "$ref": "#/GuidId"
                                            },
                                            "categoryValueName": {
                                                "type": "string"
                                            }
                                        },
                                        "additionalProperties": false,
                                        "required": [
                                            "categoryId",
                                            "categoryName",
                                            "categoryValueId",
                                            "categoryValueName"
                                        ]
                                    }
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "favorite",
                                "elementType",
                                "classifications",
                                "properties",
                                "categoryValues"
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
            "favoriteDetails"
        ]
    })";
}

static GS::UniString PropertyValueStatusToString (API_PropertyValueStatus status)
{
    switch (status) {
        case API_Property_HasValue:     return "HasValue";
        case API_Property_NotEvaluated: return "NotEvaluated";
        default:                        return "NotAvailable";
    }
}

GS::ObjectState GetFavoriteDetailsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::UniString> favorites;
    parameters.Get ("favorites", favorites);

    GS::ObjectState response;
    const auto& favoriteDetails = response.AddList<GS::ObjectState> ("favoriteDetails");

    for (const GS::UniString& favoriteName : favorites) {
        API_Favorite favorite;
        favorite.name = favoriteName;
        favorite.properties.New ();
        favorite.classifications.New ();
        favorite.elemCategoryValues.New ();

        const GSErrCode err = ACAPI_Favorite_Get (&favorite);
        if (err != NoError) {
            favoriteDetails (CreateErrorResponse (err, GS::UniString::Printf ("Failed to get favorite '%T'.", favoriteName.ToPrintf ())));
            continue;
        }

        GS::ObjectState details;
        details.Add ("favorite", favoriteName);
        details.Add ("elementType", GetElementTypeNonLocalizedName (GetElemTypeId (favorite.element.header)));

        const auto& classifications = details.AddList<GS::ObjectState> ("classifications");
        for (const GS::Pair<API_Guid, API_Guid>& pair : *favorite.classifications) {
            GS::ObjectState classificationId;
            classificationId.Add ("classificationSystemId", CreateGuidObjectState (pair.first));
            classificationId.Add ("classificationItemId", CreateGuidObjectState (pair.second));
            classifications (GS::ObjectState ("classificationId", classificationId));
        }

        // The value is only valid for API_Property_HasValue, so it is left out otherwise
        // rather than reported as an empty string.
        const auto& properties = details.AddList<GS::ObjectState> ("properties");
        for (const API_Property& property : *favorite.properties) {
            GS::ObjectState propertyDetails;
            propertyDetails.Add ("propertyId", CreateGuidObjectState (property.definition.guid));
            propertyDetails.Add ("propertyName", property.definition.name);
            propertyDetails.Add ("status", PropertyValueStatusToString (property.status));
            propertyDetails.Add ("isDefault", property.isDefault);
            if (property.status == API_Property_HasValue) {
                GS::UniString valueString;
                const GSErrCode valueErr = GetPropertyValueString (property, valueString);
                if (valueErr == NoError) {
                    propertyDetails.Add ("propertyValue", GS::ObjectState ("value", valueString));
                } else {
                    GS::ObjectState error;
                    error.Add ("code", valueErr);
                    error.Add ("message", "Failed to get property value as string");
                    propertyDetails.Add ("error", error);
                }
            }
            properties (propertyDetails);
        }

        const auto& categoryValues = details.AddList<GS::ObjectState> ("categoryValues");
        for (const API_ElemCategoryValue& categoryValue : *favorite.elemCategoryValues) {
            GS::ObjectState categoryValueDetails;
            categoryValueDetails.Add ("categoryId", CreateGuidObjectState (categoryValue.category.guid));
            categoryValueDetails.Add ("categoryName", GS::UniString (categoryValue.category.name));
            categoryValueDetails.Add ("categoryValueId", CreateGuidObjectState (categoryValue.guid));
            categoryValueDetails.Add ("categoryValueName", GS::UniString (categoryValue.name));
            categoryValues (categoryValueDetails);
        }

        favoriteDetails (details);
    }

    return response;
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

#ifdef ServerMainVers_2900

// The conflict policies of the ACAPI::Favorite::FavoriteManager calls (Archicad 29+), which differ
// from those of ACAPI_Favorite_Import above.
static bool ParseFavoriteManagerConflictPolicy (const GS::UniString& s, ACAPI::Favorite::FavoriteManager::ConflictPolicy& policy)
{
    if (s == "Error") {
        policy = ACAPI::Favorite::FavoriteManager::ConflictPolicy::Error;
    } else if (s == "Skip") {
        policy = ACAPI::Favorite::FavoriteManager::ConflictPolicy::Skip;
    } else if (s == "OverwriteExisting") {
        policy = ACAPI::Favorite::FavoriteManager::ConflictPolicy::OverwriteExisting;
    } else if (s == "RenameNew") {
        policy = ACAPI::Favorite::FavoriteManager::ConflictPolicy::RenameNew;
    } else {
        return false;
    }
    return true;
}

// Finds the folder of the Favorites palette at `path`, the folder names from the root down; an
// empty path is the root folder.
static GSErrCode FindFavoriteFolderOfPath (const ACAPI::Favorite::FavoriteManager& manager, const GS::Array<GS::UniString>& path, std::optional<ACAPI::Favorite::FavoriteFolder>& folder, GS::UniString& errorMessage)
{
    const auto rootFolder = manager.GetRootFolder ();
    if (rootFolder.IsErr ()) {
        errorMessage = "Failed to get the root folder of the Favorites.";
        return rootFolder.UnwrapErr ().kind;
    }
    folder.emplace (rootFolder.Unwrap ());

    for (const GS::UniString& name : path) {
        std::optional<ACAPI::Favorite::FavoriteFolder> found;
        for (const ACAPI::Favorite::FavoriteFolder& subFolder : folder->GetSubFolders ()) {
            const auto subFolderName = subFolder.GetName ();
            if (subFolderName.IsErr ()) {
                errorMessage = "Failed to get the name of a Favorites folder.";
                return subFolderName.UnwrapErr ().kind;
            }
            if (subFolderName.Unwrap () == name) {
                found.emplace (subFolder);
                break;
            }
        }
        if (!found.has_value ()) {
            errorMessage = GS::UniString::Printf ("The Favorites folder '%T' was not found.", name.ToPrintf ());
            return APIERR_BADNAME;
        }
        folder.emplace (*found);
    }

    return NoError;
}

// The path of `folder` from the root down, found by walking up its parents until the root.
static GSErrCode GetPathOfFavoriteFolder (const ACAPI::Favorite::FavoriteFolder& folder, GS::Array<GS::UniString>& path, GS::UniString& errorMessage)
{
    // A guard against a parent chain that never reaches the root.
    const UIndex maxDepth = 1000;

    GS::Array<GS::UniString> namesUpwards;
    std::optional<ACAPI::Favorite::FavoriteFolder> current;
    current.emplace (folder);
    for (UIndex depth = 0; ; ++depth) {
        if (depth >= maxDepth) {
            errorMessage = "The Favorites folder hierarchy is too deep.";
            return APIERR_GENERAL;
        }

        const auto isRoot = current->IsRoot ();
        if (isRoot.IsErr ()) {
            errorMessage = "Failed to check whether a Favorites folder is the root.";
            return isRoot.UnwrapErr ().kind;
        }
        if (isRoot.Unwrap ()) {
            break;
        }

        const auto name = current->GetName ();
        if (name.IsErr ()) {
            errorMessage = "Failed to get the name of a Favorites folder.";
            return name.UnwrapErr ().kind;
        }
        namesUpwards.Push (name.Unwrap ());

        const auto parent = current->GetParentFolder ();
        if (parent.IsErr ()) {
            errorMessage = "Failed to get the parent of a Favorites folder.";
            return parent.UnwrapErr ().kind;
        }
        current.emplace (parent.Unwrap ());
    }

    path.Clear ();
    for (UIndex ii = namesUpwards.GetSize (); ii >= 1; --ii) {
        path.Push (namesUpwards[ii - 1]);
    }
    return NoError;
}

// Calls `func` with the settings a new or changed Favorite takes from `source`: those of another
// Favorite ({favoriteName}) or the current tool default ({toolDefault}, only Opening so far, the
// one tool whose ACAPI::ElementDefault the API can create).
template<typename Func>
static GSErrCode WithFavoriteSource (const ACAPI::Favorite::FavoriteManager& manager, const GS::ObjectState& source, GS::UniString& errorMessage, Func&& func)
{
    GS::UniString sourceFavoriteName;
    GS::UniString toolDefault;
    const bool hasFavoriteName = source.Get ("favoriteName", sourceFavoriteName);
    const bool hasToolDefault = source.Get ("toolDefault", toolDefault);
    if (hasFavoriteName == hasToolDefault) {
        errorMessage = "The source must have exactly one of favoriteName and toolDefault.";
        return APIERR_BADPARS;
    }

    if (hasFavoriteName) {
        const auto sourceFavorite = manager.GetFavoriteByName (sourceFavoriteName);
        if (sourceFavorite.IsErr ()) {
            errorMessage = GS::UniString::Printf ("The source Favorite '%T' was not found.", sourceFavoriteName.ToPrintf ());
            return sourceFavorite.UnwrapErr ().kind;
        }
        const auto elementDefault = sourceFavorite.Unwrap ().GetElementDefault ();
        if (elementDefault.IsErr ()) {
            errorMessage = GS::UniString::Printf ("Failed to get the settings of the source Favorite '%T': %s", sourceFavoriteName.ToPrintf (), elementDefault.UnwrapErr ().text.c_str ());
            return elementDefault.UnwrapErr ().kind;
        }
        return func (elementDefault.Unwrap ());
    }

    if (toolDefault != "Opening") {
        errorMessage = GS::UniString::Printf ("Invalid toolDefault '%T'.", toolDefault.ToPrintf ());
        return APIERR_BADPARS;
    }
    const ACAPI::Result<ACAPI::Element::OpeningDefault> openingDefault = ACAPI::Element::CreateOpeningDefault ();
    if (openingDefault.IsErr ()) {
        errorMessage = GS::UniString::Printf ("Failed to get the Opening tool default: %s", openingDefault.UnwrapErr ().text.c_str ());
        return openingDefault.UnwrapErr ().kind;
    }
    const ACAPI::ElementDefault& elementDefault = openingDefault.Unwrap ();
    return func (elementDefault);
}

#endif

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
                "description": "Absolute path on the AC host. With the prefs format: a Favorites file (.prefs) or folder. With the xml format: the folder of Favorite files, as ExportFavorites writes it. With the legacyPRF format: the .prf file."
            },
            "format": {
                "type": "string",
                "enum": ["prefs", "xml", "legacyPRF"],
                "description": "The format to import from, the counterpart of the format of ExportFavorites. prefs (the default) works in every Archicad version. xml imports Favorite files from a folder, and legacyPRF a Favorites file of earlier Archicad versions; these two require Archicad 29 or newer."
            },
            "fileNames": {
                "type": "array",
                "items": { "type": "string" },
                "description": "Only with the xml format: the names of the Favorite files in the folder to import. Default: every Favorite file of the folder."
            },
            "targetFolder": {
                "type": "array",
                "items": { "type": "string" },
                "description": "Folder hierarchy under which to import. Empty = root. With the xml and legacyPRF formats the folder must exist, and omitting it keeps the folders stored with the Favorites."
            },
            "importFolders": {
                "type": "boolean",
                "description": "Only with the prefs format: if true and `path` is a folder, the folder structure is preserved."
            },
            "conflictPolicy": {
                "type": "string",
                "enum": ["Error", "Skip", "Overwrite", "Append", "OverwriteExisting", "RenameNew"],
                "description": "How to resolve name conflicts. The prefs format takes Error, Skip, Overwrite or Append, default Overwrite. The xml and legacyPRF formats take Error, Skip, OverwriteExisting or RenameNew, default Error."
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

    GS::UniString format = "prefs";
    parameters.Get ("format", format);
    if (format != "prefs" && format != "xml" && format != "legacyPRF") {
        return CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("Invalid format '%T'.", format.ToPrintf ()));
    }

    GS::Array<GS::UniString> fileNames;
    const bool hasFileNames = parameters.Get ("fileNames", fileNames);
    if (hasFileNames && format != "xml") {
        return CreateErrorResponse (APIERR_BADPARS, "fileNames can only be given with the xml format.");
    }

    API_FavoriteFolderHierarchy targetFolder;
    GS::Array<GS::UniString> folders;
    const bool hasTargetFolder = parameters.Get ("targetFolder", folders);
    for (const GS::UniString& f : folders) {
        targetFolder.Push (f);
    }

    const IO::Location location = LocationFromPath (path);

    if (format != "prefs") {
#ifdef ServerMainVers_2900
        GS::UniString managerPolicyStr = "Error";
        parameters.Get ("conflictPolicy", managerPolicyStr);
        ACAPI::Favorite::FavoriteManager::ConflictPolicy managerPolicy = ACAPI::Favorite::FavoriteManager::ConflictPolicy::Error;
        if (!ParseFavoriteManagerConflictPolicy (managerPolicyStr, managerPolicy)) {
            return CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("The %T format does not take conflictPolicy '%T'; use Error, Skip, OverwriteExisting or RenameNew.", format.ToPrintf (), managerPolicyStr.ToPrintf ()));
        }

        const ACAPI::Favorite::FavoriteManager manager = ACAPI::Favorite::GetFavoriteManager ();

        // Without a target folder Archicad keeps the folders stored with the Favorites.
        std::optional<ACAPI::Favorite::FavoriteFolder> targetOrKeep;
        if (hasTargetFolder) {
            GS::UniString errorMessage;
            const GSErrCode findErr = FindFavoriteFolderOfPath (manager, folders, targetOrKeep, errorMessage);
            if (findErr != NoError) {
                return CreateErrorResponse (findErr, errorMessage);
            }
        }

        const ACAPI::Result<void> importResult = [&] () -> ACAPI::Result<void> {
            if (format == "legacyPRF") {
                return manager.ImportFavoritesFromLegacyPRF (location, managerPolicy, targetOrKeep);
            }
            if (!hasFileNames) {
                return manager.ImportAllFavorites (location, managerPolicy, targetOrKeep);
            }
            std::vector<IO::Location> favoriteXMLs;
            for (const GS::UniString& fileName : fileNames) {
                IO::Location fileLocation = location;
                fileLocation.AppendToLocal (IO::Name (fileName));
                favoriteXMLs.push_back (fileLocation);
            }
            return manager.ImportFavorites (favoriteXMLs, managerPolicy, targetOrKeep);
        } ();
        if (importResult.IsErr ()) {
            return CreateErrorResponse (importResult.UnwrapErr ().kind, GS::UniString::Printf ("Failed to import favorites: %s", importResult.UnwrapErr ().text.c_str ()));
        }
        return GS::ObjectState ();
#else
        UNUSED_VARIABLE (hasTargetFolder);
        return CreateErrorResponse (APIERR_NOTSUPPORTED, GS::UniString::Printf ("The %T format requires Archicad 29 or newer.", format.ToPrintf ()));
#endif
    }

    bool importFolders = false;
    parameters.Get ("importFolders", importFolders);

    GS::UniString policyStr = "Overwrite";
    parameters.Get ("conflictPolicy", policyStr);
    const auto policy = ParseConflictPolicy (policyStr, API_FavoriteOverwrite);
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

// Puts the target back on the story it was on before the Favorite was applied, should
// ACAPI_Element_ChangeParameters have moved it despite the target's own story being passed in
// with the Favorite (#1565). Done with the same call SetDetailsOfElements changes floorIndex with.
static GSErrCode RestoreHomeStory (const API_Element& elementBefore)
{
    API_Element element = {};
    element.header.guid = elementBefore.header.guid;
    GSErrCode err = ACAPI_Element_Get (&element);
    if (err != NoError) {
        return err;
    }
    if (element.header.floorInd == elementBefore.header.floorInd) {
        return NoError;
    }

    element.header.floorInd = elementBefore.header.floorInd;
    API_Element mask = {};
    ACAPI_ELEMENT_MASK_CLEAR (mask);
    ACAPI_ELEMENT_MASK_SET (mask, API_Elem_Head, floorInd);
    return ACAPI_Element_Change (&element, &mask, nullptr, 0, true);
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
                "description": "Whether to apply the Favorite's settings-type parameters (structure, materials, pens, etc. - never geometry; the element stays on its own home story). For the hierarchical types (Stair, Railing, Curtain Wall) the settings of the sub-elements are not applied, because they are inseparable from the Favorite's own geometry. A Dimension's text size, text pen and text frame are not changed (tested in Archicad 28): the dimension creation commands take textHeight and textPen for them. Default is true."
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

                // The home story is part of the header the full mask covers, and the Favorite
                // carries the story of the element it was captured from: applied as is, it moved
                // a Wall from story 2 down to story 0 (#1565). The target keeps its own story.
                favorite.element.header.floorInd = targetElement.header.floorInd;
                mask.header.floorInd = 0;

                err = ACAPI_Element_ChangeParameters (targetGuids, &favorite.element, memoToApply, &mask);
                if (err == NoError) {
                    err = RestoreHomeStory (targetElement);
                }
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

#ifdef ServerMainVers_2900

// Writes the folder, its Favorites and its subfolders recursively. The path is
// built while walking down from the root, whose name and path are empty.
static GSErrCode AddFavoriteFolderToObjectState (const ACAPI::Favorite::FavoriteFolder& folder, const GS::UniString& name, const GS::Array<GS::UniString>& path, GS::ObjectState& os, GS::UniString& errorMessage)
{
    os.Add ("name", name);
    os.Add ("path", path);

    GS::Array<GS::UniString> favoriteNames;
    for (const ACAPI::Favorite::Favorite& favorite : folder.GetFavorites ()) {
        const auto favoriteName = favorite.GetName ();
        if (favoriteName.IsErr ()) {
            errorMessage = "Failed to get the name of a Favorite.";
            return favoriteName.UnwrapErr ().kind;
        }
        favoriteNames.Push (favoriteName.Unwrap ());
    }
    os.Add ("favorites", favoriteNames);

    const auto& subFoldersList = os.AddList<GS::ObjectState> ("subFolders");
    for (const ACAPI::Favorite::FavoriteFolder& subFolder : folder.GetSubFolders ()) {
        const auto subFolderName = subFolder.GetName ();
        if (subFolderName.IsErr ()) {
            errorMessage = "Failed to get the name of a Favorite folder.";
            return subFolderName.UnwrapErr ().kind;
        }
        GS::Array<GS::UniString> subFolderPath = path;
        subFolderPath.Push (subFolderName.Unwrap ());

        GS::ObjectState subFolderOs;
        const GSErrCode err = AddFavoriteFolderToObjectState (subFolder, subFolderName.Unwrap (), subFolderPath, subFolderOs, errorMessage);
        if (err != NoError) {
            return err;
        }
        subFoldersList (subFolderOs);
    }

    return NoError;
}

#endif

GetFavoriteFoldersCommand::GetFavoriteFoldersCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetFavoriteFoldersCommand::GetName () const
{
    return "GetFavoriteFolders";
}

GS::Optional<GS::UniString> GetFavoriteFoldersCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "rootFolder": {
                "$ref": "#/FavoriteFolderDetails"
            }
        },
        "additionalProperties": false,
        "required": [
            "rootFolder"
        ]
    })";
}

GS::ObjectState GetFavoriteFoldersCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    const ACAPI::Favorite::FavoriteManager manager = ACAPI::Favorite::GetFavoriteManager ();
    const auto rootFolder = manager.GetRootFolder ();
    if (rootFolder.IsErr ()) {
        return CreateErrorResponse (rootFolder.UnwrapErr ().kind, "Failed to get the root folder of the Favorites.");
    }

    GS::ObjectState rootFolderOs;
    GS::UniString errorMessage;
    const GSErrCode err = AddFavoriteFolderToObjectState (rootFolder.Unwrap (), GS::UniString (), GS::Array<GS::UniString> (), rootFolderOs, errorMessage);
    if (err != NoError) {
        return CreateErrorResponse (err, errorMessage);
    }

    GS::ObjectState response;
    response.Add ("rootFolder", rootFolderOs);
    return response;
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}

// ============================================================================
// ReadFavoritesFile — wraps ACAPI_Favorite_Read, which reads the Favorites of a
// .prefs file or folder without importing them, so a caller can compare them
// with the project's Favorites before ImportFavorites overwrites anything.
// ============================================================================

ReadFavoritesFileCommand::ReadFavoritesFileCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ReadFavoritesFileCommand::GetName () const
{
    return "ReadFavoritesFile";
}

GS::Optional<GS::UniString> ReadFavoritesFileCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "path": {
                "type": "string",
                "description": "Absolute path on the AC host to a Favorites file (.prefs) or folder."
            }
        },
        "additionalProperties": false,
        "required": ["path"]
    })";
}

GS::Optional<GS::UniString> ReadFavoritesFileCommand::GetRawResponseSchema () const
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
                            "$ref": "#/ElementType"
                        },
                        "folder": {
                            "type": "array",
                            "items": { "type": "string" },
                            "description": "The folder hierarchy of the Favorite in the file or folder. Empty = root."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "elementType",
                        "folder"
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

GS::ObjectState ReadFavoritesFileCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString path;
    parameters.Get ("path", path);
    if (path.IsEmpty ()) {
        return CreateErrorResponse (APIERR_BADPARS, "path is required");
    }

    // Nothing is imported: ACAPI_Favorite_Read only fills this array, the project's
    // Favorites stay as they are.
    GS::Array<API_Favorite> favorites;
    const GSErrCode err = ACAPI_Favorite_Read (LocationFromPath (path), &favorites);

    GS::ObjectState response;
    const auto& favoritesList = response.AddList<GS::ObjectState> ("favorites");
    for (API_Favorite& favorite : favorites) {
        if (err == NoError) {
            GS::ObjectState favoriteOS;
            favoriteOS.Add ("name", favorite.name);
            favoriteOS.Add ("elementType", GetElementTypeNonLocalizedName (GetElemTypeId (favorite.element.header)));
            const auto& folder = favoriteOS.AddList<GS::UniString> ("folder");
            if (favorite.folder.HasValue ()) {
                for (const GS::UniString& part : *favorite.folder) {
                    folder (part);
                }
            }
            favoritesList (favoriteOS);
        }

        // Only the name, type and folder are returned; the memo handles the read
        // allocated are released here, also when it failed part way.
        if (favorite.memo.HasValue ()) {
            ACAPI_DisposeElemMemoHdls (&favorite.memo.Get ());
        }
        if (favorite.memoMarker.HasValue ()) {
            ACAPI_DisposeElemMemoHdls (&favorite.memoMarker.Get ());
        }
    }

    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to read the Favorites file or folder.");
    }

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
// CreateFavorite / ChangeFavorite / ApplyFavoriteToToolDefault / MoveFavoriteFolder
// — the ACAPI::Favorite::FavoriteManager calls of Archicad 29, which take the
// settings of a Favorite as an ACAPI::ElementDefault (from another Favorite or
// from a tool default, see WithFavoriteSource) instead of an element.
// ============================================================================

CreateFavoriteCommand::CreateFavoriteCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateFavoriteCommand::GetName () const
{
    return "CreateFavorite";
}

GS::Optional<GS::UniString> CreateFavoriteCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "name": {
                "type": "string",
                "description": "The name of the new Favorite."
            },
            "folder": {
                "type": "array",
                "items": { "type": "string" },
                "description": "The folder of the Favorites palette to create the Favorite in, as the folder names from the root down. The folder must exist. Empty or omitted = root."
            },
            "source": {
                "type": "object",
                "description": "Where the settings of the new Favorite come from: exactly one of favoriteName and toolDefault.",
                "properties": {
                    "favoriteName": {
                        "type": "string",
                        "description": "Take the settings of this Favorite."
                    },
                    "toolDefault": {
                        "type": "string",
                        "enum": ["Opening"],
                        "description": "Take the current settings of this tool."
                    }
                },
                "additionalProperties": false
            },
            "conflictPolicy": {
                "type": "string",
                "enum": ["Error", "Skip", "OverwriteExisting", "RenameNew"],
                "description": "What to do when a Favorite with this name already exists: Error fails, Skip leaves the existing Favorite unchanged, OverwriteExisting gives the existing Favorite the new settings (its folder does not change), RenameNew creates the Favorite with ' (2)', ' (3)', ... appended to the name. Default Error."
            }
        },
        "additionalProperties": false,
        "required": [
            "name",
            "source"
        ]
    })";
}

GS::Optional<GS::UniString> CreateFavoriteCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "name": {
                "type": "string",
                "description": "The name of the Favorite: the given name, or the new name with RenameNew."
            },
            "folder": {
                "type": "array",
                "items": { "type": "string" },
                "description": "The folder of the Favorite, as the folder names from the root down. Empty for the root. Missing when Archicad cannot read it."
            },
            "outcome": {
                "type": "string",
                "enum": ["Created", "Skipped", "Overwritten"],
                "description": "Created for a new Favorite (also with RenameNew), Skipped or Overwritten when a Favorite with the name already existed."
            }
        },
        "additionalProperties": false,
        "required": [
            "name",
            "outcome"
        ]
    })";
}

GS::ObjectState CreateFavoriteCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    GS::UniString name;
    parameters.Get ("name", name);
    if (name.IsEmpty ()) {
        return CreateErrorResponse (APIERR_BADPARS, "name is required.");
    }

    GS::Array<GS::UniString> folderPath;
    parameters.Get ("folder", folderPath);

    const GS::ObjectState* source = parameters.Get ("source");
    if (source == nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, "source is required.");
    }

    GS::UniString policyStr = "Error";
    parameters.Get ("conflictPolicy", policyStr);
    if (policyStr != "Error" && policyStr != "Skip" && policyStr != "OverwriteExisting" && policyStr != "RenameNew") {
        return CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("Invalid conflictPolicy '%T'.", policyStr.ToPrintf ()));
    }

    GS::UniString usedName = name;
    GS::UniString outcome = "Created";
    std::optional<ACAPI::Favorite::Favorite> resultFavorite;
    GS::UniString errorMessage;

    const GSErrCode err = ACAPI_CallUndoableCommand ("CreateFavorite", [&] () -> GSErrCode {
        const ACAPI::Favorite::FavoriteManager manager = ACAPI::Favorite::GetFavoriteManager ();

        const auto existing = manager.GetFavoriteByName (name);
        if (existing.IsOk ()) {
            if (policyStr == "Error") {
                errorMessage = GS::UniString::Printf ("A Favorite named '%T' already exists.", name.ToPrintf ());
                return APIERR_NAMEALREADYUSED;
            }
            if (policyStr == "Skip") {
                outcome = "Skipped";
                resultFavorite.emplace (existing.Unwrap ());
                return NoError;
            }
            if (policyStr == "OverwriteExisting") {
                outcome = "Overwritten";
                resultFavorite.emplace (existing.Unwrap ());
                return WithFavoriteSource (manager, *source, errorMessage, [&] (const ACAPI::ElementDefault& elementDefault) -> GSErrCode {
                    const auto changeResult = manager.ChangeFavorite (existing.Unwrap (), elementDefault);
                    if (changeResult.IsErr ()) {
                        errorMessage = GS::UniString::Printf ("Failed to change the Favorite: %s", changeResult.UnwrapErr ().text.c_str ());
                        return changeResult.UnwrapErr ().kind;
                    }
                    return NoError;
                });
            }

            // RenameNew: the first of "name (2)", "name (3)", ... that is not taken.
            bool foundFreeName = false;
            for (UInt32 suffix = 2; suffix < 10000 && !foundFreeName; ++suffix) {
                usedName = GS::UniString::Printf ("%T (%u)", name.ToPrintf (), suffix);
                foundFreeName = manager.GetFavoriteByName (usedName).IsErr ();
            }
            if (!foundFreeName) {
                errorMessage = GS::UniString::Printf ("Found no free name for the Favorite '%T'.", name.ToPrintf ());
                return APIERR_NAMEALREADYUSED;
            }
        }

        std::optional<ACAPI::Favorite::FavoriteFolder> folder;
        const GSErrCode findErr = FindFavoriteFolderOfPath (manager, folderPath, folder, errorMessage);
        if (findErr != NoError) {
            return findErr;
        }

        return WithFavoriteSource (manager, *source, errorMessage, [&] (const ACAPI::ElementDefault& elementDefault) -> GSErrCode {
            const auto created = manager.CreateFavorite (usedName, *folder, elementDefault);
            if (created.IsErr ()) {
                errorMessage = GS::UniString::Printf ("Failed to create the Favorite: %s", created.UnwrapErr ().text.c_str ());
                return created.UnwrapErr ().kind;
            }
            resultFavorite.emplace (created.Unwrap ());
            return NoError;
        });
    });

    if (err != NoError) {
        return CreateErrorResponse (err, errorMessage.IsEmpty () ? GS::UniString ("Failed to create the Favorite.") : errorMessage);
    }

    GS::ObjectState response;
    response.Add ("name", usedName);
    response.Add ("outcome", outcome);
    if (resultFavorite.has_value ()) {
        const auto parentFolder = resultFavorite->GetParentFolder ();
        GS::Array<GS::UniString> resultFolderPath;
        GS::UniString pathErrorMessage;
        if (parentFolder.IsOk () && GetPathOfFavoriteFolder (parentFolder.Unwrap (), resultFolderPath, pathErrorMessage) == NoError) {
            const auto& folderAdder = response.AddList<GS::UniString> ("folder");
            for (const GS::UniString& folderName : resultFolderPath) {
                folderAdder (folderName);
            }
        }
    }
    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}

ChangeFavoriteCommand::ChangeFavoriteCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ChangeFavoriteCommand::GetName () const
{
    return "ChangeFavorite";
}

GS::Optional<GS::UniString> ChangeFavoriteCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "favoriteName": {
                "type": "string",
                "description": "The name of the Favorite to change."
            },
            "source": {
                "type": "object",
                "description": "Where the new settings of the Favorite come from: exactly one of favoriteName and toolDefault.",
                "properties": {
                    "favoriteName": {
                        "type": "string",
                        "description": "Take the settings of this Favorite."
                    },
                    "toolDefault": {
                        "type": "string",
                        "enum": ["Opening"],
                        "description": "Take the current settings of this tool."
                    }
                },
                "additionalProperties": false
            }
        },
        "additionalProperties": false,
        "required": [
            "favoriteName",
            "source"
        ]
    })";
}

GS::Optional<GS::UniString> ChangeFavoriteCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState ChangeFavoriteCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    GS::UniString favoriteName;
    parameters.Get ("favoriteName", favoriteName);

    const GS::ObjectState* source = parameters.Get ("source");
    if (source == nullptr) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "source is required.");
    }

    GS::UniString errorMessage;
    const GSErrCode err = ACAPI_CallUndoableCommand ("ChangeFavorite", [&] () -> GSErrCode {
        const ACAPI::Favorite::FavoriteManager manager = ACAPI::Favorite::GetFavoriteManager ();
        const auto favorite = manager.GetFavoriteByName (favoriteName);
        if (favorite.IsErr ()) {
            errorMessage = GS::UniString::Printf ("The Favorite '%T' was not found.", favoriteName.ToPrintf ());
            return favorite.UnwrapErr ().kind;
        }

        return WithFavoriteSource (manager, *source, errorMessage, [&] (const ACAPI::ElementDefault& elementDefault) -> GSErrCode {
            const auto changeResult = manager.ChangeFavorite (favorite.Unwrap (), elementDefault);
            if (changeResult.IsErr ()) {
                errorMessage = GS::UniString::Printf ("Failed to change the Favorite: %s", changeResult.UnwrapErr ().text.c_str ());
                return changeResult.UnwrapErr ().kind;
            }
            return NoError;
        });
    });

    if (err != NoError) {
        return CreateFailedExecutionResult (err, errorMessage.IsEmpty () ? GS::UniString ("Failed to change the Favorite.") : errorMessage);
    }
    return CreateSuccessfulExecutionResult ();
#else
    UNUSED_PARAMETER (parameters);
    return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}

ApplyFavoriteToToolDefaultCommand::ApplyFavoriteToToolDefaultCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ApplyFavoriteToToolDefaultCommand::GetName () const
{
    return "ApplyFavoriteToToolDefault";
}

GS::Optional<GS::UniString> ApplyFavoriteToToolDefaultCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "favoriteName": {
                "type": "string",
                "description": "The name of the Favorite whose settings become the settings of its tool."
            }
        },
        "additionalProperties": false,
        "required": [
            "favoriteName"
        ]
    })";
}

GS::Optional<GS::UniString> ApplyFavoriteToToolDefaultCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementType": {
                "$ref": "#/ElementType",
                "description": "The element type of the tool whose settings changed."
            }
        },
        "additionalProperties": false,
        "required": [
            "elementType"
        ]
    })";
}

GS::ObjectState ApplyFavoriteToToolDefaultCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    GS::UniString favoriteName;
    parameters.Get ("favoriteName", favoriteName);

    const ACAPI::Favorite::FavoriteManager manager = ACAPI::Favorite::GetFavoriteManager ();
    const auto favorite = manager.GetFavoriteByName (favoriteName);
    if (favorite.IsErr ()) {
        return CreateErrorResponse (favorite.UnwrapErr ().kind, GS::UniString::Printf ("The Favorite '%T' was not found.", favoriteName.ToPrintf ()));
    }

    const auto elementDefault = favorite.Unwrap ().GetElementDefault ();
    if (elementDefault.IsErr ()) {
        return CreateErrorResponse (elementDefault.UnwrapErr ().kind, GS::UniString::Printf ("Failed to get the settings of the Favorite: %s", elementDefault.UnwrapErr ().text.c_str ()));
    }

    const auto setResult = elementDefault.Unwrap ().SetAsArchicadDefault ();
    if (setResult.IsErr ()) {
        return CreateErrorResponse (setResult.UnwrapErr ().kind, GS::UniString::Printf ("Failed to make the Favorite's settings the tool's settings: %s", setResult.UnwrapErr ().text.c_str ()));
    }

    return GS::ObjectState ("elementType", GetElementTypeNonLocalizedName (elementDefault.Unwrap ().GetElemType ().typeID));
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}

MoveFavoriteFolderCommand::MoveFavoriteFolderCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String MoveFavoriteFolderCommand::GetName () const
{
    return "MoveFavoriteFolder";
}

GS::Optional<GS::UniString> MoveFavoriteFolderCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "folder": {
                "type": "array",
                "items": { "type": "string" },
                "description": "The folder of the Favorites palette to move, as the folder names from the root down. The root folder cannot be moved."
            },
            "targetFolder": {
                "type": "array",
                "items": { "type": "string" },
                "description": "The folder to move it into, as the folder names from the root down. Empty = root. It cannot be the moved folder or one of its subfolders."
            }
        },
        "additionalProperties": false,
        "required": [
            "folder",
            "targetFolder"
        ]
    })";
}

GS::Optional<GS::UniString> MoveFavoriteFolderCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "folder": {
                "type": "array",
                "items": { "type": "string" },
                "description": "The new path of the moved folder, as the folder names from the root down."
            }
        },
        "additionalProperties": false,
        "required": [
            "folder"
        ]
    })";
}

GS::ObjectState MoveFavoriteFolderCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    GS::Array<GS::UniString> folderPath;
    GS::Array<GS::UniString> targetPath;
    parameters.Get ("folder", folderPath);
    parameters.Get ("targetFolder", targetPath);

    if (folderPath.IsEmpty ()) {
        return CreateErrorResponse (APIERR_BADPARS, "The root folder cannot be moved.");
    }

    bool targetIsInFolder = targetPath.GetSize () >= folderPath.GetSize ();
    for (UIndex ii = 0; targetIsInFolder && ii < folderPath.GetSize (); ++ii) {
        targetIsInFolder = targetPath[ii] == folderPath[ii];
    }
    if (targetIsInFolder) {
        return CreateErrorResponse (APIERR_BADPARS, "A folder cannot be moved into itself or into one of its subfolders.");
    }

    std::optional<ACAPI::Favorite::FavoriteFolder> folder;
    GS::UniString errorMessage;
    const GSErrCode err = ACAPI_CallUndoableCommand ("MoveFavoriteFolder", [&] () -> GSErrCode {
        const ACAPI::Favorite::FavoriteManager manager = ACAPI::Favorite::GetFavoriteManager ();

        GSErrCode findErr = FindFavoriteFolderOfPath (manager, folderPath, folder, errorMessage);
        if (findErr != NoError) {
            return findErr;
        }

        std::optional<ACAPI::Favorite::FavoriteFolder> targetFolder;
        findErr = FindFavoriteFolderOfPath (manager, targetPath, targetFolder, errorMessage);
        if (findErr != NoError) {
            return findErr;
        }

        const auto moveResult = manager.MoveFolder (*folder, *targetFolder);
        if (moveResult.IsErr ()) {
            errorMessage = GS::UniString::Printf ("Failed to move the folder: %s", moveResult.UnwrapErr ().text.c_str ());
            return moveResult.UnwrapErr ().kind;
        }
        return NoError;
    });

    if (err != NoError) {
        return CreateErrorResponse (err, errorMessage.IsEmpty () ? GS::UniString ("Failed to move the folder.") : errorMessage);
    }

    // Read after the undoable command, so that a failure here cannot undo the move.
    GS::Array<GS::UniString> newPath;
    const GSErrCode pathErr = GetPathOfFavoriteFolder (*folder, newPath, errorMessage);
    if (pathErr != NoError) {
        return CreateErrorResponse (pathErr, GS::UniString ("The folder was moved, but its new path could not be read: ") + errorMessage);
    }

    GS::ObjectState response;
    const auto& folderAdder = response.AddList<GS::UniString> ("folder");
    for (const GS::UniString& folderName : newPath) {
        folderAdder (folderName);
    }
    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}