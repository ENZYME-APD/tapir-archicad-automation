#include "Model3DCommands.hpp"
#include "MigrationHelper.hpp"

#ifdef ServerMainVers_2800

// Creates a sight and selects it, so that ACAPI_ModelAccess_GenerateModelWithSeparateComponents
// builds its model there instead of in the sight of the 3D window (which it refuses). The user's
// sight is selected again and the temporary one deleted when the guard goes out of scope, on
// every path, as the DevKit's ThermalBridge example does at its end.
class TemporarySight
{
public:
    TemporarySight ()
    {
        error = ACAPI_Sight_CreateSight (&sight);
        if (error != NoError) {
            sight = nullptr;
            return;
        }

        error = ACAPI_Sight_SelectSight (sight, &originalSight);
        if (error != NoError) {
            ACAPI_Sight_DeleteSight (sight);
            sight = nullptr;
        }
    }

    ~TemporarySight ()
    {
        if (sight == nullptr) {
            return;
        }

        void* replacedSight = nullptr;
        ACAPI_Sight_SelectSight (originalSight, &replacedSight);
        ACAPI_Sight_DeleteSight (sight);
    }

    TemporarySight (const TemporarySight&) = delete;
    TemporarySight& operator= (const TemporarySight&) = delete;

    GSErrCode GetError () const { return error; }

private:
    void*       sight = nullptr;
    void*       originalSight = nullptr;
    GSErrCode   error = NoError;
};

// The coordinates of a connection polygon are 2D in the polygon's plane; this is what
// Geometry::Plane::PlaneToWorld does in the DevKit's example, with the plane built from
// basePoint, axisX and axisY.
static API_Coord3D PlaneToWorld (const API_Plane3D& plane, const API_Coord& coord)
{
    API_Coord3D world = plane.basePoint;
    world.x += coord.x * plane.axisX.x + coord.y * plane.axisY.x;
    world.y += coord.x * plane.axisX.y + coord.y * plane.axisY.y;
    world.z += coord.x * plane.axisX.z + coord.y * plane.axisY.z;
    return world;
}

#endif

GetElementConnectionsCommand::GetElementConnectionsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetElementConnectionsCommand::GetName () const
{
    return "GetElementConnections";
}

GS::Optional<GS::UniString> GetElementConnectionsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements",
                "description": "The elements to search connections among. Only connections between two of these elements are returned."
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> GetElementConnectionsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "connections": {
                "type": "array",
                "description": "The connected pairs of the given elements. Each pair appears once.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId1": {
                            "$ref": "#/ElementId"
                        },
                        "elementId2": {
                            "$ref": "#/ElementId"
                        },
                        "polygons": {
                            "type": "array",
                            "description": "The polygons of the surface where the two elements touch, in world coordinates. The last point of a polygon repeats its first.",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "coordinates": {
                                        "type": "array",
                                        "items": {
                                            "$ref": "#/Coordinate3D"
                                        }
                                    }
                                },
                                "additionalProperties": false,
                                "required": [
                                    "coordinates"
                                ]
                            }
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId1",
                        "elementId2",
                        "polygons"
                    ]
                }
            },
            "executionResults": {
                "$ref": "#/ExecutionResults",
                "description": "Aligned with the input array of elements: whether the element was found and taken into the search."
            }
        },
        "additionalProperties": false,
        "required": [
            "connections",
            "executionResults"
        ]
    })";
}

GS::ObjectState GetElementConnectionsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    // An unknown guid would make GenerateModelWithSeparateComponents fail for all elements
    // (APIERR_BADID), so each element is checked first and only the found ones are passed on.
    GS::Array<API_Guid> elementGuids;
    GS::HashSet<API_Guid> elementGuidSet;
    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        API_Elem_Head elemHead = {};
        elemHead.guid = GetGuidFromObjectState (*elementId);
        const GSErrCode err = ACAPI_Element_GetHeader (&elemHead);
        if (err != NoError) {
            executionResults (CreateFailedExecutionResult (err, "Failed to find element in Archicad"));
            continue;
        }

        if (!elementGuidSet.Contains (elemHead.guid)) {
            elementGuidSet.Add (elemHead.guid);
            elementGuids.Push (elemHead.guid);
        }
        executionResults (CreateSuccessfulExecutionResult ());
    }

    const auto& connections = response.AddList<GS::ObjectState> ("connections");
    if (elementGuids.GetSize () < 2) {
        return response;
    }

    const TemporarySight temporarySight;
    if (temporarySight.GetError () != NoError) {
        return CreateErrorResponse (temporarySight.GetError (), "Failed to create a temporary sight for the 3D model");
    }

    GSErrCode err = ACAPI_ModelAccess_GenerateModelWithSeparateComponents (elementGuids);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to generate the 3D model of the elements");
    }

    API_ElementConnectionTable table;
    const GS::OnExit disposeMemos ([&table] () {
        for (GS::Array<API_PolygonExt>& polygonArray : table.Values ()) {
            for (API_PolygonExt& polygon : polygonArray) {
                ACAPI_DisposeElemMemoHdls (&polygon.polygonData);
            }
        }
    });

    err = ACAPI_ModelAccess_GetConnectionTable (elementGuidSet, &table);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the connections of the elements");
    }

    for (const auto& [elemPair, connectionPolygons] : table) {
        GS::ObjectState connection;
        connection.Add ("elementId1", CreateGuidObjectState (elemPair.first));
        connection.Add ("elementId2", CreateGuidObjectState (elemPair.second));

        const auto& polygons = connection.AddList<GS::ObjectState> ("polygons");
        for (const API_PolygonExt& polygon : connectionPolygons) {
            if (polygon.polygonData.coords == nullptr) {
                continue;
            }

            // Index 0 of the coordinates is unused and the last one repeats the first.
            GS::ObjectState polygonOS;
            const auto& coordinates = polygonOS.AddList<GS::ObjectState> ("coordinates");
            for (Int32 coordIdx = 1; coordIdx <= polygon.polygon.nCoords; ++coordIdx) {
                const API_Coord& coord = (*polygon.polygonData.coords)[coordIdx];
                coordinates (Create3DCoordinateObjectState (PlaneToWorld (polygon.plane, coord)));
            }
            polygons (polygonOS);
        }

        connections (connection);
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command is only supported in Archicad 28 or later.");
#endif
}
