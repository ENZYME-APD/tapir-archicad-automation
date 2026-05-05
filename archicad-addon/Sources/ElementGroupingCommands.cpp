#include "ElementGroupingCommands.hpp"
#include "MigrationHelper.hpp"

#ifdef ServerMainVers_2600
static GS::UniString GetGroupCreationErrorMessage(const GS::Array<API_Guid>& elemGuids, GSErrCode /*originalErr*/)
{
    if (elemGuids.GetSize() < 2) {
        return "At least two elements or groups are required to create a group.";
    }

    for (const API_Guid& guid : elemGuids) {
        API_Guid currentGroup = APINULLGuid;
        if (ACAPI_Grouping_GetGroup(guid, &currentGroup) == NoError && currentGroup != APINULLGuid) {
            return "Failed to create group. One or more elements are already part of an existing group.";
        }
    }

    return "Failed to create group.";
}
#endif

CreateGroupsCommand::CreateGroupsCommand() :
    CommandBase(CommonSchema::Used)
{
}

GS::String CreateGroupsCommand::GetName() const
{
    return "CreateGroups";
}

GS::Optional<GS::UniString> CreateGroupsCommand::GetInputParametersSchema() const
{
    return R"({
        "type": "object",
        "properties": {
            "elementGroups": {
                "type": "array",
                "description": "A list of element groups to create.",
                "items": {
                    "$ref": "#/ElementGroupParameters"
                }
            }
        },
        "additionalProperties": false,
        "required":[
            "elementGroups"
        ]
    })";
}

GS::Optional<GS::UniString> CreateGroupsCommand::GetResponseSchema() const
{
    return R"({
        "type": "object",
        "properties": {
            "groupGuids": {
                "type": "array",
                "description": "The results of the group creation operations.",
                "items": {
                    "$ref": "#/GroupIdOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "groupGuids"
        ]
    })";
}

GS::ObjectState CreateGroupsCommand::Execute(const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elementGroups;
    parameters.Get("elementGroups", elementGroups);

    GS::ObjectState response;
    const auto& groupGuids = response.AddList<GS::ObjectState>("groupGuids");

#ifdef ServerMainVers_2600

    ACAPI_CallUndoableCommand("Create Element Groups", [&]() -> GSErrCode {
        for (const GS::ObjectState& groupParam : elementGroups) {

            GS::Array<GS::ObjectState> elements;
            if (!groupParam.Get("elements", elements) || elements.IsEmpty()) {
                groupGuids(CreateErrorResponse(APIERR_BADPARS, "Elements array is missing or empty."));
                continue;
            }

            GS::Array<API_Guid> elemGuids;
            for (const GS::ObjectState& item : elements) {
                if (item.Contains("elementId")) {
                    elemGuids.Push(GetGuidFromArrayItem("elementId", item));
                }
                else if (item.Contains("groupId")) {
                    elemGuids.Push(GetGuidFromArrayItem("groupId", item));
                }
            }

            API_Guid parentGroupGuid = APINULLGuid;
            const GS::ObjectState* parentGroupId = groupParam.Get("parentGroupId");
            if (parentGroupId != nullptr) {
                parentGroupGuid = GetGuidFromObjectState(*parentGroupId);
            }

            API_Guid newGroupGuid = APINULLGuid;
            GSErrCode err = ACAPI_Grouping_CreateGroup(
                elemGuids,
                &newGroupGuid,
                parentGroupGuid == APINULLGuid ? nullptr : &parentGroupGuid
            );

            if (err != NoError) {
                GS::UniString errorMsg = GetGroupCreationErrorMessage(elemGuids, err);
                groupGuids(CreateErrorResponse(err, errorMsg));
            }
            else {
                GS::ObjectState successResult;
                successResult.Add("groupId", CreateGuidObjectState(newGroupGuid));
                groupGuids(successResult);
            }
        }
        return NoError;
        });
#else
    GS::UniString notSupportedMsg = "The Create Groups command is not supported in Archicad 25 or older.";
    groupGuids (CreateErrorResponse (APIERR_NOTSUPPORTED, notSupportedMsg));
#endif
    return response;
}