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

GS::Optional<GS::UniString> GetFavoritesByTypeCommand::GetResponseSchema () const
{
    return R"({
        "$ref": "#/GetFavoritesByTypeResponseOrError"
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

GS::Optional<GS::UniString> GetFavoritePreviewImageCommand::GetResponseSchema () const
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

GS::Optional<GS::UniString> ApplyFavoritesToElementDefaultsCommand::GetResponseSchema () const
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

GS::Optional<GS::UniString> CreateFavoritesFromElementsCommand::GetResponseSchema () const
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

    ACAPI_CallUndoableCommand ("CreateFavoritesFromElements", [&]() -> GSErrCode {
        for (const GS::ObjectState& favoriteFromElement : favoritesFromElements) {
            favoriteFromElement.Get ("favorite", favorite.name);

            favorite.element.header.guid = GetGuidFromElementsArrayItem (favoriteFromElement);
            GSErrCode err = ACAPI_Element_Get (&favorite.element);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to find element"));
                continue;
            }

            err = ACAPI_Element_GetClassificationItems (favorite.element.header.guid, favorite.classifications.Get ());
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to get the classifications of the element"));
                continue;
            }

            GS::Array<API_PropertyDefinition> definitions;
            err = ACAPI_Element_GetPropertyDefinitions (favorite.element.header.guid, API_PropertyDefinitionFilter_UserDefined, definitions);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to get the properties of the element"));
                continue;
            }

            err = ACAPI_Element_GetPropertyValues (favorite.element.header.guid, definitions, favorite.properties.Get ());
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to get the property values of the element"));
                continue;
            }
            for (UIndex ii = favorite.properties->GetSize (); ii >= 1; --ii) {
                const API_Property& p = favorite.properties->Get (ii - 1);
                if (p.isDefault || p.definition.canValueBeEditable == false || p.status != API_Property_HasValue)
                    favorite.properties->Delete (ii - 1);
            }

            *favorite.memo = {};

            err = ACAPI_Element_GetMemo	(favorite.element.header.guid, favorite.memo.GetPtr ());
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to get details of the element"));
                continue;
            }

            err = ACAPI_Favorite_Create (favorite);
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