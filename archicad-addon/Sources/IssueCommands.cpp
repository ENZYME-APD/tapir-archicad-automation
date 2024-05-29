#include "IssueCommands.hpp"
#include "MigrationHelper.hpp"

GetIssueCommand::GetIssueCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetIssueCommand::GetName () const
{
    return "GetIssue";
}

GS::Optional<GS::UniString> GetIssueCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "issues": {
                "type": "array",
                "description": "A list of existing issues.",
                "items": {
                    "type": "object",
                    "properties": {
                        "guid": {
                            "type": "string",
                            "description": "Issue identifier"
                        },
                        "name": {
                            "type": "string",
                            "description": "Issue name"
                        },
                        "parentGuid": {
                            "type": "string",
                            "description": "The identifier of the parent issue"
                        },
                        "creaTime": {
                            "type": "integer",
                            "description": "Issue creation time"
                        },
                        "modiTime": {
                            "type": "integer",
                            "description": "Issue modification time"
                        },
                        "tagText": {
                            "type": "string",
                            "description": "Issue tag text - labels"
                        },
                        "tagTextElemGuid": {
                            "type": "string",
                            "description": "The identifier of the attached tag text element"
                        },
                        "isTagTextElemVisible": {
                            "type": "boolean",
                            "description": "The visibility of the attached tag text element"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "guid",
                        "name",
                        "parentGuid",
                        "creaTime",
                        "modiTime",
                        "tagText",
                        "tagTextElemGuid",
                        "isTagTextElemVisible"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "issues"
        ]
    })";
}

GS::ObjectState GetIssueCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<API_MarkUpType> issueList;
    GSErrCode err = ACAPI_MarkUp_GetList (APINULLGuid, &issueList);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to retrive issues.");
    }

    GS::ObjectState response;
    const auto& listAdder = response.AddList<GS::ObjectState> ("issues");

    for (auto i = issueList.Enumerate (); i != nullptr; ++i) {
        GS::ObjectState issueData;
        issueData.Add ("guid", APIGuidToString(i->guid));
        issueData.Add ("name", i->name);
        issueData.Add ("parentGuid", APIGuidToString (i->parentGuid));
        issueData.Add ("creaTime", i->creaTime);
        issueData.Add ("modiTime", i->modiTime);
        issueData.Add ("tagText", i->tagText);
        issueData.Add ("tagTextElemGuid", APIGuidToString (i->tagTextElemGuid));
        issueData.Add ("isTagTextElemVisible", i->isTagTextElemVisible);

        listAdder (issueData);
    }

    return response;
}