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

GS::Optional<GS::UniString> GetClassificationsOfElementsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementClassifications": {
                "$ref": "#/ElementClassificationsOrErrors",
                "description": "The list of element classification item identifiers. Order of the ids are the same as in the input. Non-existing elements or non-existing classification systems are represented by error objects."
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

GS::Optional<GS::UniString> SetClassificationsOfElementsCommand::GetResponseSchema () const
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

GS::Optional<GS::UniString> CreateClassificationSystemsCommand::GetResponseSchema () const
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

GS::Optional<GS::UniString> CreateClassificationItemsCommand::GetResponseSchema () const
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

GS::Optional<GS::UniString> DeleteClassificationSystemsCommand::GetResponseSchema () const
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

GS::Optional<GS::UniString> DeleteClassificationItemsCommand::GetResponseSchema () const
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
