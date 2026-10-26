#include "ClassificationCommands.hpp"
#include "MigrationHelper.hpp"

#ifdef ServerMainVers_2900
#include <chrono>
#endif

GetClassificationsOfElementsCommand::GetClassificationsOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetClassificationsOfElementsCommand::GetName () const
{
    return "GetClassificationsOfElements";
}

GS::Optional<GS::UniString> GetClassificationsOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            },
            "classificationSystemIds": {
                "$ref": "#/ClassificationSystemIds"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements",
            "classificationSystemIds"
        ]
    })";
}

GS::Optional<GS::UniString> GetClassificationsOfElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementClassifications": {
                "$ref": "#/ElementClassificationsOrErrors",
                "description": "Order of the ids are the same as in the input. Non-existing elements or non-existing classification systems are represented by error objects."
            }
        },
        "additionalProperties": false,
        "required": [
            "elementClassifications"
        ]
    })";
}

GS::ObjectState GetClassificationsOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::Array<GS::ObjectState> classificationSystemIds;
    parameters.Get ("classificationSystemIds", classificationSystemIds);

    GS::ObjectState response;
    const auto& elementClassifications = response.AddList<GS::ObjectState> ("elementClassifications");

    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            elementClassifications (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        const API_Guid elemGuid = GetGuidFromObjectState (*elementId);

        GS::ObjectState elementClassification;
        const auto& classificationIds = elementClassification.AddList<GS::ObjectState> ("classificationIds");

        for (const GS::ObjectState& classificationSystemId : classificationSystemIds) {
            const GS::ObjectState* systemId = classificationSystemId.Get ("classificationSystemId");
            if (systemId == nullptr) {
                classificationIds (CreateErrorResponse (APIERR_BADPARS, "classificationSystemId is missing"));
                continue;
            }

            const API_Guid systemGuid = GetGuidFromObjectState (*systemId);

            API_ClassificationItem item;
            GSErrCode err = ACAPI_Element_GetClassificationInSystem (elemGuid, systemGuid, item);

            if (err != NoError) {
                classificationIds (CreateErrorResponse (err, "Failed to get classification item"));
                continue;
            }

            classificationIds (GS::ObjectState (
                "classificationSystemId", GS::ObjectState ("guid", APIGuidToString (systemGuid)),
                "classificationItemId", GS::ObjectState ("guid", APIGuidToString (item.guid))));
        }

        elementClassifications (elementClassification);
    }

    return response;
}

SetClassificationsOfElementsCommand::SetClassificationsOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetClassificationsOfElementsCommand::GetName () const
{
    return "SetClassificationsOfElements";
}

GS::Optional<GS::UniString> SetClassificationsOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementClassifications": {
                "$ref": "#/ElementClassifications"
            }
        },
        "additionalProperties": false,
        "required": [
            "elementClassifications"
        ]
    })";
}

GS::Optional<GS::UniString> SetClassificationsOfElementsCommand::GetRawResponseSchema () const
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

GS::ObjectState SetClassificationsOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elementClassifications;
    parameters.Get ("elementClassifications", elementClassifications);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("SetClassificationsOfElementsCommand", [&] () -> GSErrCode {
        for (const GS::ObjectState& elementClassification : elementClassifications) {
            const GS::ObjectState* elementId = elementClassification.Get ("elementId");
            if (elementId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
                continue;
            }

            const API_Guid elemGuid = GetGuidFromObjectState (*elementId);

            const GS::ObjectState* classificationId = elementClassification.Get ("classificationId");
            if (classificationId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "classificationId is missing"));
                continue;
            }

            const GS::ObjectState* classificationItemId = classificationId->Get ("classificationItemId");
            if (classificationItemId != nullptr) {
                const API_Guid classificationItemGuid = GetGuidFromObjectState (*classificationItemId);

                const GSErrCode err = ACAPI_Element_AddClassificationItem (elemGuid, classificationItemGuid);
                if (err != NoError) {
                    executionResults (CreateFailedExecutionResult (err, "Failed to set classification item for element"));
                    continue;
                }
            } else {
                const GS::ObjectState* classificationSystemId = classificationId->Get ("classificationSystemId");
                if (classificationSystemId == nullptr) {
                    executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "classificationSystemId is missing"));
                    continue;
                }

                const API_Guid classificationSystemGuid = GetGuidFromObjectState (*classificationSystemId);

                API_ClassificationItem item;
                GSErrCode err = ACAPI_Element_GetClassificationInSystem (elemGuid, classificationSystemGuid, item);
                if (err != NoError) {
                    executionResults (CreateFailedExecutionResult (err, "Failed to get classification item for element"));
                    continue;
                }

                err = ACAPI_Element_RemoveClassificationItem (elemGuid, item.guid);
                if (err != NoError) {
                    executionResults (CreateFailedExecutionResult (err, "Failed to remove classification item for element"));
                    continue;
                }
            }

            executionResults (CreateSuccessfulExecutionResult ());
        }

        return NoError;
    });

    return response;
}

GetClassificationsOfElementDefaultsCommand::GetClassificationsOfElementDefaultsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetClassificationsOfElementDefaultsCommand::GetName () const
{
    return "GetClassificationsOfElementDefaults";
}

GS::Optional<GS::UniString> GetClassificationsOfElementDefaultsCommand::GetInputParametersSchema () const
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
            "classificationSystemIds": {
                "$ref": "#/ClassificationSystemIds"
            }
        },
        "additionalProperties": false,
        "required": [
            "elementTypes",
            "classificationSystemIds"
        ]
    })";
}

GS::Optional<GS::UniString> GetClassificationsOfElementDefaultsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementDefaultClassifications": {
                "$ref": "#/ElementClassificationsOrErrors",
                "description": "Order of the outer list is that of the given element types, the inner list that of the given classification systems. Invalid element types or non-existing classification systems are represented by error objects."
            }
        },
        "additionalProperties": false,
        "required": [
            "elementDefaultClassifications"
        ]
    })";
}

GS::ObjectState GetClassificationsOfElementDefaultsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::UniString> elementTypes;
    parameters.Get ("elementTypes", elementTypes);

    GS::Array<GS::ObjectState> classificationSystemIds;
    parameters.Get ("classificationSystemIds", classificationSystemIds);

    GS::ObjectState response;
    const auto& elementDefaultClassifications = response.AddList<GS::ObjectState> ("elementDefaultClassifications");

    for (const GS::UniString& elementTypeStr : elementTypes) {
        const API_ElemTypeID typeID = GetElementTypeFromNonLocalizedName (elementTypeStr);
        if (typeID == API_ZombieElemID) {
            elementDefaultClassifications (CreateErrorResponse (APIERR_BADPARS,
                GS::UniString::Printf ("Invalid elementType '%T'.", elementTypeStr.ToPrintf ())));
            continue;
        }

        GS::ObjectState elementClassification;
        const auto& classificationIds = elementClassification.AddList<GS::ObjectState> ("classificationIds");

        for (const GS::ObjectState& classificationSystemId : classificationSystemIds) {
            const GS::ObjectState* systemId = classificationSystemId.Get ("classificationSystemId");
            if (systemId == nullptr) {
                classificationIds (CreateErrorResponse (APIERR_BADPARS, "classificationSystemId is missing"));
                continue;
            }

            const API_Guid systemGuid = GetGuidFromObjectState (*systemId);

            API_ClassificationItem item;
            const GSErrCode err = TAPIR_Element_GetClassificationInSystemDefault (typeID, systemGuid, item);
            if (err != NoError) {
                classificationIds (CreateErrorResponse (err, "Failed to get classification item"));
                continue;
            }

            GS::ObjectState classificationId ("classificationSystemId", GS::ObjectState ("guid", APIGuidToString (systemGuid)));
            if (item.guid != APINULLGuid) {
                classificationId.Add ("classificationItemId", GS::ObjectState ("guid", APIGuidToString (item.guid)));
            }
            classificationIds (classificationId);
        }

        elementDefaultClassifications (elementClassification);
    }

    return response;
}

SetClassificationsOfElementDefaultsCommand::SetClassificationsOfElementDefaultsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetClassificationsOfElementDefaultsCommand::GetName () const
{
    return "SetClassificationsOfElementDefaults";
}

GS::Optional<GS::UniString> SetClassificationsOfElementDefaultsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementDefaultClassifications": {
                "type": "array",
                "description": "The classifications to set on the tool defaults (the settings new elements get).",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementType": {
                            "$ref": "#/ElementType"
                        },
                        "classificationId": {
                            "$ref": "#/ClassificationId"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementType",
                        "classificationId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementDefaultClassifications"
        ]
    })";
}

GS::Optional<GS::UniString> SetClassificationsOfElementDefaultsCommand::GetRawResponseSchema () const
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

GS::ObjectState SetClassificationsOfElementDefaultsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elementDefaultClassifications;
    parameters.Get ("elementDefaultClassifications", elementDefaultClassifications);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("SetClassificationsOfElementDefaultsCommand", [&] () -> GSErrCode {
        for (const GS::ObjectState& elementDefaultClassification : elementDefaultClassifications) {
            GS::UniString elementTypeStr;
            if (!elementDefaultClassification.Get ("elementType", elementTypeStr)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "elementType is missing"));
                continue;
            }

            const API_ElemTypeID typeID = GetElementTypeFromNonLocalizedName (elementTypeStr);
            if (typeID == API_ZombieElemID) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS,
                    GS::UniString::Printf ("Invalid elementType '%T'.", elementTypeStr.ToPrintf ())));
                continue;
            }

            const GS::ObjectState* classificationId = elementDefaultClassification.Get ("classificationId");
            if (classificationId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "classificationId is missing"));
                continue;
            }

            const GS::ObjectState* classificationItemId = classificationId->Get ("classificationItemId");
            if (classificationItemId != nullptr) {
                const API_Guid classificationItemGuid = GetGuidFromObjectState (*classificationItemId);

                const GSErrCode err = TAPIR_Element_AddClassificationItemDefault (typeID, classificationItemGuid);
                if (err != NoError) {
                    executionResults (CreateFailedExecutionResult (err, "Failed to set classification item for the element defaults"));
                    continue;
                }
            } else {
                const GS::ObjectState* classificationSystemId = classificationId->Get ("classificationSystemId");
                if (classificationSystemId == nullptr) {
                    executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "classificationSystemId is missing"));
                    continue;
                }

                const API_Guid classificationSystemGuid = GetGuidFromObjectState (*classificationSystemId);

                API_ClassificationItem item;
                GSErrCode err = TAPIR_Element_GetClassificationInSystemDefault (typeID, classificationSystemGuid, item);
                if (err != NoError) {
                    executionResults (CreateFailedExecutionResult (err, "Failed to get classification item for the element defaults"));
                    continue;
                }

                if (item.guid != APINULLGuid) {
                    err = TAPIR_Element_RemoveClassificationItemDefault (typeID, item.guid);
                    if (err != NoError) {
                        executionResults (CreateFailedExecutionResult (err, "Failed to remove classification item for the element defaults"));
                        continue;
                    }
                }
            }

            executionResults (CreateSuccessfulExecutionResult ());
        }

        return NoError;
    });

    return response;
}

CreateClassificationSystemsCommand::CreateClassificationSystemsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateClassificationSystemsCommand::GetName () const
{
    return "CreateClassificationSystems";
}

GS::Optional<GS::UniString> CreateClassificationSystemsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "classificationSystemsWithItems": {
                "$ref": "#/ClassificationSystemsWithItems"
            }
        },
        "additionalProperties": false,
        "required": [
           "classificationSystemsWithItems"
        ]
    })";
}

GS::Optional<GS::UniString> CreateClassificationSystemsCommand::GetRawResponseSchema () const
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

GSErrCode CreateClassificationItemsRecursively (const GS::Array<GS::ObjectState>& classificationItems, const API_Guid& classificationSystemGuid, const API_Guid& currentParentGuid)
{
    for (const GS::ObjectState& classificationItem : classificationItems) {
        API_ClassificationItem apiClassificationItem = {};
        apiClassificationItem.guid = APINULLGuid;
        classificationItem.Get ("name", apiClassificationItem.name);
        classificationItem.Get ("description", apiClassificationItem.description);
        classificationItem.Get ("id", apiClassificationItem.id);
        GSErrCode err = ACAPI_Classification_CreateClassificationItem(apiClassificationItem, classificationSystemGuid, currentParentGuid, APINULLGuid);
        if (err != NoError) {
            return err;
        }
        GS::Array<GS::ObjectState> childItems;
        if (classificationItem.Get ("children", childItems) && childItems.GetSize() > 0) {
            CreateClassificationItemsRecursively(childItems, classificationSystemGuid, apiClassificationItem.guid);
        }
    }

    return NoError;
}

GS::ObjectState CreateClassificationSystemsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> classificationSystemsWithItems;
    parameters.Get ("classificationSystemsWithItems", classificationSystemsWithItems);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("CreateClassificationSystemsCommand", [&] () -> GSErrCode {
        for (const GS::ObjectState& classificationSystemWithItems : classificationSystemsWithItems) {
            GS::ObjectState classificationSystem;
            if (!classificationSystemWithItems.Get ("classificationSystem", classificationSystem)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Failed to get classification system details"));
                continue;
            }

            API_ClassificationSystem apiClassificationSystem = {};
            apiClassificationSystem.guid = APINULLGuid;
            classificationSystem.Get ("name", apiClassificationSystem.name);
            classificationSystem.Get ("description", apiClassificationSystem.description);
            classificationSystem.Get ("source", apiClassificationSystem.source);
            classificationSystem.Get ("version", apiClassificationSystem.editionVersion);
            GS::UniString date;
            classificationSystem.Get ("date", date);
            unsigned int year, month, day;
            date.SScanf ("%4u-%2u-%2u", &year, &month, &day);
#ifdef ServerMainVers_2900
            apiClassificationSystem.editionDate = std::chrono::year_month_day(std::chrono::year (year), std::chrono::month (month), std::chrono::day (day));
#else
            apiClassificationSystem.editionDate = GSDateRecord((unsigned short)year, (unsigned short)month, (unsigned short)day);
#endif

            GSErrCode err = ACAPI_Classification_CreateClassificationSystem (apiClassificationSystem);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to create classification system"));
                continue;
            }

            GS::Array<GS::ObjectState> classificationItems;
            if (!classificationSystemWithItems.Get ("classificationItems", classificationItems) || classificationItems.GetSize() == 0) {
                continue;
            }

            err = CreateClassificationItemsRecursively(classificationItems, apiClassificationSystem.guid, APINULLGuid);
            if (err == NoError) {
                executionResults (CreateSuccessfulExecutionResult ());
            } else {
                executionResults (CreateFailedExecutionResult (err, "Failed to create items into classification system"));
            }
        }

        return NoError;
    });

    return response;
}

CreateClassificationItemsCommand::CreateClassificationItemsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateClassificationItemsCommand::GetName () const
{
    return "CreateClassificationItems";
}

GS::Optional<GS::UniString> CreateClassificationItemsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "newClassificationItems": {
                "$ref": "#/NewClassificationItems"
            }
        },
        "additionalProperties": false,
        "required": [
           "newClassificationItems"
        ]
    })";
}

GS::Optional<GS::UniString> CreateClassificationItemsCommand::GetRawResponseSchema () const
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

GS::ObjectState CreateClassificationItemsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> newClassificationItems;
    if (!parameters.Get ("newClassificationItems", newClassificationItems) || newClassificationItems.GetSize() == 0) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "newClassificationItems is missing or invalid");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("CreateClassificationItemsCommand", [&] () -> GSErrCode {
        for (const GS::ObjectState& newClassificationItem : newClassificationItems) {
            GS::ObjectState classificationItemDetails;
            if (!newClassificationItem.Get ("classificationItemDetails", classificationItemDetails)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Failed to get classification item details"));
                continue;
            }

            const GS::ObjectState* classificationSystemId = newClassificationItem.Get ("classificationSystemId");
            if (classificationSystemId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "classificationSystemId is missing"));
                continue;
            }

            const GS::ObjectState* parentClassificationItemId = newClassificationItem.Get ("parentClassificationItemId");
            const GS::ObjectState* nextClassificationItemId = newClassificationItem.Get ("nextClassificationItemId");

            const API_Guid classificationSystemGuid = GetGuidFromObjectState (*classificationSystemId);
            const API_Guid currentParentGuid = parentClassificationItemId ? GetGuidFromObjectState (*parentClassificationItemId) : APINULLGuid;
            const API_Guid nextClassificationGuid = nextClassificationItemId ? GetGuidFromObjectState (*nextClassificationItemId) : APINULLGuid;

            API_ClassificationItem apiClassificationItem = {};
            apiClassificationItem.guid = APINULLGuid;
            classificationItemDetails.Get ("name", apiClassificationItem.name);
            classificationItemDetails.Get ("description", apiClassificationItem.description);
            classificationItemDetails.Get ("id", apiClassificationItem.id);
            GSErrCode err = ACAPI_Classification_CreateClassificationItem(apiClassificationItem, classificationSystemGuid, currentParentGuid, nextClassificationGuid);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to create classification item"));
                continue;
            }
            GS::Array<GS::ObjectState> childItems;
            if (classificationItemDetails.Get ("children", childItems) && childItems.GetSize() > 0) {
                CreateClassificationItemsRecursively(childItems, classificationSystemGuid, apiClassificationItem.guid);
            }
        }

        return NoError;
    });

    return response;
}

DeleteClassificationSystemsCommand::DeleteClassificationSystemsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteClassificationSystemsCommand::GetName () const
{
    return "DeleteClassificationSystems";
}

GS::Optional<GS::UniString> DeleteClassificationSystemsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "classificationSystemIds": {
                "$ref": "#/ClassificationSystemIds"
            }
        },
        "additionalProperties": false,
        "required": [
            "classificationSystemIds"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteClassificationSystemsCommand::GetRawResponseSchema () const
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

GS::ObjectState DeleteClassificationSystemsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> classificationSystemIds;
    parameters.Get ("classificationSystemIds", classificationSystemIds);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("DeleteClassificationSystemsCommand", [&]() -> GSErrCode {
        for (const GS::ObjectState& classificationSystemId : classificationSystemIds) {
            const GS::ObjectState* systemId = classificationSystemId.Get ("classificationSystemId");
            if (systemId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "classificationSystemId is missing"));
                continue;
            }

            const API_Guid systemGuid = GetGuidFromObjectState (*systemId);

            GSErrCode err = ACAPI_Classification_DeleteClassificationSystem (systemGuid);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "failed to delete classification system"));
                continue;
            }

            executionResults (CreateSuccessfulExecutionResult ());
        }

        return NoError;
    });

    return response;
}

DeleteClassificationItemsCommand::DeleteClassificationItemsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteClassificationItemsCommand::GetName () const
{
    return "DeleteClassificationItems";
}

GS::Optional<GS::UniString> DeleteClassificationItemsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "classificationItemIds": {
                "$ref": "#/ClassificationItemIds"
            }
        },
        "additionalProperties": false,
        "required": [
            "classificationItemIds"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteClassificationItemsCommand::GetRawResponseSchema () const
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

GS::ObjectState DeleteClassificationItemsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> classificationItemIds;
    parameters.Get ("classificationItemIds", classificationItemIds);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("DeleteClassificationItemsCommand", [&]() -> GSErrCode {
        for (const GS::ObjectState& classificationItemId : classificationItemIds) {
            const GS::ObjectState* itemId = classificationItemId.Get ("classificationItemId");
            if (itemId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "classificationItemId is missing"));
                continue;
            }

            const API_Guid itemGuid = GetGuidFromObjectState (*itemId);

            GSErrCode err = ACAPI_Classification_DeleteClassificationItem (itemGuid);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "failed to delete classification item"));
                continue;
            }

            executionResults (CreateSuccessfulExecutionResult ());
        }

        return NoError;
    });

    return response;
}

UpdateClassificationSystemsCommand::UpdateClassificationSystemsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String UpdateClassificationSystemsCommand::GetName () const
{
    return "UpdateClassificationSystems";
}

GS::Optional<GS::UniString> UpdateClassificationSystemsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "classificationSystems": {
                "type": "array",
                "description": "The classification systems to update. Only the fields given change.",
                "items": {
                    "$ref": "#/ClassificationSystemUpdate"
                }
            }
        },
        "additionalProperties": false,
        "required": [ "classificationSystems" ]
    })";
}

GS::Optional<GS::UniString> UpdateClassificationSystemsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": { "executionResults": { "$ref": "#/ExecutionResults" } },
        "additionalProperties": false,
        "required": [ "executionResults" ]
    })";
}

GS::ObjectState UpdateClassificationSystemsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> systems;
    parameters.Get ("classificationSystems", systems);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("UpdateClassificationSystems", [&]() -> GSErrCode {
        for (const GS::ObjectState& item : systems) {
            const GS::ObjectState* systemId = item.Get ("classificationSystemId");
            if (systemId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "classificationSystemId is missing"));
                continue;
            }
            API_ClassificationSystem system;
            system.guid = GetGuidFromObjectState (*systemId);
            if (ACAPI_Classification_GetClassificationSystem (system) != NoError) {
                executionResults (CreateFailedExecutionResult (APIERR_BADID, "classification system not found"));
                continue;
            }
            item.Get ("name", system.name);
            item.Get ("description", system.description);
            item.Get ("source", system.source);
            item.Get ("version", system.editionVersion);
            GS::UniString date;
            if (item.Get ("date", date)) {
                unsigned int year = 0, month = 0, day = 0;
                if (date.SScanf ("%4u-%2u-%2u", &year, &month, &day) != 3) {
                    executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "date must be YYYY-MM-DD"));
                    continue;
                }
#ifdef ServerMainVers_2900
                system.editionDate = std::chrono::year_month_day (std::chrono::year (year), std::chrono::month (month), std::chrono::day (day));
#else
                system.editionDate = GSDateRecord ((unsigned short) year, (unsigned short) month, (unsigned short) day);
#endif
            }
            const GSErrCode err = ACAPI_Classification_ChangeClassificationSystem (system);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, DescribeDefinitionChangeError (err)));
                continue;
            }
            executionResults (CreateSuccessfulExecutionResult ());
        }
        return NoError;
    });

    return response;
}

UpdateClassificationItemsCommand::UpdateClassificationItemsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String UpdateClassificationItemsCommand::GetName () const
{
    return "UpdateClassificationItems";
}

GS::Optional<GS::UniString> UpdateClassificationItemsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "classificationItems": {
                "type": "array",
                "description": "The classification items to update. Only the fields given change; items keep their guid and their parent.",
                "items": {
                    "$ref": "#/ClassificationItemUpdate"
                }
            }
        },
        "additionalProperties": false,
        "required": [ "classificationItems" ]
    })";
}

GS::Optional<GS::UniString> UpdateClassificationItemsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": { "executionResults": { "$ref": "#/ExecutionResults" } },
        "additionalProperties": false,
        "required": [ "executionResults" ]
    })";
}

GS::ObjectState UpdateClassificationItemsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> items;
    parameters.Get ("classificationItems", items);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("UpdateClassificationItems", [&]() -> GSErrCode {
        for (const GS::ObjectState& item : items) {
            const GS::ObjectState* itemId = item.Get ("classificationItemId");
            if (itemId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "classificationItemId is missing"));
                continue;
            }
            API_ClassificationItem classificationItem;
            classificationItem.guid = GetGuidFromObjectState (*itemId);
            if (ACAPI_Classification_GetClassificationItem (classificationItem) != NoError) {
                executionResults (CreateFailedExecutionResult (APIERR_BADID, "classification item not found"));
                continue;
            }
            item.Get ("id", classificationItem.id);
            item.Get ("name", classificationItem.name);
            item.Get ("description", classificationItem.description);
            const GSErrCode err = ACAPI_Classification_ChangeClassificationItem (classificationItem);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, DescribeDefinitionChangeError (err)));
                continue;
            }
            executionResults (CreateSuccessfulExecutionResult ());
        }
        return NoError;
    });

    return response;
}

static void AddClassificationItemGuids (const GS::Array<API_ClassificationItem>& items, GS::HashSet<API_Guid>& guids)
{
    for (const API_ClassificationItem& item : items) {
        guids.Add (item.guid);
        GS::Array<API_ClassificationItem> children;
        if (ACAPI_Classification_GetClassificationItemChildren (item.guid, children) == NoError) {
            AddClassificationItemGuids (children, guids);
        }
    }
}

// Every classification system and item guid, to tell what an import created and removed.
static GS::HashSet<API_Guid> AllClassificationGuids ()
{
    GS::HashSet<API_Guid> guids;
    GS::Array<API_ClassificationSystem> systems;
    ACAPI_Classification_GetClassificationSystems (systems);
    for (const API_ClassificationSystem& system : systems) {
        guids.Add (system.guid);
        GS::Array<API_ClassificationItem> roots;
        if (ACAPI_Classification_GetClassificationSystemRootItems (system.guid, roots) == NoError) {
            AddClassificationItemGuids (roots, guids);
        }
    }
    return guids;
}

GetClassificationItemParentsCommand::GetClassificationItemParentsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String GetClassificationItemParentsCommand::GetName () const
{
    return "GetClassificationItemParents";
}

GS::Optional<GS::UniString> GetClassificationItemParentsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "classificationItemIds": {
                "$ref": "#/ClassificationItemIds"
            }
        },
        "additionalProperties": false,
        "required": [
            "classificationItemIds"
        ]
    })";
}

GS::Optional<GS::UniString> GetClassificationItemParentsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "classificationItemParents": {
                "type": "array",
                "description": "The parent and the system of each given item, in the order of the input. A non-existing item is an error item.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "classificationSystemId": {
                                    "$ref": "#/ClassificationSystemId",
                                    "description": "The classification system the item belongs to."
                                },
                                "parentClassificationItemId": {
                                    "$ref": "#/ClassificationItemId",
                                    "description": "The parent item. Missing for a top level item of its system."
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "classificationSystemId"
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
            "classificationItemParents"
        ]
    })";
}

static bool IsRootItemOfClassificationSystem (const API_Guid& systemGuid, const API_Guid& itemGuid)
{
    GS::Array<API_ClassificationItem> roots;
    if (ACAPI_Classification_GetClassificationSystemRootItems (systemGuid, roots) != NoError) {
        return false;
    }
    for (const API_ClassificationItem& root : roots) {
        if (root.guid == itemGuid) {
            return true;
        }
    }
    return false;
}

GS::ObjectState GetClassificationItemParentsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> classificationItemIds;
    parameters.Get ("classificationItemIds", classificationItemIds);

    GS::ObjectState response;
    const auto& classificationItemParents = response.AddList<GS::ObjectState> ("classificationItemParents");

    for (const GS::ObjectState& classificationItemId : classificationItemIds) {
        const GS::ObjectState* itemId = classificationItemId.Get ("classificationItemId");
        if (itemId == nullptr) {
            classificationItemParents (CreateErrorResponse (APIERR_BADPARS, "classificationItemId is missing"));
            continue;
        }

        API_ClassificationItem item;
        item.guid = GetGuidFromObjectState (*itemId);
        if (ACAPI_Classification_GetClassificationItem (item) != NoError) {
            classificationItemParents (CreateErrorResponse (APIERR_BADID, "classification item not found"));
            continue;
        }

        API_ClassificationSystem system;
        GSErrCode err = ACAPI_Classification_GetClassificationItemSystem (item.guid, system);
        if (err != NoError) {
            classificationItemParents (CreateErrorResponse (err, "Failed to get the classification system of the item"));
            continue;
        }

        GS::ObjectState itemParent ("classificationSystemId", CreateGuidObjectState (system.guid));

        API_ClassificationItem parent;
        err = ACAPI_Classification_GetClassificationItemParent (item.guid, parent);
        if (err == NoError && parent.guid != APINULLGuid) {
            itemParent.Add ("parentClassificationItemId", CreateGuidObjectState (parent.guid));
        } else if (!IsRootItemOfClassificationSystem (system.guid, item.guid)) {
            // A top level item has no parent; whether Archicad reports that with an error or
            // an empty parent, the root items of the system tell the two cases apart.
            classificationItemParents (CreateErrorResponse (err != NoError ? err : APIERR_GENERAL, "Failed to get the parent of the classification item"));
            continue;
        }

        classificationItemParents (itemParent);
    }

    return response;
}

ImportClassificationsXmlCommand::ImportClassificationsXmlCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String ImportClassificationsXmlCommand::GetName () const
{
    return "ImportClassificationsXml";
}

GS::Optional<GS::UniString> ImportClassificationsXmlCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "xml": { "type": "string", "description": "A Classification Manager export (XML) to import." },
            "systemConflictPolicy": { "$ref": "#/ClassificationSystemConflictPolicy" },
            "itemConflictPolicy": { "$ref": "#/ClassificationItemConflictPolicy" }
        },
        "additionalProperties": false,
        "required": [ "xml", "systemConflictPolicy", "itemConflictPolicy" ]
    })";
}

GS::Optional<GS::UniString> ImportClassificationsXmlCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResult": { "$ref": "#/ExecutionResult" },
            "created": { "type": "array", "items": { "$ref": "#/GuidId" } },
            "removed": { "type": "array", "items": { "$ref": "#/GuidId" } }
        },
        "additionalProperties": false,
        "required": [ "executionResult", "created", "removed" ]
    })";
}

GS::ObjectState ImportClassificationsXmlCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString xml, systemPolicyStr, itemPolicyStr;
    parameters.Get ("xml", xml);
    parameters.Get ("systemConflictPolicy", systemPolicyStr);
    parameters.Get ("itemConflictPolicy", itemPolicyStr);
    const API_ClassificationSystemNameConflictResolutionPolicy systemPolicy =
        systemPolicyStr == "replace" ? API_ReplaceConflictingSystems :
        systemPolicyStr == "skip"    ? API_SkipConflictingSystems :
                                       API_MergeConflictingSystems;
    const API_ClassificationItemNameConflictResolutionPolicy itemPolicy =
        itemPolicyStr == "replace" ? API_ReplaceConflictingItems : API_SkipConflicitingItems;

    GS::ObjectState response;
    const GS::HashSet<API_Guid> before = AllClassificationGuids ();
    GSErrCode err = NoError;
    ACAPI_CallUndoableCommand ("ImportClassificationsXml", [&]() -> GSErrCode {
        err = ACAPI_Classification_Import (xml, systemPolicy, itemPolicy);
        return err;
    });
    const GS::HashSet<API_Guid> after = AllClassificationGuids ();

    response.Add ("executionResult", err == NoError ? CreateSuccessfulExecutionResult ()
                                                    : CreateFailedExecutionResult (err, err == APIERR_BADPARS ? GS::UniString ("invalid classification XML") : DescribeDefinitionChangeError (err)));
    AddGuidDifference (after, before, response.AddList<GS::ObjectState> ("created"));
    AddGuidDifference (before, after, response.AddList<GS::ObjectState> ("removed"));
    return response;
}
