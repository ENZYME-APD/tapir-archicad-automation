#include "ElementFilletCommands.hpp"
#include "MigrationHelper.hpp"
#include "NotificationCommands.hpp"

#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------------
// FilletElements
//
// Rounds (fillet) or cuts off (chamfer) the corners of the polygon of a slab, zone, mesh or fill,
// or the corners of a polyline. Each corner is replaced by two points, where a circle of the given
// radius touches the two edges of the corner, joined by an arc (fillet) or a straight edge
// (chamfer). The new point is added with Archicad's own polygon editing function
// (ACAPI_Polygon_InsertPolyNode), as the DevKit's Element_Modify_Polygon.cpp example and
// ModifySlabs do it, so every other point keeps its vertex ID; then the points, the arcs and the
// data of each edge (slab edge trims and side surfaces) or point (mesh heights) are written for
// the new polygon and the element is changed with ACAPI_Element_ChangeMemo. The element is not
// recreated, so it keeps every setting, its layer, classifications and properties.
// ---------------------------------------------------------------------------

namespace {

constexpr double FilletPi = 3.14159265358979323846;

// A length below this (in meters) is no length: an edge must keep at least this much.
constexpr double FilletTolerance = 1e-4;

// The ACAPI_Polygon_* functions do not exist before Archicad 27; AC25/26 reach the same function
// through ACAPI_Goodies (see TAPIR_Polygon_InsertPolyNode in ExtendedElementCommands.cpp).
#ifdef ServerMainVers_2700
GSErrCode InsertFilletPolyNode (API_ElementMemo* memo, Int32* nodeIndex, API_Coord* coord)
{
    return ACAPI_Polygon_InsertPolyNode (memo, nodeIndex, coord);
}
#else
GSErrCode InsertFilletPolyNode (API_ElementMemo* memo, Int32* nodeIndex, API_Coord* coord)
{
    return ACAPI_Goodies (APIAny_InsertPolyNodeID, memo, nodeIndex, coord);
}
#endif

// One point of a contour, and the edge that starts at it.
struct FilletVertex {
    API_Coord coord;
    double    z;            // the height of a mesh point, 0 for the other types
    Int32     edgeSource;   // the memo index of the old point whose edge's data (slab edge trim and side surface) this edge takes
    double    arcAngle;     // the arc angle of the edge, 0 for a straight one
};

// One contour of the polygon (the outline, then the holes) or the polyline. The first point is not
// repeated at the end. A polygon has one edge per point, a polyline one less.
struct FilletContour {
    Int32                   firstIndex;   // the memo index of the first point
    GS::Array<FilletVertex> vertices;
};

// Where a circle of the radius touches the two edges of a corner, and the edge that joins them.
struct FilletCorner {
    API_Coord begin;      // on the edge before the corner
    API_Coord end;        // on the edge after the corner
    double    arcAngle;   // 0 for a chamfer
    double    setback;    // the distance of begin and end from the corner
};

bool IsFilletType (API_ElemTypeID typeID)
{
    return typeID == API_SlabID || typeID == API_ZoneID || typeID == API_MeshID || typeID == API_HatchID || typeID == API_PolyLineID;
}

double GetFilletDistance (const API_Coord& a, const API_Coord& b)
{
    return std::sqrt ((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
}

// The arc angle of the edge from memo index a to b (an arc may be stored in either direction, and
// the closing edge of a contour may end at the contour's first index), 0 for a straight edge.
double GetFilletArcAngle (const GS::Array<API_PolyArc>& arcs, Int32 a, Int32 b, Int32 otherB)
{
    for (const API_PolyArc& arc : arcs) {
        if (arc.begIndex == a && (arc.endIndex == b || arc.endIndex == otherB)) {
            return arc.arcAngle;
        }
        if (arc.endIndex == a && (arc.begIndex == b || arc.begIndex == otherB)) {
            return -arc.arcAngle;
        }
    }
    return 0.0;
}

// Reads the contours of the element's polygon or polyline from its memo.
bool GetFilletContours (const API_ElementMemo& memo, bool isPolyline, bool hasZ, GS::Array<FilletContour>& contours)
{
    if (memo.coords == nullptr || memo.pends == nullptr) {
        return false;
    }
    const Int32 nCoords = (Int32) (BMGetHandleSize ((GSHandle) memo.coords) / sizeof (API_Coord)) - 1;
    const Int32 nSubPolys = (Int32) (BMGetHandleSize ((GSHandle) memo.pends) / sizeof (Int32)) - 1;
    const Int32 nZ = (hasZ && memo.meshPolyZ != nullptr) ? (Int32) (BMGetHandleSize ((GSHandle) memo.meshPolyZ) / sizeof (double)) - 1 : 0;
    if (nSubPolys < 1 || (hasZ && nZ < nCoords)) {
        return false;
    }

    GS::Array<API_PolyArc> arcs;
    const Int32 nArcs = memo.parcs == nullptr ? 0 : (Int32) (BMGetHandleSize ((GSHandle) memo.parcs) / sizeof (API_PolyArc));
    for (Int32 i = 0; i < nArcs; ++i) {
        arcs.Push ((*memo.parcs)[i]);
    }

    for (Int32 subPoly = 1; subPoly <= nSubPolys; ++subPoly) {
        const Int32 first = (*memo.pends)[subPoly - 1] + 1;
        const Int32 last = (*memo.pends)[subPoly];
        // A polygon's contour ends with a point that repeats its first one.
        const Int32 n = isPolyline ? last - first + 1 : last - first;
        if (last > nCoords || n < (isPolyline ? 2 : 3)) {
            return false;
        }

        FilletContour contour;
        contour.firstIndex = first;
        for (Int32 j = 0; j < n; ++j) {
            const Int32 index = first + j;
            FilletVertex vertex = {};
            vertex.coord = (*memo.coords)[index];
            vertex.z = hasZ ? (*memo.meshPolyZ)[index] : 0.0;
            vertex.edgeSource = index;
            const bool hasEdge = !isPolyline || j < n - 1;
            vertex.arcAngle = hasEdge ? GetFilletArcAngle (arcs, index, index + 1, (!isPolyline && j == n - 1) ? first : index + 1) : 0.0;
            contour.vertices.Push (vertex);
        }
        contours.Push (contour);
    }
    return true;
}

// Calculates the fillet or chamfer of the corner at 'corner' between the straight edges from
// 'prev' and to 'next'. Returns why the corner cannot be filleted, or nothing.
GS::Optional<GS::UniString> CalculateFilletCorner (const API_Coord& prev, const API_Coord& corner, const API_Coord& next, double radius, bool chamfer, FilletCorner& result)
{
    const double prevLength = GetFilletDistance (corner, prev);
    const double nextLength = GetFilletDistance (corner, next);
    if (prevLength < FilletTolerance || nextLength < FilletTolerance) {
        return GS::UniString ("one of its edges has no length");
    }
    const API_Coord u = { (prev.x - corner.x) / prevLength, (prev.y - corner.y) / prevLength };
    const API_Coord w = { (next.x - corner.x) / nextLength, (next.y - corner.y) / nextLength };

    const double cosAngle = std::max (-1.0, std::min (1.0, u.x * w.x + u.y * w.y));
    const double sinAngle = u.x * w.y - u.y * w.x;
    if (std::fabs (sinAngle) < 1e-6) {
        return GS::UniString ("its two edges are in line");
    }

    // The angle between the two edges (0 to pi), and how far from the corner a circle of the
    // radius touches them.
    const double angle = std::acos (cosAngle);
    result.setback = radius / std::tan (angle / 2.0);
    result.begin = { corner.x + u.x * result.setback, corner.y + u.y * result.setback };
    result.end = { corner.x + w.x * result.setback, corner.y + w.y * result.setback };

    // The arc turns as the edges do: the path prev -> corner -> next turns left when sinAngle is
    // negative, and an arc that turns left lies on the right-hand side of its chord, which is a
    // positive arc angle (API_PolyArc).
    const double turn = FilletPi - angle;
    result.arcAngle = chamfer ? 0.0 : (sinAngle < 0.0 ? turn : -turn);
    return GS::NoValue;
}

// The height on the straight mesh edge from a to b at the given distance from a.
double InterpolateFilletHeight (const FilletVertex& a, const FilletVertex& b, double distanceFromA)
{
    const double length = GetFilletDistance (a.coord, b.coord);
    if (length < FilletTolerance) {
        return a.z;
    }
    return a.z + (b.z - a.z) * (distanceFromA / length);
}

// Allocates the memo handles ACAPI_Polygon_InsertPolyNode needs ("coords, pends, parcs and
// vertexIDs must be initialized"); ACAPI_Element_GetMemo may leave vertexIDs and parcs null,
// as ApplySlabPolygonChange (ExtendedElementCommands.cpp) found.
void InitFilletMemoHandles (API_ElementMemo& memo)
{
    if (memo.vertexIDs == nullptr) {
        const Int32 nCoords = (Int32) (BMGetHandleSize ((GSHandle) memo.coords) / sizeof (API_Coord)) - 1;
        memo.vertexIDs = reinterpret_cast<UInt32**> (BMAllocateHandle ((nCoords + 1) * sizeof (UInt32), ALLOCATE_CLEAR, 0));
    }
    if (memo.parcs == nullptr) {
        memo.parcs = reinterpret_cast<API_PolyArc**> (BMAllocateHandle (0, ALLOCATE_CLEAR, 0));
    }
}

// Gives the memo the new contours. The points are added with ACAPI_Polygon_InsertPolyNode, one
// after each filleted corner (the last ones first, so the memo indices of the others stay), then
// every point, arc and edge or point data is written from newContours. Returns an error text,
// or nothing.
GS::Optional<GS::UniString> WriteFilletContours (
    API_ElementMemo&                 memo,
    API_ElemTypeID                   typeID,
    const API_Element&               element,
    const GS::Array<FilletContour>&  oldContours,
    const GS::Array<FilletContour>&  newContours,
    const GS::Array<GS::Array<bool>>& filleted)
{
    const bool isPolyline = typeID == API_PolyLineID;

    // The edge data of the old polygon, read before anything is changed.
    GS::Array<API_EdgeTrim> oldTrims;
    GS::Array<API_OverriddenAttribute> oldSideMaterials;
    if (typeID == API_SlabID) {
        const GSSize nTrims = memo.edgeTrims == nullptr ? 0 : BMGetHandleSize ((GSHandle) memo.edgeTrims) / sizeof (API_EdgeTrim);
        for (GSSize i = 0; i < nTrims; ++i) {
            oldTrims.Push ((*memo.edgeTrims)[i]);
        }
        const GSSize nSideMaterials = memo.sideMaterials == nullptr ? 0 : BMGetPtrSize ((GSPtr) memo.sideMaterials) / sizeof (API_OverriddenAttribute);
        for (GSSize i = 0; i < nSideMaterials; ++i) {
            oldSideMaterials.Push (memo.sideMaterials[i]);
        }
    }

    InitFilletMemoHandles (memo);
    if (memo.vertexIDs == nullptr || memo.parcs == nullptr) {
        return GS::UniString ("out of memory");
    }

    for (UIndex c = oldContours.GetSize (); c-- > 0;) {
        const FilletContour& contour = oldContours[c];
        const UIndex n = contour.vertices.GetSize ();
        for (UIndex j = n; j-- > 0;) {
            if (!filleted[c][j]) {
                continue;
            }
            // nodeIndex is the index the new point gets (see ApplySlabPolygonChange): right
            // after the corner, on the edge that leaves it.
            Int32 nodeIndex = contour.firstIndex + (Int32) j + 1;
            const API_Coord& next = contour.vertices[(j + 1) % n].coord;
            API_Coord coord = { (contour.vertices[j].coord.x + next.x) / 2.0, (contour.vertices[j].coord.y + next.y) / 2.0 };
            const GSErrCode err = InsertFilletPolyNode (&memo, &nodeIndex, &coord);
            if (err != NoError) {
                return GS::UniString ("Archicad could not add a point to the polygon");
            }
        }
    }

    // Check that the points went where they were meant to: each contour has its new point count.
    const Int32 nSubPolys = (Int32) (BMGetHandleSize ((GSHandle) memo.pends) / sizeof (Int32)) - 1;
    if (nSubPolys != (Int32) newContours.GetSize ()) {
        return GS::UniString ("Archicad changed the number of contours of the polygon");
    }
    Int32 expectedEnd = 0;
    for (Int32 c = 0; c < nSubPolys; ++c) {
        expectedEnd += (Int32) newContours[(UIndex) c].vertices.GetSize () + (isPolyline ? 0 : 1);
        if ((*memo.pends)[c + 1] != expectedEnd) {
            return GS::UniString ("Archicad did not add the new points where expected");
        }
    }
    const Int32 nCoords = expectedEnd;
    if ((Int32) (BMGetHandleSize ((GSHandle) memo.coords) / sizeof (API_Coord)) - 1 < nCoords) {
        return GS::UniString ("Archicad did not add the new points where expected");
    }

    // The points, and the arcs, rewritten for the whole polygon.
    GS::Array<API_PolyArc> arcs;
    Int32 index = 1;
    for (const FilletContour& contour : newContours) {
        const Int32 first = index;
        const Int32 n = (Int32) contour.vertices.GetSize ();
        for (Int32 j = 0; j < n; ++j) {
            (*memo.coords)[index] = contour.vertices[j].coord;
            const bool hasEdge = !isPolyline || j < n - 1;
            if (hasEdge && std::fabs (contour.vertices[j].arcAngle) > 1e-9) {
                API_PolyArc arc = {};
                arc.begIndex = index;
                arc.endIndex = index + 1;
                arc.arcAngle = contour.vertices[j].arcAngle;
                arcs.Push (arc);
            }
            ++index;
        }
        if (!isPolyline) {
            (*memo.coords)[index] = (*memo.coords)[first];
            ++index;
        }
    }

    // Replaced with a handle of the exact size, never resized in place (see ApplySlabPolygonChange).
    BMKillHandle (reinterpret_cast<GSHandle*> (&memo.parcs));
    memo.parcs = reinterpret_cast<API_PolyArc**> (BMAllocateHandle ((GSSize) arcs.GetSize () * sizeof (API_PolyArc), ALLOCATE_CLEAR, 0));
    if (memo.parcs == nullptr) {
        return GS::UniString ("out of memory");
    }
    for (UIndex i = 0; i < arcs.GetSize (); ++i) {
        (*memo.parcs)[i] = arcs[i];
    }

    if (typeID == API_MeshID) {
        if (memo.meshPolyZ != nullptr) {
            BMKillHandle (reinterpret_cast<GSHandle*> (&memo.meshPolyZ));
        }
        memo.meshPolyZ = reinterpret_cast<double**> (BMAllocateHandle ((nCoords + 1) * sizeof (double), ALLOCATE_CLEAR, 0));
        if (memo.meshPolyZ == nullptr) {
            return GS::UniString ("out of memory");
        }
        index = 1;
        for (const FilletContour& contour : newContours) {
            const Int32 first = index;
            for (const FilletVertex& vertex : contour.vertices) {
                (*memo.meshPolyZ)[index++] = vertex.z;
            }
            (*memo.meshPolyZ)[index++] = (*memo.meshPolyZ)[first];
        }
    }

    if (typeID == API_SlabID) {
        // Each edge takes the trim and side surface of the old edge it comes from; edgeTrims and
        // sideMaterials are not touched by ACAPI_Polygon_InsertPolyNode, and are allocated fresh,
        // never resized in place (#452, see ApplySlabPolygonChange).
        if (memo.edgeTrims != nullptr) {
            BMKillHandle (reinterpret_cast<GSHandle*> (&memo.edgeTrims));
        }
        memo.edgeTrims = reinterpret_cast<API_EdgeTrim**> (BMAllocateHandle ((nCoords + 1) * sizeof (API_EdgeTrim), ALLOCATE_CLEAR, 0));
        if (memo.sideMaterials != nullptr) {
            BMKillPtr (reinterpret_cast<GSPtr*> (&memo.sideMaterials));
        }
        memo.sideMaterials = reinterpret_cast<API_OverriddenAttribute*> (BMAllocatePtr ((nCoords + 1) * sizeof (API_OverriddenAttribute), ALLOCATE_CLEAR, 0));
        if (memo.edgeTrims == nullptr || memo.sideMaterials == nullptr) {
            return GS::UniString ("out of memory");
        }
        index = 1;
        for (const FilletContour& contour : newContours) {
            const Int32 first = index;
            for (const FilletVertex& vertex : contour.vertices) {
                const Int32 source = vertex.edgeSource;
                if (source >= 0 && source < (Int32) oldTrims.GetSize ()) {
                    (*memo.edgeTrims)[index] = oldTrims[source];
                } else {
                    (*memo.edgeTrims)[index].sideType = APIEdgeTrim_Vertical;
                }
                memo.sideMaterials[index] = (source >= 0 && source < (Int32) oldSideMaterials.GetSize ()) ? oldSideMaterials[source] : element.slab.sideMat;
                ++index;
            }
            // The closing point repeats the data of the first one, as AddPolyToMemo writes it.
            (*memo.edgeTrims)[index] = (*memo.edgeTrims)[first];
            memo.sideMaterials[index] = memo.sideMaterials[first];
            ++index;
        }
    }

    return GS::NoValue;
}

GS::UniString GetFilletCornerList (const GS::Array<Int32>& corners)
{
    GS::UniString list;
    for (UIndex i = 0; i < corners.GetSize (); ++i) {
        if (i > 0) {
            list += i + 1 == corners.GetSize () ? " and " : ", ";
        }
        list += GS::ValueToUniString (corners[i]);
    }
    return list;
}

GS::ObjectState FilletElement (const API_Element& element, double radius, bool chamfer, const GS::Optional<GS::Array<Int32>>& requestedCorners, ElementChangeReporter& changeReporter)
{
    const API_ElemTypeID typeID = GetElemTypeId (element.header);
    if (!IsFilletType (typeID)) {
        return CreateErrorResponse (APIERR_BADELEMENTTYPE, "Elements of type " + GetElementTypeNonLocalizedName (typeID) + " cannot be filleted or chamfered. Supported types: Slab, Zone, Mesh, Hatch (Fill), PolyLine.");
    }
    if (typeID == API_ZoneID && !element.zone.manual) {
        return CreateErrorResponse (APIERR_BADELEMENTTYPE, "The polygon of this zone is found from the walls and other elements around its reference point, so it cannot be filleted or chamfered. Only zones drawn as a polygon (manual zones) can.");
    }
    if (typeID == API_MeshID && element.mesh.levelLines.nSubLines > 0) {
        return CreateErrorResponse (APIERR_BADELEMENTTYPE, "Meshes with level lines or level points cannot be filleted or chamfered yet.");
    }

    API_ElementMemo memo = {};
    const GS::OnExit guard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
    GSErrCode err = ACAPI_Element_GetMemo (element.header.guid, &memo);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to read the polygon of the element.");
    }

    const bool isPolyline = typeID == API_PolyLineID;
    GS::Array<FilletContour> contours;
    if (!GetFilletContours (memo, isPolyline, typeID == API_MeshID, contours)) {
        return CreateErrorResponse (APIERR_GENERAL, "Failed to read the polygon of the element.");
    }
    if (isPolyline && contours.GetSize () > 1) {
        return CreateErrorResponse (APIERR_BADELEMENTTYPE, "Polylines with more than one contour cannot be filleted or chamfered yet.");
    }

    // Every corner, numbered over the contours (outline, then holes), and its fillet or why it
    // has none (an empty refusal means it can be filleted).
    GS::Array<GS::Array<FilletCorner>> cornerData;
    GS::Array<GS::Array<GS::UniString>> cornerRefusals;
    Int32 nCorners = 0;
    for (const FilletContour& contour : contours) {
        const UIndex n = contour.vertices.GetSize ();
        GS::Array<FilletCorner> data;
        GS::Array<GS::UniString> refusals;
        for (UIndex j = 0; j < n; ++j) {
            FilletCorner corner = {};
            GS::UniString refusal;
            if (isPolyline && (j == 0 || j == n - 1)) {
                refusal = "it is an end of the polyline";
            } else {
                const FilletVertex& prev = contour.vertices[(j + n - 1) % n];
                const FilletVertex& next = contour.vertices[(j + 1) % n];
                if (std::fabs (prev.arcAngle) > 1e-9 || std::fabs (contour.vertices[j].arcAngle) > 1e-9) {
                    refusal = "one of its edges is curved";
                } else {
                    const GS::Optional<GS::UniString> cornerError = CalculateFilletCorner (prev.coord, contour.vertices[j].coord, next.coord, radius, chamfer, corner);
                    if (cornerError.HasValue ()) {
                        refusal = *cornerError;
                    }
                }
            }
            data.Push (corner);
            refusals.Push (refusal);
        }
        cornerData.Push (data);
        cornerRefusals.Push (refusals);
        nCorners += (Int32) n;
    }

    // Which corners are filleted: the requested ones, each of which must be possible, or every
    // corner that can be.
    GS::Array<GS::Array<bool>> filleted;
    for (const FilletContour& contour : contours) {
        GS::Array<bool> flags;
        for (UIndex j = 0; j < contour.vertices.GetSize (); ++j) {
            flags.Push (false);
        }
        filleted.Push (flags);
    }
    auto locate = [&contours] (Int32 corner, UIndex& c, UIndex& j) {
        c = 0;
        while (corner >= (Int32) contours[c].vertices.GetSize ()) {
            corner -= (Int32) contours[c].vertices.GetSize ();
            ++c;
        }
        j = (UIndex) corner;
    };
    if (requestedCorners.HasValue ()) {
        for (const Int32 corner : *requestedCorners) {
            if (corner < 0 || corner >= nCorners) {
                return CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("Corner %d does not exist: the element has %d corners (0 to %d). Nothing was changed.", (int) corner, (int) nCorners, (int) nCorners - 1));
            }
            UIndex c = 0;
            UIndex j = 0;
            locate (corner, c, j);
            if (!cornerRefusals[c][j].IsEmpty ()) {
                return CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("Corner %d cannot be filleted or chamfered, because %T. Nothing was changed.", (int) corner, cornerRefusals[c][j].ToPrintf ()));
            }
            filleted[c][j] = true;
        }
    } else {
        for (UIndex c = 0; c < contours.GetSize (); ++c) {
            for (UIndex j = 0; j < contours[c].vertices.GetSize (); ++j) {
                filleted[c][j] = cornerRefusals[c][j].IsEmpty ();
            }
        }
    }

    // The fillets at the two ends of an edge must fit on it, and leave some of it.
    GS::Array<Int32> filletedCorners;
    Int32 cornerOffset = 0;
    for (UIndex c = 0; c < contours.GetSize (); ++c) {
        const FilletContour& contour = contours[c];
        const UIndex n = contour.vertices.GetSize ();
        for (UIndex j = 0; j < n; ++j) {
            if (filleted[c][j]) {
                filletedCorners.Push (cornerOffset + (Int32) j);
            }
            if (isPolyline && j == n - 1) {
                continue;
            }
            const UIndex k = (j + 1) % n;
            const double needed = (filleted[c][j] ? cornerData[c][j].setback : 0.0) + (filleted[c][k] ? cornerData[c][k].setback : 0.0);
            const double length = GetFilletDistance (contour.vertices[j].coord, contour.vertices[k].coord);
            if (needed > 0.0 && needed > length - FilletTolerance) {
                // In ascending order: k is 0 on the closing edge of a contour.
                GS::Array<Int32> tooLarge;
                if (filleted[c][k] && k < j) {
                    tooLarge.Push (cornerOffset + (Int32) k);
                }
                if (filleted[c][j]) {
                    tooLarge.Push (cornerOffset + (Int32) j);
                }
                if (filleted[c][k] && k > j) {
                    tooLarge.Push (cornerOffset + (Int32) k);
                }
                return CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("The radius is too large: corner%s %T need%s %.4f m of an edge that is %.4f m long. Use a smaller radius or fewer corners. Nothing was changed.",
                    tooLarge.GetSize () > 1 ? "s" : "", GetFilletCornerList (tooLarge).ToPrintf (), tooLarge.GetSize () > 1 ? "" : "s", needed, length));
            }
        }
        cornerOffset += (Int32) n;
    }

    GS::ObjectState result;
    const auto& resultCorners = result.AddList<Int32> ("filletedCorners");
    for (const Int32 corner : filletedCorners) {
        resultCorners (corner);
    }
    if (filletedCorners.IsEmpty ()) {
        return result;
    }

    // The new contours: each filleted corner becomes the point on the edge before it (whose edge is
    // the arc or the chamfer) and the point on the edge after it.
    GS::Array<FilletContour> newContours;
    for (UIndex c = 0; c < contours.GetSize (); ++c) {
        const FilletContour& contour = contours[c];
        const UIndex n = contour.vertices.GetSize ();
        FilletContour newContour;
        newContour.firstIndex = 0;
        for (UIndex j = 0; j < n; ++j) {
            const FilletVertex& vertex = contour.vertices[j];
            if (!filleted[c][j]) {
                newContour.vertices.Push (vertex);
                continue;
            }
            const FilletCorner& corner = cornerData[c][j];
            const FilletVertex& prev = contour.vertices[(j + n - 1) % n];
            const FilletVertex& next = contour.vertices[(j + 1) % n];

            FilletVertex begin = {};
            begin.coord = corner.begin;
            begin.z = InterpolateFilletHeight (vertex, prev, corner.setback);
            begin.edgeSource = prev.edgeSource;
            begin.arcAngle = corner.arcAngle;
            newContour.vertices.Push (begin);

            FilletVertex end = {};
            end.coord = corner.end;
            end.z = InterpolateFilletHeight (vertex, next, corner.setback);
            end.edgeSource = vertex.edgeSource;
            end.arcAngle = vertex.arcAngle;
            newContour.vertices.Push (end);
        }
        newContours.Push (newContour);
    }

    const GS::Optional<GS::UniString> writeError = WriteFilletContours (memo, typeID, element, contours, newContours, filleted);
    if (writeError.HasValue ()) {
        return CreateErrorResponse (APIERR_GENERAL, "Failed to fillet the element (" + *writeError + "). Nothing was changed.");
    }

    UInt64 memoMask = APIMemoMask_Polygon;
    if (typeID == API_SlabID) {
        memoMask |= APIMemoMask_EdgeTrims | APIMemoMask_SideMaterials;
    } else if (typeID == API_MeshID) {
        memoMask |= APIMemoMask_MeshPolyZ;
    }

    API_Guid elemGuid = element.header.guid;
    changeReporter.Add (elemGuid);
    err = ACAPI_Element_ChangeMemo (elemGuid, memoMask, &memo);
    if (err != NoError) {
        const GS::UniString reason = DescribeElementEditRefusal (err, element.header);
        return CreateErrorResponse (err, reason.IsEmpty ()
            ? GS::UniString ("Archicad did not accept the filleted polygon. Nothing was changed.")
            : "The element was not changed, because " + reason + ".");
    }
    changeReporter.MarkChanged (elemGuid);
    return result;
}

} // namespace

FilletElementsCommand::FilletElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String FilletElementsCommand::GetName () const
{
    return "FilletElements";
}

GS::Optional<GS::UniString> FilletElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements",
                "description": "The elements whose corners are rounded or cut off. Supported: slabs, zones drawn as a polygon (manual zones), meshes (without level lines or points), fills and polylines (one contour). Other types are reported with their type name."
            },
            "radius": {
                "type": "number",
                "exclusiveMinimum": 0,
                "description": "The radius of the fillet in meters. A chamfer cuts the corner off with a straight edge between the two points where a fillet of this radius would touch the edges."
            },
            "mode": {
                "type": "string",
                "enum": ["fillet", "chamfer"],
                "description": "fillet rounds each corner with an arc, chamfer cuts it off with a straight edge. Default is fillet."
            },
            "corners": {
                "type": "array",
                "description": "The corners to change, the same for every given element; default is every corner whose two edges are straight and not in line. A corner is the index of a point of the element's polygon: the points of the outline from 0, in the order GetDetailsOfElements lists them without the point that repeats the first one at the end, then the points of each hole continuing the count. The ends of a polyline are no corners. A given corner that cannot be changed (it does not exist, one of its edges is curved, its edges are in line) fails the element.",
                "items": {
                    "type": "integer",
                    "minimum": 0
                }
            }
        },
        "additionalProperties": false,
        "required": ["elements", "radius"]
    })";
}

GS::Optional<GS::UniString> FilletElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "results": {
                "type": "array",
                "description": "One item per given element, in the same order.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "description": "The element was changed (or left as it was when no corner could be changed).",
                            "properties": {
                                "filletedCorners": {
                                    "type": "array",
                                    "description": "The corners that were filleted or chamfered, numbered as in the element before the change. Each gave two points, so the numbers of the later points grew.",
                                    "items": {
                                        "type": "integer"
                                    }
                                }
                            },
                            "additionalProperties": false,
                            "required": ["filletedCorners"]
                        },
                        {
                            "$ref": "#/ErrorItem"
                        }
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": ["results"]
    })";
}

GS::ObjectState FilletElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    double radius = 0.0;
    parameters.Get ("radius", radius);
    if (!(radius > 0.0)) {
        return CreateErrorResponse (APIERR_BADPARS, "radius must be greater than 0.");
    }

    GS::UniString mode = "fillet";
    parameters.Get ("mode", mode);
    if (mode != "fillet" && mode != "chamfer") {
        return CreateErrorResponse (APIERR_BADPARS, "mode must be fillet or chamfer.");
    }
    const bool chamfer = mode == "chamfer";

    GS::Optional<GS::Array<Int32>> corners;
    if (parameters.Contains ("corners")) {
        GS::Array<Int32> requested;
        parameters.Get ("corners", requested);
        corners = requested;
    }

    GS::ObjectState response;
    const auto& results = response.AddList<GS::ObjectState> ("results");

    ElementChangeReporter changeReporter;

    const GSErrCode undoErr = ACAPI_CallUndoableCommand (chamfer ? "Chamfer Elements" : "Fillet Elements", [&] () -> GSErrCode {
        for (const GS::ObjectState& item : elements) {
            const API_Guid elemGuid = GetGuidFromElementsArrayItem (item);

            API_Element element = {};
            element.header.guid = elemGuid;
            if (elemGuid == APINULLGuid || ACAPI_Element_Get (&element) != NoError) {
                results (CreateErrorResponse (APIERR_BADID, "Element does not exist."));
                continue;
            }

            results (FilletElement (element, radius, chamfer, corners, changeReporter));
        }
        return NoError;
    });

    if (undoErr != NoError) {
        return CreateErrorResponse (undoErr, "Failed to open the undo step of the fillet.");
    }

    changeReporter.Report ();

    return response;
}
