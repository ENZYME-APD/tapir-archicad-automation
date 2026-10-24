#include "ProjectCommands.hpp"
#include "MigrationHelper.hpp"
#include "Location.hpp"

#ifdef ServerMainVers_2800
#include "ACAPI/IFCObjectAccessor.hpp"
#include "ACAPI/IFCObjectID.hpp"
#endif

#include <cmath>
#include <initializer_list>

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

GetAutoTextKeysCommand::GetAutoTextKeysCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAutoTextKeysCommand::GetName () const
{
    return "GetAutoTextKeys";
}

GS::Optional<GS::UniString> GetAutoTextKeysCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementId": {
                "$ref": "#/ElementId",
                "description": "Optional. The element to retrieve context dependent autotext keys for (its own properties, plus the ones common to all element types, e.g. 'Element ID', 'Area'). When omitted, only the autotext keys common to all element types are returned."
            }
        },
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> GetAutoTextKeysCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "autoTextKeys": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "The autotext's name, as shown in the Insert Autotext dialog of Archicad."
                        },
                        "key": {
                            "type": "string",
                            "description": "The autotext's key. To embed it in the content of a Text or Label element, surround it with '<' and '>', e.g. '<PROPERTY-69A58F6F-DD3B-478D-B5EF-09A16BD0C548>'."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "key"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "autoTextKeys"
        ]
    })";
}

GS::ObjectState GetAutoTextKeysCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    // guid can legitimately be APINULLGuid here (no elementId given) - per the DevKit doc this
    // still returns the autotext keys common to all element types (e.g. "Element ID", "Area"),
    // just not the ones specific to a single element's own type (e.g. "Thickness of the wall").
    const GS::ObjectState* elementIdOS = parameters.Get ("elementId");
    API_Guid guid = (elementIdOS != nullptr) ? GetGuidFromObjectState (*elementIdOS) : APINULLGuid;
    if (elementIdOS != nullptr && guid == APINULLGuid) {
        // A given but unparseable guid must not fall back to the generic list.
        return CreateErrorResponse (APIERR_BADPARS, "Invalid elementId.");
    }

    // Before AC27 the call is the APIAny_GetPropertyAutoTextKeyTableID goodie, wrapped in
    // MigrationHelper.hpp like the other ACAPI_AutoText_* functions.
    GS::HashTable<GS::UniString, GS::UniString> keyTable;
    GSErrCode err = ACAPI_AutoText_GetPropertyAutoTextKeyTable (&guid, &keyTable);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to retrieve the autotext keys.");
    }

    GS::ObjectState response;
    const auto& listAdder = response.AddList<GS::ObjectState> ("autoTextKeys");

    // GS::HashTable's pair iterator hands out references from AC28 and pointers before
    // (GSRoot/HashTable.hpp: ConstCurrentPair::key is `const Key&` vs `const Key*`).
    for (const auto& keyPair : keyTable) {
        GS::ObjectState keyData;
#ifdef ServerMainVers_2800
        keyData.Add ("name", keyPair.key);
        keyData.Add ("key", keyPair.value);
#else
        keyData.Add ("name", *keyPair.key);
        keyData.Add ("key", *keyPair.value);
#endif
        listAdder (keyData);
    }

    return response;
}

GetAutoTextNameCommand::GetAutoTextNameCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAutoTextNameCommand::GetName () const
{
    return "GetAutoTextName";
}

GS::Optional<GS::UniString> GetAutoTextNameCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "keys": {
                "type": "array",
                "description": "Autotext keys as returned by GetAutoTextKeys or GetProjectInfoFields (without the surrounding '<' and '>'), e.g. 'PROPERTY-69A58F6F-DD3B-478D-B5EF-09A16BD0C548' or 'PROJECTNAME'.",
                "items": {
                    "$ref": "#/AutoTextKeyValue"
                },
                "minItems": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "keys"
        ]
    })";
}

GS::Optional<GS::UniString> GetAutoTextNameCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "autoTextNames": {
                "$ref": "#/AutoTextNamesOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "autoTextNames"
        ]
    })";
}

// Resolves a single key against an already-fetched generic autotext list (avoids
// re-enumerating the project info fields once per key when called from a batch).
static GS::ObjectState ResolveAutoTextName (const GS::UniString& key, GSErrCode genericAutoTextsErr, const GS::Array<GS::ArrayFB<GS::UniString, 3>>& genericAutoTexts)
{
    if (key.IsEmpty ()) {
        return CreateErrorResponse (APIERR_BADPARS, "Empty autotext key.");
    }

    static const GS::UniString propertyPrefix ("PROPERTY-");
    if (key.BeginsWith (propertyPrefix)) {
        // Property-based (context dependent) autotext key: everything after the prefix is the
        // guid of a real property definition. A single direct lookup by guid - no need to
        // enumerate every property definition in the project just to resolve one name.
        const GS::UniString guidPart (key.ToCStr () + propertyPrefix.GetLength ());

        API_PropertyDefinition definition = {};
        definition.guid = APIGuidFromString (guidPart.ToCStr ());
        GSErrCode err = ACAPI_Property_GetPropertyDefinition (definition);
        if (err != NoError) {
            return CreateErrorResponse (err, "Failed to find a property definition for this autotext key.");
        }
        return GS::ObjectState ("name", definition.name);
    }

    // Generic (project-level) autotext key: the SDK has no single-key metadata lookup for
    // these (ACAPI_AutoText_InterpretAutoText resolves the *value*, not the name), so the
    // small, fixed list of project info fields is matched against the list fetched once
    // upfront by the caller, instead of being re-fetched for every key in the batch. A failed
    // fetch is reported as such, not as an unknown key.
    if (genericAutoTextsErr != NoError) {
        return CreateErrorResponse (genericAutoTextsErr, "Failed to retrieve the autotexts.");
    }
    for (const auto& autoText : genericAutoTexts) {
        if (autoText[1] == key) {
            return GS::ObjectState ("name", autoText[0]);
        }
    }

    return CreateErrorResponse (APIERR_BADID, "No autotext found for this key.");
}

GS::ObjectState GetAutoTextNameCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::UniString> keys;
    if (!parameters.Get ("keys", keys) || keys.IsEmpty ()) {
        return CreateErrorResponse (APIERR_BADPARS, "Missing 'keys' parameter.");
    }

    GS::Array<GS::ArrayFB<GS::UniString, 3>> genericAutoTexts;
    const GSErrCode genericAutoTextsErr = ACAPI_AutoText_GetAutoTexts (&genericAutoTexts, APIAutoText_All);

    GS::ObjectState response;
    const auto& listAdder = response.AddList<GS::ObjectState> ("autoTextNames");
    for (const GS::UniString& key : keys) {
        listAdder (ResolveAutoTextName (key, genericAutoTextsErr, genericAutoTexts));
    }

    return response;
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
                    "$ref": "#/ProjectInfoFieldData"
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
    // The array holds one record more than there are stories: the virtual story above
    // the top one, whose level is where the top story ends (API_StoryInfo says it is
    // there so that the height of the top story can be calculated). The record count
    // comes from the handle itself, so a level is never read past the array.
    const short recordCount = (short) (BMGetHandleSize ((GSHandle) storyInfo.data) / sizeof (API_StoryType));
#ifdef ServerMainVers_2800
    auto ifcAccessor = IFCAPI::GetObjectAccessor ();
#endif
    for (short i = 0; i < storyCount; i++) {
        const API_StoryType& story = (*storyInfo.data)[i];
        GS::ObjectState storyData;
        GS::UniString uName = story.uName;

        storyData.Add ("index", story.index);
        storyData.Add ("floorId", story.floorId);
        storyData.Add ("dispOnSections", story.dispOnSections);
        storyData.Add ("level", story.level);
        if (i + 1 < recordCount) {
            storyData.Add ("height", (*storyInfo.data)[i + 1].level - story.level);
        }
        storyData.Add ("name", uName);

#ifdef ServerMainVers_2800
        // The IFC GlobalId of the IfcBuildingStorey the story is exported as. The IFC
        // object accessor only exists from Archicad 28; older versions leave the field out.
        auto storyObjectID = ifcAccessor.CreateIfcBuildingStoreyObjectID (story.index);
        if (storyObjectID.IsOk ()) {
            auto unWrappedStoryObjectID = storyObjectID.Unwrap ();
            auto ifcId = ifcAccessor.GetGlobalId (unWrappedStoryObjectID);
            if (ifcId.IsOk ()) {
                storyData.Add ("ifcId", ifcId.Unwrap ());
            }
        }
#endif

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
    GS::ucscpy (storyCmd.uName, name.ToUStr (0, GS::Min (name.GetLength (), (USize) GS::ArraySize (storyCmd.uName) - 1)).Get ());
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
            GS::ucscpy (storyCmd.uName, name.ToUStr (0, GS::Min (name.GetLength (), (USize) GS::ArraySize (storyCmd.uName) - 1)).Get ());
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
    // Measured on a live AC28 (#747): APIStory_SetHeight moves the story above the
    // boundary even when that one is the active story, so a story below the active one
    // cannot be moved through the height of the boundary between them - the active story
    // moves instead. The two agree on every boundary above the active story, so the active
    // story is moved down to the lowest story whose level changes for the duration of the
    // level pass; then no boundary below the anchor has to change at all. The previously
    // active story is restored afterwards.
    const short originalActStory = storyInfo.actStory;
    bool actStoryMoved = false;
    const GS::OnExit restoreActStory ([&] () {
        if (actStoryMoved) {
            API_StoryCmdType storyCmd = {};
            storyCmd.action = APIStory_GoTo;
            storyCmd.index  = originalActStory;
            ACAPI_ProjectSetting_ChangeStorySettings (&storyCmd);
        }
    });

    bool hasChangingStory = false;
    GS::UIndex lowestChangingStory = 0;
    for (GS::UIndex i = 0; i < storyCount; ++i) {
        if (std::abs (currentStories[i].level - targetLevels[i]) >= StoryLevelTolerance) {
            hasChangingStory = true;
            lowestChangingStory = i;
            break;
        }
    }

    const bool actStoryIsValid = storyInfo.actStory >= storyInfo.firstStory && storyInfo.actStory <= storyInfo.lastStory;

    if (hasChangingStory && actStoryIsValid && storyInfo.actStory > currentStories[lowestChangingStory].index) {
        API_StoryCmdType storyCmd = {};
        storyCmd.action = APIStory_GoTo;
        storyCmd.index  = currentStories[lowestChangingStory].index;

        // Where the active story cannot be switched (#748) this falls back to anchoring on
        // the story which is active, and the final check reports the levels it could not
        // set, exactly as before.
        if (ACAPI_ProjectSetting_ChangeStorySettings (&storyCmd) == NoError) {
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
            actStoryMoved = storyInfo.actStory != originalActStory;
        }
    }

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
    // message says what actually happened, and the story the ladder was anchored on.
    for (GS::UIndex i = 0; i < storyCount; ++i) {
        if (std::abs (currentStories[i].level - targetLevels[i]) >= StoryLevelTolerance) {
            const GS::UniString message = GS::UniString::Printf (
                "Failed to set the level of story %d: requested %.4f, got %.4f (active story: %d).",
                static_cast<int> (currentStories[i].index), targetLevels[i], currentStories[i].level,
                static_cast<int> (storyInfo.actStory));
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

GS::Optional<GS::UniString> SaveProjectCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "projectPath": {
                "type": "string",
                "description": "Optional. Full path of a .pln file to save the project to, as File > Save As does: the open project then refers to that file. This saves an untitled project, or one opened from an older Archicad version, which cannot be saved in place. An existing file at the path is overwritten. When omitted, or when it is the project's own file, the project is saved in place, which keeps the refusal of a project opened read-only."
            }
        },
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> SaveProjectCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

// Whether the location is the file of the open project (a solo project saved before). The paths
// are compared as text without case, as for the hotlink sources below, so a path that differs from
// the project's own only in case still counts as its own file.
static bool IsFileOfOpenProject (const IO::Location& location)
{
    API_ProjectInfo projectInfo = {};
    if (ACAPI_ProjectOperation_Project (&projectInfo) != NoError) {
        return false;
    }
    return !projectInfo.untitled && projectInfo.location != nullptr &&
        projectInfo.location->ToDisplayText ().Compare (location.ToDisplayText (), CaseInsensitive) == GS::UniString::Equal;
}

GS::ObjectState SaveProjectCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString projectPath;
    if (parameters.Get ("projectPath", projectPath) && !projectPath.IsEmpty ()) {
        IO::Location fileLoc (projectPath);
        // The project's own file takes the plain save below: a Save As to its own location would
        // also overwrite a project that Archicad opened read-only, which the plain save refuses.
        if (!IsFileOfOpenProject (fileLoc)) {
            API_FileSavePars fsp = {};
            fsp.fileTypeID = APIFType_PlanFile;
            fsp.file = &fileLoc;
            GSErrCode err = ACAPI_ProjectOperation_Save (&fsp);
            if (err != NoError) {
                return CreateFailedExecutionResult (err, "Failed to save the project to the given path.");
            }
            return CreateSuccessfulExecutionResult ();
        }
    }

    // The parameterless save acts on the current window - "saves the content of
    // the current window", ACAPI_Automate.h - so from a 3D, section or other
    // non-plan window it tries to save that window's content, which has no file
    // of its own, and answers APIERR_READONLY (#681). So the Floor Plan window
    // is activated around the save and the previous window restored afterwards.
    //
    // Two shortcuts were measured on Archicad 29 and rejected. Switching only
    // the current database leaves the save failing exactly as before. Saving
    // as a plan file to the project's own location does work from the 3D
    // window, but it also overwrites a project that Archicad opened read-only
    // (a stale lock, another user editing it), where File > Save refuses -
    // and API_ProjectInfo has no read-only flag to check first. The plain save
    // keeps every one of those refusals, so it is the one to use.
    API_WindowInfo previousWindow = {};
    bool windowSwitched = false;
    if (ACAPI_Window_GetCurrentWindow (&previousWindow) == NoError &&
        previousWindow.typeID != APIWind_FloorPlanID) {
        API_WindowInfo floorPlanWindow = {};
        floorPlanWindow.typeID = APIWind_FloorPlanID;
        windowSwitched = ACAPI_Window_ChangeWindow (&floorPlanWindow) == NoError;
    }
    const GS::OnExit restoreWindow ([&] () {
        if (windowSwitched) {
            ACAPI_Window_ChangeWindow (&previousWindow);
        }
    });

    const GSErrCode err = ACAPI_ProjectOperation_Save ();
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to save the project.");
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

SaveProjectAsArchiveCommand::SaveProjectAsArchiveCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SaveProjectAsArchiveCommand::GetName () const
{
    return "SaveProjectAsArchive";
}

GS::Optional<GS::UniString> SaveProjectAsArchiveCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "archiveFilePath": {
                "type": "string",
                "description": "Absolute path of the .pla archive to write. An existing file is overwritten. The archive becomes the open project, as Save As does."
            },
            "includeLibraryParts": {
                "type": "boolean",
                "description": "Optional, true by default. Whether the library parts the project uses go into the archive, which makes the archive usable as a linked library of another project."
            },
            "includeProperties": {
                "type": "boolean",
                "description": "Optional, true by default. Whether the properties go into the archive."
            },
            "includeTextures": {
                "type": "boolean",
                "description": "Optional, false by default. Whether the linked textures go into the archive."
            },
            "includeBackgroundPicture": {
                "type": "boolean",
                "description": "Optional, false by default. Whether the background picture goes into the archive."
            }
        },
        "additionalProperties": false,
        "required": [
            "archiveFilePath"
        ]
    })";
}

GS::Optional<GS::UniString> SaveProjectAsArchiveCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState SaveProjectAsArchiveCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString archiveFilePath;
    if (!parameters.Get ("archiveFilePath", archiveFilePath) || archiveFilePath.IsEmpty ()) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "archiveFilePath is missing.");
    }
    IO::Location location (archiveFilePath);

    bool includeLibraryParts = true;
    bool includeProperties = true;
    bool includeTextures = false;
    bool includeBackgroundPicture = false;
    parameters.Get ("includeLibraryParts", includeLibraryParts);
    parameters.Get ("includeProperties", includeProperties);
    parameters.Get ("includeTextures", includeTextures);
    parameters.Get ("includeBackgroundPicture", includeBackgroundPicture);

    API_FileSavePars fileSavePars = {};
    fileSavePars.fileTypeID = APIFType_A_PlanFile;
    fileSavePars.file = &location;

    API_SavePars_Archive savePars = {};
    savePars.picturesInTIFF = false;
    savePars.texturesOn = includeTextures;
    savePars.backgroundPictOn = includeBackgroundPicture;
    savePars.propertiesOn = includeProperties;
    savePars.libraryPartsOn = includeLibraryParts;

    const GSErrCode err = ACAPI_ProjectOperation_Save (&fileSavePars, &savePars);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to save the project as an archive: no project is open, or the file cannot be written.");
    }
    return CreateSuccessfulExecutionResult ();
}

SaveCurrentWindowAsImageCommand::SaveCurrentWindowAsImageCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SaveCurrentWindowAsImageCommand::GetName () const
{
    return "SaveCurrentWindowAsImage";
}

GS::Optional<GS::UniString> SaveCurrentWindowAsImageCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "imageFilePath": {
                "type": "string",
                "description": "Absolute path of the image file to write. An existing file is overwritten."
            },
            "format": {
                "type": "string",
                "description": "Optional. The image file format; omitted, it comes from the extension of imageFilePath (.png, .jpg or .jpeg, .bmp, .tif or .tiff, .gif).",
                "enum": ["png", "jpg", "bmp", "tiff", "gif"]
            },
            "colorDepth": {
                "type": "string",
                "description": "Optional, trueColor by default. The color depth of the image, as in the Save as picture dialog.",
                "enum": ["blackAndWhite", "16Colors", "256Colors", "16Grays", "256Grays", "trueColor", "trueColorWithAlpha"]
            },
            "dithered": {
                "type": "boolean",
                "description": "Optional, false by default. Whether the image is dithered when its color depth drops colors."
            },
            "entireDrawing": {
                "type": "boolean",
                "description": "Optional, false by default. Whether the image covers the entire drawing of a 2D window instead of its current zoom, as the Entire Drawing option of the Save as picture dialog does."
            }
        },
        "additionalProperties": false,
        "required": [
            "imageFilePath"
        ]
    })";
}

GS::Optional<GS::UniString> SaveCurrentWindowAsImageCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "imageFilePath": {
                "type": "string",
                "description": "The path of the written image file."
            },
            "zoom": {
                "$ref": "#/Zoom",
                "description": "The zoom of the window the image shows, as GetView2DTransformations reports it. Absent for the 3D window."
            },
            "rotation": {
                "type": "number",
                "description": "The orientation of the window in radian, as GetView2DTransformations reports it. Absent for the 3D window."
            }
        },
        "additionalProperties": false,
        "required": [
            "imageFilePath"
        ]
    })";
}

static bool ParseImageFileType (const GS::UniString& format, API_FTypeID& fileTypeID)
{
    if (format == "png") {
        fileTypeID = APIFType_PNGFile;
    } else if (format == "jpg") {
        fileTypeID = APIFType_JPEGFile;
    } else if (format == "bmp") {
        fileTypeID = APIFType_BMPFile;
    } else if (format == "tiff") {
        fileTypeID = APIFType_TIFFFile;
    } else if (format == "gif") {
        fileTypeID = APIFType_GIFFile;
    } else {
        return false;
    }
    return true;
}

static bool ParseImageFileTypeFromExtension (const GS::UniString& imageFilePath, API_FTypeID& fileTypeID)
{
    const UIndex dotIndex = imageFilePath.FindLast (GS::UniChar ('.'));
    if (dotIndex == MaxUIndex) {
        return false;
    }
    GS::UniString extension (imageFilePath.GetSubstring (dotIndex + 1, imageFilePath.GetLength () - dotIndex - 1));
    extension.SetToLowerCase ();
    if (extension == "jpeg") {
        extension = "jpg";
    } else if (extension == "tif") {
        extension = "tiff";
    }
    return ParseImageFileType (extension, fileTypeID);
}

// The true color depths are platform-specific enumerators of API_ColorDepthID.
#if defined (macintosh)
static const API_ColorDepthID TrueColorDepth = APIColorDepth_MiC;            // Millions of Colors
static const API_ColorDepthID TrueColorWithAlphaDepth = APIColorDepth_MiCP;  // Millions of Colors+
#else
static const API_ColorDepthID TrueColorDepth = APIColorDepth_TC24;           // True Color - 24 bit
static const API_ColorDepthID TrueColorWithAlphaDepth = APIColorDepth_TC32;  // True Color - 32 bit
#endif

static bool ParseColorDepth (const GS::UniString& colorDepth, API_ColorDepthID& colorDepthID)
{
    if (colorDepth == "blackAndWhite") {
        colorDepthID = APIColorDepth_BW;
    } else if (colorDepth == "16Colors") {
        colorDepthID = APIColorDepth_16C;
    } else if (colorDepth == "256Colors") {
        colorDepthID = APIColorDepth_256C;
    } else if (colorDepth == "16Grays") {
        colorDepthID = APIColorDepth_16G;
    } else if (colorDepth == "256Grays") {
        colorDepthID = APIColorDepth_256G;
    } else if (colorDepth == "trueColor") {
        colorDepthID = TrueColorDepth;
    } else if (colorDepth == "trueColorWithAlpha") {
        colorDepthID = TrueColorWithAlphaDepth;
    } else {
        return false;
    }
    return true;
}

GS::ObjectState SaveCurrentWindowAsImageCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString imageFilePath;
    if (!parameters.Get ("imageFilePath", imageFilePath) || imageFilePath.IsEmpty ()) {
        return CreateErrorResponse (APIERR_BADPARS, "imageFilePath is missing.");
    }
    IO::Location location (imageFilePath);

    API_FTypeID fileTypeID = APIFType_PNGFile;
    GS::UniString format;
    if (parameters.Get ("format", format)) {
        if (!ParseImageFileType (format, fileTypeID)) {
            return CreateErrorResponse (APIERR_BADPARS, "format must be one of png, jpg, bmp, tiff and gif.");
        }
    } else if (!ParseImageFileTypeFromExtension (imageFilePath, fileTypeID)) {
        return CreateErrorResponse (APIERR_BADPARS, "format is missing and the extension of imageFilePath is not one of .png, .jpg, .jpeg, .bmp, .tif, .tiff and .gif.");
    }

    API_ColorDepthID colorDepthID = TrueColorDepth;
    GS::UniString colorDepth;
    if (parameters.Get ("colorDepth", colorDepth) && !ParseColorDepth (colorDepth, colorDepthID)) {
        return CreateErrorResponse (APIERR_BADPARS, "colorDepth is not one of the allowed values.");
    }

    bool dithered = false;
    bool entireDrawing = false;
    parameters.Get ("dithered", dithered);
    parameters.Get ("entireDrawing", entireDrawing);

    // The picture save renders the front window: the 2D content of a model or
    // layout window, or the 3D window. view2D tells the two apart.
    API_WindowInfo currentWindow = {};
    const GSErrCode windowErr = ACAPI_Window_GetCurrentWindow (&currentWindow);
    if (windowErr != NoError) {
        return CreateErrorResponse (windowErr, "Failed to get the current window.");
    }
    const bool view2D = currentWindow.typeID != APIWind_3DModelID;

    API_FileSavePars fileSavePars = {};
    fileSavePars.fileTypeID = fileTypeID;
    fileSavePars.file = &location;

    API_SavePars_Picture savePars = {};
    savePars.colorDepth = colorDepthID;
    savePars.dithered = dithered;
    savePars.view2D = view2D;
    // crop means "crop to the current zoom"; the Entire Drawing option is its opposite.
    savePars.crop = !entireDrawing;

    const GSErrCode err = ACAPI_ProjectOperation_Save (&fileSavePars, &savePars);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to save the current window as an image: the current window cannot be saved as a picture (only model, layout and 3D windows can), or the file cannot be written.");
    }

    GS::ObjectState response ("imageFilePath", imageFilePath);
    if (view2D) {
        // The same data GetView2DTransformations reports for the current database,
        // so a caller can map the pixels of the image back to model coordinates.
        API_Box zoomBox = {};
        API_Tranmat tranmat = {};
        if (ACAPI_View_GetZoom (&zoomBox, &tranmat) == NoError) {
            response.Add ("zoom", GS::ObjectState (
                "xMin", zoomBox.xMin,
                "yMin", zoomBox.yMin,
                "xMax", zoomBox.xMax,
                "yMax", zoomBox.yMax));
            response.Add ("rotation", std::atan2 (tranmat.tmx[4], tranmat.tmx[0]));
        }
    }
    return response;
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
                "$ref": "#/ProjectLocation"
            },
            "surveyPoint": {
                "$ref": "#/SurveyPoint"
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
            "positionInProject", Create3DCoordinateObjectState (apiGeoLocation.surveyPointPosition),
            "geoReferencingParameters", GS::ObjectState (
                "crsName", apiGeoLocation.geoReferenceData.name,
                "description", apiGeoLocation.geoReferenceData.description,
                "geodeticDatum", apiGeoLocation.geoReferenceData.geodeticDatum,
                "verticalDatum", apiGeoLocation.geoReferenceData.verticalDatum,
                "mapProjection", apiGeoLocation.geoReferenceData.mapProjection,
                "mapZone", apiGeoLocation.geoReferenceData.mapZone,
                "xAxisAbscissa", apiGeoLocation.geoReferenceData.xAxisAbscissa,
                "xAxisOrdinate", apiGeoLocation.geoReferenceData.xAxisOrdinate,
                "scale", apiGeoLocation.geoReferenceData.scale)));
}

GetSurveyPointTransformationCommand::GetSurveyPointTransformationCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetSurveyPointTransformationCommand::GetName () const
{
    return "GetSurveyPointTransformation";
}

GS::Optional<GS::UniString> GetSurveyPointTransformationCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "projectToSurvey": {
                "type": "array",
                "description": "The 3x4 matrix that transforms a point given in the Project Origin coordinate system into the Survey Point coordinate system, row by row (12 numbers): x' = m[0]*x + m[1]*y + m[2]*z + m[3], y' = m[4]*x + m[5]*y + m[6]*z + m[7], z' = m[8]*x + m[9]*y + m[10]*z + m[11].",
                "items": {
                    "type": "number"
                },
                "minItems": 12,
                "maxItems": 12
            },
            "surveyToProject": {
                "type": "array",
                "description": "The inverse of projectToSurvey in the same row by row layout: it transforms a point given in the Survey Point coordinate system into the Project Origin coordinate system. Missing when projectToSurvey is not invertible.",
                "items": {
                    "type": "number"
                },
                "minItems": 12,
                "maxItems": 12
            }
        },
        "additionalProperties": false,
        "required": [
            "projectToSurvey"
        ]
    })";
}

#ifdef ServerMainVers_2700
// Inverts the affine transformation of a 3x4 API_Tranmat (rotation/scale part plus translation).
static bool InvertTranmat (const API_Tranmat& tranmat, API_Tranmat& inverse)
{
    const double* m = tranmat.tmx;
    const double c00 = m[5] * m[10] - m[6] * m[9];
    const double c01 = m[6] * m[8] - m[4] * m[10];
    const double c02 = m[4] * m[9] - m[5] * m[8];
    const double det = m[0] * c00 + m[1] * c01 + m[2] * c02;
    if (std::fabs (det) < 1e-12) {
        return false;
    }

    double* r = inverse.tmx;
    r[0]  = c00 / det;
    r[1]  = (m[2] * m[9] - m[1] * m[10]) / det;
    r[2]  = (m[1] * m[6] - m[2] * m[5]) / det;
    r[4]  = c01 / det;
    r[5]  = (m[0] * m[10] - m[2] * m[8]) / det;
    r[6]  = (m[2] * m[4] - m[0] * m[6]) / det;
    r[8]  = c02 / det;
    r[9]  = (m[1] * m[8] - m[0] * m[9]) / det;
    r[10] = (m[0] * m[5] - m[1] * m[4]) / det;

    r[3]  = -(r[0] * m[3] + r[1] * m[7] + r[2] * m[11]);
    r[7]  = -(r[4] * m[3] + r[5] * m[7] + r[6] * m[11]);
    r[11] = -(r[8] * m[3] + r[9] * m[7] + r[10] * m[11]);
    return true;
}
#endif

GS::ObjectState GetSurveyPointTransformationCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    API_Tranmat projectToSurvey = {};
    const GSErrCode err = ACAPI_SurveyPoint_GetSurveyPointTransformation (&projectToSurvey);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the survey point transformation.");
    }

    GS::ObjectState response;
    const auto& projectToSurveyList = response.AddList<double> ("projectToSurvey");
    for (double value : projectToSurvey.tmx) {
        projectToSurveyList (value);
    }

    API_Tranmat surveyToProject = {};
    if (InvertTranmat (projectToSurvey, surveyToProject)) {
        const auto& surveyToProjectList = response.AddList<double> ("surveyToProject");
        for (double value : surveyToProject.tmx) {
            surveyToProjectList (value);
        }
    }
    return response;
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
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
                "$ref": "#/ProjectLocationUpdate"
            },
            "surveyPoint": {
                "$ref": "#/SurveyPointUpdate"
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
        GS::ObjectState positionInProject;
        if (surveyPoint.Get ("positionInProject", positionInProject)) {
            apiGeoLocation.surveyPointPosition = Get3DCoordinateFromObjectState (positionInProject);
            hasAnyInput = true;
        }
        GS::ObjectState geoReferencingParameters;
        if (surveyPoint.Get ("geoReferencingParameters", geoReferencingParameters)) {
            hasAnyInput |= geoReferencingParameters.Get ("crsName", apiGeoLocation.geoReferenceData.name);
            hasAnyInput |= geoReferencingParameters.Get ("description", apiGeoLocation.geoReferenceData.description);
            hasAnyInput |= geoReferencingParameters.Get ("geodeticDatum", apiGeoLocation.geoReferenceData.geodeticDatum);
            hasAnyInput |= geoReferencingParameters.Get ("verticalDatum", apiGeoLocation.geoReferenceData.verticalDatum);
            hasAnyInput |= geoReferencingParameters.Get ("mapProjection", apiGeoLocation.geoReferenceData.mapProjection);
            hasAnyInput |= geoReferencingParameters.Get ("mapZone", apiGeoLocation.geoReferenceData.mapZone);
            hasAnyInput |= geoReferencingParameters.Get ("xAxisAbscissa", apiGeoLocation.geoReferenceData.xAxisAbscissa);
            hasAnyInput |= geoReferencingParameters.Get ("xAxisOrdinate", apiGeoLocation.geoReferenceData.xAxisOrdinate);
            hasAnyInput |= geoReferencingParameters.Get ("scale", apiGeoLocation.geoReferenceData.scale);
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

// The fields of File > Info > Project Notes, in the order of API_ProjectNoteInfo.
static const char* ProjectNotesPropertiesSchema = R"(
            "client": {
                "type": "string",
                "description": "Client."
            },
            "projectName": {
                "type": "string",
                "description": "Project name."
            },
            "company": {
                "type": "string",
                "description": "Company."
            },
            "street": {
                "type": "string",
                "description": "Street."
            },
            "city": {
                "type": "string",
                "description": "City."
            },
            "country": {
                "type": "string",
                "description": "Country."
            },
            "postalCode": {
                "type": "string",
                "description": "Postal code."
            },
            "architect": {
                "type": "string",
                "description": "Architect."
            },
            "draftsmen": {
                "type": "string",
                "description": "Draftsmen."
            },
            "projectStatus": {
                "type": "string",
                "description": "Project status."
            },
            "dateOfIssue": {
                "type": "string",
                "description": "Date of issue (free text, as typed in Project Notes)."
            },
            "keywords": {
                "type": "string",
                "description": "Keywords."
            },
            "notes": {
                "type": "string",
                "description": "Notes."
            })";

GetProjectNotesCommand::GetProjectNotesCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetProjectNotesCommand::GetName () const
{
    return "GetProjectNotes";
}

GS::Optional<GS::UniString> GetProjectNotesCommand::GetRawResponseSchema () const
{
    return GS::UniString (R"({
        "type": "object",
        "properties": {)") + ProjectNotesPropertiesSchema + R"(
        },
        "additionalProperties": false,
        "required": [
            "client",
            "projectName",
            "company",
            "street",
            "city",
            "country",
            "postalCode",
            "architect",
            "draftsmen",
            "projectStatus",
            "dateOfIssue",
            "keywords",
            "notes"
        ]
    })";
}

GS::ObjectState GetProjectNotesCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    API_ProjectNoteInfo projectNotes = {};
    GSErrCode err = ACAPI_ProjectSetting_GetProjectNotes (&projectNotes);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the project notes.");
    }

    return GS::ObjectState (
        "client", GS::UniString (projectNotes.client),
        "projectName", GS::UniString (projectNotes.projectName),
        "company", GS::UniString (projectNotes.company),
        "street", GS::UniString (projectNotes.street),
        "city", GS::UniString (projectNotes.city),
        "country", GS::UniString (projectNotes.country),
        "postalCode", GS::UniString (projectNotes.code),
        "architect", GS::UniString (projectNotes.architect),
        "draftsmen", GS::UniString (projectNotes.draftsmen),
        "projectStatus", GS::UniString (projectNotes.projectStatus),
        "dateOfIssue", GS::UniString (projectNotes.dateOfIssue),
        "keywords", GS::UniString (projectNotes.keywords),
        "notes", GS::UniString (projectNotes.notes));
}

SetProjectNotesCommand::SetProjectNotesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetProjectNotesCommand::GetName () const
{
    return "SetProjectNotes";
}

GS::Optional<GS::UniString> SetProjectNotesCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
        "type": "object",
        "properties": {)") + ProjectNotesPropertiesSchema + R"(
        },
        "additionalProperties": false,
        "required": [
        ]
    })";
}

GS::Optional<GS::UniString> SetProjectNotesCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState SetProjectNotesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    API_ProjectNoteInfo projectNotes = {};
    GSErrCode err = ACAPI_ProjectSetting_GetProjectNotes (&projectNotes);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to get the project notes.");
    }

    // Fields that are not given keep their current value. Longer texts are truncated to the field's length.
    bool hasAnyInput = false;
    hasAnyInput |= SetCharProperty (&parameters, "client", projectNotes.client);
    hasAnyInput |= SetCharProperty (&parameters, "projectName", projectNotes.projectName);
    hasAnyInput |= SetCharProperty (&parameters, "company", projectNotes.company);
    hasAnyInput |= SetCharProperty (&parameters, "street", projectNotes.street);
    hasAnyInput |= SetCharProperty (&parameters, "city", projectNotes.city);
    hasAnyInput |= SetCharProperty (&parameters, "country", projectNotes.country);
    hasAnyInput |= SetCharProperty (&parameters, "postalCode", projectNotes.code);
    hasAnyInput |= SetCharProperty (&parameters, "architect", projectNotes.architect);
    hasAnyInput |= SetCharProperty (&parameters, "draftsmen", projectNotes.draftsmen);
    hasAnyInput |= SetCharProperty (&parameters, "projectStatus", projectNotes.projectStatus);
    hasAnyInput |= SetCharProperty (&parameters, "dateOfIssue", projectNotes.dateOfIssue);
    hasAnyInput |= SetCharProperty (&parameters, "keywords", projectNotes.keywords);
    hasAnyInput |= SetCharProperty (&parameters, "notes", projectNotes.notes);

    if (!hasAnyInput) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "No project notes field was given to change.");
    }

    err = ACAPI_CallUndoableCommand ("Change Project Notes", [&] () -> GSErrCode {
        return ACAPI_ProjectSetting_ChangeProjectNotes (&projectNotes);
    });
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to change the project notes.");
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

static GS::ObjectState CreateCalculationUnitsObjectState (const API_CalcUnitPrefs& unitPrefs)
{
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

GS::ObjectState GetCalculationUnitsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    API_CalcUnitPrefs unitPrefs = {};
    ACAPI_ProjectSetting_GetPreferences (&unitPrefs, APIPrefs_CalcUnitsID);

    return CreateCalculationUnitsObjectState (unitPrefs);
}

// Project preferences (Options > Project Preferences)
//
// Every page is one DevKit struct, read whole with ACAPI_ProjectSetting_GetPreferences
// and written back whole with ACAPI_ProjectSetting_SetPreferences. So SetProjectPreferences
// reads the page first, changes only the fields it was given and keeps the rest as it is.
// The names of the units are the ones GetCalculationUnits answers with, parsed back here.
// Writing a page back needs ACAPI_ProjectSetting_SetPreferences, which Archicad 27
// introduced (ACAPI_Environment has no set-preferences function before it), so
// SetProjectPreferences is available from Archicad 27.

static GS::ObjectState CreateWorkingUnitsObjectState (const API_WorkingUnitPrefs& prefs)
{
    return GS::ObjectState (
        "length", GS::ObjectState (
            "unit", ConvertAPILengthType (prefs.lengthUnit),
            "decimals", prefs.lenDecimals,
            "roundInch", prefs.roundInch),
        "area", GS::ObjectState (
            "unit", ConvertAPIAreaType (prefs.areaUnit),
            "decimals", prefs.areaDecimals),
        "volume", GS::ObjectState (
            "unit", ConvertAPIVolumeType (prefs.volumeUnit),
            "decimals", prefs.volumeDecimals),
        "angle", GS::ObjectState (
            "unit", ConvertAPIAngleType (prefs.angleUnit),
            "decimals", prefs.angleDecimals,
            "accuracy", prefs.angleAccuracy));
}

// The flag fields of the dimension formats are bool or char in the DevKit, answered as booleans.
static GS::ObjectState CreateLengthDimFormatObjectState (const API_LengthDimFormat& format)
{
    return GS::ObjectState (
        "unit", ConvertAPILengthType (format.unit),
        "decimals", format.lenDecimals,
        "roundInch", format.roundInch,
        "accuracy", ConvertAPIExtraAccuracyType (static_cast<API_ExtraAccuracyID> (format.showSmall5)),
        "showZeroWhole", format.show0Whole != 0,
        "showZeroInch", format.show0Inch != 0,
        "hideZeroDecimals", format.hide0Dec != 0);
}

static GS::ObjectState CreateAngleDimFormatObjectState (const API_AngleDimFormat& format)
{
    return GS::ObjectState (
        "unit", ConvertAPIAngleType (format.unit),
        "decimals", format.angleDecimals,
        "hideZeroDecimals", format.hide0Dec != 0);
}

static GS::ObjectState CreateAreaDimFormatObjectState (const API_AreaDimFormat& format)
{
    return GS::ObjectState (
        "unit", ConvertAPIAreaType (format.unit),
        "decimals", format.lenDecimals,
        "hideZeroDecimals", format.hide0Dec != 0);
}

static GS::ObjectState CreateDimensionsObjectState (const API_DimensionPrefs& prefs)
{
    return GS::ObjectState (
        "dimensionStandardIndex", prefs.index,
        "linear", CreateLengthDimFormatObjectState (prefs.linear),
        "angle", CreateAngleDimFormatObjectState (prefs.angle),
        "radial", CreateLengthDimFormatObjectState (prefs.radial),
        "level", CreateLengthDimFormatObjectState (prefs.level),
        "elevation", CreateLengthDimFormatObjectState (prefs.elevation),
        "doorWindow", CreateLengthDimFormatObjectState (prefs.doorwindow),
        "parapet", CreateLengthDimFormatObjectState (prefs.parapet),
        "area", CreateAreaDimFormatObjectState (prefs.area));
}

#ifdef ServerMainVers_2700

template <typename EnumType>
static bool ParseEnumByName (const GS::UniString& str, std::initializer_list<EnumType> values, GS::UniString (*toString) (EnumType), EnumType& result)
{
    for (EnumType value : values) {
        if (toString (value) == str) {
            result = value;
            return true;
        }
    }
    return false;
}

static bool ParseAPILengthType (const GS::UniString& str, API_LengthTypeID& result)
{
    return ParseEnumByName (str, {
        API_LengthTypeID::Meter,
        API_LengthTypeID::Decimeter,
        API_LengthTypeID::Centimeter,
        API_LengthTypeID::Millimeter,
        API_LengthTypeID::FootFracInch,
        API_LengthTypeID::FootDecInch,
        API_LengthTypeID::DecFoot,
        API_LengthTypeID::FracInch,
        API_LengthTypeID::DecInch
    }, ConvertAPILengthType, result);
}

static bool ParseAPIAreaType (const GS::UniString& str, API_AreaTypeID& result)
{
    return ParseEnumByName (str, {
        API_AreaTypeID::SquareMeter,
        API_AreaTypeID::SquareCentimeter,
        API_AreaTypeID::SquareMillimeter,
        API_AreaTypeID::SquareFoot,
        API_AreaTypeID::SquareInch
    }, ConvertAPIAreaType, result);
}

static bool ParseAPIVolumeType (const GS::UniString& str, API_VolumeTypeID& result)
{
    return ParseEnumByName (str, {
        API_VolumeTypeID::CubicMeter,
        API_VolumeTypeID::Liter,
        API_VolumeTypeID::CubicCentimeter,
        API_VolumeTypeID::CubicMillimeter,
        API_VolumeTypeID::CubicFoot,
        API_VolumeTypeID::CubicInch,
        API_VolumeTypeID::CubicYard,
        API_VolumeTypeID::Gallon
    }, ConvertAPIVolumeType, result);
}

static bool ParseAPIAngleType (const GS::UniString& str, API_AngleTypeID& result)
{
    return ParseEnumByName (str, {
        API_AngleTypeID::DecimalDegree,
        API_AngleTypeID::DegreeMinSec,
        API_AngleTypeID::Grad,
        API_AngleTypeID::Radian,
        API_AngleTypeID::Surveyors
    }, ConvertAPIAngleType, result);
}

static bool ParseAPIExtraAccuracyType (const GS::UniString& str, API_ExtraAccuracyID& result)
{
    return ParseEnumByName (str, {
        API_ExtraAccuracyID::APIExtAc_Off,
        API_ExtraAccuracyID::APIExtAc_Small5,
        API_ExtraAccuracyID::APIExtAc_Small25,
        API_ExtraAccuracyID::APIExtAc_Small1,
        API_ExtraAccuracyID::APIExtAc_Small01,
        API_ExtraAccuracyID::APIExtAc_Fractions
    }, ConvertAPIExtraAccuracyType, result);
}

// Reads an integer field of the preferences from `os`, into the (mostly short) struct field.
template <typename IntType>
static bool GetPreferenceIntegerField (const GS::ObjectState& os, const char* key, IntType& target)
{
    Int32 value = 0;
    if (!os.Get (key, value)) {
        return false;
    }
    target = static_cast<IntType> (value);
    return true;
}

// Reads a boolean field of the preferences from `os`, into the bool or char struct field.
template <typename FlagType>
static bool GetPreferenceFlagField (const GS::ObjectState& os, const char* key, FlagType& target)
{
    bool value = false;
    if (!os.Get (key, value)) {
        return false;
    }
    target = static_cast<FlagType> (value ? 1 : 0);
    return true;
}

// Reads the "unit" field of a unit object into `target`, with the parser of its unit type.
// Returns the error message for a name no unit has, NoValue otherwise.
template <typename UnitType>
static GS::Optional<GS::UniString> GetPreferenceUnitField (const GS::ObjectState& os, const char* quantity, bool (*parse) (const GS::UniString&, UnitType&), UnitType& target, bool& changed)
{
    GS::UniString unitStr;
    if (!os.Get ("unit", unitStr)) {
        return GS::NoValue;
    }
    if (!parse (unitStr, target)) {
        return GS::UniString ("Unknown ") + quantity + " unit: " + unitStr;
    }
    changed = true;
    return GS::NoValue;
}

static GS::Optional<GS::UniString> GetPreferenceAccuracyField (const GS::ObjectState& os, const char* quantity, API_ExtraAccuracyID& target, bool& changed)
{
    GS::UniString accuracyStr;
    if (!os.Get ("accuracy", accuracyStr)) {
        return GS::NoValue;
    }
    if (!ParseAPIExtraAccuracyType (accuracyStr, target)) {
        return GS::UniString ("Unknown ") + quantity + " accuracy: " + accuracyStr;
    }
    changed = true;
    return GS::NoValue;
}

// --- Working Units page

// The Apply functions change the fields of `prefs` that `os` gives and set `changed`
// when there was one. They return the error message for a value that cannot be
// applied, NoValue otherwise.
static GS::Optional<GS::UniString> ApplyWorkingUnits (const GS::ObjectState& os, API_WorkingUnitPrefs& prefs, bool& changed)
{
    GS::Optional<GS::UniString> error;

    GS::ObjectState length;
    if (os.Get ("length", length)) {
        error = GetPreferenceUnitField (length, "length", ParseAPILengthType, prefs.lengthUnit, changed);
        if (error.HasValue ()) {
            return error;
        }
        changed |= GetPreferenceIntegerField (length, "decimals", prefs.lenDecimals);
        changed |= GetPreferenceIntegerField (length, "roundInch", prefs.roundInch);
    }

    GS::ObjectState area;
    if (os.Get ("area", area)) {
        error = GetPreferenceUnitField (area, "area", ParseAPIAreaType, prefs.areaUnit, changed);
        if (error.HasValue ()) {
            return error;
        }
        changed |= GetPreferenceIntegerField (area, "decimals", prefs.areaDecimals);
    }

    GS::ObjectState volume;
    if (os.Get ("volume", volume)) {
        error = GetPreferenceUnitField (volume, "volume", ParseAPIVolumeType, prefs.volumeUnit, changed);
        if (error.HasValue ()) {
            return error;
        }
        changed |= GetPreferenceIntegerField (volume, "decimals", prefs.volumeDecimals);
    }

    GS::ObjectState angle;
    if (os.Get ("angle", angle)) {
        error = GetPreferenceUnitField (angle, "angle", ParseAPIAngleType, prefs.angleUnit, changed);
        if (error.HasValue ()) {
            return error;
        }
        changed |= GetPreferenceIntegerField (angle, "decimals", prefs.angleDecimals);
        changed |= GetPreferenceIntegerField (angle, "accuracy", prefs.angleAccuracy);
    }

    return GS::NoValue;
}

// --- Calculation Units page

static GS::Optional<GS::UniString> ApplyCalculationUnits (const GS::ObjectState& os, API_CalcUnitPrefs& prefs, bool& changed)
{
    GS::Optional<GS::UniString> error;

    GS::ObjectState length;
    if (os.Get ("length", length)) {
        error = GetPreferenceUnitField (length, "length", ParseAPILengthType, prefs.length.unit, changed);
        if (error.HasValue ()) {
            return error;
        }
        error = GetPreferenceAccuracyField (length, "length", prefs.length.accuracy, changed);
        if (error.HasValue ()) {
            return error;
        }
        changed |= GetPreferenceIntegerField (length, "decimals", prefs.length.decimals);
    }

    GS::ObjectState area;
    if (os.Get ("area", area)) {
        error = GetPreferenceUnitField (area, "area", ParseAPIAreaType, prefs.area.unit, changed);
        if (error.HasValue ()) {
            return error;
        }
        error = GetPreferenceAccuracyField (area, "area", prefs.area.accuracy, changed);
        if (error.HasValue ()) {
            return error;
        }
        changed |= GetPreferenceIntegerField (area, "decimals", prefs.area.decimals);
    }

    GS::ObjectState volume;
    if (os.Get ("volume", volume)) {
        error = GetPreferenceUnitField (volume, "volume", ParseAPIVolumeType, prefs.volume.unit, changed);
        if (error.HasValue ()) {
            return error;
        }
        error = GetPreferenceAccuracyField (volume, "volume", prefs.volume.accuracy, changed);
        if (error.HasValue ()) {
            return error;
        }
        changed |= GetPreferenceIntegerField (volume, "decimals", prefs.volume.decimals);
    }

    GS::ObjectState angle;
    if (os.Get ("angle", angle)) {
        error = GetPreferenceUnitField (angle, "angle", ParseAPIAngleType, prefs.angle.unit, changed);
        if (error.HasValue ()) {
            return error;
        }
        changed |= GetPreferenceIntegerField (angle, "decimals", prefs.angle.decimals);
        changed |= GetPreferenceIntegerField (angle, "accuracy", prefs.angle.accuracy);
    }

    return GS::NoValue;
}

// --- Dimensions page

static GS::Optional<GS::UniString> ApplyLengthDimFormat (const GS::ObjectState& os, const char* name, API_LengthDimFormat& format, bool& changed)
{
    GS::Optional<GS::UniString> error = GetPreferenceUnitField (os, name, ParseAPILengthType, format.unit, changed);
    if (error.HasValue ()) {
        return error;
    }

    API_ExtraAccuracyID accuracy = static_cast<API_ExtraAccuracyID> (format.showSmall5);
    error = GetPreferenceAccuracyField (os, name, accuracy, changed);
    if (error.HasValue ()) {
        return error;
    }
    format.showSmall5 = static_cast<decltype (format.showSmall5)> (accuracy);

    changed |= GetPreferenceIntegerField (os, "decimals", format.lenDecimals);
    changed |= GetPreferenceIntegerField (os, "roundInch", format.roundInch);
    changed |= GetPreferenceFlagField (os, "showZeroWhole", format.show0Whole);
    changed |= GetPreferenceFlagField (os, "showZeroInch", format.show0Inch);
    changed |= GetPreferenceFlagField (os, "hideZeroDecimals", format.hide0Dec);

    return GS::NoValue;
}

static GS::Optional<GS::UniString> ApplyAngleDimFormat (const GS::ObjectState& os, API_AngleDimFormat& format, bool& changed)
{
    const GS::Optional<GS::UniString> error = GetPreferenceUnitField (os, "angle dimension", ParseAPIAngleType, format.unit, changed);
    if (error.HasValue ()) {
        return error;
    }

    changed |= GetPreferenceIntegerField (os, "decimals", format.angleDecimals);
    changed |= GetPreferenceFlagField (os, "hideZeroDecimals", format.hide0Dec);

    return GS::NoValue;
}

static GS::Optional<GS::UniString> ApplyAreaDimFormat (const GS::ObjectState& os, API_AreaDimFormat& format, bool& changed)
{
    const GS::Optional<GS::UniString> error = GetPreferenceUnitField (os, "area dimension", ParseAPIAreaType, format.unit, changed);
    if (error.HasValue ()) {
        return error;
    }

    changed |= GetPreferenceIntegerField (os, "decimals", format.lenDecimals);
    changed |= GetPreferenceFlagField (os, "hideZeroDecimals", format.hide0Dec);

    return GS::NoValue;
}

static GS::Optional<GS::UniString> ApplyDimensions (const GS::ObjectState& os, API_DimensionPrefs& prefs, bool& changed)
{
    changed |= GetPreferenceIntegerField (os, "dimensionStandardIndex", prefs.index);

    struct LengthDimFormatField {
        const char* key;
        const char* name;
        API_LengthDimFormat API_DimensionPrefs::* format;
    };
    static const LengthDimFormatField lengthDimFormatFields[] = {
        { "linear", "linear dimension", &API_DimensionPrefs::linear },
        { "radial", "radial dimension", &API_DimensionPrefs::radial },
        { "level", "level dimension", &API_DimensionPrefs::level },
        { "elevation", "elevation dimension", &API_DimensionPrefs::elevation },
        { "doorWindow", "door/window dimension", &API_DimensionPrefs::doorwindow },
        { "parapet", "sill height dimension", &API_DimensionPrefs::parapet }
    };

    GS::Optional<GS::UniString> error;
    for (const LengthDimFormatField& field : lengthDimFormatFields) {
        GS::ObjectState format;
        if (os.Get (field.key, format)) {
            error = ApplyLengthDimFormat (format, field.name, prefs.*(field.format), changed);
            if (error.HasValue ()) {
                return error;
            }
        }
    }

    GS::ObjectState angle;
    if (os.Get ("angle", angle)) {
        error = ApplyAngleDimFormat (angle, prefs.angle, changed);
        if (error.HasValue ()) {
            return error;
        }
    }

    GS::ObjectState area;
    if (os.Get ("area", area)) {
        error = ApplyAreaDimFormat (area, prefs.area, changed);
        if (error.HasValue ()) {
            return error;
        }
    }

    return GS::NoValue;
}

// --- The commands

// Reads the page `prefsType`, applies the fields of `pageFields` to it with `apply`
// (one of the Apply functions above) and writes it back. Answers an ExecutionResult.
template <typename PrefsType, typename ApplyFunction>
static GS::ObjectState SetProjectPreferencesPage (const char* pageName, API_PrefsTypeID prefsType, const GS::ObjectState& pageFields, ApplyFunction apply)
{
    PrefsType prefs = {};
    GSErrCode err = ACAPI_ProjectSetting_GetPreferences (&prefs, prefsType);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, GS::UniString ("Failed to read the ") + pageName + " preferences.");
    }

    bool changed = false;
    const GS::Optional<GS::UniString> error = apply (pageFields, prefs, changed);
    if (error.HasValue ()) {
        return CreateFailedExecutionResult (APIERR_BADPARS, error.Get ());
    }
    if (!changed) {
        return CreateFailedExecutionResult (APIERR_BADPARS, GS::UniString ("No field of the ") + pageName + " preferences given.");
    }

    err = ACAPI_CallUndoableCommand ("Set Project Preferences", [&] () -> GSErrCode {
        return ACAPI_ProjectSetting_SetPreferences (&prefs, prefsType);
    });
    if (err != NoError) {
        return CreateFailedExecutionResult (err, GS::UniString ("Failed to set the ") + pageName + " preferences. In Teamwork the PreferencesDialog lockable object set has to be reserved first.");
    }

    return CreateSuccessfulExecutionResult ();
}

#endif

// The "required" list of a preferences object: every field in the response of
// GetProjectPreferences, none in the input of SetProjectPreferences, where a
// missing field keeps its value.
static GS::UniString GetProjectPreferencesRequiredFields (bool allFieldsRequired, const GS::UniString& fields)
{
    if (allFieldsRequired) {
        return GS::UniString ("\"required\": [") + fields + "]";
    }
    return "\"required\": []";
}

static GS::UniString GetWorkingUnitsSchema (bool allFieldsRequired)
{
    GS::UniString schema = R"({
        "type": "object",
        "description": "The Working Units page: the units and the number of decimals the model's lengths, areas, volumes and angles are shown with.",
        "properties": {
            "length": {
                "type": "object",
                "properties": {
                    "unit": {
                        "$ref": "#/LengthType"
                    },
                    "decimals": {
                        "type": "integer",
                        "description": "Number of decimals to display for length values."
                    },
                    "roundInch": {
                        "type": "integer",
                        "description": "Rounding of the fractional inch units, as the denominator of the smallest fraction shown: 1, 2, 4, 8, 16, 32 or 64."
                    }
                },
                "additionalProperties": false,
                )";
    schema += GetProjectPreferencesRequiredFields (allFieldsRequired, R"("unit", "decimals", "roundInch")");
    schema += R"(
            },
            "area": {
                "type": "object",
                "properties": {
                    "unit": {
                        "$ref": "#/AreaType"
                    },
                    "decimals": {
                        "type": "integer",
                        "description": "Number of decimals to display for area values."
                    }
                },
                "additionalProperties": false,
                )";
    schema += GetProjectPreferencesRequiredFields (allFieldsRequired, R"("unit", "decimals")");
    schema += R"(
            },
            "volume": {
                "type": "object",
                "properties": {
                    "unit": {
                        "$ref": "#/VolumeType"
                    },
                    "decimals": {
                        "type": "integer",
                        "description": "Number of decimals to display for volume values."
                    }
                },
                "additionalProperties": false,
                )";
    schema += GetProjectPreferencesRequiredFields (allFieldsRequired, R"("unit", "decimals")");
    schema += R"(
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
                )";
    schema += GetProjectPreferencesRequiredFields (allFieldsRequired, R"("unit", "decimals", "accuracy")");
    schema += R"(
            }
        },
        "additionalProperties": false,
        )";
    schema += GetProjectPreferencesRequiredFields (allFieldsRequired, R"("length", "area", "volume", "angle")");
    schema += R"(
    })";
    return schema;
}

static GS::UniString GetLengthDimFormatSchema (bool allFieldsRequired, const GS::UniString& description)
{
    GS::UniString schema = GS::UniString (R"({
                "type": "object",
                "description": ")") + description + R"(",
                "properties": {
                    "unit": {
                        "$ref": "#/LengthType"
                    },
                    "decimals": {
                        "type": "integer",
                        "description": "Number of decimals to display."
                    },
                    "roundInch": {
                        "type": "integer",
                        "description": "Rounding of the fractional inch units, as the denominator of the smallest fraction shown: 1, 2, 4, 8, 16, 32 or 64."
                    },
                    "accuracy": {
                        "$ref": "#/AccuracyType"
                    },
                    "showZeroWhole": {
                        "type": "boolean",
                        "description": "Show the zero before the decimal point of values below one."
                    },
                    "showZeroInch": {
                        "type": "boolean",
                        "description": "Show zero inches in feet and inches values."
                    },
                    "hideZeroDecimals": {
                        "type": "boolean",
                        "description": "Hide the zero decimals at the end of the values."
                    }
                },
                "additionalProperties": false,
                )";
    schema += GetProjectPreferencesRequiredFields (allFieldsRequired, R"("unit", "decimals", "roundInch", "accuracy", "showZeroWhole", "showZeroInch", "hideZeroDecimals")");
    schema += R"(
            })";
    return schema;
}

static GS::UniString GetAngleDimFormatSchema (bool allFieldsRequired)
{
    GS::UniString schema = R"({
                "type": "object",
                "description": "The format of the angle dimensions.",
                "properties": {
                    "unit": {
                        "$ref": "#/AngleType"
                    },
                    "decimals": {
                        "type": "integer",
                        "description": "Number of decimals to display."
                    },
                    "hideZeroDecimals": {
                        "type": "boolean",
                        "description": "Hide the zero decimals at the end of the values."
                    }
                },
                "additionalProperties": false,
                )";
    schema += GetProjectPreferencesRequiredFields (allFieldsRequired, R"("unit", "decimals", "hideZeroDecimals")");
    schema += R"(
            })";
    return schema;
}

static GS::UniString GetAreaDimFormatSchema (bool allFieldsRequired)
{
    GS::UniString schema = R"({
                "type": "object",
                "description": "The format of the area values the dimensions show (area calculations).",
                "properties": {
                    "unit": {
                        "$ref": "#/AreaType"
                    },
                    "decimals": {
                        "type": "integer",
                        "description": "Number of decimals to display."
                    },
                    "hideZeroDecimals": {
                        "type": "boolean",
                        "description": "Hide the zero decimals at the end of the values."
                    }
                },
                "additionalProperties": false,
                )";
    schema += GetProjectPreferencesRequiredFields (allFieldsRequired, R"("unit", "decimals", "hideZeroDecimals")");
    schema += R"(
            })";
    return schema;
}

static GS::UniString GetDimensionsSchema (bool allFieldsRequired)
{
    GS::UniString schema = R"({
        "type": "object",
        "description": "The Dimensions page: the dimension standard and the format (unit, decimals, extra accuracy and zero handling) of each dimension type.",
        "properties": {
            "dimensionStandardIndex": {
                "type": "integer",
                "description": "The index of the dimension standard the page follows."
            },
            "linear": )";
    schema += GetLengthDimFormatSchema (allFieldsRequired, "The format of the linear dimensions.");
    schema += R"(,
            "angle": )";
    schema += GetAngleDimFormatSchema (allFieldsRequired);
    schema += R"(,
            "radial": )";
    schema += GetLengthDimFormatSchema (allFieldsRequired, "The format of the radial dimensions.");
    schema += R"(,
            "level": )";
    schema += GetLengthDimFormatSchema (allFieldsRequired, "The format of the level dimensions.");
    schema += R"(,
            "elevation": )";
    schema += GetLengthDimFormatSchema (allFieldsRequired, "The format of the elevation dimensions.");
    schema += R"(,
            "doorWindow": )";
    schema += GetLengthDimFormatSchema (allFieldsRequired, "The format of the door, window and skylight dimensions.");
    schema += R"(,
            "parapet": )";
    schema += GetLengthDimFormatSchema (allFieldsRequired, "The format of the sill height (parapet) dimensions.");
    schema += R"(,
            "area": )";
    schema += GetAreaDimFormatSchema (allFieldsRequired);
    schema += R"(
        },
        "additionalProperties": false,
        )";
    schema += GetProjectPreferencesRequiredFields (allFieldsRequired, R"("dimensionStandardIndex", "linear", "angle", "radial", "level", "elevation", "doorWindow", "parapet", "area")");
    schema += R"(
    })";
    return schema;
}

static GS::UniString GetCalculationUnitsPageSchema (bool allFieldsRequired)
{
    GS::UniString schema = R"({
        "type": "object",
        "description": "The Calculation Units page: the units, the number of decimals and the extra accuracy of the values in element lists and schedules. GetCalculationUnits answers the same page.",
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
                    }
                },
                "additionalProperties": false,
                )";
    schema += GetProjectPreferencesRequiredFields (allFieldsRequired, R"("unit", "accuracy", "decimals")");
    schema += R"(
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
                )";
    schema += GetProjectPreferencesRequiredFields (allFieldsRequired, R"("unit", "accuracy", "decimals")");
    schema += R"(
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
                )";
    schema += GetProjectPreferencesRequiredFields (allFieldsRequired, R"("unit", "accuracy", "decimals")");
    schema += R"(
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
                )";
    schema += GetProjectPreferencesRequiredFields (allFieldsRequired, R"("unit", "decimals", "accuracy")");
    schema += R"(
            }
        },
        "additionalProperties": false,
        )";
    schema += GetProjectPreferencesRequiredFields (allFieldsRequired, R"("length", "area", "volume", "angle")");
    schema += R"(
    })";
    return schema;
}

GetProjectPreferencesCommand::GetProjectPreferencesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetProjectPreferencesCommand::GetName () const
{
    return "GetProjectPreferences";
}

GS::Optional<GS::UniString> GetProjectPreferencesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "preferenceTypes": {
                "type": "array",
                "description": "The preference pages to get. Every page when missing.",
                "items": {
                    "type": "string",
                    "enum": [
                        "WorkingUnits",
                        "Dimensions",
                        "CalculationUnits"
                    ]
                },
                "minItems": 1
            }
        },
        "additionalProperties": false,
        "required": [
        ]
    })";
}

GS::Optional<GS::UniString> GetProjectPreferencesCommand::GetRawResponseSchema () const
{
    GS::UniString schema = R"({
        "type": "object",
        "description": "The pages asked for, by name.",
        "properties": {
            "workingUnits": )";
    schema += GetWorkingUnitsSchema (true);
    schema += R"(,
            "dimensions": )";
    schema += GetDimensionsSchema (true);
    schema += R"(,
            "calculationUnits": )";
    schema += GetCalculationUnitsPageSchema (true);
    schema += R"(
        },
        "additionalProperties": false,
        "required": [
        ]
    })";
    return schema;
}

GS::ObjectState GetProjectPreferencesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::UniString> preferenceTypes;
    if (!parameters.Get ("preferenceTypes", preferenceTypes)) {
        preferenceTypes.Push ("WorkingUnits");
        preferenceTypes.Push ("Dimensions");
        preferenceTypes.Push ("CalculationUnits");
    }

    GS::ObjectState response;
    for (const GS::UniString& preferenceType : preferenceTypes) {
        if (preferenceType == "WorkingUnits") {
            API_WorkingUnitPrefs prefs = {};
            const GSErrCode err = ACAPI_ProjectSetting_GetPreferences (&prefs, APIPrefs_WorkingUnitsID);
            if (err != NoError) {
                return CreateErrorResponse (err, "Failed to read the working units preferences.");
            }
            response.Add ("workingUnits", CreateWorkingUnitsObjectState (prefs));
        } else if (preferenceType == "Dimensions") {
            API_DimensionPrefs prefs = {};
            const GSErrCode err = ACAPI_ProjectSetting_GetPreferences (&prefs, APIPrefs_DimensionsID);
            if (err != NoError) {
                return CreateErrorResponse (err, "Failed to read the dimensions preferences.");
            }
            response.Add ("dimensions", CreateDimensionsObjectState (prefs));
        } else if (preferenceType == "CalculationUnits") {
            API_CalcUnitPrefs prefs = {};
            const GSErrCode err = ACAPI_ProjectSetting_GetPreferences (&prefs, APIPrefs_CalcUnitsID);
            if (err != NoError) {
                return CreateErrorResponse (err, "Failed to read the calculation units preferences.");
            }
            response.Add ("calculationUnits", CreateCalculationUnitsObjectState (prefs));
        } else {
            return CreateErrorResponse (APIERR_BADPARS, GS::UniString ("Unknown preference type: ") + preferenceType);
        }
    }

    return response;
}

SetProjectPreferencesCommand::SetProjectPreferencesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetProjectPreferencesCommand::GetName () const
{
    return "SetProjectPreferences";
}

GS::Optional<GS::UniString> SetProjectPreferencesCommand::GetInputParametersSchema () const
{
    GS::UniString schema = R"({
        "type": "object",
        "description": "The pages to change, by name. Only the given fields of a page change, the rest keeps its value. In Teamwork the PreferencesDialog lockable object set has to be reserved first (ReserveLockableObjectSet).",
        "properties": {
            "workingUnits": )";
    schema += GetWorkingUnitsSchema (false);
    schema += R"(,
            "dimensions": )";
    schema += GetDimensionsSchema (false);
    schema += R"(,
            "calculationUnits": )";
    schema += GetCalculationUnitsPageSchema (false);
    schema += R"(
        },
        "additionalProperties": false,
        "required": [
        ]
    })";
    return schema;
}

GS::Optional<GS::UniString> SetProjectPreferencesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "description": "One execution result per page given in the input, by name.",
        "properties": {
            "workingUnits": {
                "$ref": "#/ExecutionResult"
            },
            "dimensions": {
                "$ref": "#/ExecutionResult"
            },
            "calculationUnits": {
                "$ref": "#/ExecutionResult"
            }
        },
        "additionalProperties": false,
        "required": [
        ]
    })";
}

GS::ObjectState SetProjectPreferencesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::ObjectState response;
    bool anyPage = false;

    GS::ObjectState workingUnits;
    if (parameters.Get ("workingUnits", workingUnits)) {
        anyPage = true;
        response.Add ("workingUnits", SetProjectPreferencesPage<API_WorkingUnitPrefs> ("working units", APIPrefs_WorkingUnitsID, workingUnits, ApplyWorkingUnits));
    }

    GS::ObjectState dimensions;
    if (parameters.Get ("dimensions", dimensions)) {
        anyPage = true;
        response.Add ("dimensions", SetProjectPreferencesPage<API_DimensionPrefs> ("dimensions", APIPrefs_DimensionsID, dimensions, ApplyDimensions));
    }

    GS::ObjectState calculationUnits;
    if (parameters.Get ("calculationUnits", calculationUnits)) {
        anyPage = true;
        response.Add ("calculationUnits", SetProjectPreferencesPage<API_CalcUnitPrefs> ("calculation units", APIPrefs_CalcUnitsID, calculationUnits, ApplyCalculationUnits));
    }

    if (!anyPage) {
        return CreateErrorResponse (APIERR_BADPARS, "No preference page given. Give workingUnits, dimensions or calculationUnits.");
    }

    return response;
#else
    (void) parameters;
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "SetProjectPreferences requires Archicad 27 or newer.");
#endif
}

static bool ParseElementsToIfcExport (const GS::UniString& str, API_ElementsToIfcExportID& result)
{
    if (str == "EntireProject") {
        result = API_EntireProject;
    } else if (str == "VisibleElementsOnAllStories") {
        result = API_VisibleElementsOnAllStories;
    } else if (str == "AllElementsOnCurrentStory") {
        result = API_AllElementsOnCurrentStorey;
    } else if (str == "VisibleElementsOnCurrentStory") {
        result = API_VisibleElementsOnCurrentStorey;
    } else if (str == "SelectedElementsOnly") {
        result = API_SelectedElementsOnly;
    } else {
        return false;
    }
    return true;
}

// Saves the project as IFC with an export translator chosen by name, through the
// Automate API. The add-on communication path in Execute cannot choose one: the IFC
// add-on saves with the translator Archicad remembers from its own Save dialog, which
// an automated caller can neither see nor set. The floor plan or the 3D window has to
// be the front window, as for any IFC save.
static GS::ObjectState SaveProjectAsIfcWithTranslator (const GS::ObjectState& parameters, IO::Location& ifcFileLocation, Int32 fileTypeRefCon, const GS::UniString& translatorName)
{
    // The Automate API saves plain IFC and one packed form: IFC ZIP from Archicad 27
    // on, IFC XML before that.
    API_IfcTypeID subType;
    if (fileTypeRefCon == 1) {
        subType = API_IFC;
#ifdef ServerMainVers_2700
    } else if (fileTypeRefCon == 3) {
        subType = API_IFCZIP;
    } else {
        return CreateFailedExecutionResult (APIERR_BADPARS, "translatorName needs a fileType of ifc or ifczip");
#else
    } else if (fileTypeRefCon == 2) {
        subType = API_IFCXML;
    } else {
        return CreateFailedExecutionResult (APIERR_BADPARS, "translatorName needs a fileType of ifc or ifcxml on this Archicad version");
#endif
    }

    API_ElementsToIfcExportID elementsToExport = API_VisibleElementsOnAllStories;
    GS::UniString elementsToExportStr;
    if (parameters.Get ("elementsToExport", elementsToExportStr) && !ParseElementsToIfcExport (elementsToExportStr, elementsToExport)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "elementsToExport parameter is invalid");
    }

    GS::Array<API_IFCTranslatorIdentifier> translators;
    GSErrCode err = ACAPI_IFC_GetIFCExportTranslatorsList (translators);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to list the IFC export translators of the project");
    }

    const API_IFCTranslatorIdentifier* translator = nullptr;
    GS::UniString availableNames;
    for (const API_IFCTranslatorIdentifier& candidate : translators) {
        if (candidate.name == translatorName) {
            translator = &candidate;
        }
        if (!availableNames.IsEmpty ()) {
            availableNames += ", ";
        }
        availableNames += "\"" + candidate.name + "\"";
    }
    if (translator == nullptr) {
        return CreateFailedExecutionResult (APIERR_BADNAME, "The project has no IFC export translator named \"" + translatorName + "\". Its translators: " + availableNames);
    }

    API_FileSavePars fileSavePars = {};
    fileSavePars.fileTypeID = APIFType_IfcFile;
    fileSavePars.file = &ifcFileLocation;

    API_SavePars_Ifc savePars = {};
    savePars.subType = subType;
    savePars.translatorIdentifier = *translator;
    savePars.elementsToIfcExport = elementsToExport;
    savePars.elementsSet = nullptr;
#ifdef ServerMainVers_2600
    savePars.includeBoundingBoxGeometry = false;
#endif

    err = ACAPI_ProjectOperation_Save (&fileSavePars, &savePars);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to save the project as IFC");
    }

    return CreateSuccessfulExecutionResult ();
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
            },
            "translatorName": {
                "type": "string",
                "description": "Only for the save method: the name of the IFC export translator to save with, as GetIFCExportTranslators lists them. Without it the save runs with the translator Archicad would offer in its own Save dialog. Needs a fileType of ifc or ifczip (ifc or ifcxml on Archicad 25 and 26)."
            },
            "elementsToExport": {
                "$ref": "#/IFCElementsToExport",
                "description": "Only for the save method, and only together with translatorName. The default is VisibleElementsOnAllStories."
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
    GS::UniString translatorName;
    if (parameters.Get ("translatorName", translatorName) && !translatorName.IsEmpty ()) {
        if (ioParams.method != IO_SAVEAS) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "translatorName is only valid with the save method");
        }
        return SaveProjectAsIfcWithTranslator (parameters, ifcFileLocation, ioParams.refCon, translatorName);
    }
    if (parameters.Contains ("elementsToExport")) {
        // The add-on path below has no element filter, so a filter without a translator
        // would be dropped without a word.
        return CreateFailedExecutionResult (APIERR_BADPARS, "elementsToExport is only valid together with translatorName");
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
// nodes; these commands create nodes, place instances, move them, re-read
// the nodes' source files and repoint nodes at other files.
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
                    "$ref": "#/HotlinkInstanceCreation"
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
                    "$ref": "#/HotlinkInstanceChange"
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

// The node keeps a cache of its source file's content, and Archicad re-reads
// the file only when asked - the Hotlink Manager's Update button, which is
// UpdateHotlinks. ChangeHotlinkNodes is the Relink button: it repoints a
// node at another file. Neither is wrapped in ACAPI_CallUndoableCommand: the
// DevKit lists the hotlink node functions among the non-undoable data
// structure modifiers.

UpdateHotlinksCommand::UpdateHotlinksCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String UpdateHotlinksCommand::GetName () const
{
    return "UpdateHotlinks";
}

GS::Optional<GS::UniString> UpdateHotlinksCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "hotlinkNodes": {
                "type": "array",
                "description": "The hotlink nodes to update, from GetHotlinks or CreateHotlinkNodes. Each node's cached content is re-read from its source file, as the Hotlink Manager's Update button does.",
                "items": {
                    "type": "object",
                    "properties": {
                        "hotlinkNodeId": {
                            "$ref": "#/HotlinkNodeId"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "hotlinkNodeId"
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

GS::Optional<GS::UniString> UpdateHotlinksCommand::GetRawResponseSchema () const
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

GS::ObjectState UpdateHotlinksCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> hotlinkNodes;
    parameters.Get ("hotlinkNodes", hotlinkNodes);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    for (const GS::ObjectState& nodeData : hotlinkNodes) {
        const GS::ObjectState* hotlinkNodeId = nodeData.Get ("hotlinkNodeId");
        if (hotlinkNodeId == nullptr) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "hotlinkNodeId is missing"));
            continue;
        }

        // Reading the node first tells a wrong id apart from a failed update
        // and gives the source path for the message.
        API_HotlinkNode node = {};
        node.guid = GetGuidFromObjectState (*hotlinkNodeId);
        bool enableUnplaced = true;     // the node may have been created and not placed yet
        if (ACAPI_Hotlink_GetHotlinkNode (&node, &enableUnplaced) != NoError) {
            executionResults (CreateFailedExecutionResult (APIERR_BADID, "hotlinkNodeId is not a hotlink node"));
            continue;
        }
        const GS::UniString sourcePath = node.sourceLocation != nullptr ? node.sourceLocation->ToDisplayText () : GS::UniString ();

        const GSErrCode err = ACAPI_Hotlink_UpdateHotlinkCache (&node.guid);
        if (err != NoError) {
            executionResults (CreateFailedExecutionResult (err, "Failed to update the hotlink from " + sourcePath));
            continue;
        }
        executionResults (CreateSuccessfulExecutionResult ());
    }

    return response;
}

ChangeHotlinkNodesCommand::ChangeHotlinkNodesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ChangeHotlinkNodesCommand::GetName () const
{
    return "ChangeHotlinkNodes";
}

GS::Optional<GS::UniString> ChangeHotlinkNodesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "hotlinkNodes": {
                "type": "array",
                "description": "The hotlink nodes to repoint at another source file, as the Hotlink Manager's Relink button does. The placed instances stay where they are. Follow it with UpdateHotlinks on the same nodes to re-read the content from the new file.",
                "items": {
                    "type": "object",
                    "properties": {
                        "hotlinkNodeId": {
                            "$ref": "#/HotlinkNodeId"
                        },
                        "sourceLocation": {
                            "type": "string",
                            "description": "Absolute path of the new module source file (.mod or .pln)."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "hotlinkNodeId",
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

GS::Optional<GS::UniString> ChangeHotlinkNodesCommand::GetRawResponseSchema () const
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

GS::ObjectState ChangeHotlinkNodesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> hotlinkNodes;
    parameters.Get ("hotlinkNodes", hotlinkNodes);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    for (const GS::ObjectState& nodeData : hotlinkNodes) {
        const GS::ObjectState* hotlinkNodeId = nodeData.Get ("hotlinkNodeId");
        GS::UniString sourcePath;
        if (hotlinkNodeId == nullptr || !nodeData.Get ("sourceLocation", sourcePath) || sourcePath.IsEmpty ()) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "hotlinkNodeId or sourceLocation is missing"));
            continue;
        }
        IO::Location sourceLocation (sourcePath);
        IO::Name lastLocalName;
        if (sourceLocation.GetLastLocalName (&lastLocalName) != NoError) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "sourceLocation is not a valid path"));
            continue;
        }

        // The modify call takes the whole node, so it is read first; only the
        // source location is changed, the rest goes back as it came.
        API_HotlinkNode node = {};
        node.guid = GetGuidFromObjectState (*hotlinkNodeId);
        bool enableUnplaced = true;     // the node may have been created and not placed yet
        if (ACAPI_Hotlink_GetHotlinkNode (&node, &enableUnplaced) != NoError) {
            executionResults (CreateFailedExecutionResult (APIERR_BADID, "hotlinkNodeId is not a hotlink node"));
            continue;
        }
        // The read allocates sourceLocation on the heap and the node's
        // destructor frees it (see CreateHotlinkNodes), so the old location
        // is freed here and the new one handed over the same way.
        delete node.sourceLocation;
        node.sourceLocation = new IO::Location (sourceLocation);

        const GSErrCode err = ACAPI_Hotlink_ModifyHotlinkNode (&node);
        if (err != NoError) {
            executionResults (CreateFailedExecutionResult (err, "Failed to change the hotlink node's source to " + sourcePath));
            continue;
        }
        executionResults (CreateSuccessfulExecutionResult ());
    }

    return response;
}

// The elements inside a placed instance are proxies: copies made in this
// project with guids of their own. Archicad keeps, per instance, which element
// of the source file each proxy was made from.

GetHotlinkProxyElementTableCommand::GetHotlinkProxyElementTableCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetHotlinkProxyElementTableCommand::GetName () const
{
    return "GetHotlinkProxyElementTable";
}

GS::Optional<GS::UniString> GetHotlinkProxyElementTableCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "hotlinkInstances": {
                "type": "array",
                "description": "The placed hotlink instances: elements of type Hotlink, for example the hotlinkId of an element inside one. Not the nodes GetHotlinks returns.",
                "items": {
                    "$ref": "#/ElementIdArrayItem"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "hotlinkInstances"
        ]
    })";
}

GS::Optional<GS::UniString> GetHotlinkProxyElementTableCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "proxyElementTables": {
                "type": "array",
                "description": "One item per requested instance, in order: its proxy elements with the source elements they were made from, or an error.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "proxyElements": {
                                    "type": "array",
                                    "description": "The proxy elements of the instance, in no particular order.",
                                    "items": {
                                        "type": "object",
                                        "properties": {
                                            "proxyElementId": {
                                                "$ref": "#/ElementId",
                                                "description": "The proxy element in the current project."
                                            },
                                            "sourceElementId": {
                                                "$ref": "#/ElementId",
                                                "description": "The element in the hotlink's source file the proxy was made from."
                                            }
                                        },
                                        "additionalProperties": false,
                                        "required": [
                                            "proxyElementId",
                                            "sourceElementId"
                                        ]
                                    }
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "proxyElements"
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
            "proxyElementTables"
        ]
    })";
}

GS::ObjectState GetHotlinkProxyElementTableCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> hotlinkInstances;
    parameters.Get ("hotlinkInstances", hotlinkInstances);

    GS::ObjectState response;
    const auto& proxyElementTables = response.AddList<GS::ObjectState> ("proxyElementTables");

    for (const GS::ObjectState& instanceData : hotlinkInstances) {
        const GS::ObjectState* elementId = instanceData.Get ("elementId");
        if (elementId == nullptr) {
            proxyElementTables (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        // Checked here so a node id or another element gets a clear message
        // rather than a bare error code from the table call.
        API_Elem_Head header = {};
        header.guid = GetGuidFromObjectState (*elementId);
        GSErrCode err = ACAPI_Element_GetHeader (&header);
        if (err != NoError) {
            proxyElementTables (CreateErrorResponse (err, "Failed to get the element"));
            continue;
        }
        if (GetElemTypeId (header) != API_HotlinkID) {
            proxyElementTables (CreateErrorResponse (APIERR_BADELEMENTTYPE, "The element is not a hotlink instance"));
            continue;
        }

        GS::HashTable<API_Guid, API_Guid> proxyElementTable;
        err = ACAPI_Hotlink_GetHotlinkProxyElementTable (&header.guid, &proxyElementTable);
        if (err != NoError) {
            proxyElementTables (CreateErrorResponse (err, "Failed to get the proxy element table of the hotlink instance"));
            continue;
        }

        GS::ObjectState item;
        const auto& proxyElements = item.AddList<GS::ObjectState> ("proxyElements");
        for (const auto& kv : proxyElementTable) {
            GS::ObjectState proxyElement;
#ifdef ServerMainVers_2800
            proxyElement.Add ("proxyElementId", CreateGuidObjectState (kv.key));
            proxyElement.Add ("sourceElementId", CreateGuidObjectState (kv.value));
#else
            proxyElement.Add ("proxyElementId", CreateGuidObjectState (*kv.key));
            proxyElement.Add ("sourceElementId", CreateGuidObjectState (*kv.value));
#endif
            proxyElements (proxyElement);
        }
        proxyElementTables (item);
    }

    return response;
}
