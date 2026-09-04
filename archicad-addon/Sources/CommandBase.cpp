#include "CommandBase.hpp"

#include "SchemaDefinitions.hpp"
#include "MigrationHelper.hpp"

#include <cmath>

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

GS::Optional<GS::UniString> CommandBase::GetRawResponseSchema () const
{
    return {};
}

// Deliberately empty - see the comment on the declaration. The schema the command
// declares is documented, not handed to Archicad's response validation.
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

GS::UniString AnchorIdToString (API_AnchorID anchorId)
{
    switch (anchorId) {
        case APIAnc_LT: return "TopLeft";
        case APIAnc_MT: return "TopCenter";
        case APIAnc_RT: return "TopRight";
        case APIAnc_LM: return "MiddleLeft";
        case APIAnc_MM: return "Center";
        case APIAnc_RM: return "MiddleRight";
        case APIAnc_LB: return "BottomLeft";
        case APIAnc_MB: return "BottomCenter";
        case APIAnc_RB: return "BottomRight";
        default:        return "Center";
    }
}

API_AnchorID AnchorIdFromString (const GS::UniString& str, API_AnchorID defaultValue)
{
    if (str == "TopLeft")      return APIAnc_LT;
    if (str == "TopCenter")    return APIAnc_MT;
    if (str == "TopRight")     return APIAnc_RT;
    if (str == "MiddleLeft")   return APIAnc_LM;
    if (str == "Center")       return APIAnc_MM;
    if (str == "MiddleRight")  return APIAnc_RM;
    if (str == "BottomLeft")   return APIAnc_LB;
    if (str == "BottomCenter") return APIAnc_MB;
    if (str == "BottomRight")  return APIAnc_RB;
    return defaultValue;
}

GS::UniString WallReferenceLineLocationToString (API_WallReferenceLineLocationID location)
{
    switch (location) {
        case APIWallRefLine_Outside:     return "Outside";
        case APIWallRefLine_Center:      return "Center";
        case APIWallRefLine_Inside:      return "Inside";
        case APIWallRefLine_CoreOutside: return "CoreOutside";
        case APIWallRefLine_CoreCenter:  return "CoreCenter";
        case APIWallRefLine_CoreInside:  return "CoreInside";
        default:                         return "Outside";
    }
}

API_WallReferenceLineLocationID WallReferenceLineLocationFromString (const GS::UniString& str, API_WallReferenceLineLocationID defaultValue)
{
    if (str == "Outside")     return APIWallRefLine_Outside;
    if (str == "Center")      return APIWallRefLine_Center;
    if (str == "Inside")      return APIWallRefLine_Inside;
    if (str == "CoreOutside") return APIWallRefLine_CoreOutside;
    if (str == "CoreCenter")  return APIWallRefLine_CoreCenter;
    if (str == "CoreInside")  return APIWallRefLine_CoreInside;
    return defaultValue;
}

GS::UniString ZoneRelToString (API_ZoneRelID zoneRel)
{
    switch (zoneRel) {
        case APIZRel_Boundary:        return "Boundary";
        case APIZRel_ReduceArea:      return "ReduceArea";
        case APIZRel_None:            return "None";
#ifdef ServerMainVers_2700
        case APIZRel_SubtractFromZone:return "SubtractFromZone";
#endif
        default:                      return "Boundary";
    }
}

API_ZoneRelID ZoneRelFromString (const GS::UniString& str, API_ZoneRelID defaultValue)
{
    if (str == "Boundary")         return APIZRel_Boundary;
    if (str == "ReduceArea")       return APIZRel_ReduceArea;
    if (str == "None")             return APIZRel_None;
#ifdef ServerMainVers_2700
    if (str == "SubtractFromZone") return APIZRel_SubtractFromZone;
#endif
    return defaultValue;
}

GS::ObjectState CreateStoryVisibilityObjectState (const API_StoryVisibility& visibility)
{
    return GS::ObjectState (
        "showOnHome", visibility.showOnHome,
        "showAllAbove", visibility.showAllAbove,
        "showAllBelow", visibility.showAllBelow,
        "showRelAbove", visibility.showRelAbove,
        "showRelBelow", visibility.showRelBelow);
}

API_StoryVisibility GetStoryVisibilityFromObjectState (const GS::ObjectState& os)
{
    API_StoryVisibility visibility = {};
    os.Get ("showOnHome", visibility.showOnHome);
    os.Get ("showAllAbove", visibility.showAllAbove);
    os.Get ("showAllBelow", visibility.showAllBelow);
    short showRelAbove = 0;
    if (os.Get ("showRelAbove", showRelAbove)) {
        visibility.showRelAbove = showRelAbove;
    }
    short showRelBelow = 0;
    if (os.Get ("showRelBelow", showRelBelow)) {
        visibility.showRelBelow = showRelBelow;
    }
    return visibility;
}

GS::UniString SlabReferencePlaneLocationToString (API_SlabReferencePlaneLocationID location)
{
    switch (location) {
        case APISlabRefPlane_Top:        return "Top";
        case APISlabRefPlane_CoreTop:    return "CoreTop";
        case APISlabRefPlane_CoreBottom: return "CoreBottom";
        case APISlabRefPlane_Bottom:     return "Bottom";
        default:                         return "Top";
    }
}

API_SlabReferencePlaneLocationID SlabReferencePlaneLocationFromString (const GS::UniString& str, API_SlabReferencePlaneLocationID defaultValue)
{
    if (str == "Top")        return APISlabRefPlane_Top;
    if (str == "CoreTop")    return APISlabRefPlane_CoreTop;
    if (str == "CoreBottom") return APISlabRefPlane_CoreBottom;
    if (str == "Bottom")     return APISlabRefPlane_Bottom;
    return defaultValue;
}

GS::ObjectState CreateOverriddenMaterialObjectState (const API_OverriddenAttribute& attr)
{
#ifdef ServerMainVers_2700
    const bool overridden = attr.hasValue;
    const API_AttributeIndex index = attr.value;
#else
    const bool overridden = attr.overridden;
    const API_AttributeIndex index = attr.attributeIndex;
#endif
    GS::ObjectState os ("overridden", overridden);
    if (overridden) {
        os.Add ("attributeId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_MaterialID, index)));
    }
    return os;
}

API_OverriddenAttribute GetOverriddenMaterialFromObjectState (const GS::ObjectState& os)
{
    API_OverriddenAttribute attr = {};
    bool overridden = false;
    os.Get ("overridden", overridden);

    API_AttributeIndex index = {};
    GS::ObjectState attributeIdOs;
    if (overridden && os.Get ("attributeId", attributeIdOs)) {
        index = GetAttributeIndexFromGuid (API_MaterialID, GetGuidFromObjectState (attributeIdOs));
    }

#ifdef ServerMainVers_2700
    attr.hasValue = overridden;
    attr.value = index;
#else
    attr.overridden = overridden;
    attr.attributeIndex = index;
#endif
    return attr;
}

#ifdef ServerMainVers_2700
GS::ObjectState CreateOverriddenPenObjectState (const API_OverriddenPen& pen)
{
    GS::ObjectState os ("overridden", pen.hasValue);
    if (pen.hasValue) {
        os.Add ("penIndex", static_cast<Int32> (pen.value));
    }
    return os;
}

API_OverriddenPen GetOverriddenPenFromObjectState (const GS::ObjectState& os)
{
    API_OverriddenPen pen = {};
    bool overridden = false;
    os.Get ("overridden", overridden);
    Int32 penIndex = 0;
    if (overridden && os.Get ("penIndex", penIndex)) {
        pen = static_cast<API_PenIndex> (penIndex);
    } else {
        pen = APINullValue;
    }
    return pen;
}
#endif

GS::UniString CoverFillTransformationTypeToString (API_CoverFillTransformationTypeID type)
{
    switch (type) {
        case API_CoverFillTransformationType_Rotated:   return "Rotated";
        case API_CoverFillTransformationType_Distorted: return "Distorted";
        case API_CoverFillTransformationType_Global:
        default:                                        return "Global";
    }
}

API_CoverFillTransformationTypeID CoverFillTransformationTypeFromString (const GS::UniString& str, API_CoverFillTransformationTypeID defaultValue)
{
    if (str == "Rotated")   return API_CoverFillTransformationType_Rotated;
    if (str == "Distorted") return API_CoverFillTransformationType_Distorted;
    if (str == "Global")    return API_CoverFillTransformationType_Global;
    return defaultValue;
}

GS::ObjectState CreateCoverFillTransformationObjectState (const API_CoverFillTransformation& transformation)
{
    GS::ObjectState os;
    os.Add ("origin", Create2DCoordinateObjectState (transformation.origo));
    os.Add ("xAxis", GS::ObjectState ("x", transformation.xAxis.x, "y", transformation.xAxis.y));
    os.Add ("yAxis", GS::ObjectState ("x", transformation.yAxis.x, "y", transformation.yAxis.y));
    return os;
}

API_CoverFillTransformation GetCoverFillTransformationFromObjectState (const GS::ObjectState& os)
{
    API_CoverFillTransformation transformation = {};
    GS::ObjectState originOs;
    if (os.Get ("origin", originOs)) {
        transformation.origo = Get2DCoordinateFromObjectState (originOs);
    }
    GS::ObjectState xAxisOs;
    if (os.Get ("xAxis", xAxisOs)) {
        xAxisOs.Get ("x", transformation.xAxis.x);
        xAxisOs.Get ("y", transformation.xAxis.y);
    }
    GS::ObjectState yAxisOs;
    if (os.Get ("yAxis", yAxisOs)) {
        yAxisOs.Get ("x", transformation.yAxis.x);
        yAxisOs.Get ("y", transformation.yAxis.y);
    }
    return transformation;
}

GS::ObjectState CreateCoverFillObjectState (bool use, bool useFromSurface, bool orientationComesFrom3D, API_AttributeIndex fillIndex, short foregroundPen, short backgroundPen, API_CoverFillTransformationTypeID transformationType, const API_CoverFillTransformation& transformation)
{
    GS::ObjectState os;
    os.Add ("use", use);
    os.Add ("useFromSurface", useFromSurface);
    os.Add ("orientationComesFrom3D", orientationComesFrom3D);
    os.Add ("fillId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_FilltypeID, fillIndex)));
    os.Add ("foregroundPen", foregroundPen);
    os.Add ("backgroundPen", backgroundPen);
    os.Add ("transformationType", CoverFillTransformationTypeToString (transformationType));
    os.Add ("transformation", CreateCoverFillTransformationObjectState (transformation));
    return os;
}

GS::UniString HatchOrientationTypeToString (API_HatchOrientationTypeID type)
{
    switch (type) {
        case API_HatchRotated:   return "Rotated";
        case API_HatchDistorted: return "Distorted";
        case API_HatchCentered:  return "Centered";
        case API_HatchGlobal:
        default:                 return "Global";
    }
}

API_HatchOrientationTypeID HatchOrientationTypeFromString (const GS::UniString& str, API_HatchOrientationTypeID defaultValue)
{
    if (str == "Rotated")   return API_HatchRotated;
    if (str == "Distorted") return API_HatchDistorted;
    if (str == "Centered")  return API_HatchCentered;
    if (str == "Global")    return API_HatchGlobal;
    return defaultValue;
}

GS::ObjectState CreateHatchOrientationObjectState (const API_HatchOrientation& orientation)
{
    GS::ObjectState os;
    os.Add ("type", HatchOrientationTypeToString (orientation.type));
    os.Add ("origin", Create2DCoordinateObjectState (orientation.origo));
    os.Add ("matrix00", orientation.matrix00);
    os.Add ("matrix10", orientation.matrix10);
    os.Add ("matrix01", orientation.matrix01);
    os.Add ("matrix11", orientation.matrix11);
    os.Add ("innerRadius", orientation.innerRadius);
    return os;
}

API_HatchOrientation GetHatchOrientationFromObjectState (const GS::ObjectState& os)
{
    API_HatchOrientation orientation = {};
    GS::UniString typeStr;
    if (os.Get ("type", typeStr)) {
        orientation.type = HatchOrientationTypeFromString (typeStr);
    }
    GS::ObjectState originOs;
    if (os.Get ("origin", originOs)) {
        orientation.origo = Get2DCoordinateFromObjectState (originOs);
    }
    os.Get ("matrix00", orientation.matrix00);
    os.Get ("matrix10", orientation.matrix10);
    os.Get ("matrix01", orientation.matrix01);
    os.Get ("matrix11", orientation.matrix11);
    os.Get ("innerRadius", orientation.innerRadius);
    return orientation;
}

void AddBeamHolesFromMemo (const API_Guid& elemGuid, GS::ObjectState& os, const GS::String& holesFieldName)
{
    const auto& holes = os.AddList<GS::ObjectState> (holesFieldName);

    API_ElementMemo memo = {};
    const GS::OnExit guard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
    if (ACAPI_Element_GetMemo (elemGuid, &memo, APIMemoMask_BeamHole) != NoError || memo.beamHoles == nullptr) {
        return;
    }

    const GSSize nHoles = BMhGetSize (reinterpret_cast<GSHandle> (memo.beamHoles)) / sizeof (API_Beam_Hole);
    for (GSIndex i = 0; i < nHoles; ++i) {
        const API_Beam_Hole& hole = (*memo.beamHoles)[i];
        holes (GS::ObjectState (
            "holeId", hole.holeID,
            "type", hole.holeType == APIBHole_Circular ? GS::UniString ("Circular") : GS::UniString ("Rectangular"),
            "showContour", hole.holeContureOn,
            "centerX", hole.centerx,
            "centerZ", hole.centerz,
            "width", hole.width,
            "height", hole.height));
    }
}

void AddColumnSectionFromMemo (const API_Guid& elemGuid, GS::ObjectState& os)
{
    API_ElementMemo memo = {};
    const GS::OnExit guard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
    if (ACAPI_Element_GetMemo (elemGuid, &memo, APIMemoMask_ColumnSegment) != NoError || memo.columnSegments == nullptr) {
        return;
    }
    const GSSize nSegments = BMGetPtrSize (reinterpret_cast<GSPtr> (memo.columnSegments)) / sizeof (API_ColumnSegmentType);
    if (nSegments == 0) {
        return;
    }
    const API_AssemblySegmentData& segment = memo.columnSegments[0].assemblySegmentData;
    os.Add ("width", segment.nominalWidth);
    os.Add ("depth", segment.nominalHeight);
    os.Add ("isWidthAndHeightLinked", segment.isWidthAndHeightLinked);
    os.Add ("circleBased", segment.circleBased);
    if (segment.modelElemStructureType == API_BasicStructure) {
        os.Add ("buildingMaterialId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_BuildingMaterialID, segment.buildingMaterial)));
    } else if (segment.modelElemStructureType == API_ProfileStructure) {
        os.Add ("profileId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_ProfileID, segment.profileAttr)));
    }
}

void AddBeamSectionFromMemo (const API_Guid& elemGuid, GS::ObjectState& os)
{
    API_ElementMemo memo = {};
    const GS::OnExit guard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
    if (ACAPI_Element_GetMemo (elemGuid, &memo, APIMemoMask_BeamSegment) != NoError || memo.beamSegments == nullptr) {
        return;
    }
    const GSSize nSegments = BMGetPtrSize (reinterpret_cast<GSPtr> (memo.beamSegments)) / sizeof (API_BeamSegmentType);
    if (nSegments == 0) {
        return;
    }
    const API_AssemblySegmentData& segment = memo.beamSegments[0].assemblySegmentData;
    os.Add ("width", segment.nominalWidth);
    os.Add ("height", segment.nominalHeight);
    os.Add ("isWidthAndHeightLinked", segment.isWidthAndHeightLinked);
    if (segment.modelElemStructureType == API_BasicStructure) {
        os.Add ("buildingMaterialId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_BuildingMaterialID, segment.buildingMaterial)));
    } else if (segment.modelElemStructureType == API_ProfileStructure) {
        os.Add ("profileId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_ProfileID, segment.profileAttr)));
    }
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
            for (size_t j = 0; j < polygons[i].coords.size (); ++j) {
                holeCoords (Create3DCoordinateObjectState (API_Coord3D {
                    polygons[i].coords[j].x,
                    polygons[i].coords[j].y,
                    polygons[i].zCoords[j]
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

GS::Optional<GS::UniString> ValidateHoles (const GS::Array<GS::ObjectState>& holes)
{
    for (UIndex holeIndex = 0; holeIndex < holes.GetSize (); ++holeIndex) {
        GS::Array<GS::ObjectState> holeCoords;
        GS::Array<GS::ObjectState> holeArcs;
        if (!GetHoleGeometry (holes[holeIndex], holeCoords, holeArcs)) {
            return GS::UniString::Printf ("Invalid hole at index %d: each hole must be an object with a 'polygonOutline' (or legacy 'polygonCoordinates') coordinate array.", (int) holeIndex);
        }
        if (holeCoords.GetSize () < 3) {
            return GS::UniString::Printf ("Invalid hole at index %d: the hole outline must contain at least 3 coordinates.", (int) holeIndex);
        }
    }
    return {};
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

GS::Pair<short, double> ResolveFloorIndexAndOffset (const GS::ObjectState& parameters, const char* floorIndexFieldName, const double zPos, const Stories& stories)
{
    short floorIndex = 0;
    if (parameters.Get (floorIndexFieldName, floorIndex)) {
        return { floorIndex, zPos - GetZPos (floorIndex, 0.0, stories) };
    }
    return GetFloorIndexAndOffset (zPos, stories);
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

short ParseAnchorPointString (const GS::UniString& anchorPoint)
{
    return static_cast<short> (AnchorIdFromString (anchorPoint));
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

bool LoadElementHeaderByGuid (const API_Guid& elementGuid, API_Elem_Head& elementHeader)
{
    elementHeader = {};
    elementHeader.guid = elementGuid;
    return ACAPI_Element_GetHeader (&elementHeader) == NoError;
}

bool DoesElementExist (const API_Guid& elementGuid, API_ElemTypeID expectedTypeId)
{
    API_Elem_Head header = {};
    if (!LoadElementHeaderByGuid (elementGuid, header)) {
        return false;
    }
    return GetElemTypeId (header) == expectedTypeId;
}


API_Tranmat CreateHotlinkTransformation (const API_Coord3D& origin, double rotationAngle, bool mirrored)
{
    API_Tranmat transformation = {};
    const double co = std::cos (rotationAngle);
    const double si = std::sin (rotationAngle);
    const double mx = mirrored ? -1.0 : 1.0;

    transformation.tmx[0]  = co * mx;
    transformation.tmx[1]  = -si;
    transformation.tmx[2]  = 0.0;
    transformation.tmx[3]  = origin.x;

    transformation.tmx[4]  = si * mx;
    transformation.tmx[5]  = co;
    transformation.tmx[6]  = 0.0;
    transformation.tmx[7]  = origin.y;

    transformation.tmx[8]  = 0.0;
    transformation.tmx[9]  = 0.0;
    transformation.tmx[10] = 1.0;
    transformation.tmx[11] = origin.z;

    return transformation;
}

void DecomposeHotlinkTransformation (const API_Tranmat& transformation, API_Coord3D& origin, double& rotationAngle, bool& mirrored)
{
    origin = { transformation.tmx[3], transformation.tmx[7], transformation.tmx[11] };
    // The image of the local Y axis is the second column, (-sin, cos), and a
    // mirror of the local X axis leaves it alone - so the angle reads off it
    // regardless of mirroring, and the mirror reads off the determinant.
    rotationAngle = std::atan2 (-transformation.tmx[1], transformation.tmx[5]);
    const double det = transformation.tmx[0] * transformation.tmx[5] - transformation.tmx[1] * transformation.tmx[4];
    mirrored = det < 0.0;
}
