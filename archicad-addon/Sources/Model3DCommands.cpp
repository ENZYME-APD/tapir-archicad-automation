#include "Model3DCommands.hpp"
#include "MigrationHelper.hpp"

#ifdef ServerMainVers_2800
#include "Model.hpp"
#include "ModelElement.hpp"

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
