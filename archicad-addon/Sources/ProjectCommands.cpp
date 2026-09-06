#include "ProjectCommands.hpp"
#include "MigrationHelper.hpp"

#include <cmath>

GetProjectInfoCommand::GetProjectInfoCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetProjectInfoCommand::GetName () const
{
    return "GetProjectInfo";
}

GS::Optional<GS::UniString> GetProjectInfoCommand::GetRawResponseSchema () const
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

GS::Optional<GS::UniString> GetProjectInfoFieldsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "fields": {
                "$ref": "#/ProjectInfoFields"
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
                "description": "The new value of the project info field. An empty string clears the field."
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

    // Clearing a field is a nullptr value, not an empty string - the DevKit is explicit:
    // "You can set the autotext value empty by passing nullptr in the autotextValue
    // parameter." Passing an empty UniString instead does not clear it.
    const bool clearTheField = projectInfoValue.IsEmpty ();

    GSErrCode err = ACAPI_AutoText_SetAnAutoText (&projectInfoId,
                                                  clearTheField ? nullptr : &projectInfoValue);
    if (err != NoError) {
        return CreateErrorResponse (err, clearTheField
            ? "Failed to clear project information field."
            : "Failed to set project information field.");
    }

    return {};
}

CreateProjectInfoFieldsCommand::CreateProjectInfoFieldsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateProjectInfoFieldsCommand::GetName () const
{
    return "CreateProjectInfoFields";
}

GS::Optional<GS::UniString> CreateProjectInfoFieldsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "projectInfoFields": {
                "type": "array",
                "description": "Array of custom project info fields to create.",
                "items": {
                    "type": "object",
                    "properties": {
                        "projectInfoName": {
                            "type": "string",
                            "description": "Display name of the project info field.",
                            "minLength": 1
                        },
                        "projectInfoValue": {
                            "type": "string",
                            "description": "Initial value of the project info field."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "projectInfoName"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "projectInfoFields"
        ]
    })";
}

GS::Optional<GS::UniString> CreateProjectInfoFieldsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "fields": {
                "$ref": "#/ProjectInfoFields"
            }
        },
        "additionalProperties": false,
        "required": [
            "fields"
        ]
    })";
}

GS::ObjectState CreateProjectInfoFieldsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> projectInfoFields;
    if (!parameters.Get ("projectInfoFields", projectInfoFields) || projectInfoFields.IsEmpty ()) {
        return CreateErrorResponse (APIERR_BADPARS, "projectInfoFields is missing or empty.");
    }

    GS::ObjectState response;
    const auto& fieldsAdder = response.AddList<GS::ObjectState> ("fields");

    ACAPI_CallUndoableCommand ("CreateProjectInfoFields", [&]() -> GSErrCode {
        for (const GS::ObjectState& projectInfoField : projectInfoFields) {
            GS::UniString projectInfoName;
            if (!projectInfoField.Get ("projectInfoName", projectInfoName) || projectInfoName.IsEmpty ()) {
                fieldsAdder (CreateErrorResponse (APIERR_BADPARS, "projectInfoName is missing or empty."));
                continue;
            }

            GS::UniString projectInfoValue;
            projectInfoField.Get ("projectInfoValue", projectInfoValue);

#ifdef ServerMainVers_3000
            fieldsAdder (CreateErrorResponse (APIERR_NOTSUPPORTED, "TODO: this function was not migrated to AC30 yet. Failed to create project information field."));
#else
            GS::Guid guid;
            guid.Generate ();
            API_Guid dbKey = GSGuid2APIGuid (guid);

            GSErrCode err = ACAPI_AutoText_CreateAnAutoText (&dbKey, projectInfoName.ToCStr ());
            if (err != NoError) {
                fieldsAdder (CreateErrorResponse (err, "Failed to create project information field."));
                continue;
            }

            GS::UniString projectInfoId ("autotext-");
            projectInfoId.Append (guid.ToUniString ());

            err = ACAPI_AutoText_SetAnAutoText (&projectInfoId, &projectInfoValue);
            if (err != NoError) {
                fieldsAdder (CreateErrorResponse (err, "Failed to set the initial value of the project information field."));
                continue;
            }

            GS::ObjectState createdField;
            createdField.Add ("projectInfoId", projectInfoId);
            createdField.Add ("projectInfoName", projectInfoName);
            createdField.Add ("projectInfoValue", projectInfoValue);
            fieldsAdder (createdField);
#endif
        }

        return NoError;
    });

    return response;
}

DeleteProjectInfoFieldsCommand::DeleteProjectInfoFieldsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteProjectInfoFieldsCommand::GetName () const
{
    return "DeleteProjectInfoFields";
}

GS::Optional<GS::UniString> DeleteProjectInfoFieldsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "projectInfoIds": {
                "type": "array",
                "description": "List of project info field ids to delete. Only custom fields (ids starting with 'autotext-') can be deleted.",
                "items": {
                    "type": "string"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "projectInfoIds"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteProjectInfoFieldsCommand::GetRawResponseSchema () const
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

GS::ObjectState DeleteProjectInfoFieldsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::UniString> projectInfoIds;
    if (!parameters.Get ("projectInfoIds", projectInfoIds) || projectInfoIds.IsEmpty ()) {
        return CreateErrorResponse (APIERR_BADPARS, "projectInfoIds is missing or empty.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("DeleteProjectInfoFields", [&]() -> GSErrCode {
        for (const GS::UniString& projectInfoId : projectInfoIds) {
#ifdef ServerMainVers_3000
            (void) projectInfoId; // suppress unused variable warning
            executionResults (CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "TODO: this function was not migrated to AC30 yet. Failed to delete project info field."));
#else
            if (!projectInfoId.BeginsWith ("autotext-")) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS,
                    "Only custom project info fields (ids starting with 'autotext-') can be deleted."));
                continue;
            }

            GSErrCode err = ACAPI_AutoText_DeleteAnAutoText (projectInfoId.ToCStr ());
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to delete project info field."));
            } else {
                executionResults (CreateSuccessfulExecutionResult ());
            }
#endif
        }
        return NoError;
    });

    return response;
}

GetHotlinksCommand::GetHotlinksCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetHotlinksCommand::GetName () const
{
    return "GetHotlinks";
}

GS::Optional<GS::UniString> GetHotlinksCommand::GetRawResponseSchema () const
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

    // The node guid is what CreateHotlinkInstances needs; the name and type
    // are what a caller shows. All three are additions to the original shape.
    hotlinkNodeOS.Add ("hotlinkNodeId", CreateGuidObjectState (hotlinkGuid));
    API_HotlinkNode hotlinkNode = {};
    hotlinkNode.guid = hotlinkGuid;
    if (ACAPI_Hotlink_GetHotlinkNode (&hotlinkNode) == NoError) {
        hotlinkNodeOS.Add ("name", GS::UniString (hotlinkNode.name));
        hotlinkNodeOS.Add ("type", hotlinkNode.type == APIHotlink_XRef ? "XRef" : "Module");
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

GetStoriesCommand::GetStoriesCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetStoriesCommand::GetName () const
{
    return "GetStories";
}

GS::Optional<GS::UniString> GetStoriesCommand::GetRawResponseSchema () const
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
                "$ref": "#/StoriesParameters"
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


GS::ObjectState GetStoriesCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    API_StoryInfo storyInfo = {};
    GSErrCode err = ACAPI_ProjectSetting_GetStorySettings (&storyInfo);
    if (err != NoError) {
        BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
        return CreateErrorResponse (err, "Failed to retrive stories info.");
    }

    GS::ObjectState response;
    response.Add ("firstStory", storyInfo.firstStory);
    response.Add ("lastStory", storyInfo.lastStory);
    response.Add ("actStory", storyInfo.actStory);
    response.Add ("skipNullFloor", storyInfo.skipNullFloor);

    const auto& listAdder = response.AddList<GS::ObjectState> ("stories");

    short storyCount = storyInfo.lastStory - storyInfo.firstStory + 1;
    for (short i = 0; i < storyCount; i++) {
        const API_StoryType& story = (*storyInfo.data)[i];
        GS::ObjectState storyData;
        GS::UniString uName = story.uName;

        storyData.Add ("index", story.index);
        storyData.Add ("floorId", story.floorId);
        storyData.Add ("dispOnSections", story.dispOnSections);
        storyData.Add ("level", story.level);
        if (i + 1 < storyCount) {
            storyData.Add ("height", (*storyInfo.data)[i + 1].level - story.level);
        }
        storyData.Add ("name", uName);

        listAdder (storyData);
    }

    BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));

    return response;
}

// The requested stories are matched to the existing ones positionally, bottom-up. The
// optional index of any of them pins the numbering of the whole list: the story at
// position i is meant to become story (index - i). Without an index the numbering of
// the existing structure is kept, so the list can only grow and shrink on the top.
static bool GetNewFirstStoryIndex (const GS::Array<GS::ObjectState>& stories, const short currentFirstStory, short& newFirstStory)
{
    newFirstStory = currentFirstStory;

    bool isPinned = false;
    for (GS::UIndex i = 0; i < stories.GetSize (); ++i) {
        short index = 0;
        if (!stories[i].Get ("index", index)) {
            continue;
        }

        const short firstStoryFromIndex = index - static_cast<short> (i);
        if (!isPinned) {
            newFirstStory = firstStoryFromIndex;
            isPinned = true;
        } else if (firstStoryFromIndex != newFirstStory) {
            return false;
        }
    }

    return true;
}

// Two story levels closer to each other than this are considered to be the same.
constexpr double StoryLevelTolerance = 0.0001;

static void FillNewStoryCmd (const GS::Array<GS::ObjectState>& stories, const GS::UIndex storyPos, API_StoryCmdType& storyCmd)
{
    if (storyPos >= stories.GetSize ()) {
        return;
    }

    stories[storyPos].Get ("dispOnSections", storyCmd.dispOnSections);
    stories[storyPos].Get ("level", storyCmd.elevation);

    // The story at the top of the requested list has no next one to take the height
    // from, so fall back to the height of the story below it. The levels are set
    // exactly later on anyway, this is only to avoid creating zero height stories.
    double neighbourLevel = 0.0;
    if (storyPos + 1 < stories.GetSize () && stories[storyPos + 1].Get ("level", neighbourLevel)) {
        storyCmd.height = neighbourLevel - storyCmd.elevation;
    } else if (storyPos > 0 && stories[storyPos - 1].Get ("level", neighbourLevel)) {
        storyCmd.height = storyCmd.elevation - neighbourLevel;
    }

    GS::UniString name;
    stories[storyPos].Get ("name", name);
    GS::snuprintf (storyCmd.uName, sizeof (storyCmd.uName), name.ToCStr ());
}

// A copy of what a story looks like right now. API_StoryInfo::data is a handle which every
// refresh disposes and reallocates, so anything read out of it has to be copied before the
// next ChangeStorySettings call rather than referenced.
struct StorySnapshot {
    short           index;
    double          level;
    bool            dispOnSections;
    GS::UniString   name;
};

static void TakeStorySnapshot (const API_StoryInfo& storyInfo, GS::Array<StorySnapshot>& snapshot)
{
    snapshot.Clear ();

    if (storyInfo.data == nullptr) {
        return;
    }

    for (short index = storyInfo.firstStory; index <= storyInfo.lastStory; ++index) {
        const API_StoryType& story = (*storyInfo.data)[index - storyInfo.firstStory];
        snapshot.Push (StorySnapshot { story.index, story.level, story.dispOnSections, GS::UniString (story.uName) });
    }
}

static GSErrCode RefreshStoryInfo (API_StoryInfo& storyInfo)
{
    BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
    return ACAPI_ProjectSetting_GetStorySettings (&storyInfo);
}

SetStoriesCommand::SetStoriesCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String SetStoriesCommand::GetName () const
{
    return "SetStories";
}

GS::Optional<GS::UniString> SetStoriesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "stories": {
                "$ref": "#/StoriesSettings"
            }
        },
        "additionalProperties": false,
        "required": [
            "stories"
        ]
    })";
}

GS::Optional<GS::UniString> SetStoriesCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}


GS::ObjectState SetStoriesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> stories;
    parameters.Get ("stories", stories);

    if (stories.IsEmpty ()) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "stories is missing or empty.");
    }

    API_StoryInfo storyInfo = {};
    GSErrCode err = ACAPI_ProjectSetting_GetStorySettings (&storyInfo);
    if (err != NoError) {
        BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
        return CreateFailedExecutionResult (err, "Failed to retrive stories info.");
    }

    short newFirstStory = storyInfo.firstStory;
    if (!GetNewFirstStoryIndex (stories, storyInfo.firstStory, newFirstStory)) {
        BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
        return CreateFailedExecutionResult (APIERR_BADPARS, "The given story indices are not consecutive.");
    }

    const short newLastStory = newFirstStory + static_cast<short> (stories.GetSize ()) - 1;

    // Grow the structure first - downwards and upwards - and only then cut off the
    // stories which are not needed any more, so the project never runs out of stories.
    while (storyInfo.firstStory > newFirstStory) {
        API_StoryCmdType storyCmd = {};
        storyCmd.action = APIStory_InsBelow;
        storyCmd.index  = storyInfo.firstStory;
        FillNewStoryCmd (stories, static_cast<GS::UIndex> (storyInfo.firstStory - 1 - newFirstStory), storyCmd);

        err = ACAPI_ProjectSetting_ChangeStorySettings (&storyCmd);
        if (err != NoError) {
            BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
            return CreateFailedExecutionResult (err, "Failed to create new story below the first one.");
        }

        const short prevFirstStory = storyInfo.firstStory;
        err = RefreshStoryInfo (storyInfo);
        if (err != NoError) {
            BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
            return CreateFailedExecutionResult (err, "Failed to retrive stories info.");
        }

        if (storyInfo.firstStory >= prevFirstStory) {
            BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
            return CreateFailedExecutionResult (APIERR_GENERAL, "Failed to create new story below the first one.");
        }
    }

    while (storyInfo.lastStory < newLastStory) {
        API_StoryCmdType storyCmd = {};
        storyCmd.action = APIStory_InsAbove;
        storyCmd.index  = storyInfo.lastStory;
        FillNewStoryCmd (stories, static_cast<GS::UIndex> (storyInfo.lastStory + 1 - newFirstStory), storyCmd);

        err = ACAPI_ProjectSetting_ChangeStorySettings (&storyCmd);
        if (err != NoError) {
            BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
            return CreateFailedExecutionResult (err, "Failed to create new story.");
        }

        const short prevLastStory = storyInfo.lastStory;
        err = RefreshStoryInfo (storyInfo);
        if (err != NoError) {
            BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
            return CreateFailedExecutionResult (err, "Failed to retrive stories info.");
        }

        if (storyInfo.lastStory <= prevLastStory) {
            BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
            return CreateFailedExecutionResult (APIERR_GENERAL, "Failed to create new story.");
        }
    }

    // Deleting a story deletes its elements as well, so never delete more stories than
    // the number the grown structure has above the requested one.
    GS::USize storiesToDelete = static_cast<GS::USize> (storyInfo.lastStory - storyInfo.firstStory + 1) - stories.GetSize ();

    while (storiesToDelete > 0 && (storyInfo.firstStory < newFirstStory || storyInfo.lastStory > newLastStory)) {
        const bool deleteFromBottom = storyInfo.firstStory < newFirstStory;

        API_StoryCmdType storyCmd = {};
        storyCmd.action = APIStory_Delete;
        storyCmd.index  = deleteFromBottom ? storyInfo.firstStory : storyInfo.lastStory;

        err = ACAPI_ProjectSetting_ChangeStorySettings (&storyCmd);
        if (err != NoError) {
            BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
            return CreateFailedExecutionResult (err, "Failed to delete story.");
        }

        const short prevFirstStory = storyInfo.firstStory;
        const short prevLastStory  = storyInfo.lastStory;
        err = RefreshStoryInfo (storyInfo);
        if (err != NoError) {
            BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
            return CreateFailedExecutionResult (err, "Failed to retrive stories info.");
        }

        if (storyInfo.lastStory - storyInfo.firstStory >= prevLastStory - prevFirstStory) {
            BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
            return CreateFailedExecutionResult (APIERR_GENERAL, "Failed to delete story.");
        }

        --storiesToDelete;
    }

    const GS::USize storyCount = static_cast<GS::USize> (storyInfo.lastStory - storyInfo.firstStory + 1);

    if (storyCount != stories.GetSize ()) {
        BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
        return CreateFailedExecutionResult (APIERR_GENERAL, "Failed to set up the requested story structure.");
    }

    // Everything below works off snapshots rather than off storyInfo.data: that handle is
    // disposed and reallocated by every refresh, so a reference into it does not survive a
    // single ChangeStorySettings call - which is where the garbage story indices in the
    // error messages came from.
    GS::Array<StorySnapshot> currentStories;
    TakeStorySnapshot (storyInfo, currentStories);
    if (currentStories.GetSize () != storyCount) {
        BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
        return CreateFailedExecutionResult (APIERR_GENERAL, "Failed to read the story structure.");
    }

    // Neither renaming a story nor changing its display setting moves any of them, so
    // these can be set in one pass on the state read above.
    bool storySettingsChanged = false;

    for (GS::UIndex i = 0; i < storyCount; ++i) {
        API_StoryCmdType storyCmd = {};
        storyCmd.index = currentStories[i].index;

        stories[i].Get ("dispOnSections", storyCmd.dispOnSections);

        if (currentStories[i].dispOnSections != storyCmd.dispOnSections) {
            storyCmd.action = APIStory_SetDispOnSections;

            err = ACAPI_ProjectSetting_ChangeStorySettings (&storyCmd);
            if (err != NoError) {
                BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
                return CreateFailedExecutionResult (err, "Failed to modify dispOnSections settings.");
            }

            storySettingsChanged = true;
        }

        GS::UniString name;
        stories[i].Get ("name", name);

        if (currentStories[i].name != name) {
            GS::snuprintf (storyCmd.uName, sizeof (storyCmd.uName), name.ToCStr ());
            storyCmd.action = APIStory_Rename;

            err = ACAPI_ProjectSetting_ChangeStorySettings (&storyCmd);
            if (err != NoError) {
                BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
                return CreateFailedExecutionResult (err, "Failed to rename story.");
            }

            storySettingsChanged = true;
        }
    }

    if (storySettingsChanged) {
        err = RefreshStoryInfo (storyInfo);
        if (err != NoError) {
            BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
            return CreateFailedExecutionResult (err, "Failed to retrive stories info.");
        }

        if (static_cast<GS::USize> (storyInfo.lastStory - storyInfo.firstStory + 1) != storyCount) {
            BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
            return CreateFailedExecutionResult (APIERR_GENERAL, "The story structure changed unexpectedly.");
        }

        TakeStorySnapshot (storyInfo, currentStories);
    }

    // The requested levels, defaulting to where the story already sits when the caller did
    // not ask for one.
    GS::Array<double> targetLevels;
    for (GS::UIndex i = 0; i < storyCount; ++i) {
        double level = currentStories[i].level;
        stories[i].Get ("level", level);
        targetLevels.Push (level);
    }

    // Levels are set in two steps, because Archicad anchors the story ladder on the active
    // story: that one never moves, and every other story is positioned relative to it.
    // Measured on a live AC29: APIStory_SetElevation moves only the story it names, and
    // APIStory_SetHeight moves whichever side of the boundary is further from the anchor.
    //
    // Step one puts the anchor on its requested level. It has to come first - moving the
    // anchor afterwards would change the gap to its neighbour and undo a distance already
    // set. The anchor is the active story, the only one SetElevation is known to move.
    const GS::UIndex anchor =
        (storyInfo.actStory >= storyInfo.firstStory && storyInfo.actStory <= storyInfo.lastStory)
            ? static_cast<GS::UIndex> (storyInfo.actStory - storyInfo.firstStory)
            : 0;

    if (std::abs (currentStories[anchor].level - targetLevels[anchor]) >= StoryLevelTolerance) {
        API_StoryCmdType storyCmd = {};
        storyCmd.action    = APIStory_SetElevation;
        storyCmd.index     = currentStories[anchor].index;
        storyCmd.elevation = targetLevels[anchor];

        err = ACAPI_ProjectSetting_ChangeStorySettings (&storyCmd);
        if (err != NoError) {
            BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
            return CreateFailedExecutionResult (err, "Failed to change story elevation.");
        }

        err = RefreshStoryInfo (storyInfo);
        if (err != NoError) {
            BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
            return CreateFailedExecutionResult (err, "Failed to retrive stories info.");
        }
        if (static_cast<GS::USize> (storyInfo.lastStory - storyInfo.firstStory + 1) != storyCount) {
            BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
            return CreateFailedExecutionResult (APIERR_GENERAL, "The story structure changed unexpectedly.");
        }
        TakeStorySnapshot (storyInfo, currentStories);
    }

    // Step two sets the distances, working outwards from the anchor in both directions so
    // that every height is set against a story which is already on its requested level and
    // positions exactly one story that is not.
    for (GS::UIndex i = anchor; i + 1 < storyCount; ++i) {
        API_StoryCmdType storyCmd = {};
        storyCmd.action = APIStory_SetHeight;
        storyCmd.index  = currentStories[i].index;
        storyCmd.height = targetLevels[i + 1] - targetLevels[i];

        err = ACAPI_ProjectSetting_ChangeStorySettings (&storyCmd);
        if (err != NoError) {
            BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
            return CreateFailedExecutionResult (err, "Failed to change story height.");
        }
    }

    for (GS::UIndex i = anchor; i > 0; --i) {
        API_StoryCmdType storyCmd = {};
        storyCmd.action = APIStory_SetHeight;
        storyCmd.index  = currentStories[i - 1].index;
        storyCmd.height = targetLevels[i] - targetLevels[i - 1];

        err = ACAPI_ProjectSetting_ChangeStorySettings (&storyCmd);
        if (err != NoError) {
            BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
            return CreateFailedExecutionResult (err, "Failed to change story height.");
        }
    }

    err = RefreshStoryInfo (storyInfo);
    if (err != NoError) {
        BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
        return CreateFailedExecutionResult (err, "Failed to retrive stories info.");
    }
    if (static_cast<GS::USize> (storyInfo.lastStory - storyInfo.firstStory + 1) != storyCount) {
        BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
        return CreateFailedExecutionResult (APIERR_GENERAL, "The story structure changed unexpectedly.");
    }
    TakeStorySnapshot (storyInfo, currentStories);

    // A structure which silently ended up somewhere else than requested is worse than an
    // error, so report the first level which could not be set, naming both levels so the
    // message says what actually happened.
    for (GS::UIndex i = 0; i < storyCount; ++i) {
        if (std::abs (currentStories[i].level - targetLevels[i]) >= StoryLevelTolerance) {
            const GS::UniString message = GS::UniString::Printf (
                "Failed to set the level of story %d: requested %.4f, got %.4f.",
                static_cast<int> (currentStories[i].index), targetLevels[i], currentStories[i].level);
            BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
            return CreateFailedExecutionResult (APIERR_GENERAL, message);
        }
    }

    BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));

    return CreateSuccessfulExecutionResult ();
}

OpenProjectCommand::OpenProjectCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String OpenProjectCommand::GetName () const
{
    return "OpenProject";
}

GS::Optional<GS::UniString> OpenProjectCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "projectFilePath": {
                "type": "string",
                "description": "The target project file to open."
            }
        },
        "additionalProperties": false,
        "required": [
            "projectFilePath"
        ]
    })";
}

GS::Optional<GS::UniString> OpenProjectCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState OpenProjectCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString projectFilePath;
    if (!parameters.Get ("projectFilePath", projectFilePath)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "projectFilePath parameter is missing");
    }

    IO::Location projectLocation (projectFilePath);
    IO::Name lastLocalName;
    if (projectLocation.GetLastLocalName (&lastLocalName) != NoError) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "projectFilePath parameter is invalid");
    }

    const GS::UniString extension = lastLocalName.GetExtension ();
    API_FileOpenPars openPars = {};
    if (extension.Compare ("pln", CaseInsensitive) == GS::UniString::Equal) {
        openPars.fileTypeID = APIFType_PlanFile;
    } else if (extension.Compare ("pla", CaseInsensitive) == GS::UniString::Equal) {
        openPars.fileTypeID = APIFType_A_PlanFile;
    } else {
        return CreateFailedExecutionResult (APIERR_BADPARS, "projectFilePath parameter is invalid, the extension must be pln or pla");
    }

    openPars.libGiven = false;
    openPars.useStoredLib = true;
#ifndef ServerMainVers_2900
    openPars.enableSaveAlert = false;
#endif
    openPars.file = &projectLocation;

    const GSErrCode err = ACAPI_ProjectOperation_Open (&openPars);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to open the given project");
    }

    return CreateSuccessfulExecutionResult ();
}

CloseProjectCommand::CloseProjectCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String CloseProjectCommand::GetName () const
{
    return "CloseProject";
}

GS::Optional<GS::UniString> CloseProjectCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState CloseProjectCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GSErrCode err = ACAPI_ProjectOperation_Close ();
    if (err != NoError) {
        return CreateFailedExecutionResult (APIERR_COMMANDFAILED, "Failed to close the project. There might be none currently open.");
    }
    return CreateSuccessfulExecutionResult ();
}

SaveProjectCommand::SaveProjectCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String SaveProjectCommand::GetName () const
{
    return "SaveProject";
}

GS::Optional<GS::UniString> SaveProjectCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState SaveProjectCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GSErrCode err = ACAPI_ProjectOperation_Save ();
    if (err != NoError) {
        return CreateFailedExecutionResult (APIERR_COMMANDFAILED, "Failed to save the project.");
    }
    return CreateSuccessfulExecutionResult ();
}

SaveAsModuleFileCommand::SaveAsModuleFileCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SaveAsModuleFileCommand::GetName () const
{
    return "SaveAsModuleFile";
}

GS::Optional<GS::UniString> SaveAsModuleFileCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "moduleFilePath": {
                "type": "string",
                "description": "Absolute path of the .mod file to write. An existing file is overwritten. The current window must be a floor plan, section, elevation or detail."
            },
            "elements": {
                "$ref": "#/Elements",
                "description": "Optional. The elements that go into the module; omitted, the current selection does, as Save Selection as Module would. Pass GetAllElements for the whole project. Archicad 25 and 26 support the selection form only."
            }
        },
        "additionalProperties": false,
        "required": [
            "moduleFilePath"
        ]
    })";
}

GS::Optional<GS::UniString> SaveAsModuleFileCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState SaveAsModuleFileCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString moduleFilePath;
    if (!parameters.Get ("moduleFilePath", moduleFilePath) || moduleFilePath.IsEmpty ()) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "moduleFilePath is missing.");
    }
    IO::Location location (moduleFilePath);

    GS::Array<GS::ObjectState> elements;
    const bool listGiven = parameters.Get ("elements", elements);
    GS::Array<API_Elem_Head> heads;
    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            continue;
        }
        API_Elem_Head head = {};
        head.guid = GetGuidFromObjectState (*elementId);
        heads.Push (head);
    }
    if (listGiven && heads.IsEmpty ()) {
        // A given-but-empty list must not fall back to whatever is selected.
        return CreateFailedExecutionResult (APIERR_BADPARS, "elements was given but holds no item with an elementId; omit the parameter to export the current selection.");
    }

#ifndef ServerMainVers_2700
    if (!heads.IsEmpty ()) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "An element list needs Archicad 27 or later; on 25 and 26 select the elements and omit the list.");
    }
#endif
    const GSErrCode err = ACAPI_ProjectOperation_SaveAsModuleFile (&location, heads.IsEmpty () ? nullptr : &heads);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to save the module file: no elements to save (nothing selected?), the current window is not a model window, or the file cannot be written.");
    }
    return CreateSuccessfulExecutionResult ();
}

GetGeoLocationCommand::GetGeoLocationCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetGeoLocationCommand::GetName () const
{
    return "GetGeoLocation";
}

GS::Optional<GS::UniString> GetGeoLocationCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "projectLocation": {
                "type": "object",
                "properties": {
                    "longitude": {
                        "type": "number",
                        "description": "longitude in degrees"
                    },
                    "latitude": {
                        "type": "number",
                        "description": "latitude in degrees"
                    },
                    "altitude": {
                        "type": "number",
                        "description": "altitude in meters"
                    },
                    "north": {
                        "type": "number",
                        "description": "north direction in radians"
                    }
                },
                "additionalProperties": false,
                "required": [
                    "longitude",
                    "latitude",
                    "altitude",
                    "north"
                ]
            },
            "surveyPoint": {
                "type": "object",
                "properties": {
                    "position": {
                        "type": "object",
                        "properties": {
                            "eastings": {
                                "type": "number",
                                "description": "Location along the easting of the coordinate system of the target map coordinate reference system."
                            },
                            "northings": {
                                "type": "number",
                                "description": "Location along the northing of the coordinate system of the target map coordinate reference system."
                            },
                            "elevation": {
                                "type": "number",
                                "description": "Orthogonal height relative to the vertical datum specified."
                            }
                        },
                        "additionalProperties": false,
                        "required": [
                            "eastings",
                            "northings",
                            "elevation"
                        ]
                    },
                    "geoReferencingParameters": {
                        "type": "object",
                        "properties": {
                            "crsName": {
                                "type": "string",
                                "description": "Name by which the coordinate reference system is identified."
                            },
                            "description": {
                                "type": "string",
                                "description": "Informal description of this coordinate reference system."
                            },
                            "geodeticDatum": {
                                "type": "string",
                                "description": "Name by which this datum is identified."
                            },
                            "verticalDatum": {
                                "type": "string",
                                "description": "Name by which the vertical datum is identified."
                            },
                            "mapProjection": {
                                "type": "string",
                                "description": "Name by which the map projection is identified."
                            },
                            "mapZone": {
                                "type": "string",
                                "description": "Name by which the map zone, relating to the MapProjection, is identified."
                            }
                        },
                        "additionalProperties": false,
                        "required": [
                            "crsName",
                            "description",
                            "geodeticDatum",
                            "verticalDatum",
                            "mapProjection",
                            "mapZone"
                        ]
                    }
                },
                "additionalProperties": false,
                "required": [
                    "position",
                    "geoReferencingParameters"
                ]
            }
        },
        "additionalProperties": false,
        "required": [
            "projectLocation",
            "surveyPoint"
        ]
    })";
}

GS::ObjectState GetGeoLocationCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    API_GeoLocation apiGeoLocation = {};
    ACAPI_GeoLocation_GetGeoLocation (&apiGeoLocation);

    return GS::ObjectState (
        "projectLocation", GS::ObjectState (
            "longitude", apiGeoLocation.placeInfo.longitude,
            "latitude", apiGeoLocation.placeInfo.latitude,
            "altitude", apiGeoLocation.placeInfo.altitude,
            "north", apiGeoLocation.placeInfo.north),
        "surveyPoint", GS::ObjectState (
            "position", GS::ObjectState (
                "eastings", apiGeoLocation.geoReferenceData.eastings,
                "northings", apiGeoLocation.geoReferenceData.northings,
                "elevation", apiGeoLocation.geoReferenceData.orthogonalHeight),
            "geoReferencingParameters", GS::ObjectState (
                "crsName", apiGeoLocation.geoReferenceData.name,
                "description", apiGeoLocation.geoReferenceData.description,
                "geodeticDatum", apiGeoLocation.geoReferenceData.geodeticDatum,
                "verticalDatum", apiGeoLocation.geoReferenceData.verticalDatum,
                "mapProjection", apiGeoLocation.geoReferenceData.mapProjection,
                "mapZone", apiGeoLocation.geoReferenceData.mapZone)));
}

SetGeoLocationCommand::SetGeoLocationCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetGeoLocationCommand::GetName () const
{
    return "SetGeoLocation";
}

GS::Optional<GS::UniString> SetGeoLocationCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "projectLocation": {
                "type": "object",
                "properties": {
                    "longitude": {
                        "type": "number",
                        "description": "longitude in degrees"
                    },
                    "latitude": {
                        "type": "number",
                        "description": "latitude in degrees"
                    },
                    "altitude": {
                        "type": "number",
                        "description": "altitude in meters"
                    },
                    "north": {
                        "type": "number",
                        "description": "north direction in radians"
                    }
                },
                "additionalProperties": false,
                "required": [
                ]
            },
            "surveyPoint": {
                "type": "object",
                "properties": {
                    "position": {
                        "type": "object",
                        "properties": {
                            "eastings": {
                                "type": "number",
                                "description": "Location along the easting of the coordinate system of the target map coordinate reference system."
                            },
                            "northings": {
                                "type": "number",
                                "description": "Location along the northing of the coordinate system of the target map coordinate reference system."
                            },
                            "elevation": {
                                "type": "number",
                                "description": "Orthogonal height relative to the vertical datum specified."
                            }
                        },
                        "additionalProperties": false,
                        "required": [
                        ]
                    },
                    "geoReferencingParameters": {
                        "type": "object",
                        "properties": {
                            "crsName": {
                                "type": "string",
                                "description": "Name by which the coordinate reference system is identified."
                            },
                            "description": {
                                "type": "string",
                                "description": "Informal description of this coordinate reference system."
                            },
                            "geodeticDatum": {
                                "type": "string",
                                "description": "Name by which this datum is identified."
                            },
                            "verticalDatum": {
                                "type": "string",
                                "description": "Name by which the vertical datum is identified."
                            },
                            "mapProjection": {
                                "type": "string",
                                "description": "Name by which the map projection is identified."
                            },
                            "mapZone": {
                                "type": "string",
                                "description": "Name by which the map zone, relating to the MapProjection, is identified."
                            }
                        },
                        "additionalProperties": false,
                        "required": [
                        ]
                    }
                },
                "additionalProperties": false,
                "required": [
                ]
            }
        },
        "additionalProperties": false,
        "required": [
        ]
    })";
}

GS::Optional<GS::UniString> SetGeoLocationCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState SetGeoLocationCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    API_GeoLocation apiGeoLocation = {};
    ACAPI_GeoLocation_GetGeoLocation (&apiGeoLocation);

    bool hasAnyInput = false;

    GS::ObjectState projectLocation;
    if (parameters.Get ("projectLocation", projectLocation)) {
        hasAnyInput |= projectLocation.Get ("longitude", apiGeoLocation.placeInfo.longitude);
        hasAnyInput |= projectLocation.Get ("latitude", apiGeoLocation.placeInfo.latitude);
        hasAnyInput |= projectLocation.Get ("altitude", apiGeoLocation.placeInfo.altitude);
        hasAnyInput |= projectLocation.Get ("north", apiGeoLocation.placeInfo.north);
    }
    GS::ObjectState surveyPoint;
    if (parameters.Get ("surveyPoint", surveyPoint)) {
        GS::ObjectState position;
        if (surveyPoint.Get ("position", position)) {
            hasAnyInput |= position.Get ("eastings", apiGeoLocation.geoReferenceData.eastings);
            hasAnyInput |= position.Get ("northings", apiGeoLocation.geoReferenceData.northings);
            hasAnyInput |= position.Get ("elevation", apiGeoLocation.geoReferenceData.orthogonalHeight);
        }
        GS::ObjectState geoReferencingParameters;
        if (surveyPoint.Get ("geoReferencingParameters", geoReferencingParameters)) {
            hasAnyInput |= geoReferencingParameters.Get ("crsName", apiGeoLocation.geoReferenceData.name);
            hasAnyInput |= geoReferencingParameters.Get ("description", apiGeoLocation.geoReferenceData.description);
            hasAnyInput |= geoReferencingParameters.Get ("geodeticDatum", apiGeoLocation.geoReferenceData.geodeticDatum);
            hasAnyInput |= geoReferencingParameters.Get ("verticalDatum", apiGeoLocation.geoReferenceData.verticalDatum);
            hasAnyInput |= geoReferencingParameters.Get ("mapProjection", apiGeoLocation.geoReferenceData.mapProjection);
            hasAnyInput |= geoReferencingParameters.Get ("mapZone", apiGeoLocation.geoReferenceData.mapZone);
        }
    }

    if (!hasAnyInput) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "No valid input parameters provided to update geolocation.");
    }

	GSErrCode err = ACAPI_CallUndoableCommand ("Change GeoLocation", [&] () -> GSErrCode {
        return ACAPI_GeoLocation_SetGeoLocation (&apiGeoLocation);
    });
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to set geolocation.");
    }

    return CreateSuccessfulExecutionResult ();
}

GetCalculationUnitsCommand::GetCalculationUnitsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetCalculationUnitsCommand::GetName () const
{
    return "GetCalculationUnits";
}

GS::Optional<GS::UniString> GetCalculationUnitsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "length": {
                "type": "object",
                "properties": {
                    "unit": {
                        "$ref": "#/LengthType"
                    },
                    "accuracy": {
                        "$ref": "#/AccuracyType"
                    },
                    "decimals": {
                        "type": "integer",
                        "description": "Number of decimals to display for length values."
                    },
                    "roundInch": {
                        "type": "integer",
                        "description": "Fractional inches."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "unit",
                    "accuracy",
                    "decimals"
                ]
            },
            "area": {
                "type": "object",
                "properties": {
                    "unit": {
                        "$ref": "#/AreaType"
                    },
                    "accuracy": {
                        "$ref": "#/AccuracyType"
                    },
                    "decimals": {
                        "type": "integer",
                        "description": "Number of decimals to display for area values."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "unit",
                    "accuracy",
                    "decimals"
                ]
            },
            "volume": {
                "type": "object",
                "properties": {
                    "unit": {
                        "$ref": "#/VolumeType"
                    },
                    "accuracy": {
                        "$ref": "#/AccuracyType"
                    },
                    "decimals": {
                        "type": "integer",
                        "description": "Number of decimals to display for volume values."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "unit",
                    "accuracy",
                    "decimals"
                ]
            },
            "angle": {
                "type": "object",
                "properties": {
                    "unit": {
                        "$ref": "#/AngleType"
                    },
                    "decimals": {
                        "type": "integer",
                        "description": "Number of decimals to display for angle values."
                    },
                    "accuracy": {
                        "type": "integer",
                        "description": "Accuracy for angle values."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "unit",
                    "decimals",
                    "accuracy"
                ]
            }
        },
        "additionalProperties": false,
        "required": [
            "length",
            "area",
            "volume",
            "angle"
        ]
    })";
}

static GS::UniString ConvertAPILengthType (API_LengthTypeID apiLengthUnit)
{
    switch (apiLengthUnit) {
        case API_LengthTypeID::Meter: return "Meter";
        case API_LengthTypeID::Decimeter: return "Decimeter";
        case API_LengthTypeID::Centimeter: return "Centimeter";
        case API_LengthTypeID::Millimeter: return "Millimeter";
        case API_LengthTypeID::FootFracInch: return "FootFracInch";
        case API_LengthTypeID::FootDecInch: return "FootDecInch";
        case API_LengthTypeID::DecFoot: return "DecFoot";
        case API_LengthTypeID::FracInch: return "FracInch";
        case API_LengthTypeID::DecInch: return "DecInch";
        default: return "Unknown";
    }
}

static GS::UniString ConvertAPIAreaType (API_AreaTypeID apiAreaUnit)
{
    switch (apiAreaUnit) {
        case API_AreaTypeID::SquareMeter: return "SquareMeter";
        case API_AreaTypeID::SquareCentimeter: return "SquareCentimeter";
        case API_AreaTypeID::SquareMillimeter: return "SquareMillimeter";
        case API_AreaTypeID::SquareFoot: return "SquareFoot";
        case API_AreaTypeID::SquareInch: return "SquareInch";
        default: return "Unknown";
    }
}

static GS::UniString ConvertAPIVolumeType (API_VolumeTypeID apiVolumeUnit)
{
    switch (apiVolumeUnit) {
        case API_VolumeTypeID::CubicMeter: return "CubicMeter";
        case API_VolumeTypeID::Liter: return "Liter";
        case API_VolumeTypeID::CubicCentimeter: return "CubicCentimeter";
        case API_VolumeTypeID::CubicMillimeter: return "CubicMillimeter";
        case API_VolumeTypeID::CubicFoot: return "CubicFoot";
        case API_VolumeTypeID::CubicInch: return "CubicInch";
        case API_VolumeTypeID::CubicYard: return "CubicYard";
        case API_VolumeTypeID::Gallon: return "Gallon";
        default: return "Unknown";
    }
}

static GS::UniString ConvertAPIAngleType (API_AngleTypeID apiAngleUnit)
{
    switch (apiAngleUnit) {
        case API_AngleTypeID::DecimalDegree: return "DecimalDegree";
        case API_AngleTypeID::DegreeMinSec: return "DegreeMinSec";
        case API_AngleTypeID::Grad: return "Grad";
        case API_AngleTypeID::Radian: return "Radian";
        case API_AngleTypeID::Surveyors: return "Surveyors";
        default: return "Unknown";
    }
}

static GS::UniString ConvertAPIExtraAccuracyType (API_ExtraAccuracyID apiExtraAccuracy)
{
    switch (apiExtraAccuracy) {
        case API_ExtraAccuracyID::APIExtAc_Off: return "Off";
        case API_ExtraAccuracyID::APIExtAc_Small5: return "ShowSmall5";
        case API_ExtraAccuracyID::APIExtAc_Small25: return "ShowSmall25";
        case API_ExtraAccuracyID::APIExtAc_Small1: return "ShowSmall1";
        case API_ExtraAccuracyID::APIExtAc_Small01: return "ShowSmall01";
        case API_ExtraAccuracyID::APIExtAc_Fractions: return "InchCaseFractions";
        default: return "Unknown";
    }
}

GS::ObjectState GetCalculationUnitsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    API_CalcUnitPrefs unitPrefs;
    ACAPI_ProjectSetting_GetPreferences (&unitPrefs, APIPrefs_CalcUnitsID);

    return GS::ObjectState (
        "length", GS::ObjectState (
            "unit", ConvertAPILengthType (unitPrefs.length.unit),
            "accuracy", ConvertAPIExtraAccuracyType (unitPrefs.length.accuracy),
            "decimals", unitPrefs.length.decimals),
        "area", GS::ObjectState (
            "unit", ConvertAPIAreaType (unitPrefs.area.unit),
            "accuracy", ConvertAPIExtraAccuracyType (unitPrefs.area.accuracy),
            "decimals", unitPrefs.area.decimals),
        "volume", GS::ObjectState (
            "unit", ConvertAPIVolumeType (unitPrefs.volume.unit),
            "accuracy", ConvertAPIExtraAccuracyType (unitPrefs.volume.accuracy),
            "decimals", unitPrefs.volume.decimals),
        "angle", GS::ObjectState (
            "unit", ConvertAPIAngleType (unitPrefs.angle.unit),
            "decimals", unitPrefs.angle.decimals,
            "accuracy", unitPrefs.angle.accuracy));
}

IFCFileOperationCommand::IFCFileOperationCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String IFCFileOperationCommand::GetName () const
{
    return "IFCFileOperation";
}

GS::Optional<GS::UniString> IFCFileOperationCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "method": {
                "type": "string",
                "description": "The file operation method to use.",
                "enum": ["save", "merge", "open"]
            },
            "ifcFilePath": {
                "type": "string",
                "description": "The target IFC file to use."
            },
            "fileType": {
                "type": "string",
                "description": "The type of the IFC file. The default is 'ifc'.",
                "enum": ["ifc", "ifcxml", "ifczip", "ifcxmlzip"]
            }
        },
        "additionalProperties": false,
        "required": [
            "method",
            "ifcFilePath"
        ]
    })";
}

GS::Optional<GS::UniString> IFCFileOperationCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState IFCFileOperationCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString ifcFilePath;
    if (!parameters.Get ("ifcFilePath", ifcFilePath)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "ifcFilePath parameter is missing");
    }

    GS::UniString methodStr;
    if (!parameters.Get ("method", methodStr)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "method parameter is missing");
    }

    API_IOParams ioParams = {};
    ioParams.fileTypeID = APIFType_IfcFile;
    if (methodStr == "open") {
        ioParams.method = IO_OPEN;
    } else if (methodStr == "merge") {
        ioParams.method = IO_MERGE;
    } else if (methodStr == "save") {
        ioParams.method = IO_SAVEAS;
    } else {
        return CreateFailedExecutionResult (APIERR_BADPARS, "method parameter is invalid");
    }

    GS::UniString fileTypeStr;
    if (!parameters.Get ("fileType", fileTypeStr)) {
        ioParams.refCon = 1;
    } else {
        if (fileTypeStr == "ifc") {
            ioParams.refCon = 1;
        } else if (fileTypeStr == "ifcxml") {
            ioParams.refCon = 2;
        } else if (fileTypeStr == "ifczip") {
            ioParams.refCon = 3;
        } else if (fileTypeStr == "ifcxmlzip") {
            ioParams.refCon = 4;
        } else {
            return CreateFailedExecutionResult (APIERR_BADPARS, "fileType parameter is invalid");
        }
    }

    IO::Location ifcFileLocation (ifcFilePath);
    IO::Name lastLocalName;
    if (ifcFileLocation.GetLastLocalName (&lastLocalName) != NoError) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "ifcFilePath parameter is invalid");
    }
    ioParams.fileLoc = &ifcFileLocation;
    ioParams.saveFileIOName = &lastLocalName;
    ioParams.noDialog = true;
    ioParams.fromDragDrop = false;

    API_ModulID moduleID = { 1198731108, 138575850 };
    const GSErrCode err = ACAPI_AddOnAddOnCommunication_Call (&moduleID, 'IFCI', 1, reinterpret_cast<GSHandle>(&ioParams), nullptr, ioParams.noDialog);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to execute the IFC operation");
    }

    return CreateSuccessfulExecutionResult ();
}

PrintViewCommand::PrintViewCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String PrintViewCommand::GetName () const
{
    return "PrintView";
}

GS::Optional<GS::UniString> PrintViewCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "grid": {
                "type": "boolean",
                "description": "Print the grid. The default is false."
            },
            "fixText": {
                "type": "boolean",
                "description": "Use fixed text size. The default is false."
            },
            "scale": {
                "type": "integer",
                "description": "Print scale. The default is 100."
            },
            "printArea": {
                "type": "string",
                "description": "The area to print. The default is 'currentView'.",
                "enum": ["currentView", "entireDrawing", "marquee"]
            }
        },
        "additionalProperties": false,
        "required": [
        ]
    })";
}

GS::Optional<GS::UniString> PrintViewCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState PrintViewCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    API_PrintPars pi = {};

    GS::UniString printAreaStr;
    if (!parameters.Get ("printArea", printAreaStr) || printAreaStr == "currentView") {
        pi.printArea = PrintArea_CurrentView;
    } else if (printAreaStr == "entireDrawing") {
        pi.printArea = PrintArea_EntireDrawing;
    } else if (printAreaStr == "marquee") {
        pi.printArea = PrintArea_Marquee;
    } else {
        return CreateFailedExecutionResult (APIERR_BADPARS, "printArea parameter is invalid");
    }

    if (!parameters.Get ("grid", pi.grid)) {
        pi.grid = false;
    }
    if (!parameters.Get ("fixText", pi.fixText)) {
        pi.fixText = false;
    }
    if (!parameters.Get ("scale", pi.scale)) {
        pi.scale = 100;
    }

    const GSErrCode err = ACAPI_ProjectOperation_Print (&pi);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to print the current view.");
    }

    return CreateSuccessfulExecutionResult ();
}

RebuildViewCommand::RebuildViewCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String RebuildViewCommand::GetName () const
{
    return "RebuildView";
}

GS::Optional<GS::UniString> RebuildViewCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "regenerate": {
                "type": "boolean",
                "description": "Regenerate the view. The default is false, meaning the view will not be regenerated, but rebuilt."
            }
        },
        "additionalProperties": false,
        "required": [
        ]
    })";
}

GS::Optional<GS::UniString> RebuildViewCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState RebuildViewCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    bool regenerate = false;
    parameters.Get ("regenerate", regenerate);

    GSErrCode err = ACAPI_View_Rebuild (&regenerate);

    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to rebuild the view.");
    }

    return CreateSuccessfulExecutionResult ();
}


// ---------------------------------------------------------------------------
// Hotlink nodes and instances.
//
// A hotlink module is two things in Archicad: a NODE (the reference to the
// source file, with its cache) and any number of INSTANCES (elements of type
// API_HotlinkID, each placed by a transformation). GetHotlinks lists the
// nodes; these three commands create nodes, place instances and move them.
// Instances are ordinary elements otherwise: DeleteElements removes one,
// GetDetailsOfElements reads its placement, and the elements inside a
// placed instance report it as their hotlinkId.
//
// MoveElements and RotateElements do NOT work on an instance - the drag edit
// returns NoError and moves nothing - which is why ChangeHotlinkInstances
// exists: an instance moves by changing its transformation.
// ---------------------------------------------------------------------------

static GS::Optional<API_Guid> FindHotlinkNodeBySource (const IO::Location& sourceLocation)
{
    // A node that has been created and not placed yet is "unplaced": the node
    // reads skip it unless asked (AC26 and later; AC25's database calls have no
    // such flag). Without this a second CreateHotlinkNodes for the same file could
    // not see the first, and CreateHotlinkInstances could not read the node it was
    // given (measured on AC28).
    bool enableUnplaced = true;
    API_HotlinkTypeID type = APIHotlink_Module;
    GS::Array<API_Guid> nodes;
    if (ACAPI_Hotlink_GetHotlinkNodes (&type, &nodes, &enableUnplaced) != NoError) {
        return GS::NoValue;
    }
    for (const API_Guid& nodeGuid : nodes) {
        API_HotlinkNode node = {};
        node.guid = nodeGuid;
        if (ACAPI_Hotlink_GetHotlinkNode (&node, &enableUnplaced) == NoError && node.sourceLocation != nullptr) {
            // Case-insensitive: on Windows a differently cased path is the same file.
            if (node.sourceLocation->ToDisplayText ().Compare (sourceLocation.ToDisplayText (), CaseInsensitive) == GS::UniString::Equal) {
                return nodeGuid;
            }
        }
    }
    return GS::NoValue;
}

CreateHotlinkNodesCommand::CreateHotlinkNodesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateHotlinkNodesCommand::GetName () const
{
    return "CreateHotlinkNodes";
}

GS::Optional<GS::UniString> CreateHotlinkNodesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "hotlinkNodes": {
                "type": "array",
                "description": "The hotlink module nodes to create. A node that already points at the same source file (compared case-insensitively) is returned as it is, with existing: true, and the name and story settings asked for are ignored. On Archicad 25 a node that has not been placed yet cannot be found, so a repeated request there creates a second node.",
                "items": {
                    "type": "object",
                    "properties": {
                        "sourceLocation": {
                            "type": "string",
                            "description": "Absolute path of the module source file (.mod or .pln)."
                        },
                        "name": {
                            "type": "string",
                            "description": "Optional display name of the node. Defaults to the file name. Ignored when a node for the same file already exists."
                        },
                        "storyRangeType": {
                            "type": "string",
                            "description": "Optional. Which stories of the source are placed: all of them, or the single reference story. Ignored when a node for the same file already exists.",
                            "enum": ["AllStories", "SingleStory"]
                        },
                        "refFloorIndex": {
                            "type": "integer",
                            "description": "Optional index of the reference story in the source file. Defaults to 0. Ignored when a node for the same file already exists."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "sourceLocation"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "hotlinkNodes"
        ]
    })";
}

GS::Optional<GS::UniString> CreateHotlinkNodesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "hotlinkNodes": {
                "type": "array",
                "description": "One item per requested node, in order: the node guid with its existing flag, or an error.",
                "items": {
                    "$ref": "#/HotlinkNodeCreatedOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "hotlinkNodes"
        ]
    })";
}

GS::ObjectState CreateHotlinkNodesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> hotlinkNodes;
    parameters.Get ("hotlinkNodes", hotlinkNodes);

    GS::ObjectState response;
    const auto& results = response.AddList<GS::ObjectState> ("hotlinkNodes");

    // One undo step for the whole call, as the repo's other creating commands do.
    ACAPI_CallUndoableCommand ("Create Hotlink Nodes", [&] () -> GSErrCode {
        for (const GS::ObjectState& nodeData : hotlinkNodes) {
            GS::UniString sourcePath;
            if (!nodeData.Get ("sourceLocation", sourcePath) || sourcePath.IsEmpty ()) {
                results (CreateErrorResponse (APIERR_BADPARS, "sourceLocation is missing"));
                continue;
            }
            IO::Location sourceLocation (sourcePath);
            IO::Name lastLocalName;
            if (sourceLocation.GetLastLocalName (&lastLocalName) != NoError) {
                results (CreateErrorResponse (APIERR_BADPARS, "sourceLocation is not a valid path"));
                continue;
            }

            const GS::Optional<API_Guid> existing = FindHotlinkNodeBySource (sourceLocation);
            if (existing.HasValue ()) {
                GS::ObjectState item;
                item.Add ("hotlinkNodeId", CreateGuidObjectState (existing.Get ()));
                item.Add ("existing", true);
                results (item);
                continue;
            }

            API_HotlinkNode hotlinkNode = {};
            hotlinkNode.type = APIHotlink_Module;
            hotlinkNode.storyRangeType = APIHotlink_AllStories;
            GS::UniString storyRangeType;
            if (nodeData.Get ("storyRangeType", storyRangeType) && storyRangeType == "SingleStory") {
                hotlinkNode.storyRangeType = APIHotlink_SingleStory;
            }
            Int32 refFloorIndex = 0;
            nodeData.Get ("refFloorIndex", refFloorIndex);
            hotlinkNode.refFloorInd = static_cast<short> (refFloorIndex);
            GS::UniString name;
            if (!nodeData.Get ("name", name) || name.IsEmpty ()) {
                name = lastLocalName.ToString ();
            }
            GS::ucsncpy (hotlinkNode.name, name.ToUStr (), API_UniLongNameLen - 1);
            hotlinkNode.name[API_UniLongNameLen - 1] = 0;
            // API_HotlinkNode's destructor frees sourceLocation itself ("make sure
            // those point to memory on heap" - APIdefs_Database.h, AC25 to AC29),
            // which is also why the node reads elsewhere in this file do not leak.
            // On failure the location is freed here and nulled so the destructor
            // does not free it twice.
            IO::Location* ownedLocation = new IO::Location (sourceLocation);
            hotlinkNode.sourceLocation = ownedLocation;

    #ifdef ServerMainVers_2800
            // Fills the story info from the source so the node is created with
            // the right story settings. Not available before 28; creation works
            // without it.
            ACAPI_Hotlink_GetHotlinkStoryInfo (&hotlinkNode);
    #endif

            const GSErrCode err = ACAPI_Hotlink_CreateHotlinkNode (&hotlinkNode);
            if (err != NoError) {
                delete ownedLocation;
                hotlinkNode.sourceLocation = nullptr;
                results (CreateErrorResponse (err, "Failed to create the hotlink node from " + sourcePath));
                continue;
            }

            GS::ObjectState item;
            item.Add ("hotlinkNodeId", CreateGuidObjectState (hotlinkNode.guid));
            item.Add ("existing", false);
            results (item);
        }
        return NoError;
    });

    return response;
}

CreateHotlinkInstancesCommand::CreateHotlinkInstancesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateHotlinkInstancesCommand::GetName () const
{
    return "CreateHotlinkInstances";
}

GS::Optional<GS::UniString> CreateHotlinkInstancesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "hotlinkInstances": {
                "type": "array",
                "description": "The hotlink instances to place.",
                "items": {
                    "type": "object",
                    "properties": {
                        "hotlinkNodeId": {
                            "$ref": "#/HotlinkNodeId",
                            "description": "The node to place, from GetHotlinks or CreateHotlinkNodes. On Archicad 25 a node that has never been placed cannot be read, so a node created through the API can only be placed from Archicad 26 on."
                        },
                        "origin": {
                            "$ref": "#/HotlinkOrigin"
                        },
                        "rotationAngle": {
                            "type": "number",
                            "description": "Optional rotation about the origin, counter-clockwise, in radians. Defaults to 0."
                        },
                        "mirrored": {
                            "type": "boolean",
                            "description": "Optional. Reflects the module's local X axis before the rotation. Defaults to false."
                        },
                        "floorIndex": {
                            "type": "integer",
                            "description": "Optional story the instance is placed on. Defaults to the current story."
                        },
                        "floorDifference": {
                            "type": "integer",
                            "description": "Optional story offset applied to the module's stories. Defaults to the hotlink tool's current default."
                        },
                        "layerIndex": {
                            "type": "integer",
                            "description": "Optional layer of the instance. Defaults to the hotlink tool's current default layer."
                        },
                        "skipNested": {
                            "type": "boolean",
                            "description": "Optional. Do not place hotlinks nested inside the module. Defaults to the hotlink tool's current default."
                        },
                        "suspendFixAngle": {
                            "type": "boolean",
                            "description": "Optional. Rotate fixed-angle elements with the module. Defaults to the hotlink tool's current default."
                        },
                        "ignoreTopFloorLinks": {
                            "type": "boolean",
                            "description": "Optional. Top-linked elements keep their height rather than their top story link. Defaults to the hotlink tool's current default."
                        },
                        "relinkWallOpenings": {
                            "type": "boolean",
                            "description": "Optional. Defaults to the hotlink tool's current default."
                        },
                        "adjustLevelDiffs": {
                            "type": "boolean",
                            "description": "Optional. Defaults to the hotlink tool's current default."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "hotlinkNodeId",
                        "origin"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "hotlinkInstances"
        ]
    })";
}

GS::Optional<GS::UniString> CreateHotlinkInstancesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::ObjectState CreateHotlinkInstancesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> hotlinkInstances;
    parameters.Get ("hotlinkInstances", hotlinkInstances);

    GS::ObjectState response;
    const auto& elements = response.AddList<GS::ObjectState> ("elements");

    API_StoryInfo storyInfo = {};
    const bool haveStoryInfo = ACAPI_ProjectSetting_GetStorySettings (&storyInfo) == NoError;
    if (haveStoryInfo) {
        BMKillHandle ((GSHandle*) &storyInfo.data);
    }

    ACAPI_CallUndoableCommand ("Create Hotlink Instances", [&] () -> GSErrCode {
        for (const GS::ObjectState& instanceData : hotlinkInstances) {
            const GS::ObjectState* hotlinkNodeId = instanceData.Get ("hotlinkNodeId");
            const GS::ObjectState* origin = instanceData.Get ("origin");
            if (hotlinkNodeId == nullptr || origin == nullptr) {
                elements (CreateErrorResponse (APIERR_BADPARS, "hotlinkNodeId or origin is missing"));
                continue;
            }

            API_Element element = {};
#ifdef ServerMainVers_2600
            element.header.type   = API_HotlinkID;
#else
            element.header.typeID = API_HotlinkID;
#endif
            const GSErrCode defaultsErr = ACAPI_Element_GetDefaults (&element, nullptr);
            if (defaultsErr != NoError) {
                elements (CreateErrorResponse (defaultsErr, "Failed to get the hotlink instance defaults"));
                continue;
            }
            Int32 layerIndex = 0;
            if (instanceData.Get ("layerIndex", layerIndex) && layerIndex > 0) {
                element.header.layer = ACAPI_CreateAttributeIndex (layerIndex);
            }
            Int32 floorIndex;
            if (instanceData.Get ("floorIndex", floorIndex)) {
                element.header.floorInd = static_cast<short> (floorIndex);
            } else if (haveStoryInfo) {
                element.header.floorInd = storyInfo.actStory;
            }

            // The instance takes the node's own type: GetHotlinks hands out XRef
            // node ids as well as module ones, and a module instance of an XRef
            // node is a mismatch.
            API_HotlinkNode node = {};
            node.guid = GetGuidFromObjectState (*hotlinkNodeId);
            bool enableUnplaced = true;     // the node may have been created and not placed yet
            if (ACAPI_Hotlink_GetHotlinkNode (&node, &enableUnplaced) != NoError) {
                elements (CreateErrorResponse (APIERR_BADID, "hotlinkNodeId is not a hotlink node"));
                continue;
            }
            element.hotlink.type = node.type;
            element.hotlink.hotlinkNodeGuid = node.guid;

            double rotationAngle = 0.0;
            instanceData.Get ("rotationAngle", rotationAngle);
            bool mirrored = false;
            instanceData.Get ("mirrored", mirrored);
            element.hotlink.transformation = CreateHotlinkTransformation (Get3DCoordinateFromObjectState (*origin), rotationAngle, mirrored);

            // The placement options keep the tool defaults unless the caller sets them.
            Int32 floorDifference;
            if (instanceData.Get ("floorDifference", floorDifference)) {
                element.hotlink.floorDifference = static_cast<short> (floorDifference);
            }
            instanceData.Get ("skipNested", element.hotlink.skipNested);
            instanceData.Get ("suspendFixAngle", element.hotlink.suspendFixAngle);
            instanceData.Get ("ignoreTopFloorLinks", element.hotlink.ignoreTopFloorLinks);
            instanceData.Get ("relinkWallOpenings", element.hotlink.relinkWallOpenings);
            instanceData.Get ("adjustLevelDiffs", element.hotlink.adjustLevelDiffs);

            const GSErrCode err = ACAPI_Element_Create (&element, nullptr);
            if (err != NoError) {
                elements (CreateErrorResponse (err, "Failed to place the hotlink instance"));
                continue;
            }
            elements (CreateElementIdObjectState (element.header.guid));
        }
        return NoError;
    });

    return response;
}

ChangeHotlinkInstancesCommand::ChangeHotlinkInstancesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ChangeHotlinkInstancesCommand::GetName () const
{
    return "ChangeHotlinkInstances";
}

GS::Optional<GS::UniString> ChangeHotlinkInstancesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "hotlinkInstances": {
                "type": "array",
                "description": "The placed hotlink instances to change. Every field but elementId is optional; a field that is omitted keeps its current value.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "origin": {
                            "$ref": "#/HotlinkOrigin"
                        },
                        "rotationAngle": {
                            "type": "number",
                            "description": "Rotation about the origin, counter-clockwise, in radians."
                        },
                        "mirrored": {
                            "type": "boolean",
                            "description": "Reflect the module's local X axis before the rotation."
                        },
                        "floorDifference": {
                            "type": "integer"
                        },
                        "skipNested": {
                            "type": "boolean"
                        },
                        "suspendFixAngle": {
                            "type": "boolean"
                        },
                        "ignoreTopFloorLinks": {
                            "type": "boolean"
                        },
                        "relinkWallOpenings": {
                            "type": "boolean"
                        },
                        "adjustLevelDiffs": {
                            "type": "boolean"
                        },
                        "layerIndex": {
                            "type": "integer",
                            "description": "Move the instance to this layer."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "hotlinkInstances"
        ]
    })";
}

GS::Optional<GS::UniString> ChangeHotlinkInstancesCommand::GetRawResponseSchema () const
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

GS::ObjectState ChangeHotlinkInstancesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> hotlinkInstances;
    parameters.Get ("hotlinkInstances", hotlinkInstances);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Change Hotlink Instances", [&] () -> GSErrCode {
        for (const GS::ObjectState& instanceData : hotlinkInstances) {
            const GS::ObjectState* elementId = instanceData.Get ("elementId");
            if (elementId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
                continue;
            }

            API_Element element = {};
            element.header.guid = GetGuidFromObjectState (*elementId);
            GSErrCode err = ACAPI_Element_Get (&element);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to get the element"));
                continue;
            }
            if (GetElemTypeId (element.header) != API_HotlinkID) {
                executionResults (CreateFailedExecutionResult (APIERR_BADELEMENTTYPE, "The element is not a hotlink instance"));
                continue;
            }

            API_Element mask;
            ACAPI_ELEMENT_MASK_CLEAR (mask);

            const GS::ObjectState* newOrigin = instanceData.Get ("origin");
            const bool hasRotation = instanceData.Contains ("rotationAngle");
            const bool hasMirrored = instanceData.Contains ("mirrored");
            if (newOrigin != nullptr && !hasRotation && !hasMirrored) {
                // A move keeps the matrix as it is - scale, skew and all - and
                // replaces only the translation.
                const API_Coord3D given = Get3DCoordinateFromObjectState (*newOrigin);
                element.hotlink.transformation.tmx[3] = given.x;
                element.hotlink.transformation.tmx[7] = given.y;
                if (newOrigin->Contains ("z")) {
                    element.hotlink.transformation.tmx[11] = given.z;
                }
                ACAPI_ELEMENT_MASK_SET (mask, API_HotlinkType, transformation);
            } else if (newOrigin != nullptr || hasRotation || hasMirrored) {
                // A rotation or a mirror rebuilds the planar matrix from its
                // decomposition; a scaled or non-planar placement loses that
                // part, which is what asking for a new angle means.
                API_Coord3D origin;
                double rotationAngle;
                bool mirrored;
                DecomposeHotlinkTransformation (element.hotlink.transformation, origin, rotationAngle, mirrored);
                if (newOrigin != nullptr) {
                    const API_Coord3D given = Get3DCoordinateFromObjectState (*newOrigin);
                    origin.x = given.x;
                    origin.y = given.y;
                    if (newOrigin->Contains ("z")) {
                        origin.z = given.z;
                    }
                }
                instanceData.Get ("rotationAngle", rotationAngle);
                instanceData.Get ("mirrored", mirrored);
                element.hotlink.transformation = CreateHotlinkTransformation (origin, rotationAngle, mirrored);
                ACAPI_ELEMENT_MASK_SET (mask, API_HotlinkType, transformation);
            }

            Int32 floorDifference;
            if (instanceData.Get ("floorDifference", floorDifference)) {
                element.hotlink.floorDifference = static_cast<short> (floorDifference);
                ACAPI_ELEMENT_MASK_SET (mask, API_HotlinkType, floorDifference);
            }
            if (instanceData.Get ("skipNested", element.hotlink.skipNested)) {
                ACAPI_ELEMENT_MASK_SET (mask, API_HotlinkType, skipNested);
            }
            if (instanceData.Get ("suspendFixAngle", element.hotlink.suspendFixAngle)) {
                ACAPI_ELEMENT_MASK_SET (mask, API_HotlinkType, suspendFixAngle);
            }
            if (instanceData.Get ("ignoreTopFloorLinks", element.hotlink.ignoreTopFloorLinks)) {
                ACAPI_ELEMENT_MASK_SET (mask, API_HotlinkType, ignoreTopFloorLinks);
            }
            if (instanceData.Get ("relinkWallOpenings", element.hotlink.relinkWallOpenings)) {
                ACAPI_ELEMENT_MASK_SET (mask, API_HotlinkType, relinkWallOpenings);
            }
            if (instanceData.Get ("adjustLevelDiffs", element.hotlink.adjustLevelDiffs)) {
                ACAPI_ELEMENT_MASK_SET (mask, API_HotlinkType, adjustLevelDiffs);
            }
            Int32 layerIndex = 0;
            if (instanceData.Get ("layerIndex", layerIndex) && layerIndex > 0) {
                element.header.layer = ACAPI_CreateAttributeIndex (layerIndex);
                ACAPI_ELEMENT_MASK_SET (mask, API_Elem_Head, layer);
            }

            err = ACAPI_Element_Change (&element, &mask, nullptr, 0, true);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to change the hotlink instance"));
                continue;
            }
            executionResults (CreateSuccessfulExecutionResult ());
        }
        return NoError;
    });

    return response;
}
