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
        "$ref": "#/ElementClassificationsOrErrors",
        "description": "The list of element classification item identifiers. Order of the ids are the same as in the input. Non-existing elements or non-existing classification systems are represented by error objects."
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