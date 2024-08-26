#include "ElementCommands.hpp"
#include "MigrationHelper.hpp"

GetDetailsOfElementsCommand::GetDetailsOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetDetailsOfElementsCommand::GetName () const
{
    return "GetDetailsOfElements";
}

GS::Optional<GS::UniString> GetDetailsOfElementsCommand::GetInputParametersSchema () const
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

GS::Optional<GS::UniString> GetDetailsOfElementsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "detailsOfElements": {
                "type": "array",
                "items": {
                    "type": "object",
                    "description": "Details of an element.",
                    "properties": {
                        "type": {
                            "$ref": "#/ElementType"
                        },
                        "floorIndex": {
                            "type": "number"
                        },
                        "layerIndex": {
                            "type": "number"
                        },
                        "details": {
                            "type": "object",
                            "oneOf": [
                                {
                                    "title": "WallDetails",
                                    "properties": {
                                        "geometryType": {
                                            "type": "string",
                                             "enum": [
                                                "Straight",
                                                "Trapezoid",
                                                "Polygonal"
                                             ]
                                        },
                                        "begCoordinate": {
                                            "$ref": "#/2DCoordinate"
                                        },
                                        "endCoordinate": {
                                            "$ref": "#/2DCoordinate"
                                        },
                                        "height": {
                                            "type": "number",
                                            "description": "height relative to bottom"
                                        },
                                        "bottomOffset": {
                                            "type": "number",
                                            "description": "base level of the wall relative to the floor level"
                                        },
                                        "offset": {
                                            "type": "number",
                                            "description": "wall's base line's offset from ref. line"
                                        },
                                        "begThickness": {
                                            "type": "number",
                                            "description": "Thickness at the beginning in case of trapezoid wall"
                                        },
                                        "endThickness": {
                                            "type": "number",
                                            "description": "Thickness at the end in case of trapezoid wall"
                                        },
                                        "polygonOutline": {
                                            "type": "array",
                                            "description": "Polygon outline in case of polygonal wall",
                                            "items": {
                                                "$ref": "#/2DCoordinate"
                                            }
                                        }
                                    },
                                    "required": [
                                        "geometryType",
                                        "begCoordinate",
                                        "endCoordinate",
                                        "height",
                                        "bottomOffset",
                                        "offset"
                                    ]
                                },
                                {
                                    "title": "ColumnDetails",
                                    "properties": {
                                        "origin": {
                                            "$ref": "#/2DCoordinate"
                                        },
                                        "height": {
                                            "type": "number",
                                            "description": "height relative to bottom"
                                        },
                                        "bottomOffset": {
                                            "type": "number",
                                            "description": "base level of the column relative to the floor level"
                                        }
                                    },
                                    "required": [
                                        "origin",
                                        "height",
                                        "bottomOffset"
                                    ]
                                },
                                {
                                    "title": "NotYetSupportedElementTypeDetails",
                                    "properties": {
                                        "error": {
                                            "type": "string"
                                        }
                                    },
                                    "required": [
                                        "error"
                                    ]
                                }
                            ]
                        }
                    },
                    "required": [
                        "type",
                        "floorIndex",
                        "layerIndex",
                        "details"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "detailsOfElements"
        ]
    })";
}

GS::ObjectState GetDetailsOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& detailsOfElements = response.AddList<GS::ObjectState> ("detailsOfElements");

    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            detailsOfElements (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        API_Element elem = {};
        elem.header.guid = GetGuidFromObjectState (*elementId);
        GSErrCode err = ACAPI_Element_Get (&elem);

        if (err != NoError) {
            detailsOfElements (CreateErrorResponse (err, "Failed to get the details of element"));
            continue;
        }

        GS::ObjectState detailsOfElement;

#ifdef ServerMainVers_2600
        const API_ElemTypeID typeID = elem.header.type.typeID;
#else
        const API_ElemTypeID typeID = elem.header.typeID;
#endif

        detailsOfElement.Add ("type", GetElementTypeNonLocalizedName (typeID));
        detailsOfElement.Add ("floorIndex", elem.header.floorInd);
#ifdef ServerMainVers_2700
        detailsOfElement.Add ("layerIndex", elem.header.layer.ToInt32_Deprecated ());
#else
        detailsOfElement.Add ("layerIndex", elem.header.layer);
#endif

        GS::ObjectState typeSpecificDetails;

        switch (typeID) {
            case API_WallID:
                switch (elem.wall.type) {
                    case APIWtyp_Normal:
                        typeSpecificDetails.Add ("geometryType", "Straight");
                        break;
                    case APIWtyp_Trapez:
                        typeSpecificDetails.Add ("geometryType", "Trapezoid");
                        typeSpecificDetails.Add ("begThickness", elem.wall.thickness);
                        typeSpecificDetails.Add ("endThickness", elem.wall.thickness1);
                        break;
                    case APIWtyp_Poly: {
                        typeSpecificDetails.Add ("geometryType", "Polygonal");
                        const auto& polygonOutline = typeSpecificDetails.AddList<GS::ObjectState> ("polygonOutline");
                        API_ElementMemo memo = {};
                        err = ACAPI_Element_GetMemo (elem.header.guid, &memo, APIMemoMask_All);
                        if (err == NoError) {
                            const GSSize nCoords = BMhGetSize (reinterpret_cast<GSHandle> (memo.coords)) / sizeof (API_Coord) - 1;
                            for (GSIndex iCoord = 1; iCoord < nCoords; ++iCoord) {
                                polygonOutline (Create2DCoordinateObjectState ((*memo.coords)[iCoord]));
                            }
                        }
                        break;
                    }
                }
                typeSpecificDetails.Add ("begCoordinate", Create2DCoordinateObjectState (elem.wall.begC));
                typeSpecificDetails.Add ("endCoordinate", Create2DCoordinateObjectState (elem.wall.endC));
                typeSpecificDetails.Add ("height", elem.wall.height);
                typeSpecificDetails.Add ("bottomOffset", elem.wall.bottomOffset);
                typeSpecificDetails.Add ("offset", elem.wall.offset);
                break;

            case API_ColumnID:
                typeSpecificDetails.Add ("origin", Create2DCoordinateObjectState (elem.column.origoPos));
                typeSpecificDetails.Add ("height", elem.column.height);
                typeSpecificDetails.Add ("bottomOffset", elem.column.bottomOffset);
                break;

            default:
                typeSpecificDetails.Add ("error", "Not yet supported element type");
                break;
        }

        detailsOfElement.Add ("details", typeSpecificDetails);

        detailsOfElements (detailsOfElement);
    }

    return response;
}

GetSelectedElementsCommand::GetSelectedElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetSelectedElementsCommand::GetName () const
{
    return "GetSelectedElements";
}

GS::Optional<GS::UniString> GetSelectedElementsCommand::GetResponseSchema () const
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

GS::ObjectState GetSelectedElementsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    API_SelectionInfo selectionInfo;
    GS::Array<API_Neig> selectedNeigs;
    GSErrCode err = ACAPI_Selection_Get (&selectionInfo, &selectedNeigs, false);
    if (err != NoError && err != APIERR_NOSEL) {
        return CreateErrorResponse (err, "Failed to retrieve selected elements.");
    }

    GS::ObjectState response;
    const auto& elementsList = response.AddList<GS::ObjectState> ("elements");
    if (err == APIERR_NOSEL || selectionInfo.typeID == API_SelEmpty) {
        return response;
    }

    GS::Array<API_Guid> selElemGuids;
    for (API_Neig& selectedNeig : selectedNeigs) {
        API_ElemTypeID elemTypeID = API_ZombieElemID;
#ifdef ServerMainVers_2600
        API_ElemType apiElemType;
        if (ACAPI_Element_NeigIDToElemType (selectedNeig.neigID, apiElemType) != NoError) {
            continue;
        }
        elemTypeID = apiElemType.typeID;
#else
        if (ACAPI_Goodies (APIAny_NeigIDToElemTypeID, &selectedNeig.neigID, &elemTypeID) != NoError) {
            continue;
        }
#endif

        API_Guid elemGuid = selectedNeig.guid;
        if (elemTypeID == API_SectElemID) {
            API_Element element = {};
            element.header.guid = selectedNeig.guid;
            if (ACAPI_Element_Get (&element) != NoError) {
                continue;
            }
            elemGuid = element.sectElem.parentGuid;
        }

        elementsList (CreateElementIdObjectState (elemGuid));
    }

    return response;
}



GetSubelementsOfHierarchicalElementsCommand::GetSubelementsOfHierarchicalElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetSubelementsOfHierarchicalElementsCommand::GetName () const
{
    return "GetSubelementsOfHierarchicalElements";
}

GS::Optional<GS::UniString> GetSubelementsOfHierarchicalElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "hierarchicalElements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": [
            "hierarchicalElements"
        ]
    })";
}

GS::Optional<GS::UniString> GetSubelementsOfHierarchicalElementsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "subelementsOfHierarchicalElements": {
                "type": "array",
                "items": {
                    "type": "object",
                    "description": "Subelements grouped by type.",
                    "properties": {
                        "cWallSegments": {
                            "$ref": "#/Elements"
                        },
                        "cWallFrames": {
                            "$ref": "#/Elements"
                        },
                        "cWallPanels": {
                            "$ref": "#/Elements"
                        },
                        "cWallJunctions": {
                            "$ref": "#/Elements"
                        },
                        "cWallAccessories": {
                            "$ref": "#/Elements"
                        },
                        "stairRisers": {
                            "$ref": "#/Elements"
                        },
                        "stairTreads": {
                            "$ref": "#/Elements"
                        },
                        "stairStructures": {
                            "$ref": "#/Elements"
                        },
                        "railingNodes": {
                            "$ref": "#/Elements"
                        },
                        "railingSegments": {
                            "$ref": "#/Elements"
                        },
                        "railingPosts": {
                            "$ref": "#/Elements"
                        },
                        "railingRailEnds": {
                            "$ref": "#/Elements"
                        },
                        "railingRailConnections": {
                            "$ref": "#/Elements"
                        },
                        "railingHandrailEnds": {
                            "$ref": "#/Elements"
                        },
                        "railingHandrailConnections": {
                            "$ref": "#/Elements"
                        },
                        "railingToprailEnds": {
                            "$ref": "#/Elements"
                        },
                        "railingToprailConnections": {
                            "$ref": "#/Elements"
                        },
                        "railingRails": {
                            "$ref": "#/Elements"
                        },
                        "railingToprails": {
                            "$ref": "#/Elements"
                        },
                        "railingHandrails": {
                            "$ref": "#/Elements"
                        },
                        "railingPatterns": {
                            "$ref": "#/Elements"
                        },
                        "railingInnerPosts": {
                            "$ref": "#/Elements"
                        },
                        "railingPanels": {
                            "$ref": "#/Elements"
                        },
                        "railingBalusterSets": {
                            "$ref": "#/Elements"
                        },
                        "railingBalusters": {
                            "$ref": "#/Elements"
                        },
                        "beamSegments": {
                            "$ref": "#/Elements"
                        },
                        "columnSegments": {
                            "$ref": "#/Elements"
                        }
                    }
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "subelementsOfHierarchicalElements"
        ]
    })";
}

template<typename APIElemType>
static void AddSubelementsToObjectState (GS::ObjectState& subelements, APIElemType* subelemArray, const char* subelementType)
{
    const GSSize nSubelementsWithThisType = BMGetPtrSize (reinterpret_cast<GSPtr>(subelemArray)) / sizeof (APIElemType);
    if (nSubelementsWithThisType == 0) {
        return;
    }

    const auto& subelementsWithThisType = subelements.AddList<GS::ObjectState> (subelementType);
    for (GSIndex i = 0; i < nSubelementsWithThisType; ++i) {
        subelementsWithThisType (CreateElementIdObjectState (subelemArray[i].head.guid));
    }
}

GS::ObjectState GetSubelementsOfHierarchicalElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> hierarchicalElements;
    parameters.Get ("hierarchicalElements", hierarchicalElements);

    GS::ObjectState response;
    const auto& subelementsOfHierarchicalElements = response.AddList<GS::ObjectState> ("subelementsOfHierarchicalElements");

    for (const GS::ObjectState& hierarchicalElement : hierarchicalElements) {
        const GS::ObjectState* elementId = hierarchicalElement.Get ("elementId");
        if (elementId == nullptr) {
            subelementsOfHierarchicalElements (CreateErrorResponse (APIERR_BADPARS, "elementId of hierarchicalElement is missing"));
            continue;
        }

        const API_Guid elemGuid = GetGuidFromObjectState (*elementId);

        API_ElementMemo memo = {};
        GSErrCode err = ACAPI_Element_GetMemo (elemGuid, &memo, APIMemoMask_All);

        if (err != NoError) {
            subelementsOfHierarchicalElements (CreateErrorResponse (err, "Failed to get the subelements"));
            continue;
        }

        GS::ObjectState subelements;

#define AddSubelements(memoArrayFieldName) AddSubelementsToObjectState(subelements, memo.memoArrayFieldName, #memoArrayFieldName)

        AddSubelements (cWallSegments);
        AddSubelements (cWallFrames);
        AddSubelements (cWallPanels);
        AddSubelements (cWallJunctions);
        AddSubelements (cWallAccessories);

        AddSubelements (stairRisers);
        AddSubelements (stairTreads);
        AddSubelements (stairStructures);

        AddSubelements (railingNodes);
        AddSubelements (railingSegments);
        AddSubelements (railingPosts);
        AddSubelements (railingRailEnds);
        AddSubelements (railingRailConnections);
        AddSubelements (railingHandrailEnds);
        AddSubelements (railingHandrailConnections);
        AddSubelements (railingToprailEnds);
        AddSubelements (railingToprailConnections);
        AddSubelements (railingRails);
        AddSubelements (railingToprails);
        AddSubelements (railingHandrails);
        AddSubelements (railingPatterns);
        AddSubelements (railingInnerPosts);
        AddSubelements (railingPanels);
        AddSubelements (railingBalusterSets);
        AddSubelements (railingBalusters);

        AddSubelements (beamSegments);

        AddSubelements (columnSegments);

#undef AddSubelementsToObjectState

        subelementsOfHierarchicalElements (subelements);
    }

    return response;
}

MoveElementsCommand::MoveElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String MoveElementsCommand::GetName () const
{
    return "MoveElements";
}

GS::Optional<GS::UniString> MoveElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementsWithMoveVectors": {
                "type": "array",
                "description": "The elements with move vector pairs.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "moveVector": {
                            "type": "object",
                            "description" : "Move vector of a 3D point.",
                            "properties" : {
                                "x": {
                                    "type": "number",
                                    "description" : "X value of the vector."
                                },
                                "y" : {
                                    "type": "number",
                                    "description" : "Y value of the vector."
                                },
                                "z" : {
                                    "type": "number",
                                    "description" : "Z value of the vector."
                                }
                            },
                            "additionalProperties": false,
                            "required" : [
                                "x",
                                "y",
                                "z"
                            ]
                        },
                        "copy": {
                            "type": "boolean",
                            "description" : "Optional parameter. If true, then a copy of the element will be moved. By default it's false."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "moveVector"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementsWithMoveVectors"
        ]
    })";
}

static GSErrCode MoveElement (const API_Guid& elemGuid, const API_Vector3D& moveVector, bool withCopy)
{
    GS::Array<API_Neig> elementsToEdit = { API_Neig (elemGuid) };

    API_EditPars editPars = {};
    editPars.typeID = APIEdit_Drag;
    editPars.endC = moveVector;
    editPars.withDelete = !withCopy;

    return ACAPI_Element_Edit (&elementsToEdit, editPars);
}

GS::ObjectState	MoveElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elementsWithMoveVectors;
    parameters.Get ("elementsWithMoveVectors", elementsWithMoveVectors);

    API_Guid elemGuid;
    const GSErrCode err = ACAPI_CallUndoableCommand ("Move Elements", [&] () -> GSErrCode {
        for (const GS::ObjectState& elementWithMoveVector : elementsWithMoveVectors) {
            const GS::ObjectState* elementId = elementWithMoveVector.Get ("elementId");
            const GS::ObjectState* moveVector = elementWithMoveVector.Get ("moveVector");
            if (elementId == nullptr || moveVector == nullptr) {
                continue;
            }

            elemGuid = GetGuidFromObjectState (*elementId);

            bool copy = false;
            elementWithMoveVector.Get ("copy", copy);

            const GSErrCode err = MoveElement (elemGuid,
                                               Get3DCoordinateFromObjectState (*moveVector),
                                               copy);
            if (err != NoError) {
                return err;
            }
        }

        return NoError;
    });

    if (err != NoError) {
        const GS::UniString errorMsg = GS::UniString::Printf ("Failed to move element with guid %T!", APIGuidToString (elemGuid).ToPrintf ());
        return CreateErrorResponse (err, errorMsg);
    }

    return {};
}

GetGDLParametersOfElementsCommand::GetGDLParametersOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetGDLParametersOfElementsCommand::GetName () const
{
    return "GetGDLParametersOfElements";
}

GS::Optional<GS::UniString> GetGDLParametersOfElementsCommand::GetInputParametersSchema () const
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

GS::Optional<GS::UniString> GetGDLParametersOfElementsCommand::GetResponseSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "gdlParametersOfElements": {
            "type": "array",
            "description": "The GDL parameters of elements.",
            "items": {
                "$ref": "#/GDLParametersDictionary"
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "gdlParametersOfElements"
    ]
})";
}

static GS::UniString ConvertAddParIDToString (API_AddParID addParID)
{
    switch (addParID) {
        case APIParT_Integer:			return "Integer";
        case APIParT_Length:			return "Length";
        case APIParT_Angle:				return "Angle";
        case APIParT_RealNum:			return "RealNumber";
        case APIParT_LightSw:			return "LightSwitch";
        case APIParT_ColRGB:			return "RGBColor";
        case APIParT_Intens:			return "Intensity";
        case APIParT_LineTyp:			return "LineType";
        case APIParT_Mater:				return "Material";
        case APIParT_FillPat:			return "FillPattern";
        case APIParT_PenCol:			return "PenColor";
        case APIParT_CString:			return "String";
        case APIParT_Boolean:			return "Boolean";
        case APIParT_Separator:			return "Separator";
        case APIParT_Title:				return "Title";
        case APIParT_BuildingMaterial:	return "BuildingMaterial";
        case APIParT_Profile:			return "Profile";
        case APIParT_Dictionary:		return "Dictionary";
        default:						return "UNKNOWN";
    }
}

static API_AttrTypeID ConvertAddParIDToAttrTypeID (API_AddParID addParID)
{
    switch (addParID) {
        case APIParT_FillPat:			return API_FilltypeID;
        case APIParT_Mater:				return API_MaterialID;
        case APIParT_BuildingMaterial:	return API_BuildingMaterialID;
        case APIParT_Profile:			return API_ProfileID;
        case APIParT_LineTyp:			return API_LinetypeID;
        default:						return API_ZombieAttrID;
    }
}

static GS::UniString GetAttributeName (API_AttrTypeID typeID,
                                       Int32          index)
{
    API_Attribute	attrib = {};

    GS::UniString name;
    attrib.header.typeID = typeID;
    attrib.header.index = ACAPI_CreateAttributeIndex (index);
    attrib.header.uniStringNamePtr = &name;

    ACAPI_Attribute_Get (&attrib);

    if (typeID == API_MaterialID && attrib.material.texture.fileLoc != nullptr) {
        delete attrib.material.texture.fileLoc;
        attrib.material.texture.fileLoc = nullptr;
    }

    return name;
}

static GS::ObjectState GetAttributeObjectState (API_AttrTypeID typeID,
                                                Int32          index)
{
    GS::ObjectState attribute;
    attribute.Add ("index", index);
    attribute.Add ("name", GetAttributeName (typeID, index));
    return attribute;
}

static void AddValueInteger (GS::ObjectState& 	   gdlParameterDetails,
                             const API_AddParType& actParam)
{
    if (actParam.typeMod == API_ParSimple) {
        gdlParameterDetails.Add ("value", static_cast<Int32> (actParam.value.real));
    } else {
        const auto& arrayValueItemAdder = gdlParameterDetails.AddList<Int32> ("value");
        Int32 arrayIndex = 0;
        for (Int32 i1 = 1; i1 <= actParam.dim1; i1++) {
            for (Int32 i2 = 1; i2 <= actParam.dim2; i2++) {
                arrayValueItemAdder (static_cast<Int32> (((double*)*actParam.value.array) [arrayIndex++]));
            }
        }
    }
}

static void AddValueDouble (GS::ObjectState& 	  gdlParameterDetails,
                            const API_AddParType& actParam)
{
    if (actParam.typeMod == API_ParSimple) {
        gdlParameterDetails.Add ("value", actParam.value.real);
    } else {
        const auto& arrayValueItemAdder = gdlParameterDetails.AddList<double> ("value");
        Int32 arrayIndex = 0;
        for (Int32 i1 = 1; i1 <= actParam.dim1; i1++) {
            for (Int32 i2 = 1; i2 <= actParam.dim2; i2++) {
                arrayValueItemAdder (((double*)*actParam.value.array) [arrayIndex++]);
            }
        }
    }
}

static void AddValueAttribute (GS::ObjectState& 	 gdlParameterDetails,
                               const API_AddParType& actParam)
{
    if (actParam.typeMod == API_ParSimple) {
        gdlParameterDetails.Add ("value",
                                 GetAttributeObjectState (ConvertAddParIDToAttrTypeID (actParam.typeID),
                                                          static_cast<Int32> (actParam.value.real)));
    } else {
        const auto& arrayValueItemAdder = gdlParameterDetails.AddList<GS::ObjectState> ("value");
        Int32 arrayIndex = 0;
        for (Int32 i1 = 1; i1 <= actParam.dim1; i1++) {
            for (Int32 i2 = 1; i2 <= actParam.dim2; i2++) {
                arrayValueItemAdder (GetAttributeObjectState (ConvertAddParIDToAttrTypeID (actParam.typeID),
                                                              static_cast<Int32> (((double*)*actParam.value.array) [arrayIndex++])));
            }
        }
    }
}

template<typename T>
static void AddValueTrueFalseOptions (GS::ObjectState& 	    gdlParameterDetails,
                                      const API_AddParType& actParam,
                                      T optionTrue,
                                      T optionFalse)
{
    if (actParam.typeMod == API_ParSimple) {
        gdlParameterDetails.Add ("value", static_cast<Int32> (actParam.value.real) == 0 ? optionFalse : optionTrue);
    } else {
        const auto& arrayValueItemAdder = gdlParameterDetails.AddList<T> ("value");
        Int32 arrayIndex = 0;
        for (Int32 i1 = 1; i1 <= actParam.dim1; i1++) {
            for (Int32 i2 = 1; i2 <= actParam.dim2; i2++) {
                arrayValueItemAdder (static_cast<Int32> (((double*)*actParam.value.array) [arrayIndex++]) == 0 ? optionFalse : optionTrue);
            }
        }
    }
}

static void AddValueOnOff (GS::ObjectState& 	 gdlParameterDetails,
                           const API_AddParType& actParam)
{
    AddValueTrueFalseOptions (gdlParameterDetails, actParam, GS::String ("On"), GS::String ("Off"));
}

static void AddValueBool (GS::ObjectState& 	    gdlParameterDetails,
                          const API_AddParType& actParam)
{
    AddValueTrueFalseOptions (gdlParameterDetails, actParam, true, false);
}

static void AddValueString (GS::ObjectState& 	  gdlParameterDetails,
                            const API_AddParType& actParam)
{
    if (actParam.typeMod == API_ParSimple) {
        gdlParameterDetails.Add ("value", GS::UniString (actParam.value.uStr));
    } else {
        const auto& arrayValueItemAdder = gdlParameterDetails.AddList<GS::UniString> ("value");
        Int32 arrayIndex = 0;
        for (Int32 i1 = 1; i1 <= actParam.dim1; i1++) {
            for (Int32 i2 = 1; i2 <= actParam.dim2; i2++) {
                GS::uchar_t* uValueStr = (reinterpret_cast<GS::uchar_t*>(*actParam.value.array)) + arrayIndex;
                arrayIndex += GS::ucslen32 (uValueStr) + 1;
                arrayValueItemAdder (GS::UniString (uValueStr));
            }
        }
    }
}

constexpr const char* ParameterValueFieldName = "value";

static void SetParamValueInteger (API_ChangeParamType&   changeParam,
                                  const GS::ObjectState& parameterDetails)
{
    Int32 value;
    parameterDetails.Get (ParameterValueFieldName, value);
    changeParam.realValue = value;
}

static void SetParamValueDouble (API_ChangeParamType&   changeParam,
                                 const GS::ObjectState& parameterDetails)
{
    double value;
    parameterDetails.Get (ParameterValueFieldName, value);
    changeParam.realValue = value;
}

static void SetParamValueOnOff (API_ChangeParamType&   changeParam,
                                const GS::ObjectState& parameterDetails)
{
    GS::String value;
    parameterDetails.Get (ParameterValueFieldName, value);
    changeParam.realValue = (value == "Off" ? 0 : 1);
}

static void SetParamValueBool (API_ChangeParamType&   changeParam,
                               const GS::ObjectState& parameterDetails)
{
    bool value;
    parameterDetails.Get (ParameterValueFieldName, value);
    changeParam.realValue = (value ? 0 : 1);
}

static void SetParamValueString (API_ChangeParamType&   changeParam,
                                 const GS::ObjectState& parameterDetails)
{
    GS::UniString value;
    parameterDetails.Get (ParameterValueFieldName, value);

    constexpr USize MaxStrValueLength = 512;

    static GS::uchar_t strValuePtr[MaxStrValueLength];
    GS::ucscpy (strValuePtr, value.ToUStr (0, GS::Min(value.GetLength (), MaxStrValueLength)).Get ());

    changeParam.uStrValue = strValuePtr;
}

GS::ObjectState	GetGDLParametersOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& listAdder = response.AddList<GS::ObjectState> ("gdlParametersOfElements");

    API_Guid elemGuid;
    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            continue;
        }

        elemGuid = GetGuidFromObjectState (*elementId);

        API_ParamOwnerType paramOwner = {};
        paramOwner.libInd = 0;
#ifdef ServerMainVers_2600
        paramOwner.type   = API_ObjectID;
#else
        paramOwner.typeID = API_ObjectID;
#endif
        paramOwner.guid   = elemGuid;

        API_ElementMemo memo = {};
        const GSErrCode err = ACAPI_Element_GetMemo (elemGuid, &memo, APIMemoMask_AddPars);
        if (err == NoError) {
            const GSSize nParams = BMGetHandleSize ((GSHandle) memo.params) / sizeof (API_AddParType);
            GS::ObjectState gdlParametersDictionary;
            for (GSIndex ii = 0; ii < nParams; ++ii) {
                const API_AddParType& actParam = (*memo.params)[ii];

                if (actParam.typeID == APIParT_Separator) {
                    continue;
                }

                GS::ObjectState gdlParameterDetails;
                gdlParameterDetails.Add ("index", actParam.index);
                gdlParameterDetails.Add ("type", ConvertAddParIDToString (actParam.typeID));
                if (actParam.typeMod == API_ParArray) {
                    gdlParameterDetails.Add ("dimension1", actParam.dim1);
                    gdlParameterDetails.Add ("dimension2", actParam.dim2);
                }

                switch (actParam.typeID) {
                    case APIParT_Integer:
                    case APIParT_PenCol:			AddValueInteger (gdlParameterDetails, actParam);	break;
                    case APIParT_ColRGB:
                    case APIParT_Intens:
                    case APIParT_Length:
                    case APIParT_RealNum:
                    case APIParT_Angle:				AddValueDouble (gdlParameterDetails, actParam);		break;
                    case APIParT_LightSw:			AddValueOnOff (gdlParameterDetails, actParam); 		break;
                    case APIParT_Boolean: 			AddValueBool (gdlParameterDetails, actParam);		break;
                    case APIParT_LineTyp:
                    case APIParT_Mater:
                    case APIParT_FillPat:
                    case APIParT_BuildingMaterial:
                    case APIParT_Profile: 			AddValueAttribute (gdlParameterDetails, actParam);	break;
                    case APIParT_CString:
                    case APIParT_Title: 			AddValueString (gdlParameterDetails, actParam);		break;
                    default:
                    case APIParT_Dictionary:
                        // Not supported by the Archicad API yet
                        break;
                }

                gdlParametersDictionary.Add (actParam.name, gdlParameterDetails);
            }
            listAdder (gdlParametersDictionary);
            ACAPI_DisposeAddParHdl (&memo.params);
        } else {
            const GS::UniString errorMsg = GS::UniString::Printf ("Failed to get parameters of element with guid %T!", APIGuidToString (elemGuid).ToPrintf ());
            return CreateErrorResponse (err, errorMsg);
        }
    }

    return response;
}

SetGDLParametersOfElementsCommand::SetGDLParametersOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetGDLParametersOfElementsCommand::GetName () const
{
    return "SetGDLParametersOfElements";
}

GS::Optional<GS::UniString> SetGDLParametersOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "elementsWithGDLParameters": {
            "type": "array",
            "description": "The elements with GDL parameters dictionary pairs.",
            "items": {
                "type": "object",
                "properties": {
                    "elementId": {
                        "$ref": "#/ElementId"
                    },
                    "gdlParameters": {
                        "$ref": "#/GDLParametersDictionary"
                    }
                },
                "additionalProperties": false,
                "required": [
                    "elementId",
                    "gdlParameters"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "elementsWithGDLParameters"
    ]
})";
}

GS::ObjectState	SetGDLParametersOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elementsWithGDLParameters;
    parameters.Get ("elementsWithGDLParameters", elementsWithGDLParameters);

    bool invalidParameter = false;
    bool notAbleToChangeParameter = false;
    GS::String badParameterName;

    API_Guid elemGuid;
    const GSErrCode err = ACAPI_CallUndoableCommand ("Set GDL Parameters of Elements", [&] () -> GSErrCode {
        GSErrCode err = NoError;
        for (const GS::ObjectState& elementWithGDLParameters : elementsWithGDLParameters) {
            const GS::ObjectState* elementId = elementWithGDLParameters.Get ("elementId");
            if (elementId == nullptr) {
                continue;
            }

            const GS::ObjectState* gdlParameters = elementWithGDLParameters.Get ("gdlParameters");
            if (gdlParameters == nullptr) {
                continue;
            }

            elemGuid = GetGuidFromObjectState (*elementId);

            API_ParamOwnerType   paramOwner = {};

            paramOwner.libInd = 0;
#ifdef ServerMainVers_2600
            paramOwner.type   = API_ObjectID;
#else
            paramOwner.typeID  = API_ObjectID;
#endif
            paramOwner.guid   = elemGuid;

            err = ACAPI_LibraryPart_OpenParameters (&paramOwner);
            if (err == NoError) {
                API_GetParamsType getParams = {};
                err = ACAPI_LibraryPart_GetActParameters (&getParams);
                if (err == NoError) {
                    const GSSize nParams = BMGetHandleSize ((GSHandle) getParams.params) / sizeof (API_AddParType);
                    GS::HashTable<GS::String, API_AddParID> gdlParametersTypeDictionary;
                    for (GSIndex ii = 0; ii < nParams; ++ii) {
                        const API_AddParType& actParam = (*getParams.params)[ii];

                        if (actParam.typeID != APIParT_Separator) {
                            gdlParametersTypeDictionary.Add (GS::String (actParam.name), actParam.typeID);
                        }
                    }

                    GS::HashTable<GS::String, const GS::ObjectState*> changeParamsDictionary;
                    gdlParameters->EnumerateFields ([&] (const GS::String& parameterName) {
                        changeParamsDictionary.Add (parameterName, gdlParameters->Get (parameterName));
                    });

                    API_ChangeParamType changeParam = {};
                    for (const auto& kv : changeParamsDictionary) {
#ifdef ServerMainVers_2800
                        const GS::String& parameterName = kv.key;
                        const GS::ObjectState& parameterDetails = *kv.value;
#else
                        const GS::String& parameterName = *kv.key;
                        const GS::ObjectState& parameterDetails = **kv.value;
#endif

                        if (!gdlParametersTypeDictionary.ContainsKey (parameterName)) {
                            invalidParameter = true;
                            badParameterName = parameterName;
                            return APIERR_BADPARS;
                        }

                        CHTruncate (parameterName.ToCStr (), changeParam.name, sizeof (changeParam.name));
                        if (parameterDetails.Contains ("index1")) {
                            parameterDetails.Get ("index1", changeParam.ind1);
                            if (parameterDetails.Contains ("index2")) {
                                parameterDetails.Get ("index2", changeParam.ind2);
                            }
                        }

                        switch (gdlParametersTypeDictionary[parameterName]) {
                            case APIParT_Integer:
                            case APIParT_PenCol:			SetParamValueInteger (changeParam, parameterDetails); break;
                            case APIParT_ColRGB:
                            case APIParT_Intens:
                            case APIParT_Length:
                            case APIParT_RealNum:
                            case APIParT_Angle:				SetParamValueDouble (changeParam, parameterDetails);  break;
                            case APIParT_LightSw:			SetParamValueOnOff (changeParam, parameterDetails);   break;
                            case APIParT_Boolean: 			SetParamValueBool (changeParam, parameterDetails);    break;
                            case APIParT_LineTyp:
                            case APIParT_Mater:
                            case APIParT_FillPat:
                            case APIParT_BuildingMaterial:
                            case APIParT_Profile: 			SetParamValueInteger (changeParam, parameterDetails); break;
                            case APIParT_CString:
                            case APIParT_Title: 			SetParamValueString (changeParam, parameterDetails);  break;
                            default:
                            case APIParT_Dictionary:
                                // Not supported by the Archicad API yet
                                break;
                        }

                        err = ACAPI_LibraryPart_ChangeAParameter (&changeParam);
                        if (err != NoError) {
                            notAbleToChangeParameter = true;
                            badParameterName = parameterName;
                            return APIERR_BADPARS;
                        }

                        ACAPI_DisposeAddParHdl (&getParams.params);
                        ACAPI_LibraryPart_GetActParameters (&getParams);
                    }

                    API_Element	element = {};
                    element.header.guid = elemGuid;

                    err = ACAPI_Element_Get (&element);
                    if (err == NoError) {
                        API_Element 	mask = {};
                        API_ElementMemo memo = {};

                        ACAPI_ELEMENT_MASK_CLEAR (mask);
#ifdef ServerMainVers_2600
                        switch (element.header.type.typeID) {
#else
                        switch (element.header.typeID) {
#endif
                            case API_ObjectID:
                                element.object.xRatio = getParams.a;
                                element.object.yRatio = getParams.b;
                                ACAPI_ELEMENT_MASK_SET (mask, API_ObjectType, xRatio);
                                ACAPI_ELEMENT_MASK_SET (mask, API_ObjectType, yRatio);
                                break;
                            case API_WindowID:
                            case API_DoorID:
                                element.window.openingBase.width  = getParams.a;
                                element.window.openingBase.height = getParams.b;
                                ACAPI_ELEMENT_MASK_SET (mask, API_WindowType, openingBase.width);
                                ACAPI_ELEMENT_MASK_SET (mask, API_WindowType, openingBase.height);
                                break;
                            case API_SkylightID:
                                element.skylight.openingBase.width  = getParams.a;
                                element.skylight.openingBase.height = getParams.b;
                                ACAPI_ELEMENT_MASK_SET (mask, API_SkylightType, openingBase.width);
                                ACAPI_ELEMENT_MASK_SET (mask, API_SkylightType, openingBase.height);
                                break;
                            default:
                                // Not supported yet
                                break;
                        }

                        memo.params = getParams.params;
                        err = ACAPI_Element_Change (&element, &mask, &memo, APIMemoMask_AddPars, true);
                    }
                }
                ACAPI_LibraryPart_CloseParameters ();
                ACAPI_DisposeAddParHdl (&getParams.params);
            }
        }

        return err;
    });

    if (err != NoError) {
        GS::UniString errorMsg;
        if (invalidParameter) {
            errorMsg = GS::UniString::Printf ("Invalid input: %s is not a GDL parameter of element %T", badParameterName.ToCStr (), APIGuidToString (elemGuid).ToPrintf ());
        } else if (notAbleToChangeParameter) {
            errorMsg = GS::UniString::Printf ("Failed to change parameter %s of element with guid %T", badParameterName.ToCStr (), APIGuidToString (elemGuid).ToPrintf ());
        } else {
            errorMsg = GS::UniString::Printf ("Failed to change parameters of element with guid %T", APIGuidToString (elemGuid).ToPrintf ());
        }

        return CreateErrorResponse (err, errorMsg);
    }

    return {};
}

FilterElementsCommand::FilterElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String FilterElementsCommand::GetName () const
{
    return "FilterElements";
}

GS::Optional<GS::UniString> FilterElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            },
            "filters": {
                "type": "array",
                "items": {
                    "$ref": "#/ElementFilter"
                },
                "minItems": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> FilterElementsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "filteredElements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": [
            "filteredElements"
        ]
    })";
}

static API_ElemFilterFlags ConvertFilterStringToFlag (const GS::UniString& filter)
{
    if (filter == "IsEditable")
        return APIFilt_IsEditable;
    if (filter == "IsVisibleByLayer")
        return APIFilt_OnVisLayer;
    if (filter == "IsVisibleByRenovation")
        return APIFilt_IsVisibleByRenovation;
    if (filter == "IsVisibleByStructureDisplay")
        return APIFilt_IsInStructureDisplay;
    if (filter == "IsVisibleIn3D")
        return APIFilt_In3D;
    if (filter == "OnActualFloor")
        return APIFilt_OnActFloor;
    if (filter == "OnActualLayout")
        return APIFilt_OnActLayout;
    if (filter == "InMyWorkspace")
        return APIFilt_InMyWorkspace;
    if (filter == "IsIndependent")
        return APIFilt_IsIndependent;
    if (filter == "InCroppedView")
        return APIFilt_InCroppedView;
    if (filter == "HasAccessRight")
        return APIFilt_HasAccessRight;
    if (filter == "IsOverriddenByRenovation")
        return APIFilt_IsOverridden;
    return APIFilt_None;
}

GS::ObjectState FilterElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::Array<GS::UniString> filters;
    parameters.Get ("filters", filters);

    API_ElemFilterFlags filterFlags = APIFilt_None;
    for (const GS::UniString& filter : filters) {
        filterFlags |= ConvertFilterStringToFlag (filter);
    }
    if (filterFlags == APIFilt_None) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing filters!");
    }

    GS::ObjectState response;
    const auto& filteredElements = response.AddList<GS::ObjectState> ("filteredElements");

    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            continue;
        }

        const API_Guid elemGuid = GetGuidFromObjectState (*elementId);
        if (!ACAPI_Element_Filter (elemGuid, filterFlags)) {
            continue;
        }

        filteredElements (CreateElementIdObjectState (elemGuid));
    }

    return response;
}

HighlightElementsCommand::HighlightElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String HighlightElementsCommand::GetName () const
{
    return "HighlightElements";
}

GS::Optional<GS::UniString> HighlightElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            },
            "highlightedColors": {
                "type": "array",
                "description": "A list of colors to highlight elements.",
                "items": {
                    "type": "array",
                    "description": "Color of the highlighted element as an [r, g, b, a] array. Each component must be in the 0-255 range.",
                    "items": {
                        "type": "integer"
                    },
                    "minItems": 4,
                    "maxItems": 4
                }
            },
            "wireframe3D": {
                "type": "boolean",
                "description" : "Optional parameter. Switch non highlighted elements in the 3D window to wireframe."
            },
            "nonHighlightedColor": {
                "type": "array",
                "description": "Optional parameter. Color of the non highlighted elements as an [r, g, b, a] array. Each component must be in the 0-255 range.",
                "items": {
                    "type": "integer"
                },
                "minItems": 4,
                "maxItems": 4
            }
        },
        "additionalProperties": false,
        "required": [
            "elements",
            "highlightedColors"
        ]
    })";
}

GS::Optional<GS::UniString> HighlightElementsCommand::GetResponseSchema () const
{
    return {};
}

#ifdef ServerMainVers_2600

static API_RGBAColor GetRGBAColorFromArray (const GS::Array<GS::Int32>& color)
{
    return API_RGBAColor {
        color[0] / 255.0,
        color[1] / 255.0,
        color[2] / 255.0,
        color[3] / 255.0
    };
}

static GS::Optional<API_RGBAColor> GetRGBAColorFromObjectState (const GS::ObjectState& os, const GS::String& name)
{
    GS::Array<GS::Int32> color;
    if (os.Get (name, color)) {
        return GetRGBAColorFromArray (color);
    } else {
        return {};
    }
}

GS::ObjectState HighlightElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    if (elements.IsEmpty ()) {
        ACAPI_UserInput_ClearElementHighlight ();
        // need to call redraw for changes to take effect
        ACAPI_View_Redraw ();
        return {};
    }

    GS::Array<GS::Array<GS::Int32>> highlightedColors;
    parameters.Get ("highlightedColors", highlightedColors);

    if (highlightedColors.GetSize () != elements.GetSize ()) {
        return CreateErrorResponse (APIERR_BADPARS, "The size of 'elements' array and 'highlightedColors' array does not match.");
    }

    GS::HashTable<API_Guid, API_RGBAColor> elementsWithColors;
    for (USize i = 0; i < elements.GetSize (); ++i) {
        GS::ObjectState elementId;
        if (elements[i].Get ("elementId", elementId)) {
            const API_Guid elemGuid = GetGuidFromObjectState (elementId);
            const API_RGBAColor color = GetRGBAColorFromArray (highlightedColors[i]);
            elementsWithColors.Add (elemGuid, color);
        }
    }

    GS::Optional<bool> wireframe3D;
    bool tmp;
    if (parameters.Get ("wireframe3D", tmp)) {
        wireframe3D = tmp;
    }

    const GS::Optional<API_RGBAColor> nonHighlightedColor = GetRGBAColorFromObjectState (parameters, "nonHighlightedColor");

    ACAPI_UserInput_SetElementHighlight (elementsWithColors, wireframe3D, nonHighlightedColor);

    // need to call redraw for changes to take effect
    ACAPI_View_Redraw ();

    return {};
}

#else

GS::ObjectState HighlightElementsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    return CreateErrorResponse (APIERR_GENERAL, GetName() + " command is not supported for this AC version.");
}

#endif