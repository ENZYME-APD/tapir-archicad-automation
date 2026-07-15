#include "ElementCreationCommands.hpp"
#include "ExtendedElementCommands.hpp"
#include "ObjectState.hpp"
#include "MigrationHelper.hpp"
#include "NotificationCommands.hpp"

CreateElementsCommandBase::CreateElementsCommandBase (const GS::String& commandNameIn, API_ElemTypeID elemTypeIDIn, const GS::String& arrayFieldNameIn)
    : CommandBase (CommonSchema::Used)
    , commandName (commandNameIn)
    , elemTypeID (elemTypeIDIn)
    , arrayFieldName (arrayFieldNameIn)
{
}

GS::String CreateElementsCommandBase::GetName () const
{
    return commandName;
}

GS::Optional<GS::UniString> CreateElementsCommandBase::GetResponseSchema () const
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

GS::ObjectState	CreateElementsCommandBase::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> dataArray;
    parameters.Get (arrayFieldName, dataArray);

    GS::ObjectState response;
    const auto& elements = response.AddList<GS::ObjectState> ("elements");

    const GS::UniString elemTypeName = GetElementTypeNonLocalizedName (elemTypeID);
    const Stories stories = GetStories ();

    API_NotifyElementType notification = {};
    notification.notifID = APINotifyElement_BeginEvents;
    AddElementNotificationClientCommand::ElementEventHandlerProc (&notification);

    ACAPI_CallUndoableCommand ("Create " + elemTypeName, [&] () -> GSErrCode {
        API_Element element = {};
        API_ElementMemo memo = {};
        const GS::OnExit guard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });

#ifdef ServerMainVers_2600
        element.header.type   = elemTypeID;
#else
        element.header.typeID = elemTypeID;
#endif
        GSErrCode err = ACAPI_Element_GetDefaults (&element, &memo);

        bool savedAutoTextFlag;
        ACAPI_AutoText_GetAutoTextFlag (&savedAutoTextFlag);
        bool setAutoTextFlag = false;
        ACAPI_AutoText_ChangeAutoTextFlag (&setAutoTextFlag);

        for (const GS::ObjectState& data : dataArray) {
            auto os = SetTypeSpecificParameters (element, memo, stories, data);
            if (os.HasValue ()) {
                elements (*os);
                continue;
            }

            err = ACAPI_Element_Create (&element, &memo);
            if (err != NoError) {
                elements (CreateErrorResponse (err, "Failed to create new " + elemTypeName));
                continue;
            }

            notification = {};
            notification.notifID = APINotifyElement_New;
            notification.elemHead = element.header;
            AddElementNotificationClientCommand::ElementEventHandlerProc (&notification);

            elements (CreateElementIdObjectState (element.header.guid));
        }

        ACAPI_AutoText_ChangeAutoTextFlag (&savedAutoTextFlag);

        return NoError;
    });

    notification = {};
    notification.notifID = APINotifyElement_EndEvents;
    AddElementNotificationClientCommand::ElementEventHandlerProc (&notification);

    return response;
}

CreateColumnsCommand::CreateColumnsCommand () :
    CreateElementsCommandBase ("CreateColumns", API_ColumnID, "columnsData")
{
}

GS::Optional<GS::UniString> CreateColumnsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "columnsData": {
                "type": "array",
                "description": "Array of data to create Columns.",
                "items": {
                    "type": "object",
                    "description": "The parameters of the new Column.",
                    "properties": {
                        "coordinates": {
                            "type": "object",
                            "description" : "3D coordinate.",
                            "properties" : {
                                "x": {
                                    "type": "number",
                                    "description" : "X value of the coordinate."
                                },
                                "y" : {
                                    "type": "number",
                                    "description" : "Y value of the coordinate."
                                },
                                "z" : {
                                    "type": "number",
                                    "description" : "Z value of the coordinate."
                                }
                            },
                            "additionalProperties": false,
                            "required" : [
                                "x",
                                "y",
                                "z"
                            ]
                        },
                        "height": {
                            "type": "number",
                            "description": "Optional column height.",
                            "exclusiveMinimum": 0.0
                        },
                        "axisRotationAngle": {
                            "type": "number",
                            "description": "Optional column rotation angle in radians."
                        },
                        "width": {
                            "type": "number",
                            "description": "Cross section width of the column. Applied to all segments.",
                            "exclusiveMinimum": 0.0
                        },
                        "depth": {
                            "type": "number",
                            "description": "Cross section depth (height) of the column. Applied to all segments. Only effective for rectangular columns.",
                            "exclusiveMinimum": 0.0
                        },
                        "coreAnchor": {
                            "type": "string",
                            "description": "Optional anchor point of the column core on a 3x3 grid.",
                            "enum": ["TopLeft", "TopCenter", "TopRight", "MiddleLeft", "Center", "MiddleRight", "BottomLeft", "BottomCenter", "BottomRight"]
                        }
                    },
                    "additionalProperties": false,
                    "required" : [
                        "coordinates"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "columnsData"
        ]
    })";
}

GS::Optional<GS::ObjectState> CreateColumnsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const
{
    GS::ObjectState coordinates;
    parameters.Get ("coordinates", coordinates);
    API_Coord3D apiCoordinate = Get3DCoordinateFromObjectState (coordinates);

    const auto floorIndexAndOffset = GetFloorIndexAndOffset (apiCoordinate.z, stories);
    element.header.floorInd = floorIndexAndOffset.first;
    element.column.bottomOffset = floorIndexAndOffset.second;
    element.column.origoPos.x = apiCoordinate.x;
    element.column.origoPos.y = apiCoordinate.y;
    if (parameters.Get ("height", element.column.height)) {
        element.column.relativeTopStory = 0;
    }
    parameters.Get ("axisRotationAngle", element.column.axisRotationAngle);

    GS::UniString coreAnchor;
    if (parameters.Get ("coreAnchor", coreAnchor)) {
        element.column.coreAnchor = ParseAnchorPointString (coreAnchor);
    }

    double width = 0.0;
    double depth = 0.0;
    bool hasWidth = parameters.Get ("width", width);
    bool hasDepth = parameters.Get ("depth", depth);

    if ((hasWidth || hasDepth) && memo.columnSegments != nullptr) {
        GSSize nSegments = BMGetPtrSize (reinterpret_cast<GSPtr>(memo.columnSegments)) / sizeof (API_ColumnSegmentType);
        for (GSSize i = 0; i < nSegments; ++i) {
            if (hasWidth) {
                memo.columnSegments[i].assemblySegmentData.nominalWidth = width;
            }
            if (hasDepth) {
                memo.columnSegments[i].assemblySegmentData.nominalHeight = depth;
            }
        }
    }

    return {};
}

CreateSlabsCommand::CreateSlabsCommand () :
    CreateElementsCommandBase ("CreateSlabs", API_SlabID, "slabsData")
{
}

GS::Optional<GS::UniString> CreateSlabsCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "slabsData": {
            "type": "array",
            "description": "Array of data to create Slabs.",
            "items": {
                "type": "object",
                "description" : "The parameters of the new Slab.",
                "properties" : {
                    "level": {
                        "type": "number",
                        "description" : "The Z coordinate value of the reference line of the slab."	
                    },
                    "thickness": {
                        "type": "number",
                        "description": "Optional slab thickness.",
                        "exclusiveMinimum": 0.0
                    },
                    "referencePlaneLocation": {
                        "type": "string",
                        "description": "Optional location of the slab reference plane. For a basic (homogeneous) slab only 'Top' or 'Bottom' are valid.",
                        "enum": ["Top", "CoreTop", "CoreBottom", "Bottom"]
                    },
                    "polygonCoordinates": { 
                        "type": "array",
                        "description": "The 2D coordinates of the edge of the slab.",
                        "items": {
                            "$ref": "#/Coordinate2D"
                        },
                        "minItems": 3
                    },
                    "polygonArcs": {
                        "type": "array",
                        "description": "Polygon outline arcs of the slab.",
                        "items": {
                            "$ref": "#/PolyArc"
                        }
                    },
                    "holes" : {
                        "$ref": "#/Holes2D"
                    }    
                },
                "additionalProperties": false,
                "required" : [
                    "level",
                    "polygonCoordinates"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "slabsData"
    ]
})";
}

GS::Array<API_PolyArc> GetPolyArcs (const GS::Array<GS::ObjectState>& arcs, Int32 iStart)
{
    GS::Array<API_PolyArc> polyArcs;
    for (const GS::ObjectState& arc : arcs) {
        API_PolyArc polyArc = {};
        if (arc.Get ("begIndex", polyArc.begIndex) &&
            arc.Get ("endIndex", polyArc.endIndex) &&
            arc.Get ("arcAngle", polyArc.arcAngle)) {
            polyArc.begIndex += iStart;
            polyArc.endIndex += iStart;
            polyArcs.Push (polyArc);
        }
    }
    return polyArcs;
}

void AddPolyToMemo (const GS::Array<GS::ObjectState>& coords,
                           const GS::Array<GS::ObjectState>& arcs,
                           Int32&                            iCoord,
                           Int32&                            iArc,
                           Int32&                            iPends,
                           API_ElementMemo&                  memo,
                           const API_EdgeTrimID*             edgeTrimSideType,
                           const API_OverriddenAttribute*    sideMat,
                           bool                              processVertexIDs)
{
    Int32 iStart = iCoord;
    for (const GS::ObjectState& coord : coords) {
        (*memo.coords)[iCoord] = Get2DCoordinateFromObjectState (coord);
        if (edgeTrimSideType != nullptr) {
            (*memo.edgeTrims)[iCoord].sideType = *edgeTrimSideType;
        }
        if (sideMat != nullptr) {
            memo.sideMaterials[iCoord] = *sideMat;
        }
        if (memo.meshPolyZ != nullptr) {
            coord.Get ("z", (*memo.meshPolyZ)[iCoord]);
        }
        if (processVertexIDs && memo.vertexIDs != nullptr) {
            (*memo.vertexIDs)[iCoord] = (UInt32)iCoord;
        }
        ++iCoord;
    }
    (*memo.coords)[iCoord] = (*memo.coords)[iStart];
    if (memo.meshPolyZ != nullptr) {
        (*memo.meshPolyZ)[iCoord] = (*memo.meshPolyZ)[iStart];
    }
    if (processVertexIDs && memo.vertexIDs != nullptr) {
        (*memo.vertexIDs)[iCoord] = (*memo.vertexIDs)[iStart];
    }
    (*memo.pends)[iPends++] = iCoord;
    if (edgeTrimSideType != nullptr) {
        (*memo.edgeTrims)[iCoord].sideType = (*memo.edgeTrims)[iStart].sideType;
        (*memo.edgeTrims)[iCoord].sideAngle = (*memo.edgeTrims)[iStart].sideAngle;
    }
    if (sideMat != nullptr) {
        memo.sideMaterials[iCoord] = memo.sideMaterials[iStart];
    }
    ++iCoord;

    const GS::Array<API_PolyArc> polyArcs = GetPolyArcs (arcs, iStart);
    for (const API_PolyArc& a : polyArcs) {
        (*memo.parcs)[iArc++] = a;
    }
}

GS::Optional<GS::ObjectState> CreateSlabsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const
{
    double inputLevel = 0.0;
    parameters.Get ("level", inputLevel);
    const auto floorIndexAndOffset = GetFloorIndexAndOffset (inputLevel, stories);
    element.header.floorInd = floorIndexAndOffset.first;
    element.slab.level = floorIndexAndOffset.second;
    parameters.Get ("thickness", element.slab.thickness);

    GS::UniString referencePlaneLocation;
    if (parameters.Get ("referencePlaneLocation", referencePlaneLocation)) {
        if (referencePlaneLocation == "Top") {
            element.slab.referencePlaneLocation = APISlabRefPlane_Top;
        } else if (referencePlaneLocation == "CoreTop") {
            element.slab.referencePlaneLocation = APISlabRefPlane_CoreTop;
        } else if (referencePlaneLocation == "CoreBottom") {
            element.slab.referencePlaneLocation = APISlabRefPlane_CoreBottom;
        } else if (referencePlaneLocation == "Bottom") {
            element.slab.referencePlaneLocation = APISlabRefPlane_Bottom;
        }
    }

    GS::Array<GS::ObjectState> polygonCoordinates;
    GS::Array<GS::ObjectState> polygonArcs;
    GS::Array<GS::ObjectState> holes;
    parameters.Get ("polygonCoordinates", polygonCoordinates);
    parameters.Get ("polygonArcs", polygonArcs);
    parameters.Get ("holes", holes);

    auto slabMemoError = BuildSlabMemoFromGeometry (element, memo, polygonCoordinates, polygonArcs, holes);
    if (slabMemoError.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid slab geometry: " + slabMemoError.Get ());
    }

    return {};
}

CreateZonesCommand::CreateZonesCommand () :
    CreateElementsCommandBase ("CreateZones", API_ZoneID, "zonesData")
{
}

GS::Optional<GS::UniString> CreateZonesCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "zonesData": {
            "type": "array",
            "description": "Array of data to create Zones.",
            "items": {
                "type": "object",
                "description" : "The parameters of the new Zone.",
                "properties" : {
                    "floorIndex": {
                        "type": "number"
                    },
                    "name": {
                        "type": "string",
                        "description" : "Name of the zone."
                    },
                    "numberStr": {
                        "type": "string",
                        "description" : "Zone number."	
                    },
                    "categoryAttributeId": {
                        "$ref": "#/AttributeId",
                        "description" : "The identifier of the zone category attribute."	
                    },
                    "stampPosition": {
                        "$ref": "#/Coordinate2D",
                        "description" : "Position of the origin of the zone stamp."
                    },
                    "stampAngle": {
                        "type": "number",
                        "description" : "Optional zone stamp rotation angle in radians."
                    },
                    "fixedStampAngle": {
                        "type": "boolean",
                        "description" : "If true, the zone stamp angle remains fixed when the element is rotated."
                    },
                    "geometry": {
                        "$ref": "#/ZoneCreationGeometry"
                    }
                },
                "additionalProperties": false,
                "required": [
                    "name",
                    "numberStr",
                    "geometry"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "zonesData"
    ]
})";
}

GS::Optional<GS::ObjectState> CreateZonesCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& /*stories*/, const GS::ObjectState& parameters) const
{
    parameters.Get ("floorIndex", element.header.floorInd);

    const API_Guid categoryAttrGuid = GetGuidFromArrayItem ("categoryAttributeId", parameters);
    if (categoryAttrGuid != APINULLGuid) {
        element.zone.catInd = GetAttributeIndexFromGuid (API_ZoneCatID, categoryAttrGuid);
    }

    if (!SetUCharProperty (&parameters, "name", element.zone.roomName)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing name parameter.");
    }

    if (!SetUCharProperty (&parameters, "numberStr", element.zone.roomNoStr)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing numberStr parameter.");
    }

    GS::ObjectState geometry;
    if (!parameters.Get ("geometry", geometry)) {
        return CreateErrorResponse (APIERR_BADPARS, "geometry parameter is missing.");
    }

    GS::ObjectState stampPosition;
    parameters.Get ("stampPosition", stampPosition);
    parameters.Get ("stampAngle", element.zone.stampAngle);

    bool fixedAngle = false;
    parameters.Get ("fixedStampAngle", fixedAngle);
    element.zone.fixedAngle = fixedAngle;

    GS::ObjectState referencePosition;
    if (geometry.Get ("referencePosition", referencePosition)) {
        element.zone.manual = false;

        element.zone.refPos = Get2DCoordinateFromObjectState (referencePosition);

        element.zone.pos = stampPosition.IsEmpty() ? element.zone.refPos : Get2DCoordinateFromObjectState (stampPosition);
    } else {
        element.zone.manual = true;

        GS::Array<GS::ObjectState> polygonCoordinates;
        GS::Array<GS::ObjectState> polygonArcs;
        GS::Array<GS::ObjectState> holes;
        if (!geometry.Get ("polygonCoordinates", polygonCoordinates)) {
            return CreateErrorResponse (APIERR_BADPARS, "polygonCoordinates parameter is missing in geometry.");
        }

        geometry.Get ("polygonArcs", polygonArcs);
        geometry.Get ("holes", holes);
        if (polygonCoordinates.GetSize () < 3) {
            return CreateErrorResponse (APIERR_BADPARS, "'polygonCoordinates' must contain at least 3 coordinates.");
        }
        if (IsSame2DCoordinate (polygonCoordinates.GetFirst (), polygonCoordinates.GetLast ())) {
            polygonCoordinates.Pop ();
        }
        element.zone.poly.nCoords	= polygonCoordinates.GetSize() + 1;
        element.zone.poly.nSubPolys	= 1;
        element.zone.poly.nArcs		= polygonArcs.GetSize ();

        for (const GS::ObjectState& hole : holes) {
            GS::Array<GS::ObjectState> holePolygonOutline;
            GS::Array<GS::ObjectState> holePolygonArcs;
            if (GetHoleGeometry (hole, holePolygonOutline, holePolygonArcs)) {
                element.zone.poly.nCoords += holePolygonOutline.GetSize () + 1;
                ++element.zone.poly.nSubPolys;
                element.zone.poly.nArcs += holePolygonArcs.GetSize ();
            }
        }

        memo.coords = reinterpret_cast<API_Coord**> (BMAllocateHandle ((element.zone.poly.nCoords + 1) * sizeof (API_Coord), ALLOCATE_CLEAR, 0));
        memo.pends = reinterpret_cast<Int32**> (BMAllocateHandle ((element.zone.poly.nSubPolys + 1) * sizeof (Int32), ALLOCATE_CLEAR, 0));
        memo.parcs = reinterpret_cast<API_PolyArc**> (BMAllocateHandle (element.zone.poly.nArcs * sizeof (API_PolyArc), ALLOCATE_CLEAR, 0));

        Int32 iCoord = 1;
        Int32 iArc = 0;
        Int32 iPends = 1;
        AddPolyToMemo(polygonCoordinates,
                      polygonArcs,
                      iCoord,
                      iArc,
                      iPends,
                      memo);

        for (const GS::ObjectState& hole : holes) {
            GS::Array<GS::ObjectState> holePolygonOutline;
            GS::Array<GS::ObjectState> holePolygonArcs;
            if (GetHoleGeometry (hole, holePolygonOutline, holePolygonArcs)) {
                AddPolyToMemo (holePolygonOutline,
                              holePolygonArcs,
                              iCoord,
                              iArc,
                              iPends,
                              memo);
            }
        }

        element.zone.pos = stampPosition.IsEmpty() ? (*memo.coords)[1] : Get2DCoordinateFromObjectState (stampPosition);
    }

    return {};
}

CreatePolylinesCommand::CreatePolylinesCommand () :
    CreateElementsCommandBase ("CreatePolylines", API_PolyLineID, "polylinesData")
{
}

GS::Optional<GS::UniString> CreatePolylinesCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "polylinesData": {
            "type": "array",
            "description": "Array of data to create Polylines.",
            "items": {
                "type": "object",
                "description" : "The parameters of the new Polyline.",
                "properties" : {
                    "floorInd": {
                        "type": "number",
                        "description" : "The identifier of the floor. Optional parameter, by default the current floor is used."	
                    },
                    "layerIndex": {
                        "type": "integer",
                        "description" : "Layer attribute index to place the polyline on. Optional parameter, by default the current layer is used."
                    },
                    "linePenIndex": {
                        "type": "integer",
                        "description" : "Pen index of the polyline contour. Optional parameter, by default the current pen is used."
                    },
                    "lineTypeIndex": {
                        "type": "integer",
                        "description" : "Line type attribute index of the polyline contour. Optional parameter, by default the current line type is used."
                    },
                    "penWeightMm": {
                        "type": "number",
                        "description" : "Optional pen weight override in mm."
                    },
                    "coordinates": { 
                        "type": "array",
                        "description": "The 2D coordinates of the polyline.",
                        "items": {
                            "$ref": "#/Coordinate2D"
                        },
                        "minItems": 2
                    },
                    "arcs": { 
                        "type": "array",
                        "description": "The arcs of the polyline.",
                        "items": {
                            "$ref": "#/PolyArc"
                        }
                    }
                },
                "additionalProperties": false,
                "required" : [
                    "coordinates"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "polylinesData"
    ]
})";
}

static void AddPolyToMemo (const GS::Array<GS::ObjectState>& coordinates,
                           const GS::Array<GS::ObjectState>& arcs,
                           API_Polygon&                      poly,
                           API_ElementMemo& 				 memo)
{
    const GS::Array<API_PolyArc> polyArcs = GetPolyArcs (arcs, 1);
    poly.nCoords	= coordinates.GetSize();
    poly.nSubPolys	= 1;
    poly.nArcs		= polyArcs.GetSize ();

    memo.coords = reinterpret_cast<API_Coord**> (BMAllocateHandle ((poly.nCoords + 1) * sizeof (API_Coord), ALLOCATE_CLEAR, 0));
    memo.pends = reinterpret_cast<Int32**> (BMAllocateHandle ((poly.nSubPolys + 1) * sizeof (Int32), ALLOCATE_CLEAR, 0));
    memo.parcs = reinterpret_cast<API_PolyArc**> (BMAllocateHandle (poly.nArcs * sizeof (API_PolyArc), ALLOCATE_CLEAR, 0));

    Int32 iCoord = 0;
    for (const GS::ObjectState& c : coordinates) {
        (*memo.coords)[++iCoord] = Get2DCoordinateFromObjectState (c);
    }
    (*memo.pends)[1] = iCoord;

    Int32 iArc = 0;
    for (const API_PolyArc& a : polyArcs) {
        (*memo.parcs)[iArc++] = a;
    }
}

GS::Optional<GS::ObjectState> CreatePolylinesCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories&, const GS::ObjectState& parameters) const
{
    parameters.Get ("floorInd", element.header.floorInd);

    Int32 layerIndex = 0;
    if (parameters.Get ("layerIndex", layerIndex) && layerIndex > 0) {
        element.header.layer = ACAPI_CreateAttributeIndex (layerIndex);
    }

    short linePenIndex = 0;
    if (parameters.Get ("linePenIndex", linePenIndex) && linePenIndex > 0) {
        element.polyLine.linePen.penIndex = linePenIndex;
        element.polyLine.linePen.colorOverridePenIndex = 0;
    }

    Int32 lineTypeIndex = 0;
    if (parameters.Get ("lineTypeIndex", lineTypeIndex) && lineTypeIndex > 0) {
        element.polyLine.ltypeInd = ACAPI_CreateAttributeIndex (lineTypeIndex);
    }

    double penWeightMm = API_DefPenWeigth;
    if (parameters.Get ("penWeightMm", penWeightMm)) {
        element.polyLine.penWeight = penWeightMm;
    }

    GS::Array<GS::ObjectState> coordinates;
    GS::Array<GS::ObjectState> arcs;
    parameters.Get ("coordinates", coordinates);
    parameters.Get ("arcs", arcs);

    AddPolyToMemo(coordinates,
                  arcs,
                  element.polyLine.poly,
                  memo);

    return {};
}

// Shared schema builder for CreateObjects and CreateLamps.
//
// API_LampType is a typedef alias of API_ObjectType in APIdefs_Elements.h
// (verified across AC25..29), so the two commands differ only in name,
// elemTypeID, JSON array field name, and the singular noun / description
// used in the schema.
static GS::UniString BuildLibraryPartBasedSchema (const char* arrayFieldName,
                                                  const char* elementSingularName,
                                                  const char* libraryPartNameDescription)
{
    return GS::UniString::Printf (R"({
        "type": "object",
        "properties": {
            "%s": {
                "type": "array",
                "description": "Array of data to create %ss.",
                "items": {
                    "type": "object",
                    "description": "The parameters of the new %s.",
                    "properties": {
                        "libraryPartName": {
                            "type": "string",
                            "description" : "%s"
                        },
                        "coordinates": {
                            "$ref": "#/Coordinate3D"
                        },
                        "dimensions": {
                            "$ref": "#/Dimensions3D"
                        }
                    },
                    "additionalProperties": false,
                    "required" : [
                        "libraryPartName",
                        "coordinates"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "%s"
        ]
    })",
    arrayFieldName, elementSingularName, elementSingularName,
    libraryPartNameDescription, arrayFieldName);
}

CreateObjectsCommand::CreateObjectsCommand () :
    CreateElementsCommandBase ("CreateObjects", API_ObjectID, "objectsData")
{
}

GS::Optional<GS::UniString> CreateObjectsCommand::GetInputParametersSchema () const
{
    return BuildLibraryPartBasedSchema ("objectsData", "Object",
                                        "The name of the library part to use.");
}

constexpr const char* ParameterValueFieldName = "value";

static void SetParamValueInteger (API_AddParType&        addPar,
					              const GS::ObjectState& parameterDetails)
{
	Int32 value;
	parameterDetails.Get (ParameterValueFieldName, value);
	addPar.value.real = value;
}

static void SetParamValueDouble (API_AddParType&        addPar,
					             const GS::ObjectState&	parameterDetails)
{
	double value;
	parameterDetails.Get (ParameterValueFieldName, value);
	addPar.value.real = value;
}

static void SetParamValueOnOff (API_AddParType&         addPar,
				                const GS::ObjectState&	parameterDetails)
{
	GS::String value;
	parameterDetails.Get (ParameterValueFieldName, value);
	addPar.value.real = (value == "Off" ? 0 : 1);
}

static void SetParamValueBool (API_AddParType&        addPar,
				               const GS::ObjectState& parameterDetails)
{
	bool value;
	parameterDetails.Get (ParameterValueFieldName, value);
	addPar.value.real = (value ? 0 : 1);
}

static void SetParamValueString (API_AddParType&        addPar,
					             const GS::ObjectState&	parameterDetails)
{
	GS::UniString value;
	parameterDetails.Get (ParameterValueFieldName, value);

	GS::ucscpy (addPar.value.uStr, value.ToUStr (0, GS::Min(value.GetLength (), (USize)API_UAddParStrLen)).Get ());
}

static void ChangeParams (API_AddParType**& params, const GS::HashTable<GS::String, GS::ObjectState>& changeParamsDictionary)
{
	const GSSize nParams = BMGetHandleSize ((GSHandle) params) / sizeof (API_AddParType);
	for (GSIndex ii = 0; ii < nParams; ++ii) {
		API_AddParType& actParam = (*params)[ii];

		const GS::String name(actParam.name);
		const auto* value = changeParamsDictionary.GetPtr (name);
		if (value == nullptr)
			continue;

		switch (actParam.typeID) {
			case APIParT_Integer:
			case APIParT_PenCol:			SetParamValueInteger (actParam, *value); break;
			case APIParT_ColRGB:
			case APIParT_Intens:
			case APIParT_Length:
			case APIParT_RealNum:
			case APIParT_Angle:				SetParamValueDouble (actParam, *value);	 break;
			case APIParT_LightSw:			SetParamValueOnOff (actParam, *value); 	 break;
			case APIParT_Boolean: 			SetParamValueBool (actParam, *value);	 break;
			case APIParT_LineTyp:
			case APIParT_Mater:
			case APIParT_FillPat:
			case APIParT_BuildingMaterial:
			case APIParT_Profile: 			SetParamValueInteger (actParam, *value); break;
			case APIParT_CString:
			case APIParT_Title: 			SetParamValueString (actParam, *value);	 break;
			default:
			case APIParT_Dictionary:
				// Not supported by the Archicad API yet
				break;
		}
	}
}

// Shared placement logic for CreateObjects and CreateLamps.
//
// element.object and element.lamp alias the same union storage in
// API_Element (API_LampType is a typedef of API_ObjectType across the
// AC25..29 SDKs we target), so writing through element.object.* is
// correct for both API_ObjectID and API_LampID code paths.
static GS::Optional<GS::ObjectState> SetLibraryPartElementParameters (
    API_Element& element, API_ElementMemo& memo,
    const Stories& stories, const GS::ObjectState& parameters)
{
    GS::UniString uName;
    parameters.Get ("libraryPartName", uName);

    API_LibPart libPart = {};
    GS::ucscpy (libPart.docu_UName, uName.ToUStr ());

    GSErrCode err = ACAPI_LibraryPart_Search (&libPart, false, true);
    delete libPart.location;

    if (err != NoError) {
        return CreateErrorResponse (err, GS::UniString::Printf ("Not found library part with name '%T'", uName.ToPrintf ()));
    }

    element.object.libInd = libPart.index;

    GS::ObjectState coordinates;
    if (parameters.Get ("coordinates", coordinates)) {
        const API_Coord3D apiCoordinate = Get3DCoordinateFromObjectState (coordinates);

        element.object.pos.x = apiCoordinate.x;
        element.object.pos.y = apiCoordinate.y;

        const auto floorIndexAndOffset = GetFloorIndexAndOffset (apiCoordinate.z, stories);
        element.header.floorInd = floorIndexAndOffset.first;
        element.object.level = floorIndexAndOffset.second;
    }

    if (parameters.Get ("dimensions", coordinates)) {
        const API_Coord3D dimensions = Get3DCoordinateFromObjectState (coordinates);

        element.object.xRatio = dimensions.x;
        element.object.yRatio = dimensions.y;
        GS::ObjectState os (ParameterValueFieldName, dimensions.z);
        ChangeParams (memo.params, {{"ZZYZX", os}});
    }

    return {};
}

GS::Optional<GS::ObjectState> CreateObjectsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const
{
    return SetLibraryPartElementParameters (element, memo, stories, parameters);
}

// CreateLamps — parallel to CreateObjects but for AC subtype Lamp.
// Without this command, lamp libparts cannot be placed via Tapir —
// CreateObjects rejects them with APIERR_NOTSUPPORTED. The schema and
// placement logic are shared via BuildLibraryPartBasedSchema and
// SetLibraryPartElementParameters above.
CreateLampsCommand::CreateLampsCommand () :
    CreateElementsCommandBase ("CreateLamps", API_LampID, "lampsData")
{
}

GS::Optional<GS::UniString> CreateLampsCommand::GetInputParametersSchema () const
{
    return BuildLibraryPartBasedSchema ("lampsData", "Lamp",
                                        "The name of the lamp library part to use.");
}

GS::Optional<GS::ObjectState> CreateLampsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const
{
    return SetLibraryPartElementParameters (element, memo, stories, parameters);
}

CreateMeshesCommand::CreateMeshesCommand () :
    CreateElementsCommandBase ("CreateMeshes", API_MeshID, "meshesData")
{}

GS::Optional<GS::UniString> CreateMeshesCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "meshesData": {
            "type": "array",
            "description": "Array of data to create Meshes.",
            "items": {
                "type": "object",
                "description" : "The parameters of the new Mesh.",
                "properties" : {
                    "floorIndex": {
                        "type": "integer"
                    },
                    "level": {
                        "type": "number",
                        "description": "The Z reference level of coordinates."
                    },
                    "skirtType": {
                        "$ref": "#/MeshSkirtType"
                    },
                    "skirtLevel": {
                        "type": "number",
                        "description": "The height of the skirt."
                    },
                    "ridges": {
                        "type": "string",
                        "description": "How ridges between mesh facets are displayed in 3D: 'AllSharp' shows all ridges, 'AllSmooth' hides them, 'UserDefined' shows only ridges along user-defined level lines (the drawing-set look for contour-line topography).",
                        "enum": ["AllSharp", "AllSmooth", "UserDefined"]
                    },
                    "showLines": {
                        "type": "boolean",
                        "description": "Whether to show secondary mesh lines (level lines other than the user-defined ones) on plan."
                    },
                    "contourPen": {
                        "type": "integer",
                        "description": "Optional pen attribute index for the mesh's contour line."
                    },
                    "levelPen": {
                        "type": "integer",
                        "description": "Optional pen attribute index for the mesh's level lines."
                    },
                    "lineTypeIndex": {
                        "type": "integer",
                        "description": "Optional line type attribute index for the mesh's contour."
                    },
                    "polygonCoordinates": {
                        "type": "array",
                        "description": "The 3D coordinates of the outline polygon of the mesh.",
                        "items": {
                            "$ref": "#/Coordinate3D"
                        },
                        "minItems": 3
                    },
                    "polygonArcs": {
                        "type": "array",
                        "description": "Polygon outline arcs of the mesh.",
                        "items": {
                            "$ref": "#/PolyArc"
                        }
                    },
                    "holes" : {
                        "$ref": "#/Holes3D"
                    },
                    "sublines": {
                        "type": "array",
                        "description": "The leveling sublines inside the polygon of the mesh.",
                        "items": {
                            "type": "object",
                            "properties" : {
                                "coordinates": { 
                                    "type": "array",
                                    "description": "The 3D coordinates of the leveling subline of the mesh.",
                                    "items": {
                                        "$ref": "#/Coordinate3D"
                                    }
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "coordinates"
                            ]
                        },
                        "minItems": 1
                    }
                },
                "additionalProperties": false,
                "required": [
                    "polygonCoordinates"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "meshesData"
    ]
})";
}

GS::Optional<GS::ObjectState> CreateMeshesCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& /*stories*/, const GS::ObjectState& parameters) const
{
    parameters.Get ("floorIndex", element.header.floorInd);
    parameters.Get ("level", element.mesh.level);
    parameters.Get ("skirtLevel", element.mesh.skirtLevel);
    GS::UniString skirtType;
    parameters.Get ("skirtType", skirtType);
    GS::UniString ridges;
    if (parameters.Get ("ridges", ridges)) {
        if (ridges == "AllSharp") {
            element.mesh.smoothRidges = APIRidge_AllSharp;
        } else if (ridges == "AllSmooth") {
            element.mesh.smoothRidges = APIRidge_AllSmooth;
        } else if (ridges == "UserDefined") {
            element.mesh.smoothRidges = APIRidge_UserSharp;
        }
    }

    bool showLines = false;
    if (parameters.Get ("showLines", showLines)) {
        element.mesh.showLines = showLines ? 1 : 0;
    }

    short contourPen = 0;
    if (parameters.Get ("contourPen", contourPen) && contourPen > 0) {
        element.mesh.contPen = contourPen;
    }

    short levelPen = 0;
    if (parameters.Get ("levelPen", levelPen) && levelPen > 0) {
        element.mesh.levelPen = levelPen;
    }

    Int32 lineTypeIndex = 0;
    if (parameters.Get ("lineTypeIndex", lineTypeIndex) && lineTypeIndex > 0) {
        element.mesh.ltypeInd = ACAPI_CreateAttributeIndex (lineTypeIndex);
    }

    if (skirtType == "SurfaceOnlyWithoutSkirt") {
        element.mesh.skirt = 3;
    } else if (skirtType == "WithSkirt") {
        element.mesh.skirt = 2;
    } else if (skirtType == "SolidBodyWithSkirt") {
        element.mesh.skirt = 1;
    }

    GS::Array<GS::ObjectState> polygonCoordinates;
    GS::Array<GS::ObjectState> polygonArcs;
    GS::Array<GS::ObjectState> holes;
    parameters.Get ("polygonCoordinates", polygonCoordinates);
    parameters.Get ("polygonArcs", polygonArcs);
    parameters.Get ("holes", holes);

    auto geoErr = BuildMeshPolyMemoFromGeometry (element, memo, polygonCoordinates, polygonArcs, holes);
    if (geoErr.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, geoErr.Get ());
    }

    GS::Array<GS::ObjectState> sublines;
    parameters.Get ("sublines", sublines);
    BuildMeshSublinesMemoFromGeometry (element, memo, sublines);

    return {};
}

CreateLabelsCommand::CreateLabelsCommand () :
    CreateElementsCommandBase ("CreateLabels", API_LabelID, "labelsData")
{
}

GS::Optional<GS::UniString> CreateLabelsCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "labelsData": {
            "type": "array",
            "description": "Array of data to create Labels.",
            "items": {
                "type": "object",
                "description": "The parameters of the new Label.",
                "properties": {
                    "parentElementId": {
                        "$ref": "#/ElementId",
                        "description" : "The parent element if the label is an associative label."	
                    },
                    "text": { 
                        "type": "string",
                        "description": "The text content if the label is a text label."
                    },
                    "begCoordinate": {
                        "$ref": "#/Coordinate2D",
                        "description": "The begin coordinate of leader line. Optional parameter, but either begCoordinate or parentElementId must be provided."
                    },
                    "midCoordinate": {
                        "$ref": "#/Coordinate2D",
                        "description": "The mid coordinate of leader line. Optional parameter."
                    },
                    "endCoordinate": {
                        "$ref": "#/Coordinate2D",
                        "description": "The end coordinate of leader line. Optional parameter."
                    },

                    "floorInd": {
                        "type": "number",
                        "description" : "The identifier of the floor. Optional parameter, by default the current floor or the floor of the parent element is used."	
                    }
                },
                "additionalProperties": false,
                "required": [
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "labelsData"
    ]
})";
}

static GSErrCode SetParagraph (API_ParagraphType** paragraph, UInt32 parNum, Int32 from, Int32 range, Int32 numOfTabs, Int32 numOfRuns,
							   Int32 numOfeolPos)
{
	if (paragraph == nullptr || parNum >= (BMhGetSize (reinterpret_cast<GSHandle> (paragraph)) / sizeof (API_ParagraphType)))
		return APIERR_BADPARS;

	if (numOfTabs < 1 || numOfRuns < 1 || numOfeolPos < 0)
		return APIERR_BADPARS;

	(*paragraph)[parNum].from = from;
	(*paragraph)[parNum].range = range;

	(*paragraph)[parNum].tab = reinterpret_cast<API_TabType*> (BMpAllClear (numOfTabs * sizeof (API_TabType)));
	(*paragraph)[parNum].run = reinterpret_cast<API_RunType*> (BMpAllClear (numOfRuns * sizeof (API_RunType)));

	if (numOfeolPos > 0) {
		(*paragraph)[parNum].eolPos = reinterpret_cast<Int32*> (BMpAllClear (numOfeolPos * sizeof (Int32)));
	}

	return NoError;
}

static GSErrCode SetRun (API_ParagraphType** paragraph, UInt32 parNum, UInt32 runNum, Int32 from, Int32 range, short pen, unsigned short faceBits,
						 short font, Int32 effectBits, double size)
{
	if (paragraph == nullptr || parNum >= (BMhGetSize (reinterpret_cast<GSHandle> (paragraph)) / sizeof (API_ParagraphType)))
		return APIERR_BADPARS;

	if (runNum >= BMGetPtrSize (reinterpret_cast<GSPtr> ((*paragraph)[parNum].run)) / sizeof (API_RunType))
		return APIERR_BADPARS;

	(*paragraph)[parNum].run[runNum].from	    = from;
	(*paragraph)[parNum].run[runNum].range	    = range;
	(*paragraph)[parNum].run[runNum].pen	    = pen;
	(*paragraph)[parNum].run[runNum].faceBits   = faceBits;
	(*paragraph)[parNum].run[runNum].font	    = font;
	(*paragraph)[parNum].run[runNum].effectBits = (unsigned short)effectBits;
	(*paragraph)[parNum].run[runNum].size	    = size;

	return NoError;
}

static GSErrCode SetEOL (API_ParagraphType** paragraph, UInt32 parNum, UInt32 eolNum, Int32 offset)
{
	if (paragraph == nullptr || parNum >= (BMhGetSize (reinterpret_cast<GSHandle> (paragraph)) / sizeof (API_ParagraphType)))
		return APIERR_BADPARS;

	if (eolNum >= BMGetPtrSize (reinterpret_cast<GSPtr> ((*paragraph)[parNum].eolPos)) / sizeof (Int32))
		return APIERR_BADPARS;

	if (offset < 0)
		return APIERR_BADPARS;

	(*paragraph)[parNum].eolPos[eolNum] = offset;

	return NoError;
}

static API_JustID ParseJustificationString (const GS::UniString& justification)
{
    if (justification == "Center") {
        return APIJust_Center;
    } else if (justification == "Right") {
        return APIJust_Right;
    } else if (justification == "Full") {
        return APIJust_Full;
    }
    return APIJust_Left;
}

static void SetTextContentAndParagraphs (API_ElementMemo& memo, API_TextType& textData, const GS::UniString& text)
{
#ifdef ServerMainVers_2800
    delete memo.textContent;
    memo.textContent = new GS::UniString { text };
#else
    memo.textContent = BMhAllClear ((text.GetLength () + 1) * sizeof (GS::uchar_t));
    GS::ucscpy (reinterpret_cast<GS::uchar_t*> (*memo.textContent), text.ToUStr ());
#endif

    const GS::UniChar newlineChar = GS::UniChar (char ('\n'));
    textData.nLine = text.Count (newlineChar) + 1;
    const Int32 numOfParagraphs = 1;
    memo.paragraphs = reinterpret_cast<API_ParagraphType**> (BMhAll (numOfParagraphs * sizeof (API_ParagraphType)));
    SetParagraph (memo.paragraphs, 0, 0, text.GetLength (), 1, 1, textData.nLine);
    SetRun (memo.paragraphs, 0, 0, 0, text.GetLength (), textData.pen, textData.faceBits, textData.font, textData.effectsBits, textData.size);
    Int32 lastEolPos = 0;
    for (Int32 eolIndex = 0; eolIndex < textData.nLine; ++eolIndex) {
        Int32 eolPos = text.FindFirst (newlineChar, eolIndex == 0 ? 0 : lastEolPos + 1);
        Int32 offset = (eolPos != MaxUIndex ? eolPos : text.GetLength ()) - lastEolPos - 1;
        lastEolPos = eolPos;
        SetEOL (memo.paragraphs, 0, eolIndex, offset);
    }

    textData.width = 0;
    textData.height = 0;
    textData.nonBreaking = true;
    textData.useEolPos = true;
}

GS::Optional<GS::ObjectState> CreateLabelsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories&, const GS::ObjectState& parameters) const
{
    parameters.Get ("floorInd", element.header.floorInd);

    const GS::ObjectState* begCOS = parameters.Get ("begCoordinate");
    const GS::ObjectState* midCOS = parameters.Get ("midCoordinate");
    const GS::ObjectState* endCOS = parameters.Get ("endCoordinate");

    element.label.parent = GetGuidFromArrayItem ("parentElementId", parameters);
    API_Elem_Head parentElemHead = {};
    if (element.label.parent != APINULLGuid) {
        parentElemHead.guid = element.label.parent;
        if (ACAPI_Element_GetHeader (&parentElemHead) == NoError) {
#ifdef ServerMainVers_2600
            element.label.parentType = parentElemHead.type;
#else
            element.label.parentType = parentElemHead.typeID;
#endif
        } else {
            return CreateErrorResponse (APIERR_BADPARS, "Invalid parent element GUID");
        }

        element.header.floorInd = parentElemHead.floorInd;
    }

    if (begCOS != nullptr) {
        element.label.begC = Get2DCoordinateFromObjectState (*begCOS);
    } else if (parentElemHead.guid != APINULLGuid) {
        API_Box3D box = {};
        ACAPI_Element_CalcBounds (&parentElemHead, &box);
        element.label.begC.x = (box.xMin + box.xMax) / 2.0;
        element.label.begC.y = (box.yMin + box.yMax) / 2.0;
    } else {
        return CreateErrorResponse (APIERR_BADPARS, "Missing 'begCoordinate' parameter");
    }


    if (midCOS != nullptr && endCOS != nullptr) {
        element.label.midC = Get2DCoordinateFromObjectState (*midCOS);
        element.label.endC = Get2DCoordinateFromObjectState (*endCOS);
        element.label.createAtDefaultPosition = false;
    } else {
        element.label.createAtDefaultPosition = true;
    }

    if (element.label.labelClass == APILblClass_Text) {
        GS::UniString text;
        if (!parameters.Get ("text", text)) {
            return CreateErrorResponse (APIERR_BADPARS, "Missing 'text' parameter for text label");
        }
        SetTextContentAndParagraphs (memo, element.label.u.text, text);
    }

    return {};
}

CreateTextsCommand::CreateTextsCommand () :
    CreateElementsCommandBase ("CreateTexts", API_TextID, "textsData")
{
}

GS::Optional<GS::UniString> CreateTextsCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "textsData": {
            "type": "array",
            "description": "Array of data to create Texts.",
            "items": {
                "type": "object",
                "description": "The parameters of the new Text element.",
                "properties": {
                    "coordinate": {
                        "$ref": "#/Coordinate3D",
                        "description": "The placement position of the text. The z value is used to determine the floor when floorIndex is omitted."
                    },
                    "text": {
                        "type": "string",
                        "description": "The text content. Newlines create multiple lines."
                    },
                    "height": {
                        "type": "number",
                        "description": "The character height in millimeters. Optional; defaults to the Text tool default."
                    },
                    "pen": {
                        "type": "integer",
                        "description": "Optional pen attribute index."
                    },
                    "angle": {
                        "type": "number",
                        "description": "Optional rotation angle in radians."
                    },
                    "justification": {
                        "type": "string",
                        "description": "Optional text justification.",
                        "enum": ["Left", "Center", "Right", "Full"]
                    },
                    "floorIndex": {
                        "type": "integer",
                        "description": "Optional floor index. If omitted, derived from the coordinate's z value."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "coordinate",
                    "text"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "textsData"
    ]
})";
}

GS::Optional<GS::ObjectState> CreateTextsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const
{
    const GS::ObjectState* coordinateOS = parameters.Get ("coordinate");
    if (coordinateOS == nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, "Missing 'coordinate' parameter");
    }
    API_Coord3D apiCoordinate = Get3DCoordinateFromObjectState (*coordinateOS);

    short floorIndex = 0;
    if (parameters.Get ("floorIndex", floorIndex)) {
        element.header.floorInd = floorIndex;
    } else {
        const auto floorIndexAndOffset = GetFloorIndexAndOffset (apiCoordinate.z, stories);
        element.header.floorInd = floorIndexAndOffset.first;
    }

    element.text.loc.x = apiCoordinate.x;
    element.text.loc.y = apiCoordinate.y;

    parameters.Get ("height", element.text.size);
    parameters.Get ("pen", element.text.pen);
    parameters.Get ("angle", element.text.angle);

    GS::UniString justification;
    if (parameters.Get ("justification", justification)) {
        element.text.just = ParseJustificationString (justification);
    }

    GS::UniString text;
    if (!parameters.Get ("text", text)) {
        return CreateErrorResponse (APIERR_BADPARS, "Missing 'text' parameter");
    }

    SetTextContentAndParagraphs (memo, element.text, text);

    return {};
}
