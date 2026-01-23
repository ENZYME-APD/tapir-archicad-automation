#include "ProjectCommands.hpp"
#include "MigrationHelper.hpp"
#include "MemoryIChannel32.hpp"
#include "Ref.hpp"

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

GetStoriesCommand::GetStoriesCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetStoriesCommand::GetName () const
{
    return "GetStories";
}

GS::Optional<GS::UniString> GetStoriesCommand::GetResponseSchema () const
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
        storyData.Add ("name", uName);

        listAdder (storyData);
    }

    BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));

    return response;
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

GS::Optional<GS::UniString> SetStoriesCommand::GetResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}


GS::ObjectState SetStoriesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> stories;
    parameters.Get ("stories", stories);

    API_StoryInfo storyInfo = {};
    GSErrCode err = ACAPI_ProjectSetting_GetStorySettings (&storyInfo);
    if (err != NoError) {
        BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
        return CreateFailedExecutionResult (err, "Failed to retrive stories info.");
    }

    GS::USize storyCount = storyInfo.lastStory - storyInfo.firstStory + 1;

    if (storyCount != stories.GetSize ()) {
        if (storyCount < stories.GetSize ()) {
            for (GS::UIndex i = storyCount; i < stories.GetSize (); ++i) {
                API_StoryCmdType storyCmd = {};
                storyCmd.action = APIStory_InsAbove;
                storyCmd.index  = storyInfo.lastStory;

                stories[i].Get ("dispOnSections", storyCmd.dispOnSections);
                stories[i].Get ("level", storyCmd.elevation);
                if (storyCount > 1) {
                    storyCmd.height = (*storyInfo.data)[i - 1].level - (*storyInfo.data)[i - 2].level;
                }

                GS::UniString name;
                stories[i].Get ("name", name);
                GS::snuprintf (storyCmd.uName, sizeof (storyCmd.uName), name.ToCStr ());
            
                err = ACAPI_ProjectSetting_ChangeStorySettings (&storyCmd);
                if (err != NoError) {
                    BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
                    return CreateFailedExecutionResult (err, "Failed to create new story.");
                }
            }
        } else {
            for (GS::UIndex i = storyCount - 1; i >= stories.GetSize (); --i) {
                API_StoryCmdType storyCmd = {};
                storyCmd.action = APIStory_Delete;
                storyCmd.index  = static_cast<short> (i);
            
                err = ACAPI_ProjectSetting_ChangeStorySettings (&storyCmd);
                if (err != NoError) {
                    BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
                    return CreateFailedExecutionResult (err, "Failed to delete story.");
                }
            }
        }

        err = ACAPI_ProjectSetting_GetStorySettings (&storyInfo);
        if (err != NoError) {
            BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
            return CreateFailedExecutionResult (err, "Failed to retrive stories info.");
        }
        
        storyCount = storyInfo.lastStory - storyInfo.firstStory + 1;
    }

    GS::USize recursionCount = 0;
    constexpr GS::USize maxRecursion = 3;
    for (GS::UIndex i = 0; i < storyCount;) {
        const API_StoryType& story = (*storyInfo.data)[i];

        API_StoryCmdType storyCmd = {};
        storyCmd.index  = static_cast<short> (i);

        stories[i].Get ("dispOnSections", storyCmd.dispOnSections);

        bool changed = false;

        if (story.dispOnSections != storyCmd.dispOnSections) {
            storyCmd.action = APIStory_SetDispOnSections;
        
            err = ACAPI_ProjectSetting_ChangeStorySettings (&storyCmd);
            if (err != NoError) {
                BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
                return CreateFailedExecutionResult (err, "Failed to modify dispOnSections settings.");
            }

            changed = true;
        }

        GS::UniString name;
        stories[i].Get ("name", name);

        if (story.uName != name) {
            GS::snuprintf (storyCmd.uName, sizeof (storyCmd.uName), name.ToCStr ());
            storyCmd.action = APIStory_Rename;
        
            err = ACAPI_ProjectSetting_ChangeStorySettings (&storyCmd);
            if (err != NoError) {
                BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
                return CreateFailedExecutionResult (err, "Failed to rename story.");
            }

            changed = true;
        }

        stories[i].Get ("level", storyCmd.elevation);

        if (std::abs (story.level - storyCmd.elevation) >= 0.0001) {
            storyCmd.action = APIStory_SetElevation;
        
            err = ACAPI_ProjectSetting_ChangeStorySettings (&storyCmd);
            if (err != NoError) {
                BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
                return CreateFailedExecutionResult (err, "Failed to change story level.");
            }

            changed = true;
        } else {
            const API_StoryType*   actNextStory = i + 1 < storyCount ? &(*storyInfo.data)[i + 1] : nullptr;
            const GS::ObjectState* newNextStory = i + 1 < stories.GetSize () ? &stories[i + 1] : nullptr;

            double newNextLevel = 0;
            if (actNextStory != nullptr && newNextStory != nullptr &&
                newNextStory->Get ("level", newNextLevel) &&
                std::abs ((newNextLevel - storyCmd.elevation) - (actNextStory->level - story.level)) >= 0.0001) {
                storyCmd.height = newNextLevel - storyCmd.elevation;
                storyCmd.action = APIStory_SetHeight;
            
                err = ACAPI_ProjectSetting_ChangeStorySettings (&storyCmd);
                if (err != NoError) {
                    BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
                    return CreateFailedExecutionResult (err, "Failed to change story height.");
                }

                changed = true;
            }
        }

        if (changed) {
            BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
            err = ACAPI_ProjectSetting_GetStorySettings (&storyInfo);
            if (err != NoError) {
                BMKillHandle (reinterpret_cast<GSHandle *> (&storyInfo.data));
                return CreateFailedExecutionResult (err, "Failed to retrive stories info.");
            }
        }

        if (!changed || ++recursionCount >= maxRecursion) {
            recursionCount = 0;
            ++i;
            continue;
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

GS::Optional<GS::UniString> OpenProjectCommand::GetResponseSchema () const
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

GetGeoLocationCommand::GetGeoLocationCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetGeoLocationCommand::GetName () const
{
    return "GetGeoLocation";
}

GS::Optional<GS::UniString> GetGeoLocationCommand::GetResponseSchema () const
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

GetIFCTranslatorsCommand::GetIFCTranslatorsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetIFCTranslatorsCommand::GetName () const
{
    return "GetIFCTranslators";
}

GS::Optional<GS::UniString> GetIFCTranslatorsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "type": {
                "type": "string",
                "enum": [
                    "Import",
                    "Export"
                ]
            }
        },
        "additionalProperties": false,
        "required": [
            "type"
        ]
    })";
}

GS::Optional<GS::UniString> GetIFCTranslatorsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "ifcTranslators": {
                "type": "array",
                "items": {
                    "type": "string",
                    "description": "The name of an IFC export translator."
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "ifcTranslators"
        ]
    })";
}

GS::ObjectState GetIFCTranslatorsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    enum IOType {
        Undefined,
        Open,
        Save
    };

    GS::UniString type;
    if (!parameters.Get ("type", type)) {
        return CreateErrorResponse (APIERR_BADPARS, "type is required parameter");
    }

    IOType ioType = IOType::Undefined;
    if (type == "Import") {
        ioType = IOType::Open;
    } else if (type == "Export") {
        ioType = IOType::Save;
    }

    GS::Array<API_IFCTranslatorIdentifier> translators;
    API_ModulID moduleID = { 1198731108, 138575850 };
    const GSErrCode err = ACAPI_AddOnAddOnCommunication_Call (&moduleID, 'GTNL', 1, reinterpret_cast<GSHandle>(&ioType), reinterpret_cast<GSPtr>(&translators), true);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the IFC translators");
    }

    GS::ObjectState response;
    const auto& ifcTranslators = response.AddList<GS::UniString> ("ifcTranslators");

    for (const auto& translator : translators) {
        ifcTranslators (translator.name);
    }

    return response;
}

SaveIFCCommand::SaveIFCCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SaveIFCCommand::GetName () const
{
    return "SaveIFC";
}

GS::Optional<GS::UniString> SaveIFCCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "ifcFilePath": {
                "type": "string",
                "description": "The target IFC file to use."
            },
            "fileType": {
                "type": "string",
                "description": "The type of the IFC file. The default is 'ifc'.",
                "enum": ["ifc", "ifcxml", "ifczip", "ifcxmlzip"]
            },
            "translator": {
                "type": "string",
                "description": "The name of the IFC export translator."
            },
            "elementsToExport": {
                "type": "string",
                "enum": [
                    "EntireProject",
                    "VisibleElementsOnAllStories",
                    "AllElementsOnCurrentStorey",
                    "VisibleElementsOnCurrentStorey",
                    "SelectedElementsOnly",
                    "FilteredElements"
                ]
            },
            "filteredElements": {
                "description": "Export the given elements only. Ignored if elementsToExport is not FilteredElements.",
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": [
            "ifcFilePath",
            "translator"
        ]
    })";
}

GS::Optional<GS::UniString> SaveIFCCommand::GetResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

struct GSGuidHolder {
    GS::Guid guid;
};

static GS::ObjectState ExecuteIFCIOHiddenCommand (short method, const GS::ObjectState& parameters)
{
    GS::UniString ifcFilePath;
    if (!parameters.Get ("ifcFilePath", ifcFilePath)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "ifcFilePath parameter is missing");
    }

    API_IOParams ioParams = {};
    ioParams.fileTypeID = APIFType_IfcFile;
    ioParams.method = method;
    ioParams.refCon = 1;

    if (method == IO_SAVEAS) {
        GS::UniString fileTypeStr;
        parameters.Get ("fileType", fileTypeStr);
        if (fileTypeStr.IsEmpty () || fileTypeStr == "ifc") {
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

void IFCTranslatorInnerHandleToGuidHolder (GSConstHandle innerHandle, GSGuidHolder& guidHolder)
{
    GS::MemoryIChannel32 memoryChannel (*innerHandle, BMhGetSize (innerHandle));
    guidHolder.guid.Read (memoryChannel);
}

GS::ObjectState SaveIFCCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    /*
    GS::UniString ifcFilePath;
    if (!parameters.Get ("ifcFilePath", ifcFilePath)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "ifcFilePath parameter is missing");
    }

    GS::UniString fileTypeStr;
    parameters.Get ("fileType", fileTypeStr);

    std::unique_ptr<GS::Array<API_Guid>> elementsPtr;

    GS::UniString translator;
    if (!parameters.Get ("translator", translator)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "ifcFilePath parameter is missing");
    }
    API_SavePars_Ifc saveParsIfc = {};

    if (fileTypeStr.IsEmpty () || fileTypeStr == "ifc") {
        saveParsIfc.subType = API_IFC;
    } else if (fileTypeStr == "ifcxml") {
        saveParsIfc.subType = API_IFCXML;
    } else {
        return CreateFailedExecutionResult (APIERR_BADPARS, "fileType parameter is invalid");
    }

    GS::Array<API_IFCTranslatorIdentifier> ifcExportTranslators;
    ACAPI_IFC_GetIFCExportTranslatorsList (ifcExportTranslators);

    for (const auto& translatorIdentifier : ifcExportTranslators) {
        if (translatorIdentifier.name == translator) {
            saveParsIfc.translatorIdentifier = translatorIdentifier;
            break;
        }
    }
    if (saveParsIfc.translatorIdentifier.innerReference == nullptr) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Not found IFC export translator with the given name");
    }

    GS::UniString elementsToExport;
    parameters.Get ("elementsToExport", elementsToExport);

    if (elementsToExport.IsEmpty () || elementsToExport == "EntireProject") {
        saveParsIfc.elementsToIfcExport = API_EntireProject;
    } else if (elementsToExport == "VisibleElementsOnAllStories") {
        saveParsIfc.elementsToIfcExport = API_VisibleElementsOnAllStories;
    } else if (elementsToExport == "AllElementsOnCurrentStorey") {
        saveParsIfc.elementsToIfcExport = API_AllElementsOnCurrentStorey;
    } else if (elementsToExport == "AllElementsOnCurrentStorey") {
        saveParsIfc.elementsToIfcExport = API_AllElementsOnCurrentStorey;
    } else if (elementsToExport == "VisibleElementsOnCurrentStorey") {
        saveParsIfc.elementsToIfcExport = API_VisibleElementsOnCurrentStorey;
    } else if (elementsToExport == "SelectedElementsOnly") {
        saveParsIfc.elementsToIfcExport = API_SelectedElementsOnly;
    } else if (elementsToExport == "FilteredElements") {
        saveParsIfc.elementsToIfcExport = API_FilteredElements;
        GS::Array<GS::ObjectState> filteredElements;
        if (parameters.Get ("filteredElements", filteredElements)) {
            elementsPtr.reset (new GS::Array<API_Guid> (filteredElements.Transform<API_Guid> (GetGuidFromElementsArrayItem)));
            saveParsIfc.elementsSet = elementsPtr.get ();
        } else {
            return CreateFailedExecutionResult (APIERR_BADPARS, "filteredElements parameter is required when elementsToExport is FilteredElements");
        }
    } else {
        return CreateFailedExecutionResult (APIERR_BADPARS, "elementsToExport parameter is invalid");
    }

    IO::Location ifcFileLocation (ifcFilePath);
    API_FileSavePars fileSavePars = {};
    fileSavePars.fileTypeID = APIFType_IfcFile;
    fileSavePars.file = &ifcFileLocation;
    
    const GSErrCode err = ACAPI_ProjectOperation_Save (&fileSavePars, &saveParsIfc);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to execute the IFC operation");
    }

    return CreateSuccessfulExecutionResult ();
    */

    API_IFCTranslatorIdentifier ifcTranslatorIdentifier = {};
    GS::UniString translator;
    if (parameters.Get ("translator", translator)) {
        GS::Array<API_IFCTranslatorIdentifier> ifcExportTranslators;
        ACAPI_IFC_GetIFCExportTranslatorsList (ifcExportTranslators);

        for (const auto& translatorIdentifier : ifcExportTranslators) {
            if (translatorIdentifier.name == translator) {
                ifcTranslatorIdentifier = translatorIdentifier;
                break;
            }
        }
        if (ifcTranslatorIdentifier.innerReference == nullptr) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "Not found IFC export translator with the given name");
        }
    }

    API_ElementsToIfcExportID elementsToIfcExport = API_EntireProject;
    std::unique_ptr<GS::Array<API_Guid>> elementsPtr;
    GS::UniString elementsToExport;
    if (parameters.Get ("elementsToExport", elementsToExport)) {
        if (elementsToExport == "EntireProject") {
            elementsToIfcExport = API_EntireProject;
        } else if (elementsToExport == "VisibleElementsOnAllStories") {
            elementsToIfcExport = API_VisibleElementsOnAllStories;
        } else if (elementsToExport == "AllElementsOnCurrentStorey") {
            elementsToIfcExport = API_AllElementsOnCurrentStorey;
        } else if (elementsToExport == "AllElementsOnCurrentStorey") {
            elementsToIfcExport = API_AllElementsOnCurrentStorey;
        } else if (elementsToExport == "VisibleElementsOnCurrentStorey") {
            elementsToIfcExport = API_VisibleElementsOnCurrentStorey;
        } else if (elementsToExport == "SelectedElementsOnly") {
            elementsToIfcExport = API_SelectedElementsOnly;
        } else if (elementsToExport == "FilteredElements") {
            elementsToIfcExport = API_FilteredElements;
            GS::Array<GS::ObjectState> filteredElements;
            if (parameters.Get ("filteredElements", filteredElements)) {
                elementsPtr.reset (new GS::Array<API_Guid> (filteredElements.Transform<API_Guid> (GetGuidFromElementsArrayItem)));
            } else {
                return CreateFailedExecutionResult (APIERR_BADPARS, "filteredElements parameter is required when elementsToExport is FilteredElements");
            }
        } else {
            return CreateFailedExecutionResult (APIERR_BADPARS, "elementsToExport parameter is invalid");
        }
    }

    GSGuidHolder translatorGuidHolder;
    IFCTranslatorInnerHandleToGuidHolder (ifcTranslatorIdentifier.innerReference, translatorGuidHolder);

    GS::Pair<API_ElementsToIfcExportID, const GS::Array<GS::Guid>*> elementsConfig;
    elementsConfig.first = elementsToIfcExport;
    elementsConfig.second = (elementsPtr ? reinterpret_cast<const GS::Array<GS::Guid>*>(elementsPtr.get ()) : nullptr);
    GS::Pair<GSGuidHolder, GS::Pair<API_ElementsToIfcExportID, const GS::Array<GS::Guid>*>> configParams = {translatorGuidHolder, elementsConfig};
    struct DummyData {
        char data[512];
    };
    GS::Ref<DummyData> resultRef = nullptr;
    API_ModulID moduleID = { 1198731108, 138575850 };
    const GSErrCode err = ACAPI_AddOnAddOnCommunication_Call (&moduleID, 'CDID', 1, reinterpret_cast<GSHandle>(&configParams), reinterpret_cast<GSPtr>(&resultRef), true);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to execute the IFC operation");
    }

    return ExecuteIFCIOHiddenCommand (IO_SAVEAS, parameters);
}

MergeIFCCommand::MergeIFCCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String MergeIFCCommand::GetName () const
{
    return "MergeIFC";
}

GS::Optional<GS::UniString> MergeIFCCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "ifcFilePath": {
                "type": "string",
                "description": "The target IFC file to use."
            }
        },
        "additionalProperties": false,
        "required": [
            "ifcFilePath"
        ]
    })";
}

GS::Optional<GS::UniString> MergeIFCCommand::GetResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState MergeIFCCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    return ExecuteIFCIOHiddenCommand (IO_MERGE, parameters);
}

OpenIFCCommand::OpenIFCCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String OpenIFCCommand::GetName () const
{
    return "OpenIFC";
}

GS::Optional<GS::UniString> OpenIFCCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "ifcFilePath": {
                "type": "string",
                "description": "The target IFC file to use."
            }
        },
        "additionalProperties": false,
        "required": [
            "ifcFilePath"
        ]
    })";
}

GS::Optional<GS::UniString> OpenIFCCommand::GetResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState OpenIFCCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    return ExecuteIFCIOHiddenCommand (IO_OPEN, parameters);
}
