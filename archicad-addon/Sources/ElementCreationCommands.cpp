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

GS::Optional<GS::UniString> CreateElementsCommandBase::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/ElementIdsOrErrors"
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

        // Per-item "favoriteName" support. Favorites can only be applied through the
        // tool defaults, which are global and outlive the command, so the defaults are
        // snapshotted here and restored at the end - and also before any item that does
        // NOT name a favorite, otherwise it would silently inherit the favorite of the
        // item before it. Untouched (no allocation, no API traffic) when no item names
        // a favorite, which keeps the common path exactly as it was.
        const bool anyFavoriteName = [&] () {
            for (const GS::ObjectState& data : dataArray) {
                GS::UniString name;
                if (data.Get ("favoriteName", name) && !name.IsEmpty ()) {
                    return true;
                }
            }
            return false;
        } ();

        API_Element defaultsSnapshot = element;
        API_ElementMemo defaultsSnapshotMemo = {};
        const GS::OnExit snapshotGuard ([&] () {
            if (anyFavoriteName) {
                ACAPI_DisposeElemMemoHdls (&defaultsSnapshotMemo);
            }
        });
        if (anyFavoriteName) {
            ACAPI_Element_GetDefaults (&defaultsSnapshot, &defaultsSnapshotMemo);
        }
        bool favoriteApplied = false;

        for (const GS::ObjectState& data : dataArray) {
            if (anyFavoriteName) {
                GS::UniString favoriteName;
                const bool hasFavoriteName = data.Get ("favoriteName", favoriteName) && !favoriteName.IsEmpty ();
                if (hasFavoriteName) {
                    const GSErrCode favoriteErr = ApplyFavoriteToElementDefaults (favoriteName, elemTypeID);
                    if (favoriteErr != NoError) {
                        elements (CreateErrorResponse (favoriteErr,
                            "Failed to apply favoriteName '" + favoriteName + "' to the " + elemTypeName + " defaults."));
                        continue;
                    }
                    favoriteApplied = true;
                } else if (favoriteApplied) {
                    API_Element restoreMask;
                    ACAPI_ELEMENT_MASK_SETFULL (restoreMask);
                    ACAPI_Element_ChangeDefaults (&defaultsSnapshot, &defaultsSnapshotMemo, &restoreMask);
                    favoriteApplied = false;
                }
            }

            // Every item starts from the (favorite-applied or restored) tool defaults.
            // SetTypeSpecificParameters only writes the fields the item actually names,
            // so without this the previous item's values survive into this one: confirmed
            // live that CreateBeams with [{slantAngle: 0.3}, {}] slants BOTH beams, and
            // CreateWalls with [{arcAngle: 0.5}, {}] curves both walls.
            ACAPI_DisposeElemMemoHdls (&memo);
            memo = {};
            element = {};
#ifdef ServerMainVers_2600
            element.header.type   = elemTypeID;
#else
            element.header.typeID = elemTypeID;
#endif
            err = ACAPI_Element_GetDefaults (&element, &memo);
            if (err != NoError) {
                elements (CreateErrorResponse (err, "Failed to read the " + elemTypeName + " defaults."));
                continue;
            }

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

        // Leave the tool defaults as they were found: applying a favorite is a global
        // side effect that would otherwise change what the user's next manual placement
        // (or a later command) starts from.
        if (favoriteApplied) {
            API_Element restoreMask;
            ACAPI_ELEMENT_MASK_SETFULL (restoreMask);
            ACAPI_Element_ChangeDefaults (&defaultsSnapshot, &defaultsSnapshotMemo, &restoreMask);
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
                        "favoriteName": {
                            "type": "string",
                            "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                        },
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
                        },
                        "circleBased": {
                            "type": "boolean",
                            "description": "True for a round column cross section, false for rectangular. Ignored if profileId is also given. Applied to all segments."
                        },
                        "isWidthAndHeightLinked": {
                            "type": "boolean",
                            "description": "When true (the default), Archicad keeps width and depth equal and setting one changes the other - set to false to give width/depth independent values. Applied to all segments."
                        },
                        "buildingMaterialId": {
                            "$ref": "#/AttributeId",
                            "description": "Cross section building material (round or rectangular, per circleBased). Applied to all segments."
                        },
                        "profileId": {
                            "$ref": "#/AttributeId",
                            "description": "Switches the cross section to this custom extruded profile (circleBased becomes false). Applied to all segments."
                        },
                        "floorIndex": {
                            "type": "integer",
                            "description": "Optional floor index. If omitted, derived from the coordinate's z value."
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

    const auto floorIndexAndOffset = ResolveFloorIndexAndOffset (parameters, "floorIndex", apiCoordinate.z, stories);
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

    bool circleBased = false;
    const bool hasCircleBased = parameters.Get ("circleBased", circleBased);
    bool isWidthAndHeightLinked = false;
    const bool hasIsWidthAndHeightLinked = parameters.Get ("isWidthAndHeightLinked", isWidthAndHeightLinked);
    const GS::ObjectState* buildingMaterialIdOs = parameters.Get ("buildingMaterialId");
    const GS::ObjectState* profileIdOs = parameters.Get ("profileId");

    if ((hasWidth || hasDepth || hasCircleBased || hasIsWidthAndHeightLinked || buildingMaterialIdOs != nullptr || profileIdOs != nullptr) && memo.columnSegments != nullptr) {
        GSSize nSegments = BMGetPtrSize (reinterpret_cast<GSPtr>(memo.columnSegments)) / sizeof (API_ColumnSegmentType);
        for (GSSize i = 0; i < nSegments; ++i) {
            API_AssemblySegmentData& segment = memo.columnSegments[i].assemblySegmentData;
            if (hasIsWidthAndHeightLinked) {
                segment.isWidthAndHeightLinked = isWidthAndHeightLinked;
            }
            if (hasWidth) {
                segment.nominalWidth = width;
            }
            if (hasDepth) {
                segment.nominalHeight = depth;
            }
            if (hasCircleBased) {
                segment.circleBased = circleBased;
            }
            if (profileIdOs != nullptr) {
                segment.modelElemStructureType = API_ProfileStructure;
                segment.profileAttr = GetAttributeIndexFromGuid (API_ProfileID, GetGuidFromObjectState (*profileIdOs));
                segment.circleBased = false;
            } else if (buildingMaterialIdOs != nullptr) {
                segment.modelElemStructureType = API_BasicStructure;
                segment.buildingMaterial = GetAttributeIndexFromGuid (API_BuildingMaterialID, GetGuidFromObjectState (*buildingMaterialIdOs));
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
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                    },
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
                    },
                    "floorIndex": {
                        "type": "integer",
                        "description": "Optional floor index. If omitted, derived from level."
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
    const auto floorIndexAndOffset = ResolveFloorIndexAndOffset (parameters, "floorIndex", inputLevel, stories);
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
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                    },
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
        auto holesError = ValidateHoles (holes);
        if (holesError.HasValue ()) {
            return CreateErrorResponse (APIERR_BADPARS, holesError.Get ());
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

// Shared "line-family settings" properties for Line/PolyLine/Arc/Circle/Spline creation
// (API_LineType/API_PolyLineType/API_ArcType/API_SplineType all share this exact shape).
// NOTE: no penWeight field here - confirmed live that a raw penWeight override does not
// reliably take visible effect in Archicad; assign a linePenIndex whose pen already has the
// desired weight in the project's pen table instead.
static const char* LineFamilySettingsSchemaProperties = R"(
                    "roomSeparator": {
                        "type": "boolean",
                        "description": "Is this a zone boundary line? Optional, defaults to false."
                    },
                    "linePenIndex": {
                        "type": "integer",
                        "description": "Optional pen index. By default the current pen is used."
                    },
                    "lineTypeId": {
                        "$ref": "#/AttributeId",
                        "description": "Optional line type attribute. By default the current line type is used."
                    })";

static void SetLineFamilySettingsParameters (const GS::ObjectState& parameters, API_ExtendedPenType& linePen, API_AttributeIndex& ltypeInd, bool& roomSeparator)
{
    short penIndex = 0;
    if (parameters.Get ("linePenIndex", penIndex)) {
        linePen.penIndex = penIndex;
        linePen.colorOverridePenIndex = 0;
    }
    const GS::ObjectState* lineTypeId = parameters.Get ("lineTypeId");
    if (lineTypeId != nullptr) {
        ltypeInd = GetAttributeIndexFromGuid (API_LinetypeID, GetGuidFromObjectState (*lineTypeId));
    }
    parameters.Get ("roomSeparator", roomSeparator);
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
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                    },
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
                    "roomSeparator": {
                        "type": "boolean",
                        "description": "Is this a zone boundary line? Optional, defaults to false."
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

    parameters.Get ("roomSeparator", element.polyLine.roomSeparator);

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

CreateLineElementsCommand::CreateLineElementsCommand () :
    CreateElementsCommandBase ("CreateLineElements", API_LineID, "linesData")
{
}

GS::Optional<GS::UniString> CreateLineElementsCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
    "type": "object",
    "properties": {
        "linesData": {
            "type": "array",
            "description": "Array of data to create Lines.",
            "items": {
                "type": "object",
                "description": "The parameters of the new Line.",
                "properties": {
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                    },
                    "floorInd": {
                        "type": "number",
                        "description": "The identifier of the floor. Optional parameter, by default the current floor is used."
                    },
                    "layerIndex": {
                        "type": "integer",
                        "description": "Layer attribute index to place the line on. Optional parameter, by default the current layer is used."
                    },
                    "begCoordinate": {
                        "$ref": "#/Coordinate2D"
                    },
                    "endCoordinate": {
                        "$ref": "#/Coordinate2D"
                    },)") + LineFamilySettingsSchemaProperties + R"(
                },
                "additionalProperties": false,
                "required": [
                    "begCoordinate",
                    "endCoordinate"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "linesData"
    ]
})";
}

GS::Optional<GS::ObjectState> CreateLineElementsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& /*memo*/, const Stories& /*stories*/, const GS::ObjectState& parameters) const
{
    parameters.Get ("floorInd", element.header.floorInd);

    Int32 layerIndex = 0;
    if (parameters.Get ("layerIndex", layerIndex) && layerIndex > 0) {
        element.header.layer = ACAPI_CreateAttributeIndex (layerIndex);
    }

    GS::ObjectState begCoordinate;
    parameters.Get ("begCoordinate", begCoordinate);
    element.line.begC = Get2DCoordinateFromObjectState (begCoordinate);

    GS::ObjectState endCoordinate;
    parameters.Get ("endCoordinate", endCoordinate);
    element.line.endC = Get2DCoordinateFromObjectState (endCoordinate);

    SetLineFamilySettingsParameters (parameters, element.line.linePen, element.line.ltypeInd, element.line.roomSeparator);

    return {};
}

CreateArcsCommand::CreateArcsCommand () :
    CreateElementsCommandBase ("CreateArcs", API_ArcID, "arcsData")
{
}

GS::Optional<GS::UniString> CreateArcsCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
    "type": "object",
    "properties": {
        "arcsData": {
            "type": "array",
            "description": "Array of data to create Arcs.",
            "items": {
                "type": "object",
                "description": "The parameters of the new Arc.",
                "properties": {
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                    },
                    "floorInd": {
                        "type": "number",
                        "description": "The identifier of the floor. Optional parameter, by default the current floor is used."
                    },
                    "layerIndex": {
                        "type": "integer",
                        "description": "Layer attribute index to place the arc on. Optional parameter, by default the current layer is used."
                    },
                    "origin": {
                        "$ref": "#/Coordinate2D"
                    },
                    "radius": {
                        "type": "number"
                    },
                    "begAngle": {
                        "type": "number",
                        "description": "Beginning angle of the arc in radians."
                    },
                    "endAngle": {
                        "type": "number",
                        "description": "End angle of the arc in radians."
                    },)") + LineFamilySettingsSchemaProperties + R"(
                },
                "additionalProperties": false,
                "required": [
                    "origin",
                    "radius",
                    "begAngle",
                    "endAngle"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "arcsData"
    ]
})";
}

GS::Optional<GS::ObjectState> CreateArcsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& /*memo*/, const Stories& /*stories*/, const GS::ObjectState& parameters) const
{
    parameters.Get ("floorInd", element.header.floorInd);

    Int32 layerIndex = 0;
    if (parameters.Get ("layerIndex", layerIndex) && layerIndex > 0) {
        element.header.layer = ACAPI_CreateAttributeIndex (layerIndex);
    }

    GS::ObjectState origin;
    parameters.Get ("origin", origin);
    element.arc.origC = Get2DCoordinateFromObjectState (origin);

    parameters.Get ("radius", element.arc.r);
    element.arc.angle = 0.0;
    element.arc.ratio = 1.0;
    parameters.Get ("begAngle", element.arc.begAng);
    parameters.Get ("endAngle", element.arc.endAng);

    SetLineFamilySettingsParameters (parameters, element.arc.linePen, element.arc.ltypeInd, element.arc.roomSeparator);

    return {};
}

CreateCirclesCommand::CreateCirclesCommand () :
    CreateElementsCommandBase ("CreateCircles", API_CircleID, "circlesData")
{
}

GS::Optional<GS::UniString> CreateCirclesCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
    "type": "object",
    "properties": {
        "circlesData": {
            "type": "array",
            "description": "Array of data to create Circles.",
            "items": {
                "type": "object",
                "description": "The parameters of the new Circle.",
                "properties": {
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                    },
                    "floorInd": {
                        "type": "number",
                        "description": "The identifier of the floor. Optional parameter, by default the current floor is used."
                    },
                    "layerIndex": {
                        "type": "integer",
                        "description": "Layer attribute index to place the circle on. Optional parameter, by default the current layer is used."
                    },
                    "origin": {
                        "$ref": "#/Coordinate2D"
                    },
                    "radius": {
                        "type": "number"
                    },)") + LineFamilySettingsSchemaProperties + R"(
                },
                "additionalProperties": false,
                "required": [
                    "origin",
                    "radius"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "circlesData"
    ]
})";
}

GS::Optional<GS::ObjectState> CreateCirclesCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& /*memo*/, const Stories& /*stories*/, const GS::ObjectState& parameters) const
{
    parameters.Get ("floorInd", element.header.floorInd);

    Int32 layerIndex = 0;
    if (parameters.Get ("layerIndex", layerIndex) && layerIndex > 0) {
        element.header.layer = ACAPI_CreateAttributeIndex (layerIndex);
    }

    GS::ObjectState origin;
    parameters.Get ("origin", origin);
    element.circle.origC = Get2DCoordinateFromObjectState (origin);

    parameters.Get ("radius", element.circle.r);
    element.circle.angle = 0.0;
    element.circle.ratio = 1.0;

    SetLineFamilySettingsParameters (parameters, element.circle.linePen, element.circle.ltypeInd, element.circle.roomSeparator);

    return {};
}

CreateHotspotsCommand::CreateHotspotsCommand () :
    CreateElementsCommandBase ("CreateHotspots", API_HotspotID, "hotspotsData")
{
}

GS::Optional<GS::UniString> CreateHotspotsCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "hotspotsData": {
            "type": "array",
            "description": "Array of data to create Hotspots.",
            "items": {
                "type": "object",
                "description": "The parameters of the new Hotspot.",
                "properties": {
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                    },
                    "floorInd": {
                        "type": "number",
                        "description": "The identifier of the floor. Optional parameter, by default the current floor is used."
                    },
                    "layerIndex": {
                        "type": "integer",
                        "description": "Layer attribute index to place the hotspot on. Optional parameter, by default the current layer is used."
                    },
                    "position": {
                        "$ref": "#/Coordinate2D"
                    },
                    "height": {
                        "type": "number"
                    },
                    "penIndex": {
                        "type": "integer",
                        "description": "Optional pen index. By default the current pen is used."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "position"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "hotspotsData"
    ]
})";
}

GS::Optional<GS::ObjectState> CreateHotspotsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& /*memo*/, const Stories& /*stories*/, const GS::ObjectState& parameters) const
{
    parameters.Get ("floorInd", element.header.floorInd);

    Int32 layerIndex = 0;
    if (parameters.Get ("layerIndex", layerIndex) && layerIndex > 0) {
        element.header.layer = ACAPI_CreateAttributeIndex (layerIndex);
    }

    GS::ObjectState position;
    parameters.Get ("position", position);
    element.hotspot.pos = Get2DCoordinateFromObjectState (position);

    parameters.Get ("height", element.hotspot.height);

    short penIndex = 0;
    if (parameters.Get ("penIndex", penIndex)) {
        element.hotspot.pen = penIndex;
    }

    return {};
}

CreateHatchesCommand::CreateHatchesCommand () :
    CreateElementsCommandBase ("CreateHatches", API_HatchID, "hatchesData")
{
}

GS::Optional<GS::UniString> CreateHatchesCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "hatchesData": {
            "type": "array",
            "description": "Array of data to create Hatches.",
            "items": {
                "type": "object",
                "description": "The parameters of the new Hatch.",
                "properties": {
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                    },
                    "floorInd": {
                        "type": "number",
                        "description": "The identifier of the floor. Optional parameter, by default the current floor is used."
                    },
                    "layerIndex": {
                        "type": "integer",
                        "description": "Layer attribute index to place the hatch on. Optional parameter, by default the current layer is used."
                    },
                    "coordinates": {
                        "type": "array",
                        "description": "The 2D coordinates of the hatch outline (single contour, no holes). Do not repeat the first point at the end.",
                        "items": {
                            "$ref": "#/Coordinate2D"
                        },
                        "minItems": 3
                    },
                    "arcs": {
                        "type": "array",
                        "description": "The arcs of the hatch outline.",
                        "items": {
                            "$ref": "#/PolyArc"
                        }
                    },
                    "contourPenIndex": {
                        "type": "integer",
                        "description": "Optional pen index for the contour. By default the current pen is used."
                    },
                    "fillPenIndex": {
                        "type": "integer",
                        "description": "Optional pen index for the fill. By default the current pen is used."
                    },
                    "fillBackgroundPenIndex": {
                        "type": "integer"
                    },
                    "fillId": {
                        "$ref": "#/AttributeId",
                        "description": "Optional fill attribute. By default the current fill is used."
                    },
                    "buildingMaterialId": {
                        "$ref": "#/AttributeId"
                    },
                    "roomSpecial": {
                        "type": "integer",
                        "description": "Special area percent in a room (negative means OFF)."
                    },
                    "showArea": {
                        "type": "boolean"
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
        "hatchesData"
    ]
})";
}

GS::Optional<GS::ObjectState> CreateHatchesCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& /*stories*/, const GS::ObjectState& parameters) const
{
    parameters.Get ("floorInd", element.header.floorInd);

    Int32 layerIndex = 0;
    if (parameters.Get ("layerIndex", layerIndex) && layerIndex > 0) {
        element.header.layer = ACAPI_CreateAttributeIndex (layerIndex);
    }

    GS::Array<GS::ObjectState> coordinates;
    GS::Array<GS::ObjectState> arcs;
    parameters.Get ("coordinates", coordinates);
    parameters.Get ("arcs", arcs);
    const GS::Array<API_PolyArc> polyArcs = GetPolyArcs (arcs, 1);

    const Int32 nUnique = (Int32) coordinates.GetSize ();
    const Int32 nCoords = nUnique + 1; // + closing duplicate vertex (Hatch is a closed polygon)

    memo.coords    = reinterpret_cast<API_Coord**>   (BMAllocateHandle ((nCoords + 1) * sizeof (API_Coord), ALLOCATE_CLEAR, 0));
    memo.pends     = reinterpret_cast<Int32**>       (BMAllocateHandle (2 * sizeof (Int32), ALLOCATE_CLEAR, 0));
    memo.parcs     = reinterpret_cast<API_PolyArc**> (BMAllocateHandle (polyArcs.GetSize () * sizeof (API_PolyArc), ALLOCATE_CLEAR, 0));
    memo.vertexIDs = reinterpret_cast<UInt32**>      (BMAllocateHandle ((nCoords + 1) * sizeof (UInt32), ALLOCATE_CLEAR, 0));
    if (memo.coords != nullptr && memo.pends != nullptr && memo.vertexIDs != nullptr) {
        (*memo.coords)[0] = { 0.0, 0.0 };
        (*memo.vertexIDs)[0] = (UInt32) nUnique;
        for (Int32 i = 0; i < nUnique; ++i) {
            (*memo.coords)[i + 1] = Get2DCoordinateFromObjectState (coordinates[i]);
            (*memo.vertexIDs)[i + 1] = (UInt32) (i + 1);
        }
        (*memo.coords)[nCoords] = (*memo.coords)[1];
        (*memo.vertexIDs)[nCoords] = (*memo.vertexIDs)[1];
        (*memo.pends)[0] = 0;
        (*memo.pends)[1] = nCoords;
        Int32 iArc = 0;
        for (const API_PolyArc& a : polyArcs)
            (*memo.parcs)[iArc++] = a;

        element.hatch.poly.nCoords   = nCoords;
        element.hatch.poly.nSubPolys = 1;
        element.hatch.poly.nArcs     = (Int32) polyArcs.GetSize ();
    }

    short contourPenIndex = 0;
    if (parameters.Get ("contourPenIndex", contourPenIndex)) {
        element.hatch.contPen.penIndex = contourPenIndex;
        element.hatch.contPen.colorOverridePenIndex = 0;
    }
    short fillPenIndex = 0;
    if (parameters.Get ("fillPenIndex", fillPenIndex)) {
        element.hatch.fillPen.penIndex = fillPenIndex;
        element.hatch.fillPen.colorOverridePenIndex = 0;
    }
    parameters.Get ("fillBackgroundPenIndex", element.hatch.fillBGPen);
    const GS::ObjectState* fillId = parameters.Get ("fillId");
    if (fillId != nullptr) {
        element.hatch.fillInd = GetAttributeIndexFromGuid (API_FilltypeID, GetGuidFromObjectState (*fillId));
    }
    const GS::ObjectState* buildingMaterialId = parameters.Get ("buildingMaterialId");
    if (buildingMaterialId != nullptr) {
        element.hatch.buildingMaterial = GetAttributeIndexFromGuid (API_BuildingMaterialID, GetGuidFromObjectState (*buildingMaterialId));
    }
    Int32 roomSpecial = 0;
    if (parameters.Get ("roomSpecial", roomSpecial)) {
        element.hatch.roomSpecial = (char) roomSpecial;
    }
    parameters.Get ("showArea", element.hatch.showArea);

    return {};
}

CreateSplinesCommand::CreateSplinesCommand () :
    CreateElementsCommandBase ("CreateSplines", API_SplineID, "splinesData")
{
}

GS::Optional<GS::UniString> CreateSplinesCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
    "type": "object",
    "properties": {
        "splinesData": {
            "type": "array",
            "description": "Array of data to create Splines. Only auto-smoothed curves are supported (bezier handle positions are calculated automatically by Archicad from the point positions).",
            "items": {
                "type": "object",
                "description": "The parameters of the new Spline.",
                "properties": {
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                    },
                    "floorInd": {
                        "type": "number",
                        "description": "The identifier of the floor. Optional parameter, by default the current floor is used."
                    },
                    "layerIndex": {
                        "type": "integer",
                        "description": "Layer attribute index to place the spline on. Optional parameter, by default the current layer is used."
                    },
                    "coordinates": {
                        "type": "array",
                        "description": "The 2D coordinates of the spline points. Do not repeat the first point at the end even for a closed spline.",
                        "items": {
                            "$ref": "#/Coordinate2D"
                        },
                        "minItems": 3
                    },
                    "closed": {
                        "type": "boolean",
                        "description": "Is this a closed curve? Optional, defaults to false."
                    },)") + LineFamilySettingsSchemaProperties + R"(
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
        "splinesData"
    ]
})";
}

GS::Optional<GS::ObjectState> CreateSplinesCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& /*stories*/, const GS::ObjectState& parameters) const
{
    parameters.Get ("floorInd", element.header.floorInd);

    Int32 layerIndex = 0;
    if (parameters.Get ("layerIndex", layerIndex) && layerIndex > 0) {
        element.header.layer = ACAPI_CreateAttributeIndex (layerIndex);
    }

    GS::Array<GS::ObjectState> coordinates;
    parameters.Get ("coordinates", coordinates);
    parameters.Get ("closed", element.spline.closed);
    element.spline.autoSmooth = true; // bezierDirs handle-editing not supported by this command

    const Int32 nPoints = (Int32) coordinates.GetSize ();
    memo.coords = reinterpret_cast<API_Coord**> (BMAllocateHandle (nPoints * sizeof (API_Coord), ALLOCATE_CLEAR, 0));
    if (memo.coords != nullptr) {
        for (Int32 i = 0; i < nPoints; ++i) {
            (*memo.coords)[i] = Get2DCoordinateFromObjectState (coordinates[i]);
        }
    }

    SetLineFamilySettingsParameters (parameters, element.spline.linePen, element.spline.ltypeInd, element.spline.roomSeparator);

    return {};
}

// Shared schema builder for CreateObjects and CreateLamps.
//
// API_LampType is a typedef alias of API_ObjectType in APIdefs_Elements.h
// (verified across AC25..29), so the two commands differ only in name,
// elemTypeID, JSON array field name, and the singular noun / description
// used in the schema.
// The properties shared by CreateObjects/CreateLamps (on top of libraryPartName/coordinates) and
// ModifyObjects/ModifyLamps (all of these, nothing else) - kept as one string so the two schemas
// can't drift apart. lightColor/lightIsOn are only meaningful for Lamps, gated by `isLamp`.
static GS::UniString BuildObjectLampDetailFields (bool isLamp)
{
    GS::UniString fields = R"(
        "angle": { "type": "number" },
        "pen": { "type": "integer" },
        "lineTypeId": { "$ref": "#/AttributeId" },
        "surfaceId": { "$ref": "#/AttributeId", "description": "Material/Surface override (API_ObjectType.mat)." },
        "sectionFillId": { "$ref": "#/AttributeId" },
        "sectionFillPen": { "type": "integer" },
        "sectionFillBackgroundPen": { "type": "integer" },
        "sectionContourPen": { "type": "integer" },
        "useObjectPens": { "type": "boolean" },
        "useObjectLineTypes": { "type": "boolean" },
        "useObjectMaterials": { "type": "boolean" },
        "useObjectSectionAttributes": { "type": "boolean" },
        "reflected": { "type": "boolean" },
        "useFixSize": { "type": "boolean" },
        "fixPoint": { "type": "integer", "description": "0-based index of the hotspot to keep fixed (raw API_ObjectType.fixPoint value, not 1-based)." },
        "offset": { "$ref": "#/Coordinate2D", "description": "Offset of the symbol's origin from the insertion point. Reported accurately on Get, but confirmed live that Archicad silently discards this value through both Create and Modify (Get always reports the library part's own default hotspot offset regardless of what is sent) - same class of read-only-in-practice field as Morph's bodyType/edgeType/level." },
        "useFixedAngle": { "type": "boolean", "description": "Use a fixed rotation angle. Reported accurately on Get, but confirmed live that Archicad silently discards this value through both Create and Modify." },
        "isAutoOnStoryVisibility": { "type": "boolean" },
        "visibility": {
            "type": "object",
            "properties": {
                "showOnHome": { "type": "boolean" },
                "showAllAbove": { "type": "boolean" },
                "showAllBelow": { "type": "boolean" },
                "showRelAbove": { "type": "integer" },
                "showRelBelow": { "type": "integer" }
            },
            "additionalProperties": false
        },
        "linkToSettings": {
            "type": "object",
            "properties": {
                "homeStoryDifference": { "type": "integer" },
                "newCreationMode": { "type": "boolean" }
            },
            "additionalProperties": false
        })";
    if (isLamp) {
        fields += R"(,
        "lightColor": { "$ref": "#/ColorRGB", "description": "Reported accurately on Get, but confirmed live that Archicad silently discards this value through both Create and Modify (Get always reports the library part's own default light color). lightIsOn (the on/off state, as opposed to the color) does not have this problem." },
        "lightIsOn": { "type": "boolean" })";
    }
    return fields;
}

// API_LampType is a typedef alias of API_ObjectType in APIdefs_Elements.h (verified across
// AC25..29), so Object and Lamp share one schema shape beyond the element-type-specific
// description strings.
//
// Built via plain GS::UniString concatenation rather than a single GS::UniString::Printf call -
// confirmed live (crash dumps in APICommandBridge.dll, ACCESS_VIOLATION) that feeding the large
// BuildObjectLampDetailFields blob through Printf's %s reliably crashed Archicad at add-on
// registration time (RegisterCommand<T> calls GetInputParametersSchema() at startup); isolated by
// bisection to the SIZE of that single %s argument, not its content (a 1-line stand-in placeholder
// through the same Printf call never crashed). Root cause not confirmed further (suspected fixed-size
// internal buffer in GS::UniString::Printf/PrintfFwd's va_arg handling) - concatenating instead
// avoids the vararg path entirely for the large piece.
static GS::UniString BuildLibraryPartBasedSchema (const char* arrayFieldName,
                                                  const char* elementSingularName,
                                                  const char* libraryPartNameDescription,
                                                  bool isLamp)
{
    GS::UniString schema = GS::UniString::Printf (R"({
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
                        },
                        )",
        arrayFieldName, elementSingularName, elementSingularName, libraryPartNameDescription);

    schema += BuildObjectLampDetailFields (isLamp);

    schema += GS::UniString::Printf (R"(,
                        "floorIndex": {
                            "type": "integer",
                            "description": "Optional floor index. If omitted, derived from the coordinate's z value."
                        },
                        "favoriteName": {
                            "type": "string",
                            "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
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
    })", arrayFieldName);

    return schema;
}

CreateObjectsCommand::CreateObjectsCommand () :
    CreateElementsCommandBase ("CreateObjects", API_ObjectID, "objectsData")
{
}

GS::Optional<GS::UniString> CreateObjectsCommand::GetInputParametersSchema () const
{
    return BuildLibraryPartBasedSchema ("objectsData", "Object",
                                        "The name of the library part to use.", false);
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

// Resolves 'libraryPartName' to element.object.libInd - Create only. Modify doesn't support
// swapping an existing instance's library part, only patching its placement/attribute fields.
static GS::Optional<GS::ObjectState> ResolveLibraryPartName (API_Element& element, const GS::ObjectState& parameters)
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
    return {};
}

static bool ResolveAttributeIndex (const GS::ObjectState& attributeId, API_AttrTypeID attributeType, API_AttributeIndex& attributeIndex)
{
    API_Attribute attribute = {};
    attribute.header.typeID = attributeType;
    attribute.header.guid = GetGuidFromObjectState (attributeId);
    if (attribute.header.guid == APINULLGuid) {
        return false;
    }

    if (ACAPI_Attribute_Get (&attribute) != NoError) {
        return false;
    }

    attributeIndex = attribute.header.index;
    return true;
}

// Applies every optional API_ObjectType field beyond the library part itself - coordinates,
// dimensions, angle, pen/line type/surface/section attributes, fixed-size/angle behavior,
// per-story visibility, link-to-story, and (Lamp only) light color/on-off - shared by
// CreateObjects/CreateLamps (mask == nullptr, a freshly-defaulted element needs no mask) and
// ModifyObjects/ModifyLamps (mask != nullptr, every touched field also needs its
// ACAPI_ELEMENT_MASK_SET bit). element.object/.lamp alias the same union storage (API_LampType is
// a typedef of API_ObjectType across the AC25..29 SDKs we target), so writing through
// element.object.* is correct for both API_ObjectID and API_LampID.
static GS::Optional<GS::ObjectState> ApplyObjectLampDetails (
    API_Element& element, API_ElementMemo& memo, API_Element* mask,
    const Stories& stories, const GS::ObjectState& parameters)
{
    GS::ObjectState coordinates;
    if (parameters.Get ("coordinates", coordinates)) {
        const API_Coord3D apiCoordinate = Get3DCoordinateFromObjectState (coordinates);

        element.object.pos.x = apiCoordinate.x;
        element.object.pos.y = apiCoordinate.y;

        const auto floorIndexAndOffset = ResolveFloorIndexAndOffset (parameters, "floorIndex", apiCoordinate.z, stories);
        element.header.floorInd = floorIndexAndOffset.first;
        element.object.level = floorIndexAndOffset.second;
        if (mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET (*mask, API_Elem_Head, floorInd);
            ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, pos);
            ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, level);
        }
    }

    GS::ObjectState dimensions;
    if (parameters.Get ("dimensions", dimensions)) {
        const API_Coord3D dims = Get3DCoordinateFromObjectState (dimensions);

        element.object.xRatio = dims.x;
        element.object.yRatio = dims.y;
        GS::ObjectState os (ParameterValueFieldName, dims.z);
        ChangeParams (memo.params, {{"ZZYZX", os}});
        if (mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, xRatio);
            ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, yRatio);
        }
    }

    double angle = 0.0;
    if (parameters.Get ("angle", angle)) {
        element.object.angle = angle;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, angle);
    }

    Int32 penValue = 0;
    if (parameters.Get ("pen", penValue)) {
        element.object.pen = (short) penValue;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, pen);
    }

    const GS::ObjectState* lineTypeIdOS = parameters.Get ("lineTypeId");
    if (lineTypeIdOS != nullptr) {
        API_AttributeIndex idx = APIInvalidAttributeIndex;
        if (!ResolveAttributeIndex (*lineTypeIdOS, API_LinetypeID, idx)) {
            return CreateErrorResponse (APIERR_BADPARS, "Invalid 'lineTypeId' line type reference.");
        }
        element.object.ltypeInd = idx;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, ltypeInd);
    }

    const GS::ObjectState* surfaceIdOS = parameters.Get ("surfaceId");
    if (surfaceIdOS != nullptr) {
        API_AttributeIndex idx = APIInvalidAttributeIndex;
        if (!ResolveAttributeIndex (*surfaceIdOS, API_MaterialID, idx)) {
            return CreateErrorResponse (APIERR_BADPARS, "Invalid 'surfaceId' surface reference.");
        }
        element.object.mat = idx;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, mat);
    }

    const GS::ObjectState* sectionFillIdOS = parameters.Get ("sectionFillId");
    if (sectionFillIdOS != nullptr) {
        API_AttributeIndex idx = APIInvalidAttributeIndex;
        if (!ResolveAttributeIndex (*sectionFillIdOS, API_FilltypeID, idx)) {
            return CreateErrorResponse (APIERR_BADPARS, "Invalid 'sectionFillId' fill reference.");
        }
        element.object.sectFill = idx;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, sectFill);
    }

    Int32 shortValue = 0;
    if (parameters.Get ("sectionFillPen", shortValue)) {
        element.object.sectFillPen = (short) shortValue;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, sectFillPen);
    }
    if (parameters.Get ("sectionFillBackgroundPen", shortValue)) {
        element.object.sectBGPen = (short) shortValue;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, sectBGPen);
    }
    if (parameters.Get ("sectionContourPen", shortValue)) {
        element.object.sectContPen = (short) shortValue;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, sectContPen);
    }

    bool boolValue = false;
    if (parameters.Get ("useObjectPens", boolValue)) {
        element.object.useObjPens = boolValue;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, useObjPens);
    }
    if (parameters.Get ("useObjectLineTypes", boolValue)) {
        element.object.useObjLtypes = boolValue;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, useObjLtypes);
    }
    if (parameters.Get ("useObjectMaterials", boolValue)) {
        element.object.useObjMaterials = boolValue;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, useObjMaterials);
    }
    if (parameters.Get ("useObjectSectionAttributes", boolValue)) {
        element.object.useObjSectAttrs = boolValue;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, useObjSectAttrs);
    }
    if (parameters.Get ("reflected", boolValue)) {
        element.object.reflected = boolValue;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, reflected);
    }
    if (parameters.Get ("useFixSize", boolValue)) {
        element.object.useXYFixSize = boolValue;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, useXYFixSize);
    }

    Int32 fixPointValue = 0;
    if (parameters.Get ("fixPoint", fixPointValue)) {
        element.object.fixPoint = (short) fixPointValue;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, fixPoint);
    }

    const GS::ObjectState* offsetOS = parameters.Get ("offset");
    if (offsetOS != nullptr) {
        element.object.offset = Get2DCoordinateFromObjectState (*offsetOS);
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, offset);
    }

    if (parameters.Get ("useFixedAngle", boolValue)) {
        element.object.fixedAngle = boolValue ? 1 : 0;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, fixedAngle);
    }
    if (parameters.Get ("isAutoOnStoryVisibility", boolValue)) {
        element.object.isAutoOnStoryVisibility = boolValue;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, isAutoOnStoryVisibility);
    }

    GS::ObjectState colorOS;
    if (parameters.Get ("lightColor", colorOS)) {
        element.object.lightColor = GetColorFromObjectState (colorOS);
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, lightColor);
    }
    if (parameters.Get ("lightIsOn", boolValue)) {
        element.object.lightIsOn = boolValue;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, lightIsOn);
    }

    const GS::ObjectState* visibilityOS = parameters.Get ("visibility");
    if (visibilityOS != nullptr) {
        API_StoryVisibility vis = element.object.visibility;
        visibilityOS->Get ("showOnHome", vis.showOnHome);
        visibilityOS->Get ("showAllAbove", vis.showAllAbove);
        visibilityOS->Get ("showAllBelow", vis.showAllBelow);
        Int32 relAbove = vis.showRelAbove;
        Int32 relBelow = vis.showRelBelow;
        if (visibilityOS->Get ("showRelAbove", relAbove)) {
            vis.showRelAbove = (short) relAbove;
        }
        if (visibilityOS->Get ("showRelBelow", relBelow)) {
            vis.showRelBelow = (short) relBelow;
        }
        element.object.visibility = vis;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, visibility);
    }

    const GS::ObjectState* linkOS = parameters.Get ("linkToSettings");
    if (linkOS != nullptr) {
        API_LinkToSettings link = element.object.linkToSettings;
        Int32 homeDiff = link.homeStoryDifference;
        if (linkOS->Get ("homeStoryDifference", homeDiff)) {
            link.homeStoryDifference = (short) homeDiff;
        }
        linkOS->Get ("newCreationMode", link.newCreationMode);
        element.object.linkToSettings = link;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, linkToSettings);
    }

    return {};
}

GS::Optional<GS::ObjectState> CreateObjectsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const
{
    auto err = ResolveLibraryPartName (element, parameters);
    if (err.HasValue ()) {
        return err;
    }
    return ApplyObjectLampDetails (element, memo, nullptr, stories, parameters);
}

// CreateLamps — parallel to CreateObjects but for AC subtype Lamp.
// Without this command, lamp libparts cannot be placed via Tapir —
// CreateObjects rejects them with APIERR_NOTSUPPORTED. The schema and
// placement logic are shared via BuildLibraryPartBasedSchema and
// ApplyObjectLampDetails above.
CreateLampsCommand::CreateLampsCommand () :
    CreateElementsCommandBase ("CreateLamps", API_LampID, "lampsData")
{
}

GS::Optional<GS::UniString> CreateLampsCommand::GetInputParametersSchema () const
{
    return BuildLibraryPartBasedSchema ("lampsData", "Lamp",
                                        "The name of the lamp library part to use.", true);
}

GS::Optional<GS::ObjectState> CreateLampsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const
{
    auto err = ResolveLibraryPartName (element, parameters);
    if (err.HasValue ()) {
        return err;
    }
    return ApplyObjectLampDetails (element, memo, nullptr, stories, parameters);
}

// Shared Modify schema builder for ModifyObjects/ModifyLamps, mirroring
// BuildLibraryPartBasedSchema's Create-side counterpart - same field list via
// BuildObjectLampDetailFields, minus libraryPartName/coordinates being required (Modify only
// patches the fields actually given).
// Built via concatenation, not a single Printf %s - see BuildLibraryPartBasedSchema's comment for
// why (confirmed live crash feeding the large BuildObjectLampDetailFields blob through Printf).
static GS::UniString BuildLibraryPartBasedModifySchema (const char* arrayFieldName, bool isLamp)
{
    GS::UniString schema = GS::UniString::Printf (R"({
        "type": "object",
        "properties": {
            "%s": {
                "type": "array",
                "description": "Array of elements to modify, with the fields to change. Only provided fields are changed; omitted fields are left as-is.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "coordinates": {
                            "$ref": "#/Coordinate3D"
                        },
                        "dimensions": {
                            "$ref": "#/Dimensions3D"
                        },
                        )",
        arrayFieldName);

    schema += BuildObjectLampDetailFields (isLamp);

    schema += GS::UniString::Printf (R"(
                    },
                    "additionalProperties": false,
                    "required" : [
                        "elementId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "%s"
        ]
    })", arrayFieldName);

    return schema;
}

static GS::ObjectState ExecuteModifyObjectOrLamp (const GS::ObjectState& parameters, const char* arrayFieldName, API_ElemTypeID elemTypeID, const char* undoableCommandName)
{
    GS::Array<GS::ObjectState> itemsWithDetails;
    parameters.Get (arrayFieldName, itemsWithDetails);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand (undoableCommandName, [&] () -> GSErrCode {
        for (const GS::ObjectState& item : itemsWithDetails) {
            const GS::ObjectState* elementId = item.Get ("elementId");
            if (elementId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
                continue;
            }

            API_Element element = {};
            element.header.guid = GetGuidFromObjectState (*elementId);
            GSErrCode err = ACAPI_Element_Get (&element);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to find the element"));
                continue;
            }

            if (GetElemTypeId (element.header) != elemTypeID) {
                executionResults (CreateFailedExecutionResult (APIERR_BADID, "Element is not of the expected type."));
                continue;
            }

            API_Element mask = {};
            ACAPI_ELEMENT_MASK_CLEAR (mask);

            API_ElementMemo memo = {};
            const GS::OnExit memoGuard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
            ACAPI_Element_GetMemo (element.header.guid, &memo, APIMemoMask_AddPars);

            auto applyErr = ApplyObjectLampDetails (element, memo, &mask, GetStories (), item);
            if (applyErr.HasValue ()) {
                executionResults (*applyErr);
                continue;
            }

            err = ACAPI_Element_Change (&element, &mask, &memo, APIMemoMask_AddPars, true);
            executionResults (err == NoError ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (err, "Failed to modify element."));
        }

        return NoError;
    });

    return response;
}

ModifyObjectsCommand::ModifyObjectsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyObjectsCommand::GetName () const
{
    return "ModifyObjects";
}

GS::Optional<GS::UniString> ModifyObjectsCommand::GetInputParametersSchema () const
{
    return BuildLibraryPartBasedModifySchema ("objectsWithDetails", false);
}

GS::Optional<GS::UniString> ModifyObjectsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    })";
}

GS::ObjectState ModifyObjectsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    return ExecuteModifyObjectOrLamp (parameters, "objectsWithDetails", API_ObjectID, "ModifyObjects");
}

ModifyLampsCommand::ModifyLampsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyLampsCommand::GetName () const
{
    return "ModifyLamps";
}

GS::Optional<GS::UniString> ModifyLampsCommand::GetInputParametersSchema () const
{
    return BuildLibraryPartBasedModifySchema ("lampsWithDetails", true);
}

GS::Optional<GS::UniString> ModifyLampsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    })";
}

GS::ObjectState ModifyLampsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    return ExecuteModifyObjectOrLamp (parameters, "lampsWithDetails", API_LampID, "ModifyLamps");
}

ModifyTextsCommand::ModifyTextsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyTextsCommand::GetName () const
{
    return "ModifyTexts";
}

GS::Optional<GS::UniString> ModifyTextsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "textsWithDetails": {
                "type": "array",
                "description": "Array of Text elements to modify, with the fields to change. Only provided fields are changed; omitted fields are left as-is.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": { "$ref": "#/ElementId" },
                        "coordinate": { "$ref": "#/Coordinate3D" },
                        "text": { "type": "string" },
                        "runs": {
                            "type": "array",
                            "items": { "$ref": "#/TextRunDetails" },
                            "minItems": 1
                        },
                        "style": { "$ref": "#/TextStyleSettableDetails" }
                    },
                    "additionalProperties": false,
                    "required": ["elementId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["textsWithDetails"]
    })";
}

GS::Optional<GS::UniString> ModifyTextsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": { "$ref": "#/ExecutionResults" }
        },
        "additionalProperties": false,
        "required": ["executionResults"]
    })";
}

GS::ObjectState ModifyTextsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> itemsWithDetails;
    parameters.Get ("textsWithDetails", itemsWithDetails);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("ModifyTexts", [&] () -> GSErrCode {
        for (const GS::ObjectState& item : itemsWithDetails) {
            const GS::ObjectState* elementId = item.Get ("elementId");
            if (elementId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
                continue;
            }

            API_Element element = {};
            element.header.guid = GetGuidFromObjectState (*elementId);
            GSErrCode err = ACAPI_Element_Get (&element);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to find the element"));
                continue;
            }
            if (GetElemTypeId (element.header) != API_TextID) {
                executionResults (CreateFailedExecutionResult (APIERR_BADID, "Element is not a Text."));
                continue;
            }

            API_Element mask = {};
            ACAPI_ELEMENT_MASK_CLEAR (mask);

            const GS::ObjectState* coordinateOS = item.Get ("coordinate");
            if (coordinateOS != nullptr) {
                const API_Coord3D apiCoordinate = Get3DCoordinateFromObjectState (*coordinateOS);
                element.text.loc.x = apiCoordinate.x;
                element.text.loc.y = apiCoordinate.y;
                ACAPI_ELEMENT_MASK_SET (mask, API_TextType, loc);
            }

            const GS::ObjectState* styleOS = item.Get ("style");
            const bool styleChanged = (styleOS != nullptr);
            if (styleChanged) {
                // Apply BEFORE touching content: for a multistyle element (paragraphs/runs -
                // which every Tapir-created/modified Text has), Archicad ignores these top-level
                // fields entirely and only honours the per-run pen/faceBits/font/size stored in
                // the memo. Setting them here first means the content rebuild below (which is
                // ALWAYS needed when style changes, even with no new text) picks up the new style.
                TextLabelDetails::ApplyTextStyleSettableDetails (*styleOS, element.text, &mask, false);
            }

            API_ElementMemo memo = {};
            const GS::OnExit memoGuard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
            bool contentChanged = false;
            // item.Get ("text") != nullptr (the pointer-returning overload) never detects a plain
            // scalar/string field - only nested objects - so it always evaluated to nullptr here and
            // silently skipped every content edit. Use the value-returning overloads instead, exactly
            // like the check they gate against below.
            GS::UniString explicitContentTextCheck;
            GS::Array<GS::ObjectState> explicitContentRunsCheck;
            const bool explicitContent = item.Get ("text", explicitContentTextCheck) || item.Get ("runs", explicitContentRunsCheck);
            if (explicitContent || styleChanged) {
                // Start from a BLANK memo, not one fetched via ACAPI_Element_GetMemo first: confirmed
                // live (by cross-checking against Archicad's own official fix, commit a2a111b/54cbd36
                // upstream) that ACAPI_Element_Change only persists memo-based text content when fed a
                // from-scratch API_ElementMemo{} together with the NON-Uni memomask below - fetching
                // the existing memo first (as this used to do, with the *Uni masks) silently failed to
                // persist despite ApplyTextContent replacing textContent/paragraphs correctly.
                GS::ObjectState contentParams = item;
                if (!explicitContent) {
                    // Style-only change on an existing multistyle element: re-fetch the current
                    // flat text and rebuild the run(s) with it, so the new style actually applies.
                    GS::ObjectState existing;
                    TextLabelDetails::AddTextContent (existing, element.header.guid);
                    GS::UniString existingText;
                    existing.Get ("text", existingText);
                    contentParams = GS::ObjectState ("text", existingText);
                }
                auto applyErr = TextLabelDetails::ApplyTextContent (memo, element.text, contentParams);
                if (applyErr.HasValue ()) {
                    executionResults (*applyErr);
                    continue;
                }
                contentChanged = true;
                ACAPI_ELEMENT_MASK_SET (mask, API_TextType, nLine);
                ACAPI_ELEMENT_MASK_SET (mask, API_TextType, width);
                ACAPI_ELEMENT_MASK_SET (mask, API_TextType, height);
                ACAPI_ELEMENT_MASK_SET (mask, API_TextType, nonBreaking);
                ACAPI_ELEMENT_MASK_SET (mask, API_TextType, useEolPos);
            }

            // withdel=false was tried and made things WORSE (even the style fields stopped
            // applying) - confirmed live. withdel=true is required here.
            err = ACAPI_Element_Change (&element, &mask, contentChanged ? &memo : nullptr, contentChanged ? (APIMemoMask_TextContent | APIMemoMask_Paragraph) : 0, true);
            executionResults (err == NoError ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (err, "Failed to modify the Text."));
        }
        return NoError;
    });

    return response;
}

ModifyLabelsCommand::ModifyLabelsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyLabelsCommand::GetName () const
{
    return "ModifyLabels";
}

GS::Optional<GS::UniString> ModifyLabelsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "labelsWithDetails": {
                "type": "array",
                "description": "Array of Label elements to modify, with the fields to change. Only provided fields are changed; omitted fields are left as-is. The label's class (Text/Symbol) cannot be changed after creation.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": { "$ref": "#/ElementId" },
                        "text": { "type": "string" },
                        "runs": {
                            "type": "array",
                            "items": { "$ref": "#/TextRunDetails" },
                            "minItems": 1
                        },
                        "style": { "$ref": "#/TextStyleSettableDetails" },
                        "symbolStyle": { "$ref": "#/LabelSymbolStyleSettableDetails" },
                        "leaderLine": { "$ref": "#/LabelLeaderLineSettableDetails" }
                    },
                    "additionalProperties": false,
                    "required": ["elementId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["labelsWithDetails"]
    })";
}

GS::Optional<GS::UniString> ModifyLabelsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": { "$ref": "#/ExecutionResults" }
        },
        "additionalProperties": false,
        "required": ["executionResults"]
    })";
}

GS::ObjectState ModifyLabelsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> itemsWithDetails;
    parameters.Get ("labelsWithDetails", itemsWithDetails);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("ModifyLabels", [&] () -> GSErrCode {
        for (const GS::ObjectState& item : itemsWithDetails) {
            const GS::ObjectState* elementId = item.Get ("elementId");
            if (elementId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
                continue;
            }

            API_Element element = {};
            element.header.guid = GetGuidFromObjectState (*elementId);
            GSErrCode err = ACAPI_Element_Get (&element);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to find the element"));
                continue;
            }
            if (GetElemTypeId (element.header) != API_LabelID) {
                executionResults (CreateFailedExecutionResult (APIERR_BADID, "Element is not a Label."));
                continue;
            }

            API_Element mask = {};
            ACAPI_ELEMENT_MASK_CLEAR (mask);

            const GS::ObjectState* leaderLineOS = item.Get ("leaderLine");
            if (leaderLineOS != nullptr) {
                TextLabelDetails::ApplyLabelLeaderLineSettableDetails (*leaderLineOS, element.label, &mask);
            }

            API_ElementMemo memo = {};
            const GS::OnExit memoGuard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
            bool contentChanged = false;

            if (element.label.labelClass == APILblClass_Text) {
                const GS::ObjectState* styleOS = item.Get ("style");
                const bool styleChanged = (styleOS != nullptr);
                if (styleChanged) {
                    // Same ordering requirement as ModifyTexts: must run before the content
                    // rebuild below, since Archicad ignores these top-level fields on an
                    // existing multistyle element and only honours the per-run values.
                    TextLabelDetails::ApplyTextStyleSettableDetails (*styleOS, element.label.u.text, &mask, true);
                }
                // See ModifyTexts for why the value-returning overloads are required here (the
                // pointer-returning Get never detects a plain scalar/array field).
                GS::UniString explicitContentTextCheck;
                GS::Array<GS::ObjectState> explicitContentRunsCheck;
                const bool explicitContent = item.Get ("text", explicitContentTextCheck) || item.Get ("runs", explicitContentRunsCheck);
                if (explicitContent || styleChanged) {
                    // See ModifyTexts for why: blank memo + non-Uni memomask, not a memo fetched
                    // via GetMemo first with the *Uni masks.
                    GS::ObjectState contentParams = item;
                    if (!explicitContent) {
                        GS::ObjectState existing;
                        TextLabelDetails::AddTextContent (existing, element.header.guid);
                        GS::UniString existingText;
                        existing.Get ("text", existingText);
                        contentParams = GS::ObjectState ("text", existingText);
                    }
                    auto applyErr = TextLabelDetails::ApplyTextContent (memo, element.label.u.text, contentParams);
                    if (applyErr.HasValue ()) {
                        executionResults (*applyErr);
                        continue;
                    }
                    contentChanged = true;
                    ACAPI_ELEMENT_MASK_SET (mask, API_LabelType, u.text.nLine);
                    ACAPI_ELEMENT_MASK_SET (mask, API_LabelType, u.text.width);
                    ACAPI_ELEMENT_MASK_SET (mask, API_LabelType, u.text.height);
                    ACAPI_ELEMENT_MASK_SET (mask, API_LabelType, u.text.nonBreaking);
                    ACAPI_ELEMENT_MASK_SET (mask, API_LabelType, u.text.useEolPos);
                }
            } else {
                const GS::ObjectState* symbolStyleOS = item.Get ("symbolStyle");
                if (symbolStyleOS != nullptr) {
                    TextLabelDetails::ApplyLabelSymbolStyleSettableDetails (*symbolStyleOS, element.label, &mask);
                }
            }

            // withdel=true required - see ModifyTexts.
            err = ACAPI_Element_Change (&element, &mask, contentChanged ? &memo : nullptr, contentChanged ? (APIMemoMask_TextContent | APIMemoMask_Paragraph) : 0, true);
            executionResults (err == NoError ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (err, "Failed to modify the Label."));
        }
        return NoError;
    });

    return response;
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
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                    },
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
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                    },
                    "parentElementId": {
                        "$ref": "#/ElementId",
                        "description" : "The parent element if the label is an associative label."
                    },
                    "labelClass": {
                        "type": "string",
                        "enum": ["Text", "Symbol"],
                        "description": "Whether this is a textual or a symbol label. Optional; if omitted, inherits the current Label tool default (which may silently resolve to either class - explicitly setting this avoids ambiguity)."
                    },
                    "text": {
                        "type": "string",
                        "description": "The text content if the label is a text label. Ignored if 'runs' is also given."
                    },
                    "runs": {
                        "type": "array",
                        "description": "Multi-style text content for a text label: an array of styled runs, concatenated in order. Takes precedence over 'text' if both are given.",
                        "items": { "$ref": "#/TextRunDetails" },
                        "minItems": 1
                    },
                    "style": {
                        "$ref": "#/TextStyleSettableDetails",
                        "description": "Style settings for a text label (font, pen, size, frame, etc). Ignored for symbol labels."
                    },
                    "symbolStyle": {
                        "$ref": "#/LabelSymbolStyleSettableDetails",
                        "description": "Style settings specific to a symbol label. Ignored for text labels."
                    },
                    "leaderLine": {
                        "$ref": "#/LabelLeaderLineSettableDetails",
                        "description": "Leader line, frame and arrow settings, shared by both label classes."
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

API_JustID ParseJustificationString (const GS::UniString& justification)
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

// Used by SetDetailsOfElementsCommand's generic Text/Label write case (upstream official fix);
// TextLabelDetails::ApplyTextContent below is Tapir's own, richer equivalent used by
// CreateTexts/CreateLabels/ModifyTexts/ModifyLabels - both build memo.textContent/paragraphs but
// only ApplyTextContent supports multi-run content.
void SetTextContentAndParagraphs (API_ElementMemo& memo, API_TextType& textData, const GS::UniString& text)
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

static const char* JustificationToString (API_JustID just)
{
    switch (just) {
        case APIJust_Center: return "Center";
        case APIJust_Right:  return "Right";
        case APIJust_Full:   return "Full";
        default:             return "Left";
    }
}

static const char* AnchorToString (API_AnchorID anchor)
{
    switch (anchor) {
        case APIAnc_MT: return "MiddleTop";
        case APIAnc_RT: return "RightTop";
        case APIAnc_LM: return "LeftMiddle";
        case APIAnc_MM: return "MiddleMiddle";
        case APIAnc_RM: return "RightMiddle";
        case APIAnc_LB: return "LeftBottom";
        case APIAnc_MB: return "MiddleBottom";
        case APIAnc_RB: return "RightBottom";
        default:        return "LeftTop";
    }
}

static API_AnchorID StringToAnchor (const GS::UniString& s)
{
    if (s == "MiddleTop")    return APIAnc_MT;
    if (s == "RightTop")     return APIAnc_RT;
    if (s == "LeftMiddle")   return APIAnc_LM;
    if (s == "MiddleMiddle") return APIAnc_MM;
    if (s == "RightMiddle")  return APIAnc_RM;
    if (s == "LeftBottom")   return APIAnc_LB;
    if (s == "MiddleBottom") return APIAnc_MB;
    if (s == "RightBottom")  return APIAnc_RB;
    return APIAnc_LT;
}

#ifdef ServerMainVers_2800
static const char* TextFrameShapeToString (API_TextFrameShapeTypeID shape)
{
    switch (shape) {
        case API_TextFrameShapeType_Circle:          return "Circle";
        case API_TextFrameShapeType_RoundedRectangle: return "RoundedRectangle";
        case API_TextFrameShapeType_Pill:             return "Pill";
        default:                                      return "Rectangle";
    }
}

static API_TextFrameShapeTypeID StringToTextFrameShape (const GS::UniString& s)
{
    if (s == "Circle")           return API_TextFrameShapeType_Circle;
    if (s == "RoundedRectangle") return API_TextFrameShapeType_RoundedRectangle;
    if (s == "Pill")             return API_TextFrameShapeType_Pill;
    return API_TextFrameShapeType_Rectangle;
}
#endif

static const char* LabelAnchorPointToString (API_LblAnchorID a)
{
    switch (a) {
        case APILbl_TopAnchor:    return "Top";
        case APILbl_BottomAnchor: return "Bottom";
        case APILbl_Underlined:   return "Underlined";
        default:                  return "Middle";
    }
}

static API_LblAnchorID StringToLabelAnchorPoint (const GS::UniString& s)
{
    if (s == "Top")        return APILbl_TopAnchor;
    if (s == "Bottom")     return APILbl_BottomAnchor;
    if (s == "Underlined") return APILbl_Underlined;
    return APILbl_MiddleAnchor;
}

static const char* LeaderShapeToString (API_LeaderLineShapeID s)
{
    switch (s) {
        case API_Splinear:   return "Splinear";
        case API_SquareRoot: return "SquareRoot";
        default:             return "Segmented";
    }
}

static API_LeaderLineShapeID StringToLeaderShape (const GS::UniString& s)
{
    if (s == "Splinear")   return API_Splinear;
    if (s == "SquareRoot") return API_SquareRoot;
    return API_Segmented;
}

static const char* LabelTextWayToString (API_DirID d)
{
    switch (d) {
        case APIDir_Horizontal: return "Horizontal";
        case APIDir_Vertical:   return "Vertical";
        case APIDir_General:    return "General";
        default:                return "Parallel";
    }
}

static API_DirID StringToLabelTextWay (const GS::UniString& s)
{
    if (s == "Horizontal") return APIDir_Horizontal;
    if (s == "Vertical")   return APIDir_Vertical;
    if (s == "General")    return APIDir_General;
    return APIDir_Parallel;
}

// API_ArrowID has 31 sequential values starting at APIArr_EmptyCirc; index-parallel to
// #/LabelArrowType's enum order in CommonSchemaDefinitions.json.
static const char* const kArrowTypeNames[] = {
    "EmptyCircle", "CrossCircle", "FullCircle",
    "SlashLine15", "OpenArrow15", "ClosedArrow15", "FullArrow15",
    "SlashLine30", "OpenArrow30", "ClosedArrow30", "FullArrow30",
    "SlashLine45", "OpenArrow45", "ClosedArrow45", "FullArrow45",
    "SlashLine60", "OpenArrow60", "ClosedArrow60", "FullArrow60",
    "SlashLine90",
    "PepitaCircle", "BandArrow",
    "HalfArrowCcw15", "HalfArrowCw15", "HalfArrowCcw30", "HalfArrowCw30",
    "HalfArrowCcw45", "HalfArrowCw45", "HalfArrowCcw60", "HalfArrowCw60",
    "SlashLine75"
};
constexpr int kArrowTypeCount = sizeof (kArrowTypeNames) / sizeof (kArrowTypeNames[0]);

static const char* ArrowTypeToString (API_ArrowID arrowType)
{
    const int idx = static_cast<int> (arrowType);
    return (idx >= 0 && idx < kArrowTypeCount) ? kArrowTypeNames[idx] : kArrowTypeNames[0];
}

static API_ArrowID StringToArrowType (const GS::UniString& s)
{
    for (int i = 0; i < kArrowTypeCount; ++i) {
        if (s == kArrowTypeNames[i]) {
            return static_cast<API_ArrowID> (i);
        }
    }
    return APIArr_EmptyCirc;
}

namespace TextLabelDetails {

void AddTextStyleDetails (GS::ObjectState& os, const API_TextType& text, bool includeReadOnly)
{
    os.Add ("penIndex", text.pen);
    os.Add ("fontIndex", text.font);
    os.Add ("bold", (text.faceBits & APIFace_Bold) != 0);
    os.Add ("italic", (text.faceBits & APIFace_Italic) != 0);
    os.Add ("underline", (text.faceBits & APIFace_Underline) != 0);
    os.Add ("justification", JustificationToString (text.just));
    os.Add ("height", text.size);
    os.Add ("spacing", text.spacing);
    os.Add ("angle", text.angle);
    os.Add ("effectStrikeout", (text.effectsBits & APIEffect_StrikeOut) != 0);
    os.Add ("effectSuperscript", (text.effectsBits & APIEffect_SuperScript) != 0);
    os.Add ("effectSubscript", (text.effectsBits & APIEffect_SubScript) != 0);
    os.Add ("effectProtected", (text.effectsBits & APIEffect_Protected) != 0);
    os.Add ("widthFactor", text.widthFactor);
    os.Add ("charSpaceFactor", text.charSpaceFactor);
    os.Add ("fixedSize", text.fixedSize);
    os.Add ("usedContour", text.usedContour);
    os.Add ("usedFill", text.usedFill);
    os.Add ("contourPenIndex", text.contourPen);
    os.Add ("fillPenIndex", text.fillPen);
    os.Add ("anchor", AnchorToString (text.anchor));
    os.Add ("fixedAngle", text.fixedAngle);
    os.Add ("contourOffset", text.contourOffset);
    os.Add ("flipEnabled", text.flipEnabled);
#ifdef ServerMainVers_2800
    os.Add ("textFrameShape", TextFrameShapeToString (text.textFrame.shapeType));
    os.Add ("textFrameSizeFixed", text.textFrame.isSizeFixed);
    os.Add ("textFrameFixedWidth", text.textFrame.fixedWidth);
    os.Add ("textFrameFixedHeight", text.textFrame.fixedHeight);
#else
    os.Add ("textFrameShape", "Rectangle");
    os.Add ("textFrameSizeFixed", false);
    os.Add ("textFrameFixedWidth", 0.0);
    os.Add ("textFrameFixedHeight", 0.0);
#endif

    if (includeReadOnly) {
        os.Add ("lineCount", text.nLine);
        os.Add ("boxWidth", text.width);
        os.Add ("boxHeight", text.height);
    }
}

void ApplyTextStyleSettableDetails (const GS::ObjectState& details, API_TextType& text, API_Element* mask, bool isLabelUnion)
{
    // Mask bits must target the real top-level union member (API_TextType for a standalone
    // Text element, API_LabelType::u.text for a text-class Label) - both share byte layout for
    // 'text' vs 'u.text' (the union starts at offset 0 of API_LabelType), but the macro needs
    // the exact type name, hence the isLabelUnion branch on every mask line.
#define TEXT_MASK_SET(fieldPath) \
    if (mask != nullptr) { \
        if (isLabelUnion) { ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, u.text.fieldPath); } \
        else { ACAPI_ELEMENT_MASK_SET (*mask, API_TextType, fieldPath); } \
    }

    if (details.Get ("penIndex", text.pen)) { TEXT_MASK_SET (pen) }
    if (details.Get ("fontIndex", text.font)) { TEXT_MASK_SET (font) }

    bool bold = (text.faceBits & APIFace_Bold) != 0;
    bool italic = (text.faceBits & APIFace_Italic) != 0;
    bool underline = (text.faceBits & APIFace_Underline) != 0;
    bool faceChanged = false;
    faceChanged |= details.Get ("bold", bold);
    faceChanged |= details.Get ("italic", italic);
    faceChanged |= details.Get ("underline", underline);
    if (faceChanged) {
        text.faceBits = static_cast<unsigned short> ((bold ? APIFace_Bold : 0) | (italic ? APIFace_Italic : 0) | (underline ? APIFace_Underline : 0));
        TEXT_MASK_SET (faceBits)
    }

    GS::UniString strVal;
    if (details.Get ("justification", strVal)) {
        text.just = ParseJustificationString (strVal);
        TEXT_MASK_SET (just)
    }
    if (details.Get ("height", text.size)) { TEXT_MASK_SET (size) }
    if (details.Get ("spacing", text.spacing)) { TEXT_MASK_SET (spacing) }
    if (details.Get ("angle", text.angle)) { TEXT_MASK_SET (angle) }

    bool strikeout = (text.effectsBits & APIEffect_StrikeOut) != 0;
    bool superscript = (text.effectsBits & APIEffect_SuperScript) != 0;
    bool subscript = (text.effectsBits & APIEffect_SubScript) != 0;
    bool protectedFlag = (text.effectsBits & APIEffect_Protected) != 0;
    bool effectsChanged = false;
    effectsChanged |= details.Get ("effectStrikeout", strikeout);
    effectsChanged |= details.Get ("effectSuperscript", superscript);
    effectsChanged |= details.Get ("effectSubscript", subscript);
    effectsChanged |= details.Get ("effectProtected", protectedFlag);
    if (effectsChanged) {
        text.effectsBits = (strikeout ? APIEffect_StrikeOut : 0) | (superscript ? APIEffect_SuperScript : 0) | (subscript ? APIEffect_SubScript : 0) | (protectedFlag ? APIEffect_Protected : 0);
        TEXT_MASK_SET (effectsBits)
    }

    if (details.Get ("widthFactor", text.widthFactor)) { TEXT_MASK_SET (widthFactor) }
    if (details.Get ("charSpaceFactor", text.charSpaceFactor)) { TEXT_MASK_SET (charSpaceFactor) }
    if (details.Get ("fixedSize", text.fixedSize)) { TEXT_MASK_SET (fixedSize) }
    if (details.Get ("usedContour", text.usedContour)) { TEXT_MASK_SET (usedContour) }
    if (details.Get ("usedFill", text.usedFill)) { TEXT_MASK_SET (usedFill) }
    if (details.Get ("contourPenIndex", text.contourPen)) { TEXT_MASK_SET (contourPen) }
    if (details.Get ("fillPenIndex", text.fillPen)) { TEXT_MASK_SET (fillPen) }
    if (details.Get ("anchor", strVal)) {
        text.anchor = StringToAnchor (strVal);
        TEXT_MASK_SET (anchor)
    }
    if (details.Get ("fixedAngle", text.fixedAngle)) { TEXT_MASK_SET (fixedAngle) }
    if (details.Get ("contourOffset", text.contourOffset)) { TEXT_MASK_SET (contourOffset) }
    if (details.Get ("flipEnabled", text.flipEnabled)) { TEXT_MASK_SET (flipEnabled) }

#ifdef ServerMainVers_2800
    bool frameChanged = false;
    frameChanged = details.Get ("textFrameShape", strVal) || frameChanged;
    if (frameChanged) { text.textFrame.shapeType = StringToTextFrameShape (strVal); }
    frameChanged = details.Get ("textFrameSizeFixed", text.textFrame.isSizeFixed) || frameChanged;
    frameChanged = details.Get ("textFrameFixedWidth", text.textFrame.fixedWidth) || frameChanged;
    frameChanged = details.Get ("textFrameFixedHeight", text.textFrame.fixedHeight) || frameChanged;
    if (frameChanged) { TEXT_MASK_SET (textFrame) }
#endif

#undef TEXT_MASK_SET
}

GS::Optional<GS::ObjectState> ApplyTextContent (API_ElementMemo& memo, API_TextType& textData, const GS::ObjectState& parameters)
{
    GS::Array<GS::ObjectState> runsData;
    const bool hasRuns = parameters.Get ("runs", runsData) && !runsData.IsEmpty ();

    GS::UniString text;
    GS::Array<GS::UniString> runTexts;
    GS::Array<short> runPens;
    GS::Array<short> runFonts;
    GS::Array<unsigned short> runFaceBits;
    GS::Array<double> runSizes;

    if (hasRuns) {
        for (const GS::ObjectState& runOS : runsData) {
            GS::UniString runText;
            if (!runOS.Get ("text", runText)) {
                return CreateErrorResponse (APIERR_BADPARS, "Each entry in 'runs' requires a 'text' field.");
            }
            short pen = textData.pen;
            runOS.Get ("penIndex", pen);
            short font = textData.font;
            runOS.Get ("fontIndex", font);
            bool bold = (textData.faceBits & APIFace_Bold) != 0;
            bool italic = (textData.faceBits & APIFace_Italic) != 0;
            bool underline = (textData.faceBits & APIFace_Underline) != 0;
            runOS.Get ("bold", bold);
            runOS.Get ("italic", italic);
            runOS.Get ("underline", underline);
            double size = textData.size;
            runOS.Get ("heightOverride", size);

            runTexts.Push (runText);
            runPens.Push (pen);
            runFonts.Push (font);
            runFaceBits.Push (static_cast<unsigned short> ((bold ? APIFace_Bold : 0) | (italic ? APIFace_Italic : 0) | (underline ? APIFace_Underline : 0)));
            runSizes.Push (size);
            text += runText;
        }
    } else if (!parameters.Get ("text", text)) {
        return CreateErrorResponse (APIERR_BADPARS, "Missing 'text' (or 'runs') parameter");
    }

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
    const Int32 numOfRuns = hasRuns ? static_cast<Int32> (runTexts.GetSize ()) : 1;

    // When modifying an existing element, 'memo' may already carry handles fetched via
    // ACAPI_Element_GetMemo (paragraphs sized for the OLD content, and textLineStarts - a
    // separate short** array of line-start indices into textContent, distinct from each
    // paragraph's own eolPos). Leaving textLineStarts stale (still sized/pointing at the old
    // content) while textContent/paragraphs get replaced below produces an inconsistent memo
    // that ACAPI_Element_Change silently refuses to persist - confirmed live: this was the
    // reason content edits never stuck. Free both stale handles so Archicad rebuilds
    // textLineStarts fresh from the new paragraphs, the same as it does on a brand new element.
    if (memo.paragraphs != nullptr) {
        BMKillHandle (reinterpret_cast<GSHandle*> (&memo.paragraphs));
    }
    if (memo.textLineStarts != nullptr) {
        BMKillHandle (reinterpret_cast<GSHandle*> (&memo.textLineStarts));
    }

    memo.paragraphs = reinterpret_cast<API_ParagraphType**> (BMhAll (numOfParagraphs * sizeof (API_ParagraphType)));
    SetParagraph (memo.paragraphs, 0, 0, text.GetLength (), 1, numOfRuns, textData.nLine);

    if (hasRuns) {
        Int32 runFrom = 0;
        for (Int32 i = 0; i < numOfRuns; ++i) {
            const Int32 runRange = runTexts[i].GetLength ();
            SetRun (memo.paragraphs, 0, static_cast<UInt32> (i), runFrom, runRange, runPens[i], runFaceBits[i], runFonts[i], 0, runSizes[i]);
            runFrom += runRange;
        }
    } else {
        SetRun (memo.paragraphs, 0, 0, 0, text.GetLength (), textData.pen, textData.faceBits, textData.font, textData.effectsBits, textData.size);
    }

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

    return {};
}

void AddTextContent (GS::ObjectState& os, const API_Guid& elemGuid)
{
    API_ElementMemo memo = {};
    const GS::OnExit guard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
    ACAPI_Element_GetMemo (elemGuid, &memo, APIMemoMask_TextContent | APIMemoMask_Paragraph);

#ifdef ServerMainVers_2800
    const GS::UniString content = (memo.textContent != nullptr) ? *memo.textContent : GS::EmptyUniString;
#else
    GS::UniString content;
    if (memo.textContent != nullptr) {
        // Reconstruct by exact handle byte size (divided by sizeof(uchar_t), minus the trailing
        // null terminator uchar_t) rather than relying on NUL-termination scanning, which was
        // unreliable here.
        const GSSize byteSize = BMGetHandleSize (reinterpret_cast<GSHandle> (memo.textContent));
        const USize charCount = static_cast<USize> (byteSize / sizeof (GS::uchar_t));
        const USize contentCharCount = (charCount > 0) ? (charCount - 1) : 0;
        content = GS::UniString (reinterpret_cast<const GS::UniChar::Layout*> (*memo.textContent), contentCharCount);
    }
#endif
    os.Add ("text", content);

    const UInt32 nParagraphs = (memo.paragraphs != nullptr) ? static_cast<UInt32> (BMhGetSize (reinterpret_cast<GSHandle> (memo.paragraphs)) / sizeof (API_ParagraphType)) : 0;
    os.Add ("paragraphCount", static_cast<int> (nParagraphs));
    if (nParagraphs == 0) {
        return;
    }

    const API_ParagraphType& paragraph = (*memo.paragraphs)[0];
    const UInt32 nRuns = (paragraph.run != nullptr) ? static_cast<UInt32> (BMGetPtrSize (reinterpret_cast<GSPtr> (paragraph.run)) / sizeof (API_RunType)) : 0;
    if (nRuns <= 1) {
        return;
    }

    const auto& runList = os.AddList<GS::ObjectState> ("runs");
    for (UInt32 i = 0; i < nRuns; ++i) {
        const API_RunType& run = paragraph.run[i];
        GS::ObjectState runOS;
        runOS.Add ("text", GS::UniString (content.GetSubstring (static_cast<UIndex> (run.from), static_cast<USize> (run.range))));
        runOS.Add ("penIndex", run.pen);
        runOS.Add ("fontIndex", run.font);
        runOS.Add ("bold", (run.faceBits & APIFace_Bold) != 0);
        runOS.Add ("italic", (run.faceBits & APIFace_Italic) != 0);
        runOS.Add ("underline", (run.faceBits & APIFace_Underline) != 0);
        runOS.Add ("heightOverride", run.size);
        runList (runOS);
    }
}

void AddLabelLeaderLineDetails (GS::ObjectState& os, const API_LabelType& label)
{
    os.Add ("penIndex", label.pen);
    os.Add ("lineTypeId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_LinetypeID, label.ltypeInd)));
    os.Add ("contourOffset", label.contourOffset);
    os.Add ("framed", label.framed);
    os.Add ("hasLeaderLine", label.hasLeaderLine);
    os.Add ("anchorPoint", LabelAnchorPointToString (label.anchorPoint));
    os.Add ("leaderShape", LeaderShapeToString (label.leaderShape));
    os.Add ("squareRootAngle", label.squareRootAngle);
    os.Add ("arrowType", ArrowTypeToString (label.arrowData.arrowType));
#ifdef ServerMainVers_2800
    os.Add ("arrowVisible", label.arrowData.arrowVisibility);
#else
    os.Add ("arrowVisible", label.arrowData.endArrow);
#endif
    os.Add ("arrowPenIndex", label.arrowData.arrowPen);
    os.Add ("arrowSize", label.arrowData.arrowSize);
    os.Add ("hideWithBaseElem", label.hideWithBaseElem);
    os.Add ("begCoordinate", Create2DCoordinateObjectState (label.begC));
    os.Add ("midCoordinate", Create2DCoordinateObjectState (label.midC));
    os.Add ("endCoordinate", Create2DCoordinateObjectState (label.endC));
}

void ApplyLabelLeaderLineSettableDetails (const GS::ObjectState& details, API_LabelType& label, API_Element* mask)
{
    if (details.Get ("penIndex", label.pen)) {
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, pen);
    }
    {
        const GS::ObjectState* attrId = details.Get ("lineTypeId");
        if (attrId != nullptr) {
            ResolveAttributeIndex (*attrId, API_LinetypeID, label.ltypeInd);
            if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, ltypeInd);
        }
    }
    if (details.Get ("contourOffset", label.contourOffset)) {
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, contourOffset);
    }
    if (details.Get ("framed", label.framed)) {
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, framed);
    }
    if (details.Get ("hasLeaderLine", label.hasLeaderLine)) {
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, hasLeaderLine);
    }
    GS::UniString strVal;
    if (details.Get ("anchorPoint", strVal)) {
        label.anchorPoint = StringToLabelAnchorPoint (strVal);
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, anchorPoint);
    }
    if (details.Get ("leaderShape", strVal)) {
        label.leaderShape = StringToLeaderShape (strVal);
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, leaderShape);
    }
    if (details.Get ("squareRootAngle", label.squareRootAngle)) {
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, squareRootAngle);
    }
    // NOTE: mask each arrowData sub-field individually - ACAPI_ELEMENT_MASK_SET only marks a
    // single byte at the given field's own address, so masking the whole 'arrowData' struct only
    // ever flags its first member (arrowType) as changed; arrowVisibility/arrowPen/arrowSize
    // further into the struct were silently ignored by ACAPI_Element_Change without their own
    // mask entries. That alone was NOT enough either though: confirmed live that even with its own
    // individual mask entry, arrowVisibility changes were still dropped on ModifyLabels unless
    // EVERY other arrowData sub-field is ALSO masked (and reasserted with its current value) in the
    // same call - Archicad's internal validation for this nested struct seems to require the whole
    // unit asserted together, not just the one field actually changing. So: once any arrowData
    // sub-field is touched, mask all of them, re-populating the untouched ones from the element as
    // already fetched via ACAPI_Element_Get (so their value doesn't actually change).
    bool arrowDataTouched = false;
    if (details.Get ("arrowType", strVal)) {
        label.arrowData.arrowType = StringToArrowType (strVal);
        arrowDataTouched = true;
    }
#ifdef ServerMainVers_2800
    if (details.Get ("arrowVisible", label.arrowData.arrowVisibility)) {
        arrowDataTouched = true;
    }
#else
    bool arrowVisible = label.arrowData.endArrow;
    if (details.Get ("arrowVisible", arrowVisible)) {
        label.arrowData.begArrow = arrowVisible;
        label.arrowData.endArrow = arrowVisible;
        arrowDataTouched = true;
    }
#endif
    if (details.Get ("arrowPenIndex", label.arrowData.arrowPen)) {
        arrowDataTouched = true;
    }
    if (details.Get ("arrowSize", label.arrowData.arrowSize)) {
        arrowDataTouched = true;
    }
    if (arrowDataTouched && mask != nullptr) {
        ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, arrowData.arrowType);
        ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, arrowData.arrowPen);
        ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, arrowData.arrowSize);
#ifdef ServerMainVers_2800
        ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, arrowData.arrowVisibility);
#else
        ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, arrowData.begArrow);
        ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, arrowData.endArrow);
#endif
    }
    if (details.Get ("hideWithBaseElem", label.hideWithBaseElem)) {
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, hideWithBaseElem);
    }
}

void AddLabelSymbolStyleDetails (GS::ObjectState& os, const API_LabelType& label)
{
    os.Add ("textWay", LabelTextWayToString (label.textWay));
    os.Add ("fontIndex", label.font);
    os.Add ("bold", (label.faceBits & APIFace_Bold) != 0);
    os.Add ("italic", (label.faceBits & APIFace_Italic) != 0);
    os.Add ("underline", (label.faceBits & APIFace_Underline) != 0);
    os.Add ("flipEnabled", label.flipEnabled);
    os.Add ("nonBreaking", label.nonBreaking);
    os.Add ("textSize", label.textSize);
    os.Add ("useBackgroundFill", label.useBgFill);
    os.Add ("backgroundFillPenIndex", label.fillBgPen);
    os.Add ("effectStrikeout", (label.effectsBits & APIEffect_StrikeOut) != 0);
    os.Add ("effectSuperscript", (label.effectsBits & APIEffect_SuperScript) != 0);
    os.Add ("effectSubscript", (label.effectsBits & APIEffect_SubScript) != 0);
    os.Add ("effectProtected", (label.effectsBits & APIEffect_Protected) != 0);
}

void ApplyLabelSymbolStyleSettableDetails (const GS::ObjectState& details, API_LabelType& label, API_Element* mask)
{
    GS::UniString strVal;
    if (details.Get ("textWay", strVal)) {
        label.textWay = StringToLabelTextWay (strVal);
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, textWay);
    }
    if (details.Get ("fontIndex", label.font)) {
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, font);
    }
    bool bold = (label.faceBits & APIFace_Bold) != 0;
    bool italic = (label.faceBits & APIFace_Italic) != 0;
    bool underline = (label.faceBits & APIFace_Underline) != 0;
    bool faceChanged = false;
    faceChanged |= details.Get ("bold", bold);
    faceChanged |= details.Get ("italic", italic);
    faceChanged |= details.Get ("underline", underline);
    if (faceChanged) {
        label.faceBits = static_cast<unsigned short> ((bold ? APIFace_Bold : 0) | (italic ? APIFace_Italic : 0) | (underline ? APIFace_Underline : 0));
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, faceBits);
    }
    if (details.Get ("flipEnabled", label.flipEnabled)) {
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, flipEnabled);
    }
    if (details.Get ("nonBreaking", label.nonBreaking)) {
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, nonBreaking);
    }
    if (details.Get ("textSize", label.textSize)) {
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, textSize);
    }
    if (details.Get ("useBackgroundFill", label.useBgFill)) {
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, useBgFill);
    }
    if (details.Get ("backgroundFillPenIndex", label.fillBgPen)) {
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, fillBgPen);
    }
    bool strikeout = (label.effectsBits & APIEffect_StrikeOut) != 0;
    bool superscript = (label.effectsBits & APIEffect_SuperScript) != 0;
    bool subscript = (label.effectsBits & APIEffect_SubScript) != 0;
    bool protectedFlag = (label.effectsBits & APIEffect_Protected) != 0;
    bool effectsChanged = false;
    effectsChanged |= details.Get ("effectStrikeout", strikeout);
    effectsChanged |= details.Get ("effectSuperscript", superscript);
    effectsChanged |= details.Get ("effectSubscript", subscript);
    effectsChanged |= details.Get ("effectProtected", protectedFlag);
    if (effectsChanged) {
        label.effectsBits = (strikeout ? APIEffect_StrikeOut : 0) | (superscript ? APIEffect_SuperScript : 0) | (subscript ? APIEffect_SubScript : 0) | (protectedFlag ? APIEffect_Protected : 0);
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, effectsBits);
    }
}

} // namespace TextLabelDetails

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

    GS::UniString labelClassStr;
    if (parameters.Get ("labelClass", labelClassStr)) {
        element.label.labelClass = (labelClassStr == "Symbol") ? APILblClass_Symbol : APILblClass_Text;
    }

    const GS::ObjectState* leaderLineOS = parameters.Get ("leaderLine");
    if (leaderLineOS != nullptr) {
        TextLabelDetails::ApplyLabelLeaderLineSettableDetails (*leaderLineOS, element.label, nullptr);
    }

    if (element.label.labelClass == APILblClass_Text) {
        // Style MUST be applied before content: CreateExt builds paragraphs/runs from
        // element.label.u.text.pen/faceBits/font/size at the time ApplyTextContent runs, and
        // Archicad ignores those top-level fields afterwards for a multistyle (paragraph-based)
        // element - only the run's own copies matter once paragraphs exist.
        const GS::ObjectState* styleOS = parameters.Get ("style");
        if (styleOS != nullptr) {
            TextLabelDetails::ApplyTextStyleSettableDetails (*styleOS, element.label.u.text, nullptr, true);
        }
        auto err = TextLabelDetails::ApplyTextContent (memo, element.label.u.text, parameters);
        if (err.HasValue ()) {
            return err;
        }
    } else {
        const GS::ObjectState* symbolStyleOS = parameters.Get ("symbolStyle");
        if (symbolStyleOS != nullptr) {
            TextLabelDetails::ApplyLabelSymbolStyleSettableDetails (*symbolStyleOS, element.label, nullptr);
        }
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
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                    },
                    "coordinate": {
                        "$ref": "#/Coordinate3D",
                        "description": "The placement position of the text. The z value is used to determine the floor when floorIndex is omitted."
                    },
                    "text": {
                        "type": "string",
                        "description": "The text content. Newlines create multiple lines. Ignored if 'runs' is also given."
                    },
                    "runs": {
                        "type": "array",
                        "description": "Multi-style text content: an array of styled runs, concatenated in order. Takes precedence over 'text' if both are given.",
                        "items": { "$ref": "#/TextRunDetails" },
                        "minItems": 1
                    },
                    "height": {
                        "type": "number",
                        "description": "The character height in millimeters. Optional; defaults to the Text tool default. Equivalent to style.height."
                    },
                    "pen": {
                        "type": "integer",
                        "description": "Optional pen attribute index. Equivalent to style.penIndex."
                    },
                    "angle": {
                        "type": "number",
                        "description": "Optional rotation angle in radians. Equivalent to style.angle."
                    },
                    "justification": {
                        "type": "string",
                        "description": "Optional text justification. Equivalent to style.justification.",
                        "enum": ["Left", "Center", "Right", "Full"]
                    },
                    "style": {
                        "$ref": "#/TextStyleSettableDetails",
                        "description": "Full style settings (font, effects, frame, anchor, etc). height/pen/angle/justification above take precedence over the same fields here if both are given."
                    },
                    "floorIndex": {
                        "type": "integer",
                        "description": "Optional floor index. If omitted, derived from the coordinate's z value."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "coordinate"
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

    const GS::ObjectState* styleOS = parameters.Get ("style");
    if (styleOS != nullptr) {
        TextLabelDetails::ApplyTextStyleSettableDetails (*styleOS, element.text, nullptr, false);
    }

    // Top-level height/pen/angle/justification take precedence over 'style' for backward
    // compatibility with the original CreateTexts shape.
    parameters.Get ("height", element.text.size);
    parameters.Get ("pen", element.text.pen);
    parameters.Get ("angle", element.text.angle);
    GS::UniString justification;
    if (parameters.Get ("justification", justification)) {
        element.text.just = ParseJustificationString (justification);
    }

    auto err = TextLabelDetails::ApplyTextContent (memo, element.text, parameters);
    if (err.HasValue ()) {
        return err;
    }

    return {};
}
