#include "ClassificationCommands.hpp"
#include "MigrationHelper.hpp"

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