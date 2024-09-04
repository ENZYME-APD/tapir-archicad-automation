#include "CommandBase.hpp"

#include "SchemaDefinitions.hpp"
#include "MigrationHelper.hpp"

constexpr const char* CommandNamespace = "TapirCommand";

CommandBase::CommandBase (CommonSchema commonSchema) :
    mCommonSchema (commonSchema)
{
}

GS::String CommandBase::GetNamespace () const
{
    return CommandNamespace;
}

API_AddOnCommandExecutionPolicy CommandBase::GetExecutionPolicy () const
{
    return API_AddOnCommandExecutionPolicy::ScheduleForExecutionOnMainThread;
}

void CommandBase::OnResponseValidationFailed (const GS::ObjectState& /*response*/) const
{

}

#ifdef ServerMainVers_2600
bool CommandBase::IsProcessWindowVisible () const
{
    return false;
}
#endif

GS::Optional<GS::UniString> CommandBase::GetSchemaDefinitions () const
{
    if (mCommonSchema == CommonSchema::Used) {
        return GetCommonSchemaDefinitions ();
    } else {
        return {};
    }
}

GS::Optional<GS::UniString> CommandBase::GetInputParametersSchema () const
{
    return {};
}

GS::Optional<GS::UniString> CommandBase::GetResponseSchema () const
{
    return {};
}

GS::ObjectState CreateErrorResponse (GSErrCode errorCode, const GS::UniString& errorMessage)
{
    GS::ObjectState error;
    error.Add ("code", errorCode);
    error.Add ("message", errorMessage.ToCStr ().Get ());
    return GS::ObjectState ("error", error);
}

GS::ObjectState CreateFailedExecutionResult (GSErrCode errorCode, const GS::UniString& errorMessage)
{
    GS::ObjectState error = CreateErrorResponse (errorCode, errorMessage);
    error.Add ("success", false);
    return error;
}

GS::ObjectState CreateSuccessfulExecutionResult ()
{
    return GS::ObjectState (
        "success", true);
}

API_Guid GetGuidFromObjectState (const GS::ObjectState& os)
{
	GS::String guid;
	if (!os.Get ("guid", guid)) {
        return APINULLGuid;
    }
	return APIGuidFromString (guid.ToCStr ());
}

API_Guid GetGuidFromArrayItem (const GS::String& idFieldName, const GS::ObjectState& os)
{
    GS::ObjectState idField;
    if (!os.Get (idFieldName, idField)) {
        return APINULLGuid;
    }
	return GetGuidFromObjectState (idField);
}

API_Coord Get2DCoordinateFromObjectState (const GS::ObjectState& objectState)
{
    API_Coord coordinate = {};
    objectState.Get ("x", coordinate.x);
    objectState.Get ("y", coordinate.y);
    return coordinate;
}

GS::ObjectState Create2DCoordinateObjectState (const API_Coord& c)
{
    return GS::ObjectState ("x", c.x, "y", c.y);
}

GS::ObjectState CreateIdObjectState (const GS::String& idFieldName, const API_Guid& guid)
{
    return GS::ObjectState (idFieldName, CreateGuidObjectState (guid));
}

API_Coord3D Get3DCoordinateFromObjectState (const GS::ObjectState& objectState)
{
    API_Coord3D coordinate = {};
    objectState.Get ("x", coordinate.x);
    objectState.Get ("y", coordinate.y);
    objectState.Get ("z", coordinate.z);
    return coordinate;
}

Stories GetStories ()
{
    Stories stories;
    API_StoryInfo storyInfo = {};

    GSErrCode err = ACAPI_ProjectSetting_GetStorySettings (&storyInfo);

    if (err == NoError) {
        const short numberOfStories = storyInfo.lastStory - storyInfo.firstStory + 1;
        for (short i = 0; i < numberOfStories; ++i) {
            stories.PushNew ((*storyInfo.data)[i].index, (*storyInfo.data)[i].level);
        }
        BMKillHandle ((GSHandle*) &storyInfo.data);
    }

    return stories;
}

GS::Pair<short, double> GetFloorIndexAndOffset (const double zPos, const Stories& stories)
{
    if (stories.IsEmpty ()) {
        return { 0, zPos };
    }

    const Story* storyPtr = &stories[0];
    for (const auto& story : stories) {
        if (story.level > zPos) {
            break;
        }
        storyPtr = &story;
    }

    return { storyPtr->index, zPos - storyPtr->level };
}

GS::UniString GetElementTypeNonLocalizedName (API_ElemTypeID typeID)
{
    switch (typeID) {
        case API_WallID: return "Wall";
        case API_ColumnID: return "Column";
        case API_BeamID: return "Beam";
        case API_WindowID: return "Window";
        case API_DoorID: return "Door";
        case API_ObjectID: return "Object";
        case API_LampID: return "Lamp";
        case API_SlabID: return "Slab";
        case API_RoofID: return "Roof";
        case API_MeshID: return "Mesh";
        case API_DimensionID: return "Dimension";
        case API_RadialDimensionID: return "RadialDimension";
        case API_LevelDimensionID: return "LevelDimension";
        case API_AngleDimensionID: return "AngleDimension";
        case API_TextID: return "Text";
        case API_LabelID: return "Label";
        case API_ZoneID: return "Zone";
        case API_HatchID: return "Hatch";
        case API_LineID: return "Line";
        case API_PolyLineID: return "PolyLine";
        case API_ArcID: return "Arc";
        case API_CircleID: return "Circle";
        case API_SplineID: return "Spline";
        case API_HotspotID: return "Hotspot";
        case API_CutPlaneID: return "CutPlane";
        case API_CameraID: return "Camera";
        case API_CamSetID: return "CamSet";
        case API_GroupID: return "Group";
        case API_SectElemID: return "SectElem";
        case API_DrawingID: return "Drawing";
        case API_PictureID: return "Picture";
        case API_DetailID: return "Detail";
        case API_ElevationID: return "Elevation";
        case API_InteriorElevationID: return "InteriorElevation";
        case API_WorksheetID: return "Worksheet";
        case API_HotlinkID: return "Hotlink";
        case API_CurtainWallID: return "CurtainWall";
        case API_CurtainWallSegmentID: return "CurtainWallSegment";
        case API_CurtainWallFrameID: return "CurtainWallFrame";
        case API_CurtainWallPanelID: return "CurtainWallPanel";
        case API_CurtainWallJunctionID: return "CurtainWallJunction";
        case API_CurtainWallAccessoryID: return "CurtainWallAccessory";
        case API_ShellID: return "Shell";
        case API_SkylightID: return "Skylight";
        case API_MorphID: return "Morph";
        case API_ChangeMarkerID: return "ChangeMarker";
        case API_StairID: return "Stair";
        case API_RiserID: return "Riser";
        case API_TreadID: return "Tread";
        case API_StairStructureID: return "StairStructure";
        case API_RailingID: return "Railing";
        case API_RailingToprailID: return "RailingToprail";
        case API_RailingHandrailID: return "RailingHandrail";
        case API_RailingRailID: return "RailingRail";
        case API_RailingPostID: return "RailingPost";
        case API_RailingInnerPostID: return "RailingInnerPost";
        case API_RailingBalusterID: return "RailingBaluster";
        case API_RailingPanelID: return "RailingPanel";
        case API_RailingSegmentID: return "RailingSegment";
        case API_RailingNodeID: return "RailingNode";
        case API_RailingBalusterSetID: return "RailingBalusterSet";
        case API_RailingPatternID: return "RailingPattern";
        case API_RailingToprailEndID: return "RailingToprailEnd";
        case API_RailingHandrailEndID: return "RailingHandrailEnd";
        case API_RailingRailEndID: return "RailingRailEnd";
        case API_RailingToprailConnectionID: return "RailingToprailConnection";
        case API_RailingHandrailConnectionID: return "RailingHandrailConnection";
        case API_RailingRailConnectionID: return "RailingRailConnection";
        case API_RailingEndFinishID: return "RailingEndFinish";
        case API_BeamSegmentID: return "BeamSegment";
        case API_ColumnSegmentID: return "ColumnSegment";
        case API_OpeningID: return "Opening";
        default: return "Unknown";
    }
}