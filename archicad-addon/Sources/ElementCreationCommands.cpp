#include "ElementCreationCommands.hpp"
#include "ObjectState.hpp"
#include "OnExit.hpp"
#include "MigrationHeader.hpp"

CreateElementsCommandBase::CreateElementsCommandBase (const GS::String& commandNameIn)
    : CommandBase (CommonSchema::Used)
    , commandName (commandNameIn)
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

CreateColumnsCommand::CreateColumnsCommand () :
    CreateElementsCommandBase ("CreateColumns")
{
}

GS::Optional<GS::UniString> CreateColumnsCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "coordinates": {
            "type": "array",
            "description": "The 3D coordinates of the new columns' origos.",
            "items": {
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
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "coordinates"
    ]
})";
}

GS::ObjectState	CreateColumnsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> coordinates;
    parameters.Get ("coordinates", coordinates);

    API_Coord3D apiCoordinate = {};
    const GSErrCode err = ACAPI_CallUndoableCommand ("Create Columns", [&] () -> GSErrCode {
        API_Element element = {};
        API_ElementMemo memo = {};
        const GS::OnExit guard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });

#ifdef ServerMainVers_2600
        element.header.type   = API_ColumnID;
#else
        element.header.typeID = API_ColumnID;
#endif
        GSErrCode err = ACAPI_Element_GetDefaults (&element, &memo);

        const GS::Array<GS::Pair<short, double>> storyLevels = GetStoryLevels ();

        for (const GS::ObjectState& coordinate : coordinates) {
            apiCoordinate = Get3DCoordinateFromObjectState (coordinate);

            element.header.floorInd = GetFloorIndexAndOffset (apiCoordinate.z, storyLevels, element.column.bottomOffset);
            element.column.origoPos.x = apiCoordinate.x;
            element.column.origoPos.y = apiCoordinate.y;
            err = ACAPI_Element_Create (&element, &memo);

            if (err != NoError) {
                return err;
            }
        }

        return NoError;
    });

    if (err != NoError) {
        const GS::UniString errorMsg = GS::UniString::Printf ("Failed to create column to coordinate: {%.2f, %.2f, %.2f}!", apiCoordinate.x, apiCoordinate.y, apiCoordinate.z);
        return CreateErrorResponse (err, errorMsg);
    }

    return {};
}

CreateSlabsCommand::CreateSlabsCommand () :
    CreateElementsCommandBase ("CreateSlabs")
{
}

GS::Optional<GS::UniString> CreateSlabsCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "slabs": {
            "type": "array",
            "description": "The parameters of the new Slabs.",
            "items": {
                "type": "object",
                "description" : "The parameters of the new Slab.",
                "properties" : {
                    "level": {
                        "type": "number",
                        "description" : "The Z coordinate value of the reference line of the slab."	
                    },
                    "polygonCoordinates": { 
                        "type": "array",
                        "description": "The 2D coordinates of the edge of the slab.",
                        "items": {
                            "type": "object",
                            "description" : "Position of a 2D point.",
                            "properties" : {
                                "x": {
                                    "type": "number",
                                    "description" : "X value of the point."
                                },
                                "y" : {
                                    "type": "number",
                                    "description" : "Y value of the point."
                                }
                            },
                            "additionalProperties": false,
                            "required" : [
                                "x",
                                "y"
                            ]
                        }
                    }
                },
                "additionalProperties": true,
                "required" : [
                    "level",
                    "polygonCoordinates"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "slabs"
    ]
})";
}

static void AddPolyToMemo (const GS::Array<GS::ObjectState>& polygonCoordinates,
                           const API_OverriddenAttribute&	 sideMat,
                           Int32& 							 iCoord,
                           Int32& 							 iPends,
                           API_ElementMemo& 				 memo)
{
    Int32 iStart = iCoord;
    for (const GS::ObjectState& polygonCoordinate : polygonCoordinates) {
        (*memo.coords)[iCoord] = Get2DCoordinateFromObjectState (polygonCoordinate);
        (*memo.edgeTrims)[iCoord].sideType = APIEdgeTrim_Vertical; // Only vertical trim is supported yet by my code
        memo.sideMaterials[iCoord] = sideMat;
        ++iCoord;
    }
    (*memo.coords)[iCoord] = (*memo.coords)[iStart];
    (*memo.pends)[iPends++] = iCoord;
    (*memo.edgeTrims)[iCoord].sideType = (*memo.edgeTrims)[iStart].sideType;
    (*memo.edgeTrims)[iCoord].sideAngle = (*memo.edgeTrims)[iStart].sideAngle;
    memo.sideMaterials[iCoord] = memo.sideMaterials[iStart];
    ++iCoord;
}

GS::ObjectState	CreateSlabsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> slabs;
    parameters.Get ("slabs", slabs);

    GS::ObjectState response;
    const auto& responseElementsArray = response.AddList<GS::ObjectState> ("elements");

    GSIndex iSlab = 0;
    const GSErrCode err = ACAPI_CallUndoableCommand ("Create Slabs", [&] () -> GSErrCode {
        GSErrCode err = NoError;
        const GS::Array<GS::Pair<short, double>> storyLevels = GetStoryLevels ();
        for (const GS::ObjectState& slab : slabs) {
            if (!slab.Contains ("level") ||
                !slab.Contains ("polygonCoordinates")) {
                continue;
            }

            API_Element element = {};
            API_ElementMemo memo = {};
            const GS::OnExit guard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });

#ifdef ServerMainVers_2600
            element.header.type   = API_SlabID;
#else
            element.header.typeID = API_SlabID;
#endif
            err = ACAPI_Element_GetDefaults (&element, &memo);

            if (!slab.Get ("level", element.slab.level)) {
                responseElementsArray (CreateErrorResponse (err, "Missing 'level' input field"));
                continue;
            }
            element.header.floorInd = GetFloorIndexAndOffset (element.slab.level, storyLevels, element.slab.level);

            GS::Array<GS::ObjectState> polygonCoordinates;
            GS::Array<GS::ObjectState> holes;
            if (!slab.Get ("polygonCoordinates", polygonCoordinates)) {
                responseElementsArray (CreateErrorResponse (err, "Missing 'polygonCoordinates' input field"));
                continue;
            }
            slab.Get ("holes", holes);
            element.slab.poly.nCoords	= polygonCoordinates.GetSize() + 1;
            element.slab.poly.nSubPolys	= 1;
            element.slab.poly.nArcs		= 0; // Curved edges are not supported yet by my code

            for (const GS::ObjectState& hole : holes) {
                if (!hole.Contains ("polygonCoordinates")) {
                    continue;
                }
                GS::Array<GS::ObjectState> holePolygonCoordinates;
                hole.Get ("polygonCoordinates", holePolygonCoordinates);
                element.slab.poly.nCoords += holePolygonCoordinates.GetSize() + 1;
                ++element.slab.poly.nSubPolys;
            }

            memo.coords = reinterpret_cast<API_Coord**> (BMAllocateHandle ((element.slab.poly.nCoords + 1) * sizeof (API_Coord), ALLOCATE_CLEAR, 0));
            memo.edgeTrims = reinterpret_cast<API_EdgeTrim**> (BMAllocateHandle ((element.slab.poly.nCoords + 1) * sizeof (API_EdgeTrim), ALLOCATE_CLEAR, 0));
            memo.sideMaterials = reinterpret_cast<API_OverriddenAttribute*> (BMAllocatePtr ((element.slab.poly.nCoords + 1) * sizeof (API_OverriddenAttribute), ALLOCATE_CLEAR, 0));
            memo.pends = reinterpret_cast<Int32**> (BMAllocateHandle ((element.slab.poly.nSubPolys + 1) * sizeof (Int32), ALLOCATE_CLEAR, 0));

            Int32 iCoord = 1;
            Int32 iPends = 1;
            AddPolyToMemo(polygonCoordinates,
                          element.slab.sideMat,
                          iCoord,
                          iPends,
                          memo);

            for (const GS::ObjectState& hole : holes) {
                if (!hole.Contains ("polygonCoordinates")) {
                    continue;
                }
                GS::Array<GS::ObjectState> holePolygonCoordinates;
                hole.Get ("polygonCoordinates", holePolygonCoordinates);

                AddPolyToMemo(holePolygonCoordinates,
                            element.slab.sideMat,
                            iCoord,
                            iPends,
                            memo);
            }

            err = ACAPI_Element_Create (&element, &memo);

            if (err != NoError) {
                responseElementsArray (CreateErrorResponse (err, "Failed to create object"));
                return err;
            }

            responseElementsArray (GS::ObjectState ("elementId", GS::ObjectState ("guid", APIGuidToString (element.header.guid))));
        }

        return NoError;
    });

    if (err != NoError) {
        const GS::UniString errorMsg = GS::UniString::Printf ("Failed to create slab #%d!", iSlab);
        return CreateErrorResponse (err, errorMsg);
    }

    return responseElementsArray;
}

CreateObjectsCommand::CreateObjectsCommand () :
    CreateElementsCommandBase ("CreateObjects")
{
}

GS::Optional<GS::UniString> CreateObjectsCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "objects": {
            "type": "array",
            "description": "The parameters of the new Objects.",
            "items": {
                "type": "object",
                "description" : "The parameters of the new Object.",
                "properties" : {
                    "libraryPartName": {
                        "type": "string",
                        "description" : "The name of the library part to use."	
                    },
                    "coordinate": {
                        "$ref": "#/3DCoordinate"
                    },
                    "dimensions": {
                        "$ref": "#/3DDimensions"
                    }
                },
                "additionalProperties": true,
                "required" : [
                    "name"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "objects"
    ]
})";
}

GS::ObjectState	CreateObjectsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> objects;
    parameters.Get ("objects", objects);

    GS::ObjectState response;
    const auto& responseElementsArray = response.AddList<GS::ObjectState> ("elements");

    API_Coord3D   apiCoordinate;
    const GSErrCode err = ACAPI_CallUndoableCommand ("Create Objects", [&] () -> GSErrCode {
        API_Element element = {};
        API_ElementMemo memo = {};
        const GS::OnExit guard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });

#ifdef ServerMainVers_2600
        element.header.type   = API_ObjectID;
#else
        element.header.typeID = API_ObjectID;
#endif
        GSErrCode err = ACAPI_Element_GetDefaults (&element, &memo);

        const GS::Array<GS::Pair<short, double>> storyLevels = GetStoryLevels ();

        for (const GS::ObjectState& object : objects) {
            GS::UniString uName;
            if (!object.Get ("libraryPartName", uName)) {
                responseElementsArray (CreateErrorResponse (err, "Missing 'libraryPartName' input field"));
                continue;
            }

            API_LibPart libPart = {};
            GS::ucscpy (libPart.docu_UName, uName.ToUStr ());

            err = ACAPI_LibraryPart_Search (&libPart, false, true);
            delete libPart.location;

            if (err != NoError) {
                const GS::UniString errorMsg = GS::UniString::Printf ("Not found library part with name '%T'", uName.ToPrintf());
                responseElementsArray (CreateErrorResponse (err, errorMsg));
                continue;
            }

            element.object.libInd = libPart.index;

            GS::ObjectState coordinate;
            if (object.Get ("coordinate", coordinate)) {
                const API_Coord3D apiCoordinate = Get3DCoordinateFromObjectState (coordinate);

                element.object.pos.x = apiCoordinate.x;
                element.object.pos.y = apiCoordinate.y;
                element.header.floorInd = GetFloorIndexAndOffset (apiCoordinate.z, storyLevels, element.object.level);
            }

            if (object.Get ("dimensions", coordinate)) {
                const API_Coord3D dimensions = Get3DCoordinateFromObjectState (coordinate);

                element.object.xRatio = dimensions.x;
                element.object.yRatio = dimensions.y;
                GS::ObjectState os ("value", dimensions.z);
                ChangeParams(memo.params, {{"ZZYZX", os}});
            }

            err = ACAPI_Element_Create (&element, &memo);

            if (err != NoError) {
                responseElementsArray (CreateErrorResponse (err, "Failed to create object"));
                continue;
            }

            responseElementsArray (GS::ObjectState ("elementId", GS::ObjectState ("guid", APIGuidToString (element.header.guid))));
        }

        return NoError;
    });

    return responseElementsArray;
}