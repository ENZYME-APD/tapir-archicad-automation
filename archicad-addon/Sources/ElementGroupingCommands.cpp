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

GS::Optional<GS::UniString> CreateGroupsCommand::GetRawResponseSchema() const
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

GS::Optional<GS::UniString> GetGroupsOfElementsCommand::GetRawResponseSchema() const
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

GS::Optional<GS::UniString> GetElementsOfGroupsCommand::GetRawResponseSchema() const
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

static GSErrCode GetMainGroupGuid(const API_Guid& groupGuid, API_Guid& mainGroupGuid)
{
#ifdef ServerMainVers_2700
    return ACAPI_Grouping_GetMainGroupGuid(&groupGuid, &mainGroupGuid);
#else
    // The given guid must be a group, as ACAPI_Grouping_GetMainGroupGuid requires on 27+.
    GS::Array<API_Guid> elemGuids;
    GSErrCode err = ACAPI_Grouping_GetGroupedElems(groupGuid, &elemGuids);
    if (err != NoError) {
        return err;
    }

    // Walk up the parent groups until the top level one.
    mainGroupGuid = groupGuid;
    for (UInt32 depth = 0; depth < 1000; ++depth) {
        API_Guid parentGroupGuid = APINULLGuid;
        err = ACAPI_Grouping_GetGroup(mainGroupGuid, &parentGroupGuid);
        if (err != NoError) {
            return err;
        }
        if (parentGroupGuid == APINULLGuid) {
            return NoError;
        }
        mainGroupGuid = parentGroupGuid;
    }
    return APIERR_GENERAL;
#endif
}

GetMainGroupsCommand::GetMainGroupsCommand() :
    CommandBase(CommonSchema::Used)
{
}

GS::String GetMainGroupsCommand::GetName() const
{
    return "GetMainGroups";
}

GS::Optional<GS::UniString> GetMainGroupsCommand::GetInputParametersSchema() const
{
    return R"({
        "type": "object",
        "properties": {
            "groups": {
                "type": "array",
                "description": "The groups to get the top level group of.",
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

GS::Optional<GS::UniString> GetMainGroupsCommand::GetRawResponseSchema() const
{
    return R"({
        "type": "object",
        "properties": {
            "mainGroups": {
                "type": "array",
                "description": "The top level group of each given group (the group itself if it is not nested in another group), or an error.",
                "items": {
                    "$ref": "#/GroupIdOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "mainGroups"
        ]
    })";
}

GS::ObjectState GetMainGroupsCommand::Execute(const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> groups;
    if (!parameters.Get("groups", groups)) {
        return CreateErrorResponse(APIERR_BADPARS, "Invalid or missing 'groups' parameter.");
    }

    GS::ObjectState response;
    const auto& mainGroups = response.AddList<GS::ObjectState>("mainGroups");

    for (const GS::ObjectState& group : groups) {
        const GS::ObjectState* groupId = group.Get("groupId");
        if (groupId == nullptr) {
            mainGroups(CreateErrorResponse(APIERR_BADPARS, "groupId is missing"));
            continue;
        }

        API_Guid mainGroupGuid = APINULLGuid;
        const GSErrCode err = GetMainGroupGuid(GetGuidFromObjectState(*groupId), mainGroupGuid);
        if (err != NoError || mainGroupGuid == APINULLGuid) {
            mainGroups(CreateErrorResponse(err != NoError ? err : APIERR_GENERAL, "Failed to get the top level group of the group."));
            continue;
        }

        GS::ObjectState groupIdItem;
        groupIdItem.Add("groupId", CreateGuidObjectState(mainGroupGuid));
        mainGroups(groupIdItem);
    }

    return response;
}

GetAllElementsOfGroupsCommand::GetAllElementsOfGroupsCommand() :
    CommandBase(CommonSchema::Used)
{
}

GS::String GetAllElementsOfGroupsCommand::GetName() const
{
    return "GetAllElementsOfGroups";
}

GS::Optional<GS::UniString> GetAllElementsOfGroupsCommand::GetInputParametersSchema() const
{
    return R"({
        "type": "object",
        "properties": {
            "groups": {
                "type": "array",
                "description": "The groups to get all the elements of.",
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

GS::Optional<GS::UniString> GetAllElementsOfGroupsCommand::GetRawResponseSchema() const
{
    return R"({
        "type": "object",
        "properties": {
            "elementsOfGroups": {
                "type": "array",
                "description": "All the elements of each given group, including the elements of its nested groups, or an error.",
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

GS::ObjectState GetAllElementsOfGroupsCommand::Execute(const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
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
        const GSErrCode err = ACAPI_Grouping_GetAllGroupedElems(GetGuidFromObjectState(*groupId), &elemGuids);
        if (err != NoError) {
            elementsOfGroups(CreateErrorResponse(err, "Failed to get all the elements of the group."));
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

GetAutoGroupModeCommand::GetAutoGroupModeCommand() :
    CommandBase(CommonSchema::Used)
{
}

GS::String GetAutoGroupModeCommand::GetName() const
{
    return "GetAutoGroupMode";
}

GS::Optional<GS::UniString> GetAutoGroupModeCommand::GetRawResponseSchema() const
{
    return R"({
        "type": "object",
        "properties": {
            "autoGroup": {
                "type": "boolean",
                "description": "True if the Auto Group mode is currently on."
            }
        },
        "additionalProperties": false,
        "required": [
            "autoGroup"
        ]
    })";
}

GS::ObjectState GetAutoGroupModeCommand::Execute(const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    bool autoGroup = false;
    const GSErrCode err = ACAPI_View_IsAutoGroupOn(&autoGroup);
    if (err != NoError) {
        return CreateErrorResponse(err, "Failed to get the Auto Group mode.");
    }

    return GS::ObjectState("autoGroup", autoGroup);
}

GetSuspendGroupsModeCommand::GetSuspendGroupsModeCommand() :
    CommandBase(CommonSchema::Used)
{
}

GS::String GetSuspendGroupsModeCommand::GetName() const
{
    return "GetSuspendGroupsMode";
}

GS::Optional<GS::UniString> GetSuspendGroupsModeCommand::GetRawResponseSchema() const
{
    return R"({
        "type": "object",
        "properties": {
            "suspendGroups": {
                "type": "boolean",
                "description": "True if the Suspend Groups mode is currently on."
            }
        },
        "additionalProperties": false,
        "required": [
            "suspendGroups"
        ]
    })";
}

GS::ObjectState GetSuspendGroupsModeCommand::Execute(const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    bool suspendGroups = false;
    const GSErrCode err = ACAPI_View_IsSuspendGroupOn(&suspendGroups);
    if (err != NoError) {
        return CreateErrorResponse(err, "Failed to get the Suspend Groups mode.");
    }

    return GS::ObjectState("suspendGroups", suspendGroups);
}

SetSuspendGroupsModeCommand::SetSuspendGroupsModeCommand() :
    CommandBase(CommonSchema::Used)
{
}

GS::String SetSuspendGroupsModeCommand::GetName() const
{
    return "SetSuspendGroupsMode";
}

GS::Optional<GS::UniString> SetSuspendGroupsModeCommand::GetInputParametersSchema() const
{
    return R"({
        "type": "object",
        "properties": {
            "suspendGroups": {
                "type": "boolean",
                "description": "Turn the Suspend Groups mode on or off."
            }
        },
        "additionalProperties": false,
        "required": [
            "suspendGroups"
        ]
    })";
}

GS::Optional<GS::UniString> SetSuspendGroupsModeCommand::GetRawResponseSchema() const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResult": {
                "$ref": "#/ExecutionResult"
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResult"
        ]
    })";
}

GS::ObjectState SetSuspendGroupsModeCommand::Execute(const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    bool suspendGroups = false;
    if (!parameters.Get("suspendGroups", suspendGroups)) {
        return CreateErrorResponse(APIERR_BADPARS, "Invalid or missing 'suspendGroups' parameter.");
    }

#ifdef ServerMainVers_2700
    const GSErrCode err = ACAPI_Grouping_ChangeSuspendGroup(suspendGroups);
#else
    // Older versions only offer the APITool_SuspendGroups toggle, so the mode
    // is toggled only when the current state differs from the requested one.
    bool currentState = false;
    GSErrCode err = ACAPI_View_IsSuspendGroupOn(&currentState);
    if (err == NoError && currentState != suspendGroups) {
        err = ACAPI_Grouping_Tool(GS::Array<API_Guid>(), APITool_SuspendGroups, nullptr);
    }
#endif

    return GS::ObjectState("executionResult", err == NoError
        ? CreateSuccessfulExecutionResult()
        : CreateFailedExecutionResult(err, "Failed to set the Suspend Groups mode."));
}