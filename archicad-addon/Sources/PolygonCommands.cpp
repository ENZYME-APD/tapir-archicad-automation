#include "PolygonCommands.hpp"

#include <algorithm>

namespace {

// The new-style ACAPI_Polygon_* functions do not exist before Archicad 27; there the same
// operations are reached through the untyped ACAPI_Goodies dispatcher with the APIAny_*ID
// constants, the same split ExtendedElementCommands.cpp makes for ACAPI_Polygon_InsertPolyNode.
#ifdef ServerMainVers_2700
GSErrCode TAPIR_Polygon_RegularizePolygon (const API_RegularizedPoly* polygon, Int32* nResult, API_RegularizedPoly*** regularizedPolygons)
{
    return ACAPI_Polygon_RegularizePolygon (polygon, nResult, regularizedPolygons);
}
GSErrCode TAPIR_Polygon_RegularizePolyline (const API_RegularizedPoly* polyline, API_RegularizedPoly* regularizedPolyline)
{
    return ACAPI_Polygon_RegularizePolyline (polyline, regularizedPolyline);
}
GSErrCode TAPIR_Polygon_DisposeRegularizedPoly (API_RegularizedPoly* regularizedPoly)
{
    return ACAPI_Polygon_DisposeRegularizedPoly (regularizedPoly);
}
#else
GSErrCode TAPIR_Polygon_RegularizePolygon (const API_RegularizedPoly* polygon, Int32* nResult, API_RegularizedPoly*** regularizedPolygons)
{
    return ACAPI_Goodies (APIAny_RegularizePolygonID, const_cast<API_RegularizedPoly*> (polygon), nResult, regularizedPolygons);
}
GSErrCode TAPIR_Polygon_RegularizePolyline (const API_RegularizedPoly* polyline, API_RegularizedPoly* regularizedPolyline)
{
    return ACAPI_Goodies (APIAny_RegularizePolylineID, const_cast<API_RegularizedPoly*> (polyline), regularizedPolyline);
}
GSErrCode TAPIR_Polygon_DisposeRegularizedPoly (API_RegularizedPoly* regularizedPoly)
{
    return ACAPI_Goodies (APIAny_DisposeRegularizedPolyID, regularizedPoly);
}
#endif

// Frees the handles this add-on allocated for an input polygon/polyline. The results of the
// regularization are freed with TAPIR_Polygon_DisposeRegularizedPoly instead.
void KillInputPolyHandles (API_RegularizedPoly& poly)
{
    if (poly.coords != nullptr) {
        BMKillHandle (reinterpret_cast<GSHandle*> (&poly.coords));
    }
    if (poly.pends != nullptr) {
        BMKillHandle (reinterpret_cast<GSHandle*> (&poly.pends));
    }
    if (poly.parcs != nullptr) {
        BMKillHandle (reinterpret_cast<GSHandle*> (&poly.parcs));
    }
}

template <typename T>
Int32 GetHandleItemCount (T** handle)
{
    return handle == nullptr ? 0 : (Int32) (BMhGetSize (reinterpret_cast<GSHandle> (handle)) / sizeof (T));
}

// Reads an ancestry value (a 1-based index into the input's coordinates, 0 for a new
// vertex/edge) for index i of the result, guarding against a handle shorter than expected.
Int32 GetAncestryValue (Int32** ancestry, Int32 i)
{
    if (ancestry == nullptr || i < 0 || i >= GetHandleItemCount (ancestry)) {
        return 0;
    }
    return (*ancestry)[i];
}

// Where an input coordinate (by its 1-based index in the coords handle sent to Archicad) came
// from in the request: the contour (0 = outline, k = hole k-1) and the vertex in it.
struct InputVertex {
    Int32 contourIndex;
    Int32 vertexIndex;
    bool  isClosingPoint;
};

GS::ObjectState CreateAncestryItem (Int32 ancestryValue, const std::vector<InputVertex>& inputVertices, bool forEdge)
{
    Int32 contourIndex = -1;
    Int32 index = -1;
    if (ancestryValue > 0 && ancestryValue < (Int32) inputVertices.size ()) {
        const InputVertex& inputVertex = inputVertices[ancestryValue];
        // The closing point duplicates the first vertex of its contour; no edge starts there.
        if (!(forEdge && inputVertex.isClosingPoint)) {
            contourIndex = inputVertex.contourIndex;
            index = inputVertex.isClosingPoint ? 0 : inputVertex.vertexIndex;
        }
    }
    return GS::ObjectState ("contourIndex", contourIndex, "index", index);
}

GS::Optional<GS::UniString> ValidateArcs (const GS::Array<GS::ObjectState>& arcs, Int32 nVertices, const GS::UniString& where)
{
    for (UIndex i = 0; i < arcs.GetSize (); ++i) {
        Int32 begIndex = -1;
        Int32 endIndex = -1;
        double arcAngle = 0.0;
        if (!arcs[i].Get ("begIndex", begIndex) || !arcs[i].Get ("endIndex", endIndex) || !arcs[i].Get ("arcAngle", arcAngle)) {
            return GS::UniString::Printf ("Invalid arc at index %d of %T: begIndex, endIndex and arcAngle are required.", (int) i, where.ToPrintf ());
        }
        // nVertices is allowed: it is the closing point, the end of the arc on the last edge.
        if (begIndex < 0 || begIndex > nVertices || endIndex < 0 || endIndex > nVertices) {
            return GS::UniString::Printf ("Invalid arc at index %d of %T: begIndex and endIndex must be between 0 and %d.", (int) i, where.ToPrintf (), (int) nVertices);
        }
    }
    return {};
}

void StripClosingPoint (GS::Array<GS::ObjectState>& coords)
{
    if (coords.GetSize () > 1 && IsSame2DCoordinate (coords.GetFirst (), coords.GetLast ())) {
        coords.Pop ();
    }
}

struct InputContour {
    GS::Array<GS::ObjectState> coords;
    GS::Array<GS::ObjectState> arcs;
};

// Builds the polygon to regularize the way Archicad stores a closed polygon in an element memo:
// coords[0] unused, every contour closed by a copy of its first point, pends[k] the index of the
// closing point of contour k, and arcs indexed into coords.
GS::Optional<GS::UniString> BuildInputPolygon (const GS::ObjectState& polygonOs, API_RegularizedPoly& poly, std::vector<InputVertex>& inputVertices)
{
    std::vector<InputContour> contours (1);
    polygonOs.Get ("polygonCoordinates", contours[0].coords);
    polygonOs.Get ("polygonArcs", contours[0].arcs);
    StripClosingPoint (contours[0].coords);
    if (contours[0].coords.GetSize () < 3) {
        return GS::UniString ("'polygonCoordinates' must contain at least 3 different coordinates.");
    }
    auto arcsError = ValidateArcs (contours[0].arcs, (Int32) contours[0].coords.GetSize (), "polygonArcs");
    if (arcsError.HasValue ()) {
        return arcsError;
    }

    GS::Array<GS::ObjectState> holes;
    polygonOs.Get ("holes", holes);
    auto holesError = ValidateHoles (holes);
    if (holesError.HasValue ()) {
        return holesError;
    }
    for (UIndex holeIndex = 0; holeIndex < holes.GetSize (); ++holeIndex) {
        InputContour hole;
        GetHoleGeometry (holes[holeIndex], hole.coords, hole.arcs);
        arcsError = ValidateArcs (hole.arcs, (Int32) hole.coords.GetSize (), GS::UniString::Printf ("the hole at index %d", (int) holeIndex));
        if (arcsError.HasValue ()) {
            return arcsError;
        }
        contours.push_back (hole);
    }

    Int32 nCoords = 0;
    Int32 nArcs = 0;
    for (const InputContour& contour : contours) {
        nCoords += (Int32) contour.coords.GetSize () + 1;
        nArcs += (Int32) contour.arcs.GetSize ();
    }

    poly.coords = reinterpret_cast<API_Coord**>   (BMAllocateHandle ((nCoords + 1) * sizeof (API_Coord), ALLOCATE_CLEAR, 0));
    poly.pends  = reinterpret_cast<Int32**>       (BMAllocateHandle (((Int32) contours.size () + 1) * sizeof (Int32), ALLOCATE_CLEAR, 0));
    poly.parcs  = reinterpret_cast<API_PolyArc**> (BMAllocateHandle (nArcs * sizeof (API_PolyArc), ALLOCATE_CLEAR, 0));
    if (poly.coords == nullptr || poly.pends == nullptr || poly.parcs == nullptr) {
        return GS::UniString ("Not enough memory for the polygon.");
    }

    inputVertices.assign (nCoords + 1, InputVertex { -1, -1, false });
    Int32 iCoord = 1;
    Int32 iArc = 0;
    (*poly.pends)[0] = 0;
    for (Int32 contourIndex = 0; contourIndex < (Int32) contours.size (); ++contourIndex) {
        const InputContour& contour = contours[contourIndex];
        const Int32 iStart = iCoord;
        for (Int32 vertexIndex = 0; vertexIndex < (Int32) contour.coords.GetSize (); ++vertexIndex) {
            (*poly.coords)[iCoord] = Get2DCoordinateFromObjectState (contour.coords[vertexIndex]);
            inputVertices[iCoord] = { contourIndex, vertexIndex, false };
            ++iCoord;
        }
        (*poly.coords)[iCoord] = (*poly.coords)[iStart];
        inputVertices[iCoord] = { contourIndex, (Int32) contour.coords.GetSize (), true };
        (*poly.pends)[contourIndex + 1] = iCoord;
        ++iCoord;

        for (const API_PolyArc& arc : GetPolyArcs (contour.arcs, iStart)) {
            (*poly.parcs)[iArc++] = arc;
        }
    }

    return {};
}

// Adds the coordinates (coords[iStart..iEnd), without the closing point at iEnd) and the arcs of
// one contour to os, the arcs indexed from the first vertex of the contour.
void AddRegularizedContour (const API_RegularizedPoly& poly, Int32 iStart, Int32 iEnd, const GS::String& coordsFieldName, GS::ObjectState& os)
{
    const auto& coords = os.AddList<GS::ObjectState> (coordsFieldName);
    for (Int32 i = iStart; i < iEnd; ++i) {
        coords (Create2DCoordinateObjectState ((*poly.coords)[i]));
    }

    std::vector<API_PolyArc> contourArcs;
    const Int32 nArcs = GetHandleItemCount (poly.parcs);
    for (Int32 iArc = 0; iArc < nArcs; ++iArc) {
        API_PolyArc arc = (*poly.parcs)[iArc];
        if (arc.begIndex >= iStart && arc.endIndex <= iEnd) {
            arc.begIndex -= iStart - 1;
            arc.endIndex -= iStart - 1;
            contourArcs.push_back (arc);
        }
    }
    if (!contourArcs.empty ()) {
        const auto& arcs = os.AddList<GS::ObjectState> ("polygonArcs");
        for (const API_PolyArc& arc : contourArcs) {
            arcs (CreatePolyArcObjectState (arc));
        }
    }
}

// One regularized polygon in the format of CreateSlabs: the first contour is the outline, the
// others are holes. The closing point is left out of every contour (the create commands close
// the contours themselves); an arc on the closing edge ends at the index one past the last
// vertex, as those commands expect it.
GS::ObjectState CreateRegularizedPolygonObjectState (const API_RegularizedPoly& poly, bool withAncestry, const std::vector<InputVertex>& inputVertices)
{
    GS::ObjectState polygonOs;
    const Int32 nCoordItems = GetHandleItemCount (poly.coords);
    const Int32 nContours = GetHandleItemCount (poly.pends) - 1;

    std::vector<std::pair<Int32, Int32>> contourRanges;
    Int32 iStart = 1;
    for (Int32 contourIndex = 0; contourIndex < nContours; ++contourIndex) {
        const Int32 iEnd = (*poly.pends)[contourIndex + 1];
        if (iEnd < iStart || iEnd >= nCoordItems) {
            break;
        }
        contourRanges.emplace_back (iStart, iEnd);
        iStart = iEnd + 1;
    }

    if (contourRanges.empty ()) {
        polygonOs.AddList<GS::ObjectState> ("polygonCoordinates");
    } else {
        AddRegularizedContour (poly, contourRanges[0].first, contourRanges[0].second, "polygonCoordinates", polygonOs);
    }
    const auto& holes = polygonOs.AddList<GS::ObjectState> ("holes");
    for (size_t contourIndex = 1; contourIndex < contourRanges.size (); ++contourIndex) {
        GS::ObjectState holeOs;
        AddRegularizedContour (poly, contourRanges[contourIndex].first, contourRanges[contourIndex].second, "polygonOutline", holeOs);
        holes (holeOs);
    }

    if (withAncestry) {
        GS::ObjectState ancestryOs;
        const auto& ancestryContours = ancestryOs.AddList<GS::ObjectState> ("contours");
        for (const auto& range : contourRanges) {
            GS::ObjectState contourAncestryOs;
            const auto& vertices = contourAncestryOs.AddList<GS::ObjectState> ("vertices");
            const auto& edges = contourAncestryOs.AddList<GS::ObjectState> ("edges");
            for (Int32 i = range.first; i < range.second; ++i) {
                vertices (CreateAncestryItem (GetAncestryValue (poly.vertexAncestry, i), inputVertices, false));
                edges (CreateAncestryItem (GetAncestryValue (poly.edgeAncestry, i), inputVertices, true));
            }
            ancestryContours (contourAncestryOs);
        }
        polygonOs.Add ("ancestry", ancestryOs);
    }
    return polygonOs;
}

GS::ObjectState RegularizePolygon (const GS::ObjectState& polygonOs, bool withAncestry)
{
    API_RegularizedPoly poly = {};
    const GS::OnExit guard ([&poly] () { KillInputPolyHandles (poly); });
    std::vector<InputVertex> inputVertices;
    auto inputError = BuildInputPolygon (polygonOs, poly, inputVertices);
    if (inputError.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, inputError.Get ());
    }
    poly.needVertexAncestry = withAncestry;
    poly.needEdgeAncestry = withAncestry;

    Int32 nResult = 0;
    API_RegularizedPoly** regularizedPolys = nullptr;
    const GSErrCode err = TAPIR_Polygon_RegularizePolygon (&poly, &nResult, &regularizedPolys);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to regularize the polygon.");
    }

    GS::ObjectState result;
    const auto& polygons = result.AddList<GS::ObjectState> ("polygons");
    if (regularizedPolys != nullptr) {
        const Int32 nPolys = std::min (nResult, GetHandleItemCount (regularizedPolys));
        for (Int32 i = 0; i < nPolys; ++i) {
            polygons (CreateRegularizedPolygonObjectState ((*regularizedPolys)[i], withAncestry, inputVertices));
        }
        for (Int32 i = 0; i < nPolys; ++i) {
            TAPIR_Polygon_DisposeRegularizedPoly (&(*regularizedPolys)[i]);
        }
        BMKillHandle (reinterpret_cast<GSHandle*> (&regularizedPolys));
    }
    return result;
}

// Builds the polyline to regularize the way Archicad stores a polyline in an element memo:
// coords[0] unused, coords[1..n] the points, a single contour ending at n, arcs indexed into coords.
GS::Optional<GS::UniString> BuildInputPolyline (const GS::ObjectState& polylineOs, API_RegularizedPoly& poly)
{
    GS::Array<GS::ObjectState> coordinates;
    GS::Array<GS::ObjectState> arcs;
    polylineOs.Get ("coordinates", coordinates);
    polylineOs.Get ("arcs", arcs);
    if (coordinates.GetSize () < 2) {
        return GS::UniString ("'coordinates' must contain at least 2 coordinates.");
    }
    const Int32 nCoords = (Int32) coordinates.GetSize ();
    auto arcsError = ValidateArcs (arcs, nCoords - 1, "arcs");
    if (arcsError.HasValue ()) {
        return arcsError;
    }

    const GS::Array<API_PolyArc> polyArcs = GetPolyArcs (arcs, 1);
    poly.coords = reinterpret_cast<API_Coord**>   (BMAllocateHandle ((nCoords + 1) * sizeof (API_Coord), ALLOCATE_CLEAR, 0));
    poly.pends  = reinterpret_cast<Int32**>       (BMAllocateHandle (2 * sizeof (Int32), ALLOCATE_CLEAR, 0));
    poly.parcs  = reinterpret_cast<API_PolyArc**> (BMAllocateHandle (polyArcs.GetSize () * sizeof (API_PolyArc), ALLOCATE_CLEAR, 0));
    if (poly.coords == nullptr || poly.pends == nullptr || poly.parcs == nullptr) {
        return GS::UniString ("Not enough memory for the polyline.");
    }

    for (Int32 i = 0; i < nCoords; ++i) {
        (*poly.coords)[i + 1] = Get2DCoordinateFromObjectState (coordinates[i]);
    }
    (*poly.pends)[0] = 0;
    (*poly.pends)[1] = nCoords;
    Int32 iArc = 0;
    for (const API_PolyArc& arc : polyArcs) {
        (*poly.parcs)[iArc++] = arc;
    }
    return {};
}

GS::ObjectState CreatePolylineAncestryItem (Int32 ancestryValue, Int32 nInputCoords, bool forEdge)
{
    // A 1-based index into the input points; for an edge, the index of its first point, so the
    // last point starts no edge.
    const Int32 lastValid = forEdge ? nInputCoords - 1 : nInputCoords;
    return GS::ObjectState ("index", ancestryValue > 0 && ancestryValue <= lastValid ? ancestryValue - 1 : -1);
}

GS::ObjectState RegularizePolyline (const GS::ObjectState& polylineOs, bool withAncestry)
{
    API_RegularizedPoly poly = {};
    const GS::OnExit guard ([&poly] () { KillInputPolyHandles (poly); });
    auto inputError = BuildInputPolyline (polylineOs, poly);
    if (inputError.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, inputError.Get ());
    }
    const Int32 nInputCoords = GetHandleItemCount (poly.coords) - 1;
    poly.needVertexAncestry = withAncestry;
    poly.needEdgeAncestry = withAncestry;

    API_RegularizedPoly regularized = {};
    const GSErrCode err = TAPIR_Polygon_RegularizePolyline (&poly, &regularized);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to regularize the polyline.");
    }

    GS::ObjectState result;
    const auto& coordinates = result.AddList<GS::ObjectState> ("coordinates");
    const auto& arcs = result.AddList<GS::ObjectState> ("arcs");

    // The result may be empty (no coords handle) when nothing valid is left of the polyline.
    const Int32 nCoordItems = GetHandleItemCount (regularized.coords);
    Int32 nCoords = nCoordItems - 1;
    if (GetHandleItemCount (regularized.pends) >= 2) {
        nCoords = std::min (nCoords, (*regularized.pends)[1]);
    }
    for (Int32 i = 1; i <= nCoords; ++i) {
        coordinates (Create2DCoordinateObjectState ((*regularized.coords)[i]));
    }
    const Int32 nArcs = GetHandleItemCount (regularized.parcs);
    for (Int32 iArc = 0; iArc < nArcs; ++iArc) {
        const API_PolyArc& arc = (*regularized.parcs)[iArc];
        if (arc.begIndex >= 1 && arc.endIndex <= nCoords) {
            arcs (CreatePolyArcObjectState (arc));
        }
    }

    if (withAncestry) {
        GS::ObjectState ancestryOs;
        const auto& vertices = ancestryOs.AddList<GS::ObjectState> ("vertices");
        const auto& edges = ancestryOs.AddList<GS::ObjectState> ("edges");
        for (Int32 i = 1; i <= nCoords; ++i) {
            vertices (CreatePolylineAncestryItem (GetAncestryValue (regularized.vertexAncestry, i), nInputCoords, false));
            if (i < nCoords) {
                edges (CreatePolylineAncestryItem (GetAncestryValue (regularized.edgeAncestry, i), nInputCoords, true));
            }
        }
        result.Add ("ancestry", ancestryOs);
    }

    TAPIR_Polygon_DisposeRegularizedPoly (&regularized);
    return result;
}

}

// ---------------------------------------------------------------------------
// RegularizePolygons
// ---------------------------------------------------------------------------

RegularizePolygonsCommand::RegularizePolygonsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String RegularizePolygonsCommand::GetName () const
{
    return "RegularizePolygons";
}

GS::Optional<GS::UniString> RegularizePolygonsCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "polygons": {
            "type": "array",
            "description": "The polygons to regularize, in the format of CreateSlabs (polygonCoordinates, polygonArcs, holes). For CreateHatches, pass the outline of a result as its coordinates and arcs.",
            "items": {
                "$ref": "#/PolygonRegularizationData"
            }
        },
        "withAncestry": {
            "type": "boolean",
            "description": "Also return for every vertex and edge of the results which input vertex or edge it came from. Optional, false by default."
        }
    },
    "additionalProperties": false,
    "required": [
        "polygons"
    ]
})";
}

GS::Optional<GS::UniString> RegularizePolygonsCommand::GetRawResponseSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "results": {
            "type": "array",
            "description": "One item per input polygon, in the order of the request.",
            "items": {
                "type": "object",
                "oneOf": [
                    {
                        "$ref": "#/RegularizedPolygons"
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

GS::ObjectState RegularizePolygonsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> polygons;
    parameters.Get ("polygons", polygons);
    bool withAncestry = false;
    parameters.Get ("withAncestry", withAncestry);

    GS::ObjectState response;
    const auto& results = response.AddList<GS::ObjectState> ("results");
    for (const GS::ObjectState& polygon : polygons) {
        results (RegularizePolygon (polygon, withAncestry));
    }
    return response;
}

// ---------------------------------------------------------------------------
// RegularizePolylines
// ---------------------------------------------------------------------------

RegularizePolylinesCommand::RegularizePolylinesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String RegularizePolylinesCommand::GetName () const
{
    return "RegularizePolylines";
}

GS::Optional<GS::UniString> RegularizePolylinesCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "polylines": {
            "type": "array",
            "description": "The polylines to regularize, in the format of CreatePolylines.",
            "items": {
                "$ref": "#/PolylineRegularizationData"
            }
        },
        "withAncestry": {
            "type": "boolean",
            "description": "Also return for every point and segment of the results which input point or segment it came from. Optional, false by default."
        }
    },
    "additionalProperties": false,
    "required": [
        "polylines"
    ]
})";
}

GS::Optional<GS::UniString> RegularizePolylinesCommand::GetRawResponseSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "results": {
            "type": "array",
            "description": "One item per input polyline, in the order of the request.",
            "items": {
                "type": "object",
                "oneOf": [
                    {
                        "$ref": "#/RegularizedPolyline"
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

GS::ObjectState RegularizePolylinesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> polylines;
    parameters.Get ("polylines", polylines);
    bool withAncestry = false;
    parameters.Get ("withAncestry", withAncestry);

    GS::ObjectState response;
    const auto& results = response.AddList<GS::ObjectState> ("results");
    for (const GS::ObjectState& polyline : polylines) {
        results (RegularizePolyline (polyline, withAncestry));
    }
    return response;
}
