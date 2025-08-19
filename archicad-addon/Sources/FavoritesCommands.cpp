#include "FavoritesCommands.hpp"
#include "MigrationHelper.hpp"


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
        "type": "object",
        "properties": {
            "favorites": "#/Favorites"
        },
        "additionalProperties": false,
        "required": [
            "favorites"
        ]
    })";
}

GS::ObjectState GetFavoritesByTypeCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{   
    API_ElemTypeID elemType = API_ZombieElemID;
    GS::UniString elementTypeStr;
    if (parameters.Get ("elementType", elementTypeStr)) {
        elemType = GetElementTypeFromNonLocalizedName (elementTypeStr);
    }

    GS::Array< GS::UniString > names;

    GSErrCode err = ACAPI_Favorite_GetNum (elemType, nullptr, nullptr, &names);
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
            "favorites": "#/Favorites"
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