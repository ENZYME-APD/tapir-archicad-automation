#include "ElementGroupingCommands.hpp"
#include "MigrationHelper.hpp"
#include "Base64Converter.hpp"
#include "UserDataPairs.hpp"

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

UngroupElementsCommand::UngroupElementsCommand() :
    CommandBase(CommonSchema::Used)
{
}

GS::String UngroupElementsCommand::GetName() const
{
    return "UngroupElements";
}

GS::Optional<GS::UniString> UngroupElementsCommand::GetInputParametersSchema() const
{
    return R"({
        "type": "object",
        "properties": {
            "groups": {
                "type": "array",
                "description": "The groups to ungroup, as returned by CreateGroups or GetGroupsOfElements. A group nested in another group is reported as failed: ungroup the containing group first.",
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

GS::Optional<GS::UniString> UngroupElementsCommand::GetRawResponseSchema() const
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

GS::ObjectState UngroupElementsCommand::Execute(const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> groups;
    if (!parameters.Get("groups", groups)) {
        return CreateErrorResponse(APIERR_BADPARS, "Invalid or missing 'groups' parameter.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState>("executionResults");

    ACAPI_CallUndoableCommand("Ungroup Elements", [&]() -> GSErrCode {
        for (const GS::ObjectState& group : groups) {
            const GS::ObjectState* groupId = group.Get("groupId");
            if (groupId == nullptr) {
                executionResults(CreateFailedExecutionResult(APIERR_BADPARS, "groupId is missing"));
                continue;
            }

            // Ungroup works on the members, not on the group itself: the group is
            // dissolved one level, like Edit > Grouping > Ungroup, so the directly
            // contained elements and subgroups stay as they are.
            const API_Guid groupGuid = GetGuidFromObjectState(*groupId);
            GS::Array<API_Guid> elemGuids;
            GSErrCode err = ACAPI_Grouping_GetGroupedElems(groupGuid, &elemGuids);
            if (err != NoError) {
                executionResults(CreateFailedExecutionResult(err, "Failed to get the elements of the group."));
                continue;
            }
            if (elemGuids.IsEmpty()) {
                executionResults(CreateFailedExecutionResult(APIERR_BADID, "The group does not exist or contains no elements."));
                continue;
            }

            // Edit > Grouping > Ungroup dissolves the outermost group of the selected
            // elements, so the tool is only called on groups that are not nested in
            // another one; otherwise it could dissolve the containing group instead
            // of the given one.
            API_Guid rootGroupGuid = APINULLGuid;
            err = ACAPI_Grouping_GetRootGroup(elemGuids.GetFirst(), &rootGroupGuid);
            if (err != NoError) {
                executionResults(CreateFailedExecutionResult(err, "Failed to get the root group of the group's elements."));
                continue;
            }
            if (rootGroupGuid != groupGuid) {
                executionResults(CreateFailedExecutionResult(APIERR_BADPARS, "The group is nested in another group. Ungroup the containing group first."));
                continue;
            }

            err = ACAPI_Grouping_Tool(elemGuids, APITool_Ungroup, nullptr);
            if (err != NoError) {
                executionResults(CreateFailedExecutionResult(err, "Failed to ungroup the elements of the group."));
                continue;
            }

            // The tool call may succeed without dissolving the given group (for
            // example when its elements are not editable), so report success only
            // when a member is no longer in it.
            API_Guid currentGroupGuid = APINULLGuid;
            if (ACAPI_Grouping_GetGroup(elemGuids.GetFirst(), &currentGroupGuid) == NoError && currentGroupGuid == groupGuid) {
                executionResults(CreateFailedExecutionResult(APIERR_GENERAL, "The group was not dissolved."));
                continue;
            }

            executionResults(CreateSuccessfulExecutionResult());
        }
        return NoError;
    });

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

GetUserDataOfGroupsCommand::GetUserDataOfGroupsCommand() :
    CommandBase(CommonSchema::Used)
{
}

GS::String GetUserDataOfGroupsCommand::GetName() const
{
    return "GetUserDataOfGroups";
}

GS::Optional<GS::UniString> GetUserDataOfGroupsCommand::GetInputParametersSchema() const
{
    return R"({
        "type": "object",
        "properties": {
            "groups": {
                "type": "array",
                "description": "The groups to get the user data of.",
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

GS::Optional<GS::UniString> GetUserDataOfGroupsCommand::GetRawResponseSchema() const
{
    return R"({
        "type": "object",
        "properties": {
            "userDataOfGroups": {
                "type": "array",
                "description": "The user data attached to each given group, or an error for groups that have none.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "userData": {
                                    "type": "object",
                                    "description": "The user data an add-on attached to the group.",
                                    "properties": {
                                        "dataVersion": {
                                            "type": "integer",
                                            "description": "The version of the user data, as set by the add-on that attached it."
                                        },
                                        "platformSign": {
                                            "type": "integer",
                                            "description": "The platform sign of the user data, which tells the byte order the data was written in."
                                        },
                                        "flags": {
                                            "type": "integer",
                                            "description": "The flags of the user data (APIUserDataFlag_FillWith and APIUserDataFlag_Pickup bits), as set by the add-on that attached it."
                                        },
                                        "data": {
                                            "type": "string",
                                            "description": "The bytes of the user data, base64 encoded. Empty when the user data has no bytes."
                                        },
                                        "pairs": {
                                            "$ref": "#/UserDataPairs",
                                            "description": "The key-value pairs of the user data, decoded from data. Only present when dataVersion is 1, the layout SetUserDataOfGroups writes."
                                        }
                                    },
                                    "additionalProperties": false,
                                    "required": [
                                        "dataVersion",
                                        "platformSign",
                                        "flags",
                                        "data"
                                    ]
                                }
                            },
                            "additionalProperties": false,
                            "required": [
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
            "userDataOfGroups"
        ]
    })";
}

GS::ObjectState GetUserDataOfGroupsCommand::Execute(const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> groups;
    if (!parameters.Get("groups", groups)) {
        return CreateErrorResponse(APIERR_BADPARS, "Invalid or missing 'groups' parameter.");
    }

    GS::ObjectState response;
    const auto& userDataOfGroups = response.AddList<GS::ObjectState>("userDataOfGroups");

    for (const GS::ObjectState& group : groups) {
        const GS::ObjectState* groupId = group.Get("groupId");
        if (groupId == nullptr) {
            userDataOfGroups(CreateErrorResponse(APIERR_BADPARS, "groupId is missing"));
            continue;
        }

        API_UserData userData = {};
        const GSErrCode err = ACAPI_Grouping_GetUserData(GetGuidFromObjectState(*groupId), &userData);
        if (err != NoError) {
            BMKillHandle(&userData.dataHdl);
            userDataOfGroups(CreateErrorResponse(err, err == APIERR_NOUSERDATA
                ? "The group has no user data."
                : "Failed to get the user data of the group."));
            continue;
        }

        // The API hands over the data handle; it is the caller's to dispose.
        GS::UniString data;
        if (userData.dataHdl != nullptr) {
            data = Base64Converter::Encode(*userData.dataHdl, BMGetHandleSize(userData.dataHdl));
            data.DeleteAll(GS::UniChar(char('\n')));
        }
        const UserDataPairs pairs = ParseUserDataPairs(userData.dataHdl);
        BMKillHandle(&userData.dataHdl);

        GS::ObjectState userDataItem;
        userDataItem.Add("dataVersion", static_cast<Int32>(userData.dataVersion));
        userDataItem.Add("platformSign", static_cast<Int32>(userData.platformSign));
        userDataItem.Add("flags", static_cast<Int32>(userData.flags));
        userDataItem.Add("data", data);
        if (userData.dataVersion == UserDataPairsLayoutVersion) {
            AddUserDataPairsList(userDataItem, "pairs", pairs);
        }

        GS::ObjectState item;
        item.Add("userData", userDataItem);
        userDataOfGroups(item);
    }

    return response;
}

SetUserDataOfGroupsCommand::SetUserDataOfGroupsCommand() :
    CommandBase(CommonSchema::Used)
{
}

GS::String SetUserDataOfGroupsCommand::GetName() const
{
    return "SetUserDataOfGroups";
}

GS::Optional<GS::UniString> SetUserDataOfGroupsCommand::GetInputParametersSchema() const
{
    return R"({
        "type": "object",
        "properties": {
            "groups": {
                "type": "array",
                "description": "The groups to store the pairs on.",
                "items": {
                    "$ref": "#/GroupIdArrayItem"
                },
                "minItems": 1
            },
            "pairs": {
                "$ref": "#/UserDataPairs",
                "description": "The key-value pairs to store on each group. A key the group already holds gets the new value; other keys are left as they are."
            },
            "keysToDelete": {
                "$ref": "#/UserDataKeysToDelete"
            }
        },
        "additionalProperties": false,
        "required": [
            "groups"
        ]
    })";
}

GS::Optional<GS::UniString> SetUserDataOfGroupsCommand::GetRawResponseSchema() const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults",
                "description": "One result per group, in the input order."
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    })";
}

GS::ObjectState SetUserDataOfGroupsCommand::Execute(const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> groups;
    if (!parameters.Get("groups", groups)) {
        return CreateErrorResponse(APIERR_BADPARS, "Invalid or missing 'groups' parameter.");
    }

    UserDataPairs pairsToSet;
    GS::Array<GS::UniString> keysToDelete;
    GS::UniString errorMessage;
    if (!GetUserDataPairChanges(parameters, pairsToSet, keysToDelete, errorMessage)) {
        return CreateErrorResponse(APIERR_BADPARS, errorMessage);
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState>("executionResults");

    // ACAPI_Grouping_SetUserData needs an undo scope (APIERR_NEEDSUNDOSCOPE). One undo step for the
    // whole call; the lambda reports success when any group was changed, so a failed group does not
    // roll back the others, and each group carries its own result.
    bool executed = false;
    const GSErrCode undoErr = ACAPI_CallUndoableCommand("SetUserDataOfGroups", [&]() -> GSErrCode {
        executed = true;
        GSErrCode firstErr = NoError;
        bool anyStored = false;

        const auto fail = [&](GSErrCode err, const GS::UniString& message) {
            executionResults(CreateFailedExecutionResult(err, message));
            if (firstErr == NoError) {
                firstErr = err;
            }
        };

        for (const GS::ObjectState& group : groups) {
            const GS::ObjectState* groupId = group.Get("groupId");
            if (groupId == nullptr) {
                fail(APIERR_BADPARS, "groupId is missing");
                continue;
            }
            const API_Guid groupGuid = GetGuidFromObjectState(*groupId);

            UserDataPairs pairs;
            API_UserData storedData = {};
            GSErrCode err = ACAPI_Grouping_GetUserData(groupGuid, &storedData);
            if (err == APIERR_NOUSERDATA) {
                err = NoError;
            } else if (err == NoError && storedData.dataHdl != nullptr) {
                // Data in a layout this command does not write may belong to another add-on or a newer
                // Tapir, so it is reported rather than overwritten.
                if (storedData.dataVersion != UserDataPairsLayoutVersion) {
                    BMKillHandle(&storedData.dataHdl);
                    fail(APIERR_BADPARS, GS::UniString::Printf("The group holds user data in another layout (version %d); it is left unchanged.",
                                                               static_cast<int>(storedData.dataVersion)));
                    continue;
                }
                pairs = ParseUserDataPairs(storedData.dataHdl);
            }
            BMKillHandle(&storedData.dataHdl);
            if (err != NoError) {
                fail(err, "Failed to get the user data of the group.");
                continue;
            }

            ApplyUserDataPairChanges(pairs, pairsToSet, keysToDelete);

            API_UserData userData = {};
            userData.dataVersion = UserDataPairsLayoutVersion;
            userData.platformSign = GS::Act_Platform_Sign;
            // Not FillWith or Pickup: the pairs are script state, not a setting of the group.
            userData.flags = 0;
            userData.dataHdl = SerializeUserDataPairs(pairs);
            if (userData.dataHdl == nullptr) {
                fail(APIERR_MEMFULL, "Failed to store the user data of the group.");
                continue;
            }

            // The API copies the data, the handle stays ours to free.
            err = ACAPI_Grouping_SetUserData(groupGuid, &userData);
            BMKillHandle(&userData.dataHdl);
            if (err != NoError) {
                fail(err, "Failed to store the user data of the group.");
                continue;
            }

            anyStored = true;
            executionResults(CreateSuccessfulExecutionResult());
        }

        return anyStored ? NoError : firstErr;
    });

    if (!executed) {
        return CreateErrorResponse(undoErr != NoError ? undoErr : APIERR_GENERAL, "Failed to start the undoable operation that stores the user data.");
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

CreateElementSetsCommand::CreateElementSetsCommand() :
    CommandBase(CommonSchema::Used)
{
}

GS::String CreateElementSetsCommand::GetName() const
{
    return "CreateElementSets";
}

GS::Optional<GS::UniString> CreateElementSetsCommand::GetInputParametersSchema() const
{
    return R"({
        "type": "object",
        "properties": {
            "elementSets": {
                "type": "array",
                "description": "The element sets to create.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elements": {
                            "$ref": "#/Elements",
                            "description": "The elements to put into the element set."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elements"
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

GS::Optional<GS::UniString> CreateElementSetsCommand::GetRawResponseSchema() const
{
    return R"({
        "type": "object",
        "properties": {
            "elementSets": {
                "type": "array",
                "description": "The identifier of each created element set, or an error, in the order of the input.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
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

GS::ObjectState CreateElementSetsCommand::Execute(const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elementSets;
    if (!parameters.Get("elementSets", elementSets)) {
        return CreateErrorResponse(APIERR_BADPARS, "Invalid or missing 'elementSets' parameter.");
    }

    GS::ObjectState response;
    const auto& elementSetIds = response.AddList<GS::ObjectState>("elementSets");

    const GSErrCode undoErr = ACAPI_CallUndoableCommand("Create Element Sets", [&]() -> GSErrCode {
        for (const GS::ObjectState& elementSet : elementSets) {
            GS::Array<GS::ObjectState> elements;
            if (!elementSet.Get("elements", elements) || elements.IsEmpty()) {
                elementSetIds(CreateErrorResponse(APIERR_BADPARS, "Elements array is missing or empty."));
                continue;
            }

            GS::Array<API_Guid> elemGuids;
            for (const GS::ObjectState& item : elements) {
                elemGuids.Push(GetGuidFromElementsArrayItem(item));
            }

            // No user data is stored with the set; an empty handle is passed
            // rather than a null one, as the API copies the handle's content.
            API_UserData userData = {};
            userData.dataHdl = BMAllocateHandle(0, ALLOCATE_CLEAR, 0);

            API_Guid setGuid = APINULLGuid;
            const GSErrCode err = ACAPI_ElementSet_Create(&elemGuids, &userData, &setGuid);
            BMKillHandle(&userData.dataHdl);

            if (err != NoError) {
                elementSetIds(CreateErrorResponse(err, "Failed to create the element set."));
                continue;
            }

            elementSetIds(CreateIdObjectState("elementSetId", setGuid));
        }
        return NoError;
    });

    if (undoErr != NoError) {
        return CreateErrorResponse(undoErr, "Failed to create the element sets.");
    }

    return response;
}

DeleteElementSetsCommand::DeleteElementSetsCommand() :
    CommandBase(CommonSchema::Used)
{
}

GS::String DeleteElementSetsCommand::GetName() const
{
    return "DeleteElementSets";
}

GS::Optional<GS::UniString> DeleteElementSetsCommand::GetInputParametersSchema() const
{
    return R"({
        "type": "object",
        "properties": {
            "elementSets": {
                "type": "array",
                "description": "The element sets to delete, as returned by CreateElementSets or GetElementSetsOfElements. Only element sets created by the Tapir Add-On can be deleted. The elements of a deleted set are not deleted.",
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

GS::Optional<GS::UniString> DeleteElementSetsCommand::GetRawResponseSchema() const
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

GS::ObjectState DeleteElementSetsCommand::Execute(const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elementSets;
    if (!parameters.Get("elementSets", elementSets)) {
        return CreateErrorResponse(APIERR_BADPARS, "Invalid or missing 'elementSets' parameter.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState>("executionResults");

    const GSErrCode undoErr = ACAPI_CallUndoableCommand("Delete Element Sets", [&]() -> GSErrCode {
        for (const GS::ObjectState& elementSet : elementSets) {
            const GS::ObjectState* elementSetId = elementSet.Get("elementSetId");
            if (elementSetId == nullptr) {
                executionResults(CreateFailedExecutionResult(APIERR_BADPARS, "elementSetId is missing"));
                continue;
            }

            const API_Guid setGuid = GetGuidFromObjectState(*elementSetId);
            if (setGuid == APINULLGuid) {
                executionResults(CreateFailedExecutionResult(APIERR_BADID, "Invalid element set identifier."));
                continue;
            }

            const GSErrCode err = ACAPI_ElementSet_Delete(setGuid);
            switch (err) {
                case NoError:
                    executionResults(CreateSuccessfulExecutionResult());
                    break;
                case APIERR_NOTMINE:
                    executionResults(CreateFailedExecutionResult(err, "The element set was not created by the Tapir Add-On, so it cannot be deleted."));
                    break;
                case APIERR_DELETED:
                    executionResults(CreateFailedExecutionResult(err, "The element set is already deleted."));
                    break;
                case APIERR_BADID:
                    executionResults(CreateFailedExecutionResult(err, "The element set does not exist."));
                    break;
                default:
                    executionResults(CreateFailedExecutionResult(err, "Failed to delete the element set."));
                    break;
            }
        }
        return NoError;
    });

    if (undoErr != NoError) {
        return CreateErrorResponse(undoErr, "Failed to delete the element sets.");
    }

    return response;
}

GetElementSetsOfElementsCommand::GetElementSetsOfElementsCommand() :
    CommandBase(CommonSchema::Used)
{
}

GS::String GetElementSetsOfElementsCommand::GetName() const
{
    return "GetElementSetsOfElements";
}

GS::Optional<GS::UniString> GetElementSetsOfElementsCommand::GetInputParametersSchema() const
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

GS::Optional<GS::UniString> GetElementSetsOfElementsCommand::GetRawResponseSchema() const
{
    return R"({
        "type": "object",
        "properties": {
            "elementSetsOfElements": {
                "type": "array",
                "description": "The element sets that contain each given element, or an error, in the order of the input. Only element sets created by the Tapir Add-On are examined.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "elementSets": {
                                    "type": "array",
                                    "description": "The element sets that contain the element. Empty if the element is not part of any element set.",
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
            "elementSetsOfElements"
        ]
    })";
}

GS::ObjectState GetElementSetsOfElementsCommand::Execute(const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    if (!parameters.Get("elements", elements)) {
        return CreateErrorResponse(APIERR_BADPARS, "Invalid or missing 'elements' parameter.");
    }

    GS::ObjectState response;
    const auto& elementSetsOfElements = response.AddList<GS::ObjectState>("elementSetsOfElements");

    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get("elementId");
        if (elementId == nullptr) {
            elementSetsOfElements(CreateErrorResponse(APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        // The null GUID would make the API return every element set of the
        // add-on, which is not what an element lookup should do.
        const API_Guid elemGuid = GetGuidFromObjectState(*elementId);
        if (elemGuid == APINULLGuid) {
            elementSetsOfElements(CreateErrorResponse(APIERR_BADPARS, "Invalid element identifier."));
            continue;
        }

        GS::Array<API_Guid> setGuids;
        const GSErrCode err = ACAPI_ElementSet_Identify(elemGuid, &setGuids);
        if (err != NoError) {
            elementSetsOfElements(CreateErrorResponse(err, "Failed to get the element sets of the element."));
            continue;
        }

        GS::ObjectState elementSetsItem;
        const auto& elementSets = elementSetsItem.AddList<GS::ObjectState>("elementSets");
        for (const API_Guid& setGuid : setGuids) {
            elementSets(CreateIdObjectState("elementSetId", setGuid));
        }
        elementSetsOfElements(elementSetsItem);
    }

    return response;
}