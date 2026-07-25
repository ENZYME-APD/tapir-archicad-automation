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

GetGroupsOfElementsCommand::GetGroupsOfElementsCommand() :
    CommandBase(CommonSchema::Used)
{
}

GS::String GetGroupsOfElementsCommand::GetName() const
{
    return "GetGroupsOfElements";
}

GS::Optional<GS::UniString> GetGroupsOfElementsCommand::GetInputParametersSchema() const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> GetGroupsOfElementsCommand::GetResponseSchema() const
{
    return R"({
        "type": "object",
        "properties": {
            "groupGuids": {
                "type": "array",
                "description": "The identifier of the group that directly contains each given element, or an error for elements that are not part of any group.",
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

GS::ObjectState GetGroupsOfElementsCommand::Execute(const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    if (!parameters.Get("elements", elements)) {
        return CreateErrorResponse(APIERR_BADPARS, "Invalid or missing 'elements' parameter.");
    }

    GS::ObjectState response;
    const auto& groupGuids = response.AddList<GS::ObjectState>("groupGuids");

    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get("elementId");
        if (elementId == nullptr) {
            groupGuids(CreateErrorResponse(APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        API_Guid groupGuid = APINULLGuid;
        const GSErrCode err = ACAPI_Grouping_GetGroup(GetGuidFromObjectState(*elementId), &groupGuid);
        if (err != NoError) {
            groupGuids(CreateErrorResponse(err, "Failed to get the group of the element."));
            continue;
        }
        if (groupGuid == APINULLGuid) {
            groupGuids(CreateErrorResponse(APIERR_GENERAL, "The element is not part of any group."));
            continue;
        }

        GS::ObjectState groupIdItem;
        groupIdItem.Add("groupId", CreateGuidObjectState(groupGuid));
        groupGuids(groupIdItem);
    }

    return response;
}

GetElementsOfGroupsCommand::GetElementsOfGroupsCommand() :
    CommandBase(CommonSchema::Used)
{
}

GS::String GetElementsOfGroupsCommand::GetName() const
{
    return "GetElementsOfGroups";
}

GS::Optional<GS::UniString> GetElementsOfGroupsCommand::GetInputParametersSchema() const
{
    return R"({
        "type": "object",
        "properties": {
            "groups": {
                "type": "array",
                "description": "The groups to get the elements of.",
                "items": {
                    "$ref": "#/GroupIdArrayItem"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "groups"
        ]
    })";
}

GS::Optional<GS::UniString> GetElementsOfGroupsCommand::GetResponseSchema() const
{
    return R"({
        "type": "object",
        "properties": {
            "elementsOfGroups": {
                "type": "array",
                "description": "The elements directly contained by each given group, or an error.",
                "items": {
                    "$ref": "#/ElementsWrapperOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementsOfGroups"
        ]
    })";
}

GS::ObjectState GetElementsOfGroupsCommand::Execute(const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> groups;
    if (!parameters.Get("groups", groups)) {
        return CreateErrorResponse(APIERR_BADPARS, "Invalid or missing 'groups' parameter.");
    }

    GS::ObjectState response;
    const auto& elementsOfGroups = response.AddList<GS::ObjectState>("elementsOfGroups");

    for (const GS::ObjectState& group : groups) {
        const GS::ObjectState* groupId = group.Get("groupId");
        if (groupId == nullptr) {
            elementsOfGroups(CreateErrorResponse(APIERR_BADPARS, "groupId is missing"));
            continue;
        }

        GS::Array<API_Guid> elemGuids;
        const GSErrCode err = ACAPI_Grouping_GetGroupedElems(GetGuidFromObjectState(*groupId), &elemGuids);
        if (err != NoError) {
            elementsOfGroups(CreateErrorResponse(err, "Failed to get the elements of the group."));
            continue;
        }

        GS::ObjectState elementsItem;
        const auto& elements = elementsItem.AddList<GS::ObjectState>("elements");
        for (const API_Guid& elemGuid : elemGuids) {
            elements(CreateElementIdObjectState(elemGuid));
        }
        elementsOfGroups(elementsItem);
    }

    return response;
}