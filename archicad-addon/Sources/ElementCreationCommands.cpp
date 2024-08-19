#include "ElementCreationCommands.hpp"
#include "ObjectState.hpp"
#include "OnExit.hpp"
#include "MigrationHelper.hpp"

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

    const GS::UniString elemTypeName = ElemID_To_Name (elemTypeID);
    const Stories stories = GetStories ();

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

        for (const GS::ObjectState& data : dataArray) {
            SetTypeSpecificParameters (element, memo, stories, data);

            err = ACAPI_Element_Create (&element, &memo);
            if (err != NoError) {
                elements (CreateErrorResponse (err, "Failed to create new " + elemTypeName));
                continue;
            }

            elements (GS::ObjectState ("elementId", GS::ObjectState ("guid", APIGuidToString (element.header.guid))));
        }

        return NoError;
    });

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

GS::Optional<GS::ObjectState> CreateColumnsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo&, const Stories& stories, const GS::ObjectState& parameters) const
{
    GS::ObjectState coordinates;
    parameters.Get ("coordinates", coordinates);
    API_Coord3D apiCoordinate = Get3DCoordinateFromObjectState (coordinates);

    const auto floorIndexAndOffset = GetFloorIndexAndOffset (apiCoordinate.z, stories);
    element.header.floorInd = floorIndexAndOffset.first;
    element.column.bottomOffset = floorIndexAndOffset.second;
    element.column.origoPos.x = apiCoordinate.x;
    element.column.origoPos.y = apiCoordinate.y;

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
                    "polygonCoordinates": { 
                        "type": "array",
                        "description": "The 2D coordinates of the edge of the slab.",
                        "items": {
                            "$ref": "#/2DCoordinate"
                        }
                    },
                    "holes" : {
                        "type": "array",
                        "description": "Array of parameters of holes.",
                        "items": {
                            "type": "object",
                            "description" : "The parameters of the hole.",
                            "properties" : {
                                "polygonCoordinates": { 
                                    "type": "array",
                                    "description": "The 2D coordinates of the edge of the slab.",
                                    "items": {
                                        "$ref": "#/2DCoordinate"
                                    }
                                }
                            },
                            "additionalProperties": false,
                            "required" : [
                                "polygonCoordinates"
                            ]
                        }
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

GS::Optional<GS::ObjectState> CreateSlabsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const
{
    parameters.Get ("level", element.slab.level);
    const auto floorIndexAndOffset = GetFloorIndexAndOffset (element.slab.level, stories);
    element.header.floorInd = floorIndexAndOffset.first;

    GS::Array<GS::ObjectState> polygonCoordinates;
    GS::Array<GS::ObjectState> holes;
    parameters.Get ("polygonCoordinates", polygonCoordinates);
    parameters.Get ("holes", holes);
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

    return {};
}

CreateObjectsCommand::CreateObjectsCommand () :
    CreateElementsCommandBase ("CreateObjects", API_ObjectID, "objectsData")
{
}

GS::Optional<GS::UniString> CreateObjectsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "objectsData": {
                "type": "array",
                "description": "Array of data to create Objects.",
                "items": {
                    "type": "object",
                    "description": "The parameters of the new Object.",
                    "properties": {
                        "libraryPartName": {
                            "type": "string",
                            "description" : "The name of the library part to use."	
                        },
                        "coordinates": {
                            "$ref": "#/3DCoordinate"
                        },
                        "dimensions": {
                            "$ref": "#/3DDimensions"
                        }
                    },
                    "additionalProperties": false,
                    "required" : [
                        "libraryPartName",
                        "coordinates",
                        "dimensions"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "objectsData"
        ]
    })";
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

GS::Optional<GS::ObjectState> CreateObjectsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const
{
    GS::UniString uName;
    parameters.Get ("libraryPartName", uName);

    API_LibPart libPart = {};
    GS::ucscpy (libPart.docu_UName, uName.ToUStr ());

    GSErrCode err = ACAPI_LibraryPart_Search (&libPart, false, true);
    delete libPart.location;

    if (err != NoError) {
        return CreateErrorResponse (err, GS::UniString::Printf ("Not found library part with name '%T'", uName.ToPrintf()));
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
        ChangeParams(memo.params, {{"ZZYZX", os}});
    }

    return {};
}