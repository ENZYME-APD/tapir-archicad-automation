#include "ElementCategoryCommands.hpp"
#include "MigrationHelper.hpp"

#include <map>

// The names of the element categories (Structural Function, Position, Renovation Status ...)
// and of their values are localized, so the commands identify a category by its
// API_ElemCategoryID, carried as one of the strings below (the ElementCategoryType schema),
// and a value by its guid. The names are returned for display only.
const char* CategoryTypeToString (API_ElemCategoryID categoryID)
{
    switch (categoryID) {
        case API_ElemCategory_StructuralFunction: return "StructuralFunction";
        case API_ElemCategory_Position:           return "Position";
        case API_ElemCategory_RenovationStatus:   return "RenovationStatus";
        case API_ElemCategory_RenovationFilter:   return "RenovationFilter";
        case API_ElemCategory_BRI:                return "BRI";
        default:                                  return nullptr;
    }
}

static bool CategoryTypeFromString (const GS::UniString& str, API_ElemCategoryID& categoryID)
{
    static const API_ElemCategoryID knownCategoryIDs[] = {
        API_ElemCategory_StructuralFunction,
        API_ElemCategory_Position,
        API_ElemCategory_RenovationStatus,
        API_ElemCategory_RenovationFilter,
        API_ElemCategory_BRI
    };
    for (API_ElemCategoryID knownCategoryID : knownCategoryIDs) {
        if (str == CategoryTypeToString (knownCategoryID)) {
            categoryID = knownCategoryID;
            return true;
        }
    }
    return false;
}

static const API_ElemCategory* FindCategory (const GS::Array<API_ElemCategory>& categories, API_ElemCategoryID categoryID)
{
    for (const API_ElemCategory& category : categories) {
        if (category.categoryID == categoryID) {
            return &category;
        }
    }
    return nullptr;
}

static GS::ObjectState CreateCategoryValueObjectState (const API_ElemCategory& category, const API_ElemCategoryValue& value)
{
    return GS::ObjectState (
        "type", GS::UniString (CategoryTypeToString (category.categoryID)),
        "categoryId", CreateGuidObjectState (category.guid),
        "categoryValueId", CreateGuidObjectState (value.guid),
        "name", GS::UniString (value.name));
}

// The categories given in the optional "categories" parameter, in the requested order; every
// known category when none is given.
static GSErrCode GetRequestedCategories (const GS::ObjectState& parameters, GS::Array<API_ElemCategory>& categories, GS::UniString& errorMessage)
{
    GS::Array<API_ElemCategory> allCategories;
    const GSErrCode err = TAPIR_GetElementCategories (allCategories);
    if (err != NoError) {
        errorMessage = "Failed to get the element categories.";
        return err;
    }

    GS::Array<GS::UniString> categoryTypes;
    if (parameters.Get ("categories", categoryTypes) && !categoryTypes.IsEmpty ()) {
        for (const GS::UniString& categoryType : categoryTypes) {
            API_ElemCategoryID categoryID = API_ElemCategory_Undefined;
            if (!CategoryTypeFromString (categoryType, categoryID)) {
                errorMessage = GS::UniString::Printf ("Unknown category type '%T'.", categoryType.ToPrintf ());
                return APIERR_BADPARS;
            }
            const API_ElemCategory* category = FindCategory (allCategories, categoryID);
            if (category == nullptr) {
                errorMessage = GS::UniString::Printf ("The %T category is not available in this project.", categoryType.ToPrintf ());
                return APIERR_BADPARS;
            }
            categories.Push (*category);
        }
    } else {
        for (const API_ElemCategory& category : allCategories) {
            if (CategoryTypeToString (category.categoryID) != nullptr) {
                categories.Push (category);
            }
        }
    }

    return NoError;
}

GetElementCategoriesCommand::GetElementCategoriesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetElementCategoriesCommand::GetName () const
{
    return "GetElementCategories";
}

GS::Optional<GS::UniString> GetElementCategoriesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "categories": {
                "type": "array",
                "description": "The element categories of the project, each with its possible values.",
                "items": {
                    "type": "object",
                    "properties": {
                        "categoryId": {
                            "$ref": "#/ElementCategoryId"
                        },
                        "type": {
                            "$ref": "#/ElementCategoryType"
                        },
                        "name": {
                            "type": "string",
                            "description": "The localized name of the category, for display only."
                        },
                        "values": {
                            "type": "array",
                            "description": "The possible values of the category.",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "categoryValueId": {
                                        "$ref": "#/ElementCategoryValueId"
                                    },
                                    "name": {
                                        "type": "string",
                                        "description": "The localized name of the value, for display only."
                                    },
                                    "isVisibleOnUI": {
                                        "type": "boolean",
                                        "description": "The value is offered in the settings dialogs of Archicad."
                                    },
                                    "usableAsCriteria": {
                                        "type": "boolean",
                                        "description": "The value can be used as a criterion, for example in Find & Select and in Graphic Override rules."
                                    }
                                },
                                "additionalProperties": false,
                                "required": [
                                    "categoryValueId",
                                    "name",
                                    "isVisibleOnUI",
                                    "usableAsCriteria"
                                ]
                            }
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "categoryId",
                        "type",
                        "name",
                        "values"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "categories"
        ]
    })";
}

GS::ObjectState GetElementCategoriesCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<API_ElemCategory> categories;
    GSErrCode err = TAPIR_GetElementCategories (categories);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the element categories.");
    }

    GS::ObjectState response;
    const auto& categoryList = response.AddList<GS::ObjectState> ("categories");

    for (const API_ElemCategory& category : categories) {
        const char* type = CategoryTypeToString (category.categoryID);
        if (type == nullptr) {
            continue;
        }

        GS::Array<API_ElemCategoryValue> values;
        err = TAPIR_GetElementCategoryValues (category, values);
        if (err != NoError) {
            return CreateErrorResponse (err, GS::UniString::Printf ("Failed to get the values of the %s category.", type));
        }

        GS::ObjectState categoryOS (
            "categoryId", CreateGuidObjectState (category.guid),
            "type", GS::UniString (type),
            "name", GS::UniString (category.name));
        const auto& valueList = categoryOS.AddList<GS::ObjectState> ("values");
        for (const API_ElemCategoryValue& value : values) {
            valueList (GS::ObjectState (
                "categoryValueId", CreateGuidObjectState (value.guid),
                "name", GS::UniString (value.name),
                "isVisibleOnUI", value.isVisibleOnUI,
                "usableAsCriteria", value.usableAsCriteria));
        }
        categoryList (categoryOS);
    }

    return response;
}

GetCategoryValuesOfElementsCommand::GetCategoryValuesOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetCategoryValuesOfElementsCommand::GetName () const
{
    return "GetCategoryValuesOfElements";
}

GS::Optional<GS::UniString> GetCategoryValuesOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            },
            "categories": {
                "type": "array",
                "description": "The categories to read. Left out: every category the element has.",
                "items": {
                    "$ref": "#/ElementCategoryType"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> GetCategoryValuesOfElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "categoryValuesOfElements": {
                "type": "array",
                "description": "The category values of each element, in the order of the input. A category the element does not have is left out of its list; an element that does not exist or has none of the requested categories (an annotation, for example) is represented by an error object.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "categoryValues": {
                                    "type": "array",
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
                                }
                            },
                            "additionalProperties": false,
                            "required": [
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
            "categoryValuesOfElements"
        ]
    })";
}

GS::ObjectState GetCategoryValuesOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::Array<API_ElemCategory> categories;
    GS::UniString errorMessage;
    const GSErrCode err = GetRequestedCategories (parameters, categories, errorMessage);
    if (err != NoError) {
        return CreateErrorResponse (err, errorMessage);
    }

    GS::ObjectState response;
    const auto& categoryValuesOfElements = response.AddList<GS::ObjectState> ("categoryValuesOfElements");

    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            categoryValuesOfElements (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        const API_Guid elemGuid = GetGuidFromObjectState (*elementId);
        API_Elem_Head header = {};
        if (!LoadElementHeaderByGuid (elemGuid, header)) {
            categoryValuesOfElements (CreateErrorResponse (APIERR_BADID, "Element not found."));
            continue;
        }

        GS::ObjectState categoryValuesOfElement;
        const auto& categoryValues = categoryValuesOfElement.AddList<GS::ObjectState> ("categoryValues");
        UInt32 foundCategories = 0;
        for (const API_ElemCategory& category : categories) {
            // An element type without this category (an annotation, for example) fails here or
            // yields no value; the category is left out of the element's list.
            API_ElemCategoryValue value = {};
            if (TAPIR_Element_GetCategoryValue (elemGuid, category, value) != NoError || value.guid == APINULLGuid) {
                continue;
            }
            categoryValues (CreateCategoryValueObjectState (category, value));
            ++foundCategories;
        }

        if (foundCategories == 0) {
            categoryValuesOfElements (CreateErrorResponse (APIERR_GENERAL, "The element has none of the requested categories."));
            continue;
        }

        categoryValuesOfElements (categoryValuesOfElement);
    }

    return response;
}

GetCategoryValuesOfDefaultsCommand::GetCategoryValuesOfDefaultsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetCategoryValuesOfDefaultsCommand::GetName () const
{
    return "GetCategoryValuesOfDefaults";
}

GS::Optional<GS::UniString> GetCategoryValuesOfDefaultsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementTypes": {
                "type": "array",
                "description": "The element types whose tool defaults (the settings new elements get) are read.",
                "items": {
                    "$ref": "#/ElementType"
                }
            },
            "categories": {
                "type": "array",
                "description": "The categories to read. Left out: every category of the project.",
                "items": {
                    "$ref": "#/ElementCategoryType"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementTypes"
        ]
    })";
}

GS::Optional<GS::UniString> GetCategoryValuesOfDefaultsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "categoryValuesOfDefaults": {
                "type": "array",
                "description": "The category values of the tool defaults of each element type, in the order of the input. A category Archicad cannot read for the type is left out of its list; an invalid element type, or one with none of the requested categories, is represented by an error object.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "categoryValues": {
                                    "type": "array",
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
                                                "$ref": "#/ElementCategoryValueId",
                                                "description": "Left out when the default has no value in this category."
                                            },
                                            "name": {
                                                "type": "string",
                                                "description": "The localized name of the value, for display only. Left out when the default has no value in this category."
                                            }
                                        },
                                        "additionalProperties": false,
                                        "required": [
                                            "type",
                                            "categoryId"
                                        ]
                                    }
                                }
                            },
                            "additionalProperties": false,
                            "required": [
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
            "categoryValuesOfDefaults"
        ]
    })";
}

GS::ObjectState GetCategoryValuesOfDefaultsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::UniString> elementTypes;
    parameters.Get ("elementTypes", elementTypes);

    GS::Array<API_ElemCategory> categories;
    GS::UniString errorMessage;
    const GSErrCode err = GetRequestedCategories (parameters, categories, errorMessage);
    if (err != NoError) {
        return CreateErrorResponse (err, errorMessage);
    }

    GS::ObjectState response;
    const auto& categoryValuesOfDefaults = response.AddList<GS::ObjectState> ("categoryValuesOfDefaults");

    for (const GS::UniString& elementTypeStr : elementTypes) {
        const API_ElemTypeID typeID = GetElementTypeFromNonLocalizedName (elementTypeStr);
        if (typeID == API_ZombieElemID) {
            categoryValuesOfDefaults (CreateErrorResponse (APIERR_BADPARS,
                GS::UniString::Printf ("Invalid elementType '%T'.", elementTypeStr.ToPrintf ())));
            continue;
        }

        GS::ObjectState categoryValuesOfDefault;
        const auto& categoryValues = categoryValuesOfDefault.AddList<GS::ObjectState> ("categoryValues");
        UInt32 foundCategories = 0;
        for (const API_ElemCategory& category : categories) {
            // A type without this category (an annotation, for example) fails here; the category
            // is left out of the type's list.
            API_ElemCategoryValue value = {};
            if (TAPIR_Element_GetCategoryValueDefault (typeID, category, value) != NoError) {
                continue;
            }
            if (value.guid == APINULLGuid) {
                categoryValues (GS::ObjectState (
                    "type", GS::UniString (CategoryTypeToString (category.categoryID)),
                    "categoryId", CreateGuidObjectState (category.guid)));
            } else {
                categoryValues (CreateCategoryValueObjectState (category, value));
            }
            ++foundCategories;
        }

        if (foundCategories == 0) {
            categoryValuesOfDefaults (CreateErrorResponse (APIERR_GENERAL, GS::UniString::Printf ("The defaults of elementType '%T' have none of the requested categories.", elementTypeStr.ToPrintf ())));
            continue;
        }

        categoryValuesOfDefaults (categoryValuesOfDefault);
    }

    return response;
}

SetCategoryValuesOfElementsCommand::SetCategoryValuesOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetCategoryValuesOfElementsCommand::GetName () const
{
    return "SetCategoryValuesOfElements";
}

GS::Optional<GS::UniString> SetCategoryValuesOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementCategoryValues": {
                "type": "array",
                "description": "The category values to set: one element and one category per item. The same element can appear in several items, one per category.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "type": {
                            "$ref": "#/ElementCategoryType"
                        },
                        "categoryValueId": {
                            "$ref": "#/ElementCategoryValueId",
                            "description": "A value of the category given by type, as GetElementCategories lists them."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "type",
                        "categoryValueId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementCategoryValues"
        ]
    })";
}

GS::Optional<GS::UniString> SetCategoryValuesOfElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults",
                "description": "One result per item of the input, in the same order."
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    })";
}

GS::ObjectState SetCategoryValuesOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elementCategoryValues;
    parameters.Get ("elementCategoryValues", elementCategoryValues);

    GS::Array<API_ElemCategory> categories;
    const GSErrCode err = TAPIR_GetElementCategories (categories);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the element categories.");
    }

    // The values of each category, read once on first use: a value guid is accepted only when it
    // belongs to the category it is set for.
    std::map<API_ElemCategoryID, GS::Array<API_ElemCategoryValue>> valuesOfCategories;

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("SetCategoryValuesOfElements", [&] () -> GSErrCode {
        for (const GS::ObjectState& item : elementCategoryValues) {
            const GS::ObjectState* elementId = item.Get ("elementId");
            if (elementId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
                continue;
            }

            const GS::ObjectState* categoryValueId = item.Get ("categoryValueId");
            if (categoryValueId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "categoryValueId is missing"));
                continue;
            }

            GS::UniString categoryType;
            API_ElemCategoryID categoryID = API_ElemCategory_Undefined;
            if (!item.Get ("type", categoryType) || !CategoryTypeFromString (categoryType, categoryID)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, GS::UniString::Printf ("Unknown category type '%T'.", categoryType.ToPrintf ())));
                continue;
            }

            const API_ElemCategory* category = FindCategory (categories, categoryID);
            if (category == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, GS::UniString::Printf ("The %T category is not available in this project.", categoryType.ToPrintf ())));
                continue;
            }

            if (valuesOfCategories.find (categoryID) == valuesOfCategories.end ()) {
                GS::Array<API_ElemCategoryValue> values;
                const GSErrCode valuesErr = TAPIR_GetElementCategoryValues (*category, values);
                if (valuesErr != NoError) {
                    executionResults (CreateFailedExecutionResult (valuesErr, GS::UniString::Printf ("Failed to get the values of the %T category.", categoryType.ToPrintf ())));
                    continue;
                }
                valuesOfCategories[categoryID] = values;
            }

            const API_Guid valueGuid = GetGuidFromObjectState (*categoryValueId);
            const API_ElemCategoryValue* value = nullptr;
            for (const API_ElemCategoryValue& candidate : valuesOfCategories[categoryID]) {
                if (candidate.guid == valueGuid) {
                    value = &candidate;
                    break;
                }
            }
            if (value == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, GS::UniString::Printf ("categoryValueId %T is not a value of the %T category. GetElementCategories lists the values of each category.",
                    APIGuidToString (valueGuid).ToPrintf (), categoryType.ToPrintf ())));
                continue;
            }

            const API_Guid elemGuid = GetGuidFromObjectState (*elementId);
            API_Elem_Head header = {};
            if (!LoadElementHeaderByGuid (elemGuid, header)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADID, "Element not found."));
                continue;
            }

            API_ElemCategoryValue valueToSet = *value;
            valueToSet.category = *category;
            const GSErrCode setErr = TAPIR_Element_SetCategoryValue (elemGuid, valueToSet);
            if (setErr != NoError) {
                executionResults (CreateFailedExecutionResult (setErr, GS::UniString::Printf ("Failed to set the %T category of the element. Elements of this type may not have this category.", categoryType.ToPrintf ())));
                continue;
            }

            executionResults (CreateSuccessfulExecutionResult ());
        }

        return NoError;
    });

    return response;
}
