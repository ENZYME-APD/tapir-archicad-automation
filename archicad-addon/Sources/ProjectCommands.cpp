#include "ProjectCommands.hpp"
#include "MigrationHelper.hpp"

static GS::HashTable<GS::UniString, API_Guid> GetPublisherSetNameGuidTable ()
{
    GS::HashTable<GS::UniString, API_Guid> table;

    Int32 numberOfPublisherSets = 0;
    ACAPI_Navigator_GetNavigatorSetNum (&numberOfPublisherSets);

    API_NavigatorSet set = {};
    for (Int32 ii = 0; ii < numberOfPublisherSets; ++ii) {
        set.mapId = API_PublisherSets;
        GSErrCode err = ACAPI_Navigator_GetNavigatorSet (&set, &ii);
        if (err == NoError) {
            table.Add (set.name, set.rootGuid);
        }
    }

    return table;
}

GetProjectInfoCommand::GetProjectInfoCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetProjectInfoCommand::GetName () const
{
    return "GetProjectInfo";
}

GS::Optional<GS::UniString> GetProjectInfoCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "isUntitled": {
                "type": "boolean",
                "description": "True, if the project is not saved yet."
            },
            "isTeamwork": {
                "type": "boolean",
                "description": "True, if the project is a Teamwork (BIMcloud) project."
            },
            "projectLocation": {
                "type": "string",
                "description": "The location of the project in the filesystem or a BIMcloud project reference.",
                "minLength": 1
            },
            "projectPath": {
                "type": "string",
                "description": "The path of the project. A filesystem path or a BIMcloud server relative path.",
                "minLength": 1
            },
            "projectName": {
                "type": "string",
                "description": "The name of the project.",
                "minLength": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "isUntitled",
            "isTeamwork"
        ]
    })";
}

GS::ObjectState GetProjectInfoCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    API_ProjectInfo projectInfo = {};
    GSErrCode err = ACAPI_ProjectOperation_Project (&projectInfo);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to retrieve project information. Check the opened project!");
    }

    GS::ObjectState response;
    response.Add ("isUntitled", projectInfo.untitled);
    response.Add ("isTeamwork", projectInfo.teamwork);
    if (!projectInfo.untitled) {
        if (projectInfo.location) {
            response.Add ("projectLocation", projectInfo.location->ToDisplayText ());
        }
        if (projectInfo.projectPath) {
            response.Add ("projectPath", *projectInfo.projectPath);
        }
        if (projectInfo.projectName) {
            response.Add ("projectName", *projectInfo.projectName);
        }
    }

    return response;
}

GetProjectInfoFieldsCommand::GetProjectInfoFieldsCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetProjectInfoFieldsCommand::GetName () const
{
    return "GetProjectInfoFields";
}

GS::Optional<GS::UniString> GetProjectInfoFieldsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "fields": {
                "type": "array",
                "description": "A list of project info fields.",
                "items": {
                    "type": "object",
                    "properties": {
                        "projectInfoId": {
                            "type": "string",
                            "description": "The id of the project info field."
                        },
                        "projectInfoName": {
                            "type": "string",
                            "description": "The name of the project info field visible on UI."
                        },
                        "projectInfoValue": {
                            "type": "string",
                            "description": "The value of the project info field."
                        }
                    }
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "fields"
        ]
    })";
}

GS::ObjectState GetProjectInfoFieldsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ArrayFB<GS::UniString, 3>> autoTexts;
    API_AutotextType type = APIAutoText_All;

    GSErrCode err = ACAPI_AutoText_GetAutoTexts (&autoTexts, type);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to retrieve project information fields.");
    }

    static const GS::Array<GS::UniString> validPrefixes = {
        "PROJECT", "KEYWORD", "NOTES", "SITE", "BUILDING", "CONTACT", "CAD_TECHNICIAN", "CLIENT"
    };

    GS::ObjectState response;
    const auto& listAdder = response.AddList<GS::ObjectState> ("fields");

    for (const auto& autoText : autoTexts) {
        const GS::UniString& autoTextName = autoText[0];
        const GS::UniString& autoTextId = autoText[1];
        const GS::UniString& autoTextValue = autoText[2];

        bool isValidPrefix = false;
        for (const GS::UniString& validPrefix : validPrefixes) {
            if (autoTextId.BeginsWith (validPrefix) || autoTextId.BeginsWith ("autotext-" + validPrefix)) {
                isValidPrefix = true;
                break;
            }
        }
        if (!isValidPrefix) {
            continue;
        }

        GS::ObjectState projectInfoData;
        projectInfoData.Add ("projectInfoId", autoTextId);
        projectInfoData.Add ("projectInfoName", autoTextName);
        projectInfoData.Add ("projectInfoValue", autoTextValue);
        listAdder (projectInfoData);
    }

    return response;
}

SetProjectInfoFieldCommand::SetProjectInfoFieldCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String SetProjectInfoFieldCommand::GetName () const
{
    return "SetProjectInfoField";
}

GS::Optional<GS::UniString> SetProjectInfoFieldCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "projectInfoId": {
                "type": "string",
                "description": "The id of the project info field.",
                "minLength": 1
            },
            "projectInfoValue": {
                "type": "string",
                "description": "The new value of the project info field.",
                "minLength": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "projectInfoId",
            "projectInfoValue"
        ]
    })";
}

GS::ObjectState SetProjectInfoFieldCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString projectInfoId;
    GS::UniString projectInfoValue;
    if (!parameters.Get ("projectInfoId", projectInfoId) || !parameters.Get ("projectInfoValue", projectInfoValue)) {
        return CreateErrorResponse (Error, "Invalid input parameters.");
    }

    GSErrCode err = ACAPI_AutoText_SetAnAutoText (&projectInfoId, &projectInfoValue);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to set project information field.");
    }

    return {};
}

GetHotlinksCommand::GetHotlinksCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetHotlinksCommand::GetName () const
{
    return "GetHotlinks";
}

GS::Optional<GS::UniString> GetHotlinksCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "hotlinks": {
                "$ref": "#/Hotlinks"
            }
        },
        "additionalProperties": false,
        "required": [
            "hotlinks"
        ]
    })";
}

static GS::Optional<GS::UniString> GetLocationOfHotlink (const API_Guid& hotlinkGuid)
{
    API_HotlinkNode hotlinkNode = {};
    hotlinkNode.guid = hotlinkGuid;

    ACAPI_Hotlink_GetHotlinkNode (&hotlinkNode);
    if (hotlinkNode.sourceLocation == nullptr) {
        return GS::NoValue;
    }

    return hotlinkNode.sourceLocation->ToDisplayText ();
}

static GS::ObjectState DumpHotlinkWithChildren (const API_Guid& hotlinkGuid,
    GS::HashTable<API_Guid, GS::Array<API_Guid>>& hotlinkTree)
{
    GS::ObjectState hotlinkNodeOS;

    const auto& location = GetLocationOfHotlink (hotlinkGuid);
    if (location.HasValue ()) {
        hotlinkNodeOS.Add ("location", location.Get ());
    }

    const auto& children = hotlinkTree.Retrieve (hotlinkGuid);
    if (!children.IsEmpty ()) {
        const auto& listAdder = hotlinkNodeOS.AddList<GS::ObjectState> ("children");
        for (const API_Guid& childNodeGuid : hotlinkTree.Retrieve (hotlinkGuid)) {
            listAdder (DumpHotlinkWithChildren (childNodeGuid, hotlinkTree));
        }
    }

    return hotlinkNodeOS;
}

GS::ObjectState GetHotlinksCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::ObjectState response;
    const auto& listAdder = response.AddList<GS::ObjectState> ("hotlinks");

    for (API_HotlinkTypeID type : {APIHotlink_Module, APIHotlink_XRef}) {
        API_Guid hotlinkRootNodeGuid = APINULLGuid;
        if (ACAPI_Hotlink_GetHotlinkRootNodeGuid (&type, &hotlinkRootNodeGuid) == NoError) {
            GS::HashTable<API_Guid, GS::Array<API_Guid>> hotlinkTree;
            if (ACAPI_Hotlink_GetHotlinkNodeTree (&hotlinkRootNodeGuid, &hotlinkTree) == NoError) {
                for (const API_Guid& childNodeGuid : hotlinkTree.Retrieve (hotlinkRootNodeGuid)) {
                    listAdder (DumpHotlinkWithChildren (childNodeGuid, hotlinkTree));
                }
            }
        }
    }

    return response;
}

PublishPublisherSetCommand::PublishPublisherSetCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String PublishPublisherSetCommand::GetName () const
{
    return "PublishPublisherSet";
}

GS::Optional<GS::UniString> PublishPublisherSetCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "publisherSetName": {
                "type": "string",
                "description": "The name of the publisher set.",
                "minLength": 1
            },
            "outputPath": {
                "type": "string",
                "description": "Full local or LAN path for publishing. Optional, by default the path set in the settings of the publiser set will be used.",
                "minLength": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "publisherSetName"
        ]
    })";
}

GS::ObjectState PublishPublisherSetCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString publisherSetName;
    parameters.Get ("publisherSetName", publisherSetName);

    const auto publisherSetNameGuidTable = GetPublisherSetNameGuidTable ();
    if (!publisherSetNameGuidTable.ContainsKey (publisherSetName)) {
        return CreateErrorResponse (Error, "Not valid publisher set name.");
    }

    API_PublishPars publishPars = {};
    publishPars.guid = publisherSetNameGuidTable.Get (publisherSetName);

    if (parameters.Contains ("outputPath")) {
        GS::UniString outputPath;
        parameters.Get ("outputPath", outputPath);
        publishPars.path = new IO::Location (outputPath);
    }

    GSErrCode err = ACAPI_ProjectOperation_Publish (&publishPars);
    delete publishPars.path;

    if (err != NoError) {
        return CreateErrorResponse (err, "Publishing failed. Check output path!");
    }

    return {};
}

GetStoryInfoCommand::GetStoryInfoCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetStoryInfoCommand::GetName () const
{
    return "GetStoryInfo";
}

GS::Optional<GS::UniString> GetStoryInfoCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "firstStory": {
                "type": "integer",
                "description": "First story index."
            },
            "lastStory": {
                "type": "integer",
                "description": "Last story index."
            },
            "actStory": {
                "type": "integer",
                "description": "Actual (currently visible in 2D) story index."
            },
            "skipNullFloor": {
                "type": "boolean",
                "description": "Floor indices above ground-floor level may start with 1 instead of 0."
            },
            "stories": {
                "type": "array",
                "description": "A list of project stories.",
                "items": {
                    "type": "object",
                    "properties": {
                        "index": {
                            "type": "integer",
                            "description": "The story index."
                        },
                        "floorId": {
                            "type": "integer",
                            "description": "Unique ID of the story."
                        },
                        "dispOnSections": {
                            "type": "boolean",
                            "description": "Story level lines should appear on sections and elevations."
                        },
                        "level": {
                            "type": "number",
                            "description": "The story level."
                        },
                        "uName": {
                            "type": "string",
                            "description": "The name of the story."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "index",
                        "floorId",
                        "dispOnSections",
                        "level",
                        "uName"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "firstStory",
            "lastStory",
            "actStory",
            "skipNullFloor",
            "stories"
        ]
    })";
}


GS::ObjectState GetStoryInfoCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    API_StoryInfo storyInfo;
    BNZeroMemory (&storyInfo, sizeof (API_StoryInfo));
    GSErrCode err = ACAPI_ProjectSetting_GetStorySettings (&storyInfo);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to retrive stories info.");
    }

    GS::ObjectState response;
    response.Add ("firstStory", storyInfo.firstStory);
    response.Add ("lastStory", storyInfo.lastStory);
    response.Add ("actStory", storyInfo.actStory);
    response.Add ("skipNullFloor", storyInfo.skipNullFloor);

    const auto& listAdder = response.AddList<GS::ObjectState> ("stories");

    short storyCount = storyInfo.lastStory - storyInfo.firstStory + 1;
    for (short i = storyCount - 1; i >= 0; i--) {
        const API_StoryType& story = (*storyInfo.data)[i];
        GS::ObjectState storyData;
        GS::UniString uName = story.uName;

        storyData.Add ("index", story.index);
        storyData.Add ("floorId", story.floorId);
        storyData.Add ("dispOnSections", story.dispOnSections);
        storyData.Add ("level", story.level);
        storyData.Add ("uName", uName);

        listAdder (storyData);
    }

    return response;
}