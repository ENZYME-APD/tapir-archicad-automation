#include "ElementGroupingCommands.hpp"
#include "MigrationHelper.hpp"
#include "Base64Converter.hpp"

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

GetElementSetDataCommand::GetElementSetDataCommand() :
    CommandBase(CommonSchema::Used)
{
}

GS::String GetElementSetDataCommand::GetName() const
{
    return "GetElementSetData";
}

GS::Optional<GS::UniString> GetElementSetDataCommand::GetInputParametersSchema() const
{
    return R"({
        "type": "object",
        "properties": {
            "elementSets": {
                "type": "array",
                "description": "The element sets to get the data of.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementSetId": {
                            "type": "object",
                            "description": "The identifier of an element set.",
                            "properties": {
                                "guid": {
                                    "$ref": "#/Guid"
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "guid"
                            ]
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementSetId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementSets"
        ]
    })";
}

GS::Optional<GS::UniString> GetElementSetDataCommand::GetRawResponseSchema() const
{
    return R"({
        "type": "object",
        "properties": {
            "elementSets": {
                "type": "array",
                "description": "The data of each given element set, or an error.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "elements": {
                                    "$ref": "#/Elements",
                                    "description": "The elements of the element set."
                                },
                                "userData": {
                                    "type": "object",
                                    "description": "The custom data stored with the element set by the add-on that created it.",
                                    "properties": {
                                        "dataVersion": {
                                            "type": "integer",
                                            "description": "The version of the data, as set by the add-on that created the element set."
                                        },
                                        "platformSign": {
                                            "type": "integer",
                                            "description": "The platform sign of the data, telling the byte order it was written with."
                                        },
                                        "data": {
                                            "type": "string",
                                            "description": "The raw bytes of the data, Base64 encoded. Empty when the element set has no data."
                                        }
                                    },
                                    "additionalProperties": false,
                                    "required": [
                                        "dataVersion",
                                        "platformSign",
                                        "data"
                                    ]
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "elements",
                                "userData"
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
            "elementSets"
        ]
    })";
}

GS::ObjectState GetElementSetDataCommand::Execute(const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elementSets;
    if (!parameters.Get("elementSets", elementSets)) {
        return CreateErrorResponse(APIERR_BADPARS, "Invalid or missing 'elementSets' parameter.");
    }

    GS::ObjectState response;
    const auto& elementSetsData = response.AddList<GS::ObjectState>("elementSets");

    for (const GS::ObjectState& elementSet : elementSets) {
        const GS::ObjectState* elementSetId = elementSet.Get("elementSetId");
        if (elementSetId == nullptr) {
            elementSetsData(CreateErrorResponse(APIERR_BADPARS, "elementSetId is missing"));
            continue;
        }

        GS::Array<API_Guid> elemGuids;
        API_UserData userData = {};
        const GSErrCode err = ACAPI_ElementSet_GetData(GetGuidFromObjectState(*elementSetId), &elemGuids, &userData);
        if (err != NoError) {
            BMKillHandle(&userData.dataHdl);
            elementSetsData(CreateErrorResponse(err, "Failed to get the data of the element set."));
            continue;
        }

        GS::UniString data;
        const GSSize dataSize = BMGetHandleSize(userData.dataHdl);
        if (dataSize > 0) {
            data = Base64Converter::Encode(*userData.dataHdl, dataSize);
            data.DeleteAll(GS::UniChar(char('\n')));
        }
        BMKillHandle(&userData.dataHdl);

        GS::ObjectState elementSetData;
        const auto& elements = elementSetData.AddList<GS::ObjectState>("elements");
        for (const API_Guid& elemGuid : elemGuids) {
            elements(CreateElementIdObjectState(elemGuid));
        }
        GS::ObjectState userDataObj;
        userDataObj.Add("dataVersion", static_cast<Int32>(userData.dataVersion));
        userDataObj.Add("platformSign", static_cast<Int32>(userData.platformSign));
        userDataObj.Add("data", data);
        elementSetData.Add("userData", userDataObj);
        elementSetsData(elementSetData);
    }

    return response;
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