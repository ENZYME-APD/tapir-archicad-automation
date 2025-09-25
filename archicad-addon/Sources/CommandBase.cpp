#include "CommandBase.hpp"

#include "SchemaDefinitions.hpp"
#include "MigrationHelper.hpp"

constexpr double EPS = 0.001;
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

bool   IsSame3DCoordinate (const API_Coord3D& c1, const API_Coord3D& c2)
{
    return std::abs (c1.x - c2.x) < EPS && std::abs (c1.y - c2.y) < EPS && std::abs (c1.z - c2.z) < EPS;
}

bool   IsSame2DCoordinate (const API_Coord& c1, const API_Coord& c2)
{
    return std::abs (c1.x - c2.x) < EPS && std::abs (c1.y - c2.y) < EPS;
}

bool   IsSame2DCoordinate (const GS::ObjectState& o1, const GS::ObjectState& o2)
{
    return IsSame2DCoordinate (Get2DCoordinateFromObjectState (o1), Get2DCoordinateFromObjectState (o2));
}

bool   IsSame3DCoordinate (const GS::ObjectState& o1, const GS::ObjectState& o2)
{
    return IsSame3DCoordinate (Get3DCoordinateFromObjectState (o1), Get3DCoordinateFromObjectState (o2));
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

GS::ObjectState Create3DCoordinateObjectState (const API_Coord3D& c)
{
    return GS::ObjectState ("x", c.x, "y", c.y, "z", c.z);
}

GS::ObjectState CreatePolyArcObjectState (const API_PolyArc& a)
{
    return GS::ObjectState ("begIndex", a.begIndex - 1, "endIndex", a.endIndex - 1, "arcAngle", a.arcAngle);
}

std::vector<PolygonData> GetPolygonsFromMemoCoords (const API_Guid& elemGuid, bool includeZCoords)
{
    API_ElementMemo memo = {};
    const GS::OnExit guard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
    const UInt64 mask = includeZCoords ? APIMemoMask_Polygon | APIMemoMask_MeshPolyZ : APIMemoMask_Polygon;
    if (ACAPI_Element_GetMemo (elemGuid, &memo, mask) != NoError || memo.coords == nullptr) {
        return {};
    }

    const GSSize nPolys = memo.pends == nullptr
        ? 1
        : BMhGetSize (reinterpret_cast<GSHandle> (memo.pends)) / sizeof (Int32) - 1;
    std::vector<std::pair<GS::Int32, GS::Int32>> startEndIndices;
    startEndIndices.reserve (nPolys);
    std::vector<PolygonData> polygons (nPolys);
    Int32 startIndex = 1;
    for (GSIndex iPoly = 0; iPoly < nPolys; ++iPoly) {
        Int32 endIndex = memo.pends == nullptr
            ? (BMhGetSize (reinterpret_cast<GSHandle> (memo.coords)) / sizeof (API_Coord)) - 1
            : (*memo.pends)[iPoly + 1];
        startEndIndices.emplace_back (startIndex, endIndex);
        std::vector<API_Coord>& coords = polygons[iPoly].coords;
        coords.reserve (endIndex - startIndex + 1);
        for (GSIndex iCoord = startIndex; iCoord <= endIndex; ++iCoord) {
            coords.push_back ((*memo.coords)[iCoord]);
        }
        if (includeZCoords) {
            std::vector<double>& zCoords = polygons[iPoly].zCoords;
            zCoords.reserve (endIndex - startIndex + 1);
            for (GSIndex iCoord = startIndex; iCoord <= endIndex; ++iCoord) {
                zCoords.push_back ((*memo.meshPolyZ)[iCoord]);
            }
        }
        startIndex = endIndex + 1;
    }

    const GSSize nArcs = BMhGetSize (reinterpret_cast<GSHandle> (memo.parcs)) / sizeof (API_PolyArc);
    for (GSIndex iArc = 0; iArc < nArcs; ++iArc) {
        API_PolyArc& arc = (*memo.parcs)[iArc];
        GSIndex iPoly = 0;
        for (; iPoly < nPolys; ++iPoly) {
            const auto& startEndPair = startEndIndices[iPoly];
            if (arc.begIndex >= startEndPair.first && arc.endIndex <= startEndPair.second) {
                arc.begIndex -= startEndPair.first - 1;
                arc.endIndex -= startEndPair.first - 1;
                break;
            }
        }
        polygons[iPoly].arcs.push_back (arc);
    }

    return polygons;
}

void AddPolygonFromMemoCoords (const API_Guid& elemGuid, GS::ObjectState& os, const GS::String& coordsFieldName, const GS::Optional<GS::String>& arcsFieldName)
{
    const auto& coords = os.AddList<GS::ObjectState> (coordsFieldName);

    const auto polygons = GetPolygonsFromMemoCoords (elemGuid);
    if (polygons.empty ()) {
        return;
    }

    for (const auto& coord : polygons[0].coords) {
        coords (Create2DCoordinateObjectState (coord));
    }

    if (arcsFieldName.HasValue () && !polygons[0].arcs.empty ()) {
        const auto& arcs = os.AddList<GS::ObjectState> (*arcsFieldName);

        for (const auto& arc : polygons[0].arcs) {
            arcs (CreatePolyArcObjectState (arc));
        }
    }
}

void AddPolygonWithHolesFromMemoCoords (const API_Guid& elemGuid, GS::ObjectState& os, const GS::String& coordsFieldName, const GS::Optional<GS::String>& arcsFieldName, const GS::String& holesArrayFieldName, const GS::String& holeCoordsFieldName, const GS::Optional<GS::String>& holeArcsFieldName, bool includeZCoords)
{
    const auto& coords = os.AddList<GS::ObjectState> (coordsFieldName);
    const auto& holes = os.AddList<GS::ObjectState> (holesArrayFieldName);

    const auto polygons = GetPolygonsFromMemoCoords (elemGuid, includeZCoords);
    if (polygons.empty ()) {
        return;
    }

    if (includeZCoords) {
        for (size_t i = 0; i < polygons[0].coords.size (); ++i) {
            coords (Create3DCoordinateObjectState (API_Coord3D {
                polygons[0].coords[i].x,
                polygons[0].coords[i].y,
                polygons[0].zCoords[i]
            }));
        }
    } else {
        for (const auto& coord : polygons[0].coords) {
            coords (Create2DCoordinateObjectState (coord));
        }
    }

    if (arcsFieldName.HasValue () && !polygons[0].arcs.empty ()) {
        const auto& arcs = os.AddList<GS::ObjectState> (*arcsFieldName);

        for (const auto& arc : polygons[0].arcs) {
            arcs (CreatePolyArcObjectState (arc));
        }
    }

    for (size_t i = 1; i < polygons.size (); ++i) {
        GS::ObjectState hole;
        const auto& holeCoords = hole.AddList<GS::ObjectState> (holeCoordsFieldName);
        if (includeZCoords) {
            for (size_t i = 0; i < polygons[i].coords.size (); ++i) {
                holeCoords (Create3DCoordinateObjectState (API_Coord3D {
                    polygons[i].coords[i].x,
                    polygons[i].coords[i].y,
                    polygons[i].zCoords[i]
                }));
            }
        } else {
            for (const auto& coord : polygons[i].coords) {
                holeCoords (Create2DCoordinateObjectState (coord));
            }
        }

        if (holeArcsFieldName.HasValue () && !polygons[i].arcs.empty ()) {
            const auto& holeArcs = hole.AddList<GS::ObjectState> (*holeArcsFieldName);

            for (const auto& arc : polygons[i].arcs) {
                holeArcs (CreatePolyArcObjectState (arc));
            }
        }

        holes (hole);
    }
}

bool GetHoleGeometry (const GS::ObjectState& holeOs, GS::Array<GS::ObjectState>& outCoords, GS::Array<GS::ObjectState>& outArcs)
{
    if (!holeOs.Get ("polygonCoordinates", outCoords) && !holeOs.Get ("polygonOutline", outCoords)) { //support legacy polygonCoordinates key
        return false;
    }
    holeOs.Get ("polygonArcs", outArcs);

    if (!outCoords.IsEmpty () && IsSame2DCoordinate (outCoords.GetFirst (), outCoords.GetLast ())) {
        outCoords.Pop ();
    }
    return true;
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

API_RGBColor GetColorFromObjectState (const GS::ObjectState& objectState)
{
    API_RGBColor color = {};
    objectState.Get ("red", color.f_red);
    objectState.Get ("green", color.f_green);
    objectState.Get ("blue", color.f_blue);
    return color;
}

bool GetColor (const GS::ObjectState& objectState, const GS::String& fieldName, API_RGBColor& outColor)
{
    GS::ObjectState colorOS;
    if (!objectState.Get (fieldName, colorOS)) {
        return false;
    }

    outColor = GetColorFromObjectState(colorOS);

    return true;
}

Stories GetStories ()
{
    Stories stories;
    API_StoryInfo storyInfo = {};

    GSErrCode err = ACAPI_ProjectSetting_GetStorySettings (&storyInfo);

    if (err == NoError) {
        const short numberOfStories = storyInfo.lastStory - storyInfo.firstStory + 1;
        for (short i = 0; i < numberOfStories; ++i) {
            const Story story = { (*storyInfo.data)[i].index, (*storyInfo.data)[i].level };
            stories.emplace ((*storyInfo.data)[i].index, story);
        }
        BMKillHandle ((GSHandle*) &storyInfo.data);
    }

    return stories;
}

GS::Pair<short, double> GetFloorIndexAndOffset (const double zPos, const Stories& stories)
{
    const Story* storyPtr = nullptr;
    for (const auto& kv : stories) {
        const Story& story = kv.second;
        if (story.level > zPos) {
            break;
        }
        storyPtr = &story;
    }

    if (storyPtr == nullptr) {
        return { 0, zPos };
    }

    return { storyPtr->index, zPos - storyPtr->level };
}

double GetZPos (const short floorIndex, const double offset, const Stories& stories)
{
    if (stories.empty ()) {
        return offset;
    }

    auto it = stories.find (floorIndex);
    if (it == stories.end ()) {
        return offset;
    }

    const Story& story = it->second;
    return story.level + offset;
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

API_ElemTypeID GetElementTypeFromNonLocalizedName (const GS::UniString& typeStr)
{
    if (typeStr == "Wall") return API_WallID;
    if (typeStr == "Column") return API_ColumnID;
    if (typeStr == "Beam") return API_BeamID;
    if (typeStr == "Window") return API_WindowID;
    if (typeStr == "Door") return API_DoorID;
    if (typeStr == "Object") return API_ObjectID;
    if (typeStr == "Lamp") return API_LampID;
    if (typeStr == "Slab") return API_SlabID;
    if (typeStr == "Roof") return API_RoofID;
    if (typeStr == "Mesh") return API_MeshID;
    if (typeStr == "Dimension") return API_DimensionID;
    if (typeStr == "RadialDimension") return API_RadialDimensionID;
    if (typeStr == "LevelDimension") return API_LevelDimensionID;
    if (typeStr == "AngleDimension") return API_AngleDimensionID;
    if (typeStr == "Text") return API_TextID;
    if (typeStr == "Label") return API_LabelID;
    if (typeStr == "Zone") return API_ZoneID;
    if (typeStr == "Hatch") return API_HatchID;
    if (typeStr == "Line") return API_LineID;
    if (typeStr == "PolyLine") return API_PolyLineID;
    if (typeStr == "Arc") return API_ArcID;
    if (typeStr == "Circle") return API_CircleID;
    if (typeStr == "Spline") return API_SplineID;
    if (typeStr == "Hotspot") return API_HotspotID;
    if (typeStr == "CutPlane") return API_CutPlaneID;
    if (typeStr == "Camera") return API_CameraID;
    if (typeStr == "CamSet") return API_CamSetID;
    if (typeStr == "Group") return API_GroupID;
    if (typeStr == "SectElem") return API_SectElemID;
    if (typeStr == "Drawing") return API_DrawingID;
    if (typeStr == "Picture") return API_PictureID;
    if (typeStr == "Detail") return API_DetailID;
    if (typeStr == "Elevation") return API_ElevationID;
    if (typeStr == "InteriorElevation") return API_InteriorElevationID;
    if (typeStr == "Worksheet") return API_WorksheetID;
    if (typeStr == "Hotlink") return API_HotlinkID;
    if (typeStr == "CurtainWall") return API_CurtainWallID;
    if (typeStr == "CurtainWallSegment") return API_CurtainWallSegmentID;
    if (typeStr == "CurtainWallFrame") return API_CurtainWallFrameID;
    if (typeStr == "CurtainWallPanel") return API_CurtainWallPanelID;
    if (typeStr == "CurtainWallJunction") return API_CurtainWallJunctionID;
    if (typeStr == "CurtainWallAccessory") return API_CurtainWallAccessoryID;
    if (typeStr == "Shell") return API_ShellID;
    if (typeStr == "Skylight") return API_SkylightID;
    if (typeStr == "Morph") return API_MorphID;
    if (typeStr == "ChangeMarker") return API_ChangeMarkerID;
    if (typeStr == "Stair") return API_StairID;
    if (typeStr == "Riser") return API_RiserID;
    if (typeStr == "Tread") return API_TreadID;
    if (typeStr == "StairStructure") return API_StairStructureID;
    if (typeStr == "Railing") return API_RailingID;
    if (typeStr == "RailingToprail") return API_RailingToprailID;
    if (typeStr == "RailingHandrail") return API_RailingHandrailID;
    if (typeStr == "RailingRail") return API_RailingRailID;
    if (typeStr == "RailingPost") return API_RailingPostID;
    if (typeStr == "RailingInnerPost") return API_RailingInnerPostID;
    if (typeStr == "RailingBaluster") return API_RailingBalusterID;
    if (typeStr == "RailingPanel") return API_RailingPanelID;
    if (typeStr == "RailingSegment") return API_RailingSegmentID;
    if (typeStr == "RailingNode") return API_RailingNodeID;
    if (typeStr == "RailingBalusterSet") return API_RailingBalusterSetID;
    if (typeStr == "RailingPattern") return API_RailingPatternID;
    if (typeStr == "RailingToprailEnd") return API_RailingToprailEndID;
    if (typeStr == "RailingHandrailEnd") return API_RailingHandrailEndID;
    if (typeStr == "RailingRailEnd") return API_RailingRailEndID;
    if (typeStr == "RailingToprailConnection") return API_RailingToprailConnectionID;
    if (typeStr == "RailingHandrailConnection") return API_RailingHandrailConnectionID;
    if (typeStr == "RailingRailConnection") return API_RailingRailConnectionID;
    if (typeStr == "RailingEndFinish") return API_RailingEndFinishID;
    if (typeStr == "BeamSegment") return API_BeamSegmentID;
    if (typeStr == "ColumnSegment") return API_ColumnSegmentID;
    if (typeStr == "Opening") return API_OpeningID;
    return API_ZombieElemID;
}

API_Guid GetAttributeGuidFromIndex (API_AttrTypeID typeID, API_AttributeIndex index)
{
    API_Attribute attr = {};
    attr.header.typeID = typeID;
    attr.header.index = index;

    if (ACAPI_Attribute_Get (&attr) != NoError) {
        return APINULLGuid;
    }

    return attr.header.guid;
}

API_Attr_Head GetAttributeHeadFromGuid (API_Guid guid)
{
    API_Attr_Head attrHead = {};
    attrHead.guid = guid;

    if (ACAPI_Attribute_Search (&attrHead) != NoError) {
        attrHead.typeID = API_ZombieAttrID;
        attrHead.index = ACAPI_CreateAttributeIndex (-1);
    }

    return attrHead;
}

API_AttributeIndex GetAttributeIndexFromGuid (API_AttrTypeID typeID, API_Guid guid)
{
    API_Attribute attr = {};
    attr.header.typeID = typeID;
    attr.header.guid = guid;

    if (ACAPI_Attribute_Get (&attr) != NoError) {
        return ACAPI_CreateAttributeIndex (-1);
    }

    return attr.header.index;
}

const DatabaseIdResolver& DatabaseIdResolver::Instance ()
{
    static DatabaseIdResolver instance;
    return instance;
}

API_Guid DatabaseIdResolver::GetIdOfDatabase (const API_DatabaseInfo& database) const
{
    if (databaseTypeToIdTable.ContainsLeftKey (database.typeID)) {
        return databaseTypeToIdTable.GetRight (database.typeID);
    }
    return database.databaseUnId.elemSetId;
}

API_DatabaseInfo DatabaseIdResolver::GetDatabaseWithId (const API_Guid& id) const
{
    API_DatabaseInfo db = {};
    if (databaseTypeToIdTable.ContainsRightKey (id)) {
        db.typeID = databaseTypeToIdTable.GetLeft (id);
    } else {
        db.databaseUnId.elemSetId = id;
    }
    return db;
}

DatabaseIdResolver::DatabaseIdResolver ()
    : databaseTypeToIdTable ({
        {APIWind_FloorPlanID, APIGuidFromString ("d5d16dd4-093f-4674-895d-410d634d8c7e")},
        {APIWind_3DModelID, APIGuidFromString ("f6a45617-c97c-44c8-9b98-25dbf98c35f3")}
    })
{
}

GSErrCode ExecuteActionForEachDatabase (
    const GS::Array<API_Guid>& databaseIds,
    const std::function<GSErrCode ()>& action,
    const std::function<void ()>& actionSuccess,
    const std::function<void (GSErrCode, const GS::UniString&)>& actionFailure)
{
    API_DatabaseInfo startingDatabase;
    GSErrCode err = ACAPI_Database_GetCurrentDatabase (&startingDatabase);
    if (err != NoError) {
        return err;
    }
    for (const API_Guid databaseId : databaseIds) {
        API_DatabaseInfo targetDbInfo = DatabaseIdResolver::Instance ().GetDatabaseWithId (databaseId);
        err = ACAPI_Window_GetDatabaseInfo (&targetDbInfo);
        if (err != NoError) {
            actionFailure (err, "Failed to get database info");
            continue;
        }
        err = ACAPI_Database_ChangeCurrentDatabase (&targetDbInfo);
        if (err != NoError) {
            actionFailure (err, "Failed to switch to database");
            continue;
        }
        err = action ();
        if (err == NoError) {
            actionSuccess ();
        }
        else {
            actionFailure (err, "Failed to execute command for this database.");
        }
    }

    err = ACAPI_Database_ChangeCurrentDatabase (&startingDatabase);
    if (err != NoError) {
        return err;
    }
    return NoError;
}