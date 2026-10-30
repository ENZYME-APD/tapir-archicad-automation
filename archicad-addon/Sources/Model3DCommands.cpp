#include "Model3DCommands.hpp"
#include "MigrationHelper.hpp"

#ifdef ServerMainVers_2800
#include "Model.hpp"
#include "ModelElement.hpp"
#include "Polygon2D.hpp"

#include <optional>

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

// The building material of one body of the separated-components model. elemIdx is the API index
// of the element (its ModelerAPI index minus 1, as the DevKit's ThermalBridge example says).
static GSErrCode CreateBodyMaterialObjectState (Int32 elemIdx, UInt32 bodyIdx, GS::ObjectState& bodyOS)
{
    API_AttributeIndex materialIdx;
    const GSErrCode err = ACAPI_ModelAccess_GetBuildingMaterial (elemIdx, bodyIdx, &materialIdx);
    if (err != NoError) {
        return err;
    }

    bodyOS.Add ("bodyIndex", static_cast<Int32> (bodyIdx));

    GS::UniString name;
    API_Attribute attr = {};
    attr.header.typeID = API_BuildingMaterialID;
    attr.header.index = materialIdx;
    attr.header.uniStringNamePtr = &name;
    if (ACAPI_Attribute_Get (&attr) == NoError) {
        bodyOS.Add ("buildingMaterialId", CreateAttributeIdObjectState (attr.header.guid));
        bodyOS.Add ("buildingMaterialName", name);
    }
    DisposeAttribute (attr);

    return NoError;
}

// The first contour of a Geometry::Polygon2D is its outline, the others are its holes. The
// coordinates are 2D in the cutting plane (axisX, axisY); curved edges are given by their ends.
static GS::ObjectState CreateCutPolygonObjectState (const Geometry::Polygon2D& polygon)
{
    GS::ObjectState polygonOS;
    const auto& outline = polygonOS.AddList<GS::ObjectState> ("polygonOutline");
    const auto& holes = polygonOS.AddList<GS::ObjectState> ("holes");

    bool isOutline = true;
    for (auto contourIt = polygon.BeginContour (); contourIt != polygon.EndContour (); ++contourIt) {
        if (isOutline) {
            for (auto vertexIt = contourIt->BeginVertex (); vertexIt != contourIt->EndVertex (); ++vertexIt) {
                const Point2D& coord = vertexIt->GetCoord ();
                outline (Create2DCoordinateObjectState (API_Coord { coord.x, coord.y }));
            }
            isOutline = false;
            continue;
        }

        GS::ObjectState holeOS;
        const auto& holeOutline = holeOS.AddList<GS::ObjectState> ("polygonOutline");
        for (auto vertexIt = contourIt->BeginVertex (); vertexIt != contourIt->EndVertex (); ++vertexIt) {
            const Point2D& coord = vertexIt->GetCoord ();
            holeOutline (Create2DCoordinateObjectState (API_Coord { coord.x, coord.y }));
        }
        holes (holeOS);
    }

    return polygonOS;
}

// The surface type of every polygon of every body of one element of the model. The element is read
// whole, because ACAPI_Polygon_PolygonId2SurfaceType needs the roofClass of a Roof.
static GS::ObjectState CreateSurfaceTypesOfElement (const API_Guid& elemGuid, const ModelerAPI::Model& model)
{
    API_Element element = {};
    element.header.guid = elemGuid;
    GSErrCode err = ACAPI_Element_Get (&element);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the element");
    }

    GS::ObjectState result;
    const auto& polygons = result.AddList<GS::ObjectState> ("polygons");

    const std::optional<Int32> modelIndex = model.GetElementIndex (APIGuid2GSGuid (elemGuid));
    if (!modelIndex.has_value ()) {
        return result;
    }

    ModelerAPI::Element modelElement;
    model.GetElement (*modelIndex, &modelElement);

    // ModelerAPI indexes the bodies and the polygons from 1; the output counts them from 0.
    const Int32 bodyCount = modelElement.GetTessellatedBodyCount ();
    for (Int32 bodyIdx = 1; bodyIdx <= bodyCount; ++bodyIdx) {
        ModelerAPI::MeshBody body;
        modelElement.GetTessellatedBody (bodyIdx, &body);

        const Int32 polygonCount = body.GetPolygonCount ();
        for (Int32 polygonIdx = 1; polygonIdx <= polygonCount; ++polygonIdx) {
            ModelerAPI::Polygon polygon;
            body.GetPolygon (polygonIdx, &polygon);

            const Int32 polygonId = static_cast<Int32> (polygon.GetPolygonId ());
            API_SurfaceTypeID surfaceType = {};
            err = ACAPI_Polygon_PolygonId2SurfaceType (&element, &polygonId, &surfaceType);
            if (err != NoError) {
                return CreateErrorResponse (err, "Failed to get the surface type of a polygon of the element");
            }

            GS::ObjectState polygonOS;
            polygonOS.Add ("bodyIndex", bodyIdx - 1);
            polygonOS.Add ("polygonIndex", polygonIdx - 1);
            polygonOS.Add ("polygonId", polygonId);
            polygonOS.Add ("surfaceType", static_cast<Int32> (surfaceType));
            polygons (polygonOS);
        }
    }

    return result;
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

GetBodyMaterialsOfElementsCommand::GetBodyMaterialsOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetBodyMaterialsOfElementsCommand::GetName () const
{
    return "GetBodyMaterialsOfElements";
}

GS::Optional<GS::UniString> GetBodyMaterialsOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> GetBodyMaterialsOfElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "bodyMaterials": {
                "type": "array",
                "description": "The bodies of each element, aligned with the input, or an error.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "bodies": {
                                    "type": "array",
                                    "description": "The bodies of the element's 3D model generated with separate components: a composite structure has one body per skin. Empty when the element has no 3D model.",
                                    "items": {
                                        "type": "object",
                                        "properties": {
                                            "bodyIndex": {
                                                "type": "integer",
                                                "description": "The index of the body in the element, starting from 0."
                                            },
                                            "buildingMaterialId": {
                                                "$ref": "#/AttributeId",
                                                "description": "The building material of the body. Left out when it cannot be found among the attributes of the project."
                                            },
                                            "buildingMaterialName": {
                                                "type": "string",
                                                "description": "The name of the building material. Left out together with buildingMaterialId."
                                            }
                                        },
                                        "additionalProperties": false,
                                        "required": [
                                            "bodyIndex"
                                        ]
                                    }
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "bodies"
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
            "bodyMaterials"
        ]
    })";
}

GS::ObjectState GetBodyMaterialsOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    // The model is generated once for all found elements, so the elements are checked first.
    // An unknown guid would make GenerateModelWithSeparateComponents fail for all of them
    // (APIERR_BADID); a failed input keeps APINULLGuid and its error.
    GS::Array<API_Guid> inputGuids;
    GS::Array<GS::ObjectState> inputErrors;
    GS::Array<API_Guid> elementGuids;
    GS::HashSet<API_Guid> elementGuidSet;
    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            inputGuids.Push (APINULLGuid);
            inputErrors.Push (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        API_Elem_Head elemHead = {};
        elemHead.guid = GetGuidFromObjectState (*elementId);
        const GSErrCode err = ACAPI_Element_GetHeader (&elemHead);
        if (err != NoError) {
            inputGuids.Push (APINULLGuid);
            inputErrors.Push (CreateErrorResponse (err, "Failed to find element in Archicad"));
            continue;
        }

        inputGuids.Push (elemHead.guid);
        inputErrors.Push (GS::ObjectState ());
        if (!elementGuidSet.Contains (elemHead.guid)) {
            elementGuidSet.Add (elemHead.guid);
            elementGuids.Push (elemHead.guid);
        }
    }

    GS::ObjectState response;
    const auto& bodyMaterials = response.AddList<GS::ObjectState> ("bodyMaterials");
    if (elementGuids.IsEmpty ()) {
        for (const GS::ObjectState& inputError : inputErrors) {
            bodyMaterials (inputError);
        }
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

    ModelerAPI::Model model;
    err = ACAPI_Sight_GetSelectedSightModel (model);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to read the 3D model of the elements");
    }

    for (UIndex i = 0; i < inputGuids.GetSize (); ++i) {
        if (inputGuids[i] == APINULLGuid) {
            bodyMaterials (inputErrors[i]);
            continue;
        }

        GS::ObjectState result;
        const auto& bodies = result.AddList<GS::ObjectState> ("bodies");

        const std::optional<Int32> modelIndex = model.GetElementIndex (APIGuid2GSGuid (inputGuids[i]));
        if (!modelIndex.has_value ()) {
            bodyMaterials (result);
            continue;
        }

        ModelerAPI::Element modelElement;
        model.GetElement (*modelIndex, &modelElement);

        // ModelerAPI indexes the elements from 1, ACAPI_ModelAccess_GetBuildingMaterial from 0.
        const Int32 elemIdx = *modelIndex - 1;
        const Int32 bodyCount = modelElement.GetTessellatedBodyCount ();

        GSErrCode bodyErr = NoError;
        for (Int32 bodyIdx = 0; bodyIdx < bodyCount; ++bodyIdx) {
            GS::ObjectState bodyOS;
            bodyErr = CreateBodyMaterialObjectState (elemIdx, static_cast<UInt32> (bodyIdx), bodyOS);
            if (bodyErr != NoError) {
                break;
            }
            bodies (bodyOS);
        }

        if (bodyErr != NoError) {
            bodyMaterials (CreateErrorResponse (bodyErr, "Failed to get the building material of a body of the element"));
        } else {
            bodyMaterials (result);
        }
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command is only supported in Archicad 28 or later.");
#endif
}

GetCutPolygonsOfElementsCommand::GetCutPolygonsOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetCutPolygonsOfElementsCommand::GetName () const
{
    return "GetCutPolygonsOfElements";
}

GS::Optional<GS::UniString> GetCutPolygonsOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            },
            "cutPlane": {
                "type": "object",
                "description": "The cutting plane. axisX, axisY and axisZ should be unit vectors perpendicular to each other; axisZ is the normal of the plane.",
                "properties": {
                    "basePoint": {
                        "$ref": "#/Coordinate3D",
                        "description": "A point of the plane, the origin of the returned 2D coordinates."
                    },
                    "axisX": {
                        "$ref": "#/Coordinate3D",
                        "description": "The direction of the x axis of the returned 2D coordinates."
                    },
                    "axisY": {
                        "$ref": "#/Coordinate3D",
                        "description": "The direction of the y axis of the returned 2D coordinates."
                    },
                    "axisZ": {
                        "$ref": "#/Coordinate3D",
                        "description": "The normal of the plane."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "basePoint",
                    "axisX",
                    "axisY",
                    "axisZ"
                ]
            }
        },
        "additionalProperties": false,
        "required": [
            "elements",
            "cutPlane"
        ]
    })";
}

GS::Optional<GS::UniString> GetCutPolygonsOfElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "cutPolygons": {
                "type": "array",
                "description": "The cut of each body of each element, aligned with the input, or an error.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "bodies": {
                                    "type": "array",
                                    "description": "The bodies of the element's 3D model generated with separate components: a composite structure has one body per skin. Empty when the element has no 3D model.",
                                    "items": {
                                        "type": "object",
                                        "properties": {
                                            "bodyIndex": {
                                                "type": "integer",
                                                "description": "The index of the body in the element, starting from 0."
                                            },
                                            "buildingMaterialId": {
                                                "$ref": "#/AttributeId",
                                                "description": "The building material of the body. Left out when it cannot be found among the attributes of the project."
                                            },
                                            "buildingMaterialName": {
                                                "type": "string",
                                                "description": "The name of the building material. Left out together with buildingMaterialId."
                                            },
                                            "area": {
                                                "type": "number",
                                                "description": "The area of the cut surface of the body. 0 when the plane does not cut the body."
                                            },
                                            "polygons": {
                                                "type": "array",
                                                "description": "The polygons of the cut surface of the body, in 2D coordinates of the plane (basePoint, axisX, axisY). Curved edges are given by their end points. Empty when the plane does not cut the body.",
                                                "items": {
                                                    "type": "object",
                                                    "properties": {
                                                        "polygonOutline": {
                                                            "type": "array",
                                                            "items": {
                                                                "$ref": "#/Coordinate2D"
                                                            }
                                                        },
                                                        "holes": {
                                                            "$ref": "#/Holes2D"
                                                        }
                                                    },
                                                    "additionalProperties": false,
                                                    "required": [
                                                        "polygonOutline",
                                                        "holes"
                                                    ]
                                                }
                                            }
                                        },
                                        "additionalProperties": false,
                                        "required": [
                                            "bodyIndex",
                                            "area",
                                            "polygons"
                                        ]
                                    }
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "bodies"
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
            "cutPolygons"
        ]
    })";
}

GS::ObjectState GetCutPolygonsOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    const GS::ObjectState* cutPlaneOS = parameters.Get ("cutPlane");
    if (cutPlaneOS == nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, "cutPlane is missing");
    }

    const GS::ObjectState* basePoint = cutPlaneOS->Get ("basePoint");
    const GS::ObjectState* axisX = cutPlaneOS->Get ("axisX");
    const GS::ObjectState* axisY = cutPlaneOS->Get ("axisY");
    const GS::ObjectState* axisZ = cutPlaneOS->Get ("axisZ");
    if (basePoint == nullptr || axisX == nullptr || axisY == nullptr || axisZ == nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, "cutPlane needs basePoint, axisX, axisY and axisZ");
    }

    API_Plane3D cutPlane = {};
    cutPlane.basePoint = Get3DCoordinateFromObjectState (*basePoint);
    const API_Coord3D x = Get3DCoordinateFromObjectState (*axisX);
    const API_Coord3D y = Get3DCoordinateFromObjectState (*axisY);
    const API_Coord3D z = Get3DCoordinateFromObjectState (*axisZ);
    cutPlane.axisX = { x.x, x.y, x.z };
    cutPlane.axisY = { y.x, y.y, y.z };
    cutPlane.axisZ = { z.x, z.y, z.z };

    // The model is generated once for all found elements, so the elements are checked first.
    // An unknown guid would make GenerateModelWithSeparateComponents fail for all of them
    // (APIERR_BADID); a failed input keeps APINULLGuid and its error.
    GS::Array<API_Guid> inputGuids;
    GS::Array<GS::ObjectState> inputErrors;
    GS::Array<API_Guid> elementGuids;
    GS::HashSet<API_Guid> elementGuidSet;
    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            inputGuids.Push (APINULLGuid);
            inputErrors.Push (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        API_Elem_Head elemHead = {};
        elemHead.guid = GetGuidFromObjectState (*elementId);
        const GSErrCode err = ACAPI_Element_GetHeader (&elemHead);
        if (err != NoError) {
            inputGuids.Push (APINULLGuid);
            inputErrors.Push (CreateErrorResponse (err, "Failed to find element in Archicad"));
            continue;
        }

        inputGuids.Push (elemHead.guid);
        inputErrors.Push (GS::ObjectState ());
        if (!elementGuidSet.Contains (elemHead.guid)) {
            elementGuidSet.Add (elemHead.guid);
            elementGuids.Push (elemHead.guid);
        }
    }

    GS::ObjectState response;
    const auto& cutPolygons = response.AddList<GS::ObjectState> ("cutPolygons");
    if (elementGuids.IsEmpty ()) {
        for (const GS::ObjectState& inputError : inputErrors) {
            cutPolygons (inputError);
        }
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

    ModelerAPI::Model model;
    err = ACAPI_Sight_GetSelectedSightModel (model);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to read the 3D model of the elements");
    }

    for (UIndex i = 0; i < inputGuids.GetSize (); ++i) {
        if (inputGuids[i] == APINULLGuid) {
            cutPolygons (inputErrors[i]);
            continue;
        }

        GS::ObjectState result;
        const auto& bodies = result.AddList<GS::ObjectState> ("bodies");

        const std::optional<Int32> modelIndex = model.GetElementIndex (APIGuid2GSGuid (inputGuids[i]));
        if (!modelIndex.has_value ()) {
            cutPolygons (result);
            continue;
        }

        ModelerAPI::Element modelElement;
        model.GetElement (*modelIndex, &modelElement);

        // ModelerAPI indexes the elements from 1, ACAPI_CuttingPlane_GetCutPolygonInfo_New from 0.
        const Int32 elemIdx = *modelIndex - 1;
        const Int32 bodyCount = modelElement.GetTessellatedBodyCount ();

        GSErrCode bodyErr = NoError;
        GS::UniString bodyErrMessage;
        for (Int32 bodyIdx = 0; bodyIdx < bodyCount; ++bodyIdx) {
            GS::ObjectState bodyOS;
            bodyErr = CreateBodyMaterialObjectState (elemIdx, static_cast<UInt32> (bodyIdx), bodyOS);
            if (bodyErr != NoError) {
                bodyErrMessage = "Failed to get the building material of a body of the element";
                break;
            }

            GS::Array<Geometry::MultiPolygon2D> resPolygons;
            double area = 0.0;
            bodyErr = ACAPI_CuttingPlane_GetCutPolygonInfo_New (elemIdx, bodyIdx, cutPlane, &resPolygons, &area);
            if (bodyErr != NoError) {
                bodyErrMessage = "Failed to cut a body of the element with the plane";
                break;
            }

            bodyOS.Add ("area", area);
            const auto& polygons = bodyOS.AddList<GS::ObjectState> ("polygons");
            for (const Geometry::MultiPolygon2D& multiPolygon : resPolygons) {
                for (const Geometry::Polygon2D& polygon : multiPolygon) {
                    polygons (CreateCutPolygonObjectState (polygon));
                }
            }
            bodies (bodyOS);
        }

        if (bodyErr != NoError) {
            cutPolygons (CreateErrorResponse (bodyErr, bodyErrMessage));
        } else {
            cutPolygons (result);
        }
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command is only supported in Archicad 28 or later.");
#endif
}

GetSurfaceTypesOfElementsCommand::GetSurfaceTypesOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetSurfaceTypesOfElementsCommand::GetName () const
{
    return "GetSurfaceTypesOfElements";
}

GS::Optional<GS::UniString> GetSurfaceTypesOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> GetSurfaceTypesOfElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "surfaceTypes": {
                "type": "array",
                "description": "The polygons of the 3D model of each element with their surface types, aligned with the input, or an error.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "polygons": {
                                    "type": "array",
                                    "description": "Every polygon of every body of the element's 3D model, generated with separate components. Empty when the element has no 3D model.",
                                    "items": {
                                        "type": "object",
                                        "properties": {
                                            "bodyIndex": {
                                                "type": "integer",
                                                "description": "The index of the body in the element, starting from 0."
                                            },
                                            "polygonIndex": {
                                                "type": "integer",
                                                "description": "The index of the polygon in the body, starting from 0."
                                            },
                                            "polygonId": {
                                                "type": "integer",
                                                "description": "The identifier of the polygon in the 3D model (ModelerAPI::Polygon::GetPolygonId)."
                                            },
                                            "surfaceType": {
                                                "type": "integer",
                                                "description": "The surface type of the polygon: the numeric value of API_SurfaceTypeID as ACAPI_Polygon_PolygonId2SurfaceType returns it."
                                            }
                                        },
                                        "additionalProperties": false,
                                        "required": [
                                            "bodyIndex",
                                            "polygonIndex",
                                            "polygonId",
                                            "surfaceType"
                                        ]
                                    }
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "polygons"
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
            "surfaceTypes"
        ]
    })";
}

GS::ObjectState GetSurfaceTypesOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    // The model is generated once for all found elements, so the elements are checked first.
    // An unknown guid would make GenerateModelWithSeparateComponents fail for all of them
    // (APIERR_BADID); a failed input keeps APINULLGuid and its error.
    GS::Array<API_Guid> inputGuids;
    GS::Array<GS::ObjectState> inputErrors;
    GS::Array<API_Guid> elementGuids;
    GS::HashSet<API_Guid> elementGuidSet;
    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            inputGuids.Push (APINULLGuid);
            inputErrors.Push (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        API_Elem_Head elemHead = {};
        elemHead.guid = GetGuidFromObjectState (*elementId);
        const GSErrCode err = ACAPI_Element_GetHeader (&elemHead);
        if (err != NoError) {
            inputGuids.Push (APINULLGuid);
            inputErrors.Push (CreateErrorResponse (err, "Failed to find element in Archicad"));
            continue;
        }

        inputGuids.Push (elemHead.guid);
        inputErrors.Push (GS::ObjectState ());
        if (!elementGuidSet.Contains (elemHead.guid)) {
            elementGuidSet.Add (elemHead.guid);
            elementGuids.Push (elemHead.guid);
        }
    }

    GS::ObjectState response;
    const auto& surfaceTypes = response.AddList<GS::ObjectState> ("surfaceTypes");
    if (elementGuids.IsEmpty ()) {
        for (const GS::ObjectState& inputError : inputErrors) {
            surfaceTypes (inputError);
        }
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

    ModelerAPI::Model model;
    err = ACAPI_Sight_GetSelectedSightModel (model);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to read the 3D model of the elements");
    }

    for (UIndex i = 0; i < inputGuids.GetSize (); ++i) {
        if (inputGuids[i] == APINULLGuid) {
            surfaceTypes (inputErrors[i]);
            continue;
        }
        surfaceTypes (CreateSurfaceTypesOfElement (inputGuids[i], model));
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command is only supported in Archicad 28 or later.");
#endif
}
