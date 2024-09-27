#include "ElementCommands.hpp"
#include "MigrationHelper.hpp"

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

GetElementsByTypeCommand::GetElementsByTypeCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetElementsByTypeCommand::GetName () const
{
    return "GetElementsByType";
}

GS::Optional<GS::UniString> GetElementsByTypeCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementType": {
                "$ref": "#/ElementType"
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
            "elementType"
        ]
    })";
}

GS::Optional<GS::UniString> GetElementsByTypeCommand::GetResponseSchema () const
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

GS::ObjectState GetElementsByTypeCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    API_ElemTypeID elemType = API_ZombieElemID;
    GS::UniString elementTypeStr;
    if (parameters.Get ("elementType", elementTypeStr)) {
        elemType = GetElementTypeFromNonLocalizedName (elementTypeStr);
    }

    API_ElemFilterFlags filterFlags = APIFilt_None;
    GS::Array<GS::UniString> filters;
    if (parameters.Get ("filters", filters)) {
        for (const GS::UniString& filter : filters) {
            filterFlags |= ConvertFilterStringToFlag (filter);
        }
    }

    GS::Array<API_Guid> elemList;
    GSErrCode err = ACAPI_Element_GetElemList (elemType, &elemList, filterFlags);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to retrieve elements.");
    }

    GS::ObjectState response;
    const auto& elements = response.AddList<GS::ObjectState> ("elements");

    for (const API_Guid& elemGuid : elemList) {
        elements (CreateElementIdObjectState (elemGuid));
    }

    return response;
}

GS::String GetAllElementsCommand::GetName () const
{
    return "GetAllElements";
}

GS::Optional<GS::UniString> GetAllElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "filters": {
                "type": "array",
                "items": {
                    "$ref": "#/ElementFilter"
                },
                "minItems": 1
            }
        },
        "additionalProperties": false,
        "required": []
    })";
}

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
                        "drawIndex": {
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
                        "drawIndex",
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
        const API_ElemTypeID typeID = GetElemTypeId (elem.header);

        detailsOfElement.Add ("type", GetElementTypeNonLocalizedName (typeID));
        detailsOfElement.Add ("floorIndex", elem.header.floorInd);
        detailsOfElement.Add ("layerIndex", GetAttributeIndex (elem.header.layer));
        detailsOfElement.Add ("drawIndex", elem.header.drwIndex);

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
                    case APIWtyp_Poly:
                        {
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

SetDetailsOfElementsCommand::SetDetailsOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetDetailsOfElementsCommand::GetName () const
{
    return "SetDetailsOfElements";
}

GS::Optional<GS::UniString> SetDetailsOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementsWithDetails": {
                "type": "array",
                "description": "The elements with parameters.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "details": {
                            "type": "object",
                            "description": "Details of an element.",
                            "properties": {
                                "floorIndex": {
                                    "type": "number"
                                },
                                "layerIndex": {
                                    "type": "number"
                                },
                                "drawIndex": {
                                    "type": "number"
                                }
                            },
                            "required": []
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "details"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementsWithDetails"
        ]
    })";
}

GS::Optional<GS::UniString> SetDetailsOfElementsCommand::GetResponseSchema () const
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

GS::ObjectState SetDetailsOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elementsWithDetails;
    parameters.Get ("elementsWithDetails", elementsWithDetails);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("SetDetailsOfElementsCommand", [&]() {
        for (const GS::ObjectState& elementWithDetails : elementsWithDetails) {
            const GS::ObjectState* elementId = elementWithDetails.Get ("elementId");
            if (elementId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
                continue;
            }

            API_Element elem = {};
            elem.header.guid = GetGuidFromObjectState (*elementId);
            GSErrCode err = ACAPI_Element_Get (&elem);

            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to find the element"));
                continue;
            }

            const GS::ObjectState* details = elementWithDetails.Get ("details");
            if (details == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "details field is missing"));
                continue;
            }

            API_Element mask = {};
            ACAPI_ELEMENT_MASK_CLEAR (mask);
            if (details->Get ("floorIndex", elem.header.floorInd)) {
                ACAPI_ELEMENT_MASK_SET (mask, API_Elem_Head, floorInd);
            }
            Int32 layerIndex;
            if (details->Get ("layerIndex", layerIndex)) {
                elem.header.layer = ACAPI_CreateAttributeIndex (layerIndex);
                ACAPI_ELEMENT_MASK_SET (mask, API_Elem_Head, layer);
            }
            if (details->Get ("drawIndex", elem.header.drwIndex)) {
                ACAPI_ELEMENT_MASK_SET (mask, API_Elem_Head, drwIndex);
            }

            constexpr API_ElementMemo* memo = nullptr;
            constexpr UInt64 memoMask = 0;
            constexpr bool withDel = true;
            err = ACAPI_Element_Change (&elem, &mask, memo, memoMask, withDel);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to change element"));
                continue;
            }

            executionResults (CreateSuccessfulExecutionResult ());
        }

        return NoError;
    });

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

ChangeSelectionOfElementsCommand::ChangeSelectionOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ChangeSelectionOfElementsCommand::GetName () const
{
    return "ChangeSelectionOfElements";
}

GS::Optional<GS::UniString> ChangeSelectionOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "addElementsToSelection": {
                "$ref": "#/Elements"
            },
            "removeElementsFromSelection": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": [
        ]
    })";
}

GS::Optional<GS::UniString> ChangeSelectionOfElementsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResultsOfAddToSelection": {
                "$ref": "#/ExecutionResults"
            },
            "executionResultsOfRemoveFromSelection": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResultsOfAddToSelection",
            "executionResultsOfRemoveFromSelection"
        ]
    })";
}

GS::ObjectState ChangeSelectionOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> addElementsToSelection;
    parameters.Get ("addElementsToSelection", addElementsToSelection);
    GS::Array<GS::ObjectState> removeElementsFromSelection;
    parameters.Get ("removeElementsFromSelection", removeElementsFromSelection);

    GS::ObjectState response;
    const auto& executionResultsOfAddToSelection = response.AddList<GS::ObjectState> ("executionResultsOfAddToSelection");
    const auto& executionResultsOfRemoveFromSelection = response.AddList<GS::ObjectState> ("executionResultsOfRemoveFromSelection");

    for (const GS::ObjectState& element : addElementsToSelection) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            executionResultsOfAddToSelection (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        const GSErrCode err = ACAPI_Selection_Select ({ API_Neig (GetGuidFromObjectState (*elementId)) }, true);
        if (err != NoError) {
            executionResultsOfAddToSelection (CreateFailedExecutionResult (err, "Failed to add to selection"));
        } else {
            executionResultsOfAddToSelection (CreateSuccessfulExecutionResult ());
        }
    }

    for (const GS::ObjectState& element : removeElementsFromSelection) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            executionResultsOfRemoveFromSelection (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        const GSErrCode err = ACAPI_Selection_Select ({ API_Neig (GetGuidFromObjectState (*elementId)) }, false);
        if (err != NoError) {
            executionResultsOfRemoveFromSelection (CreateFailedExecutionResult (err, "Failed to remove from selection"));
        } else {
            executionResultsOfRemoveFromSelection (CreateSuccessfulExecutionResult ());
        }
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

GS::Optional<GS::UniString> MoveElementsCommand::GetResponseSchema () const
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

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Move Elements", [&]() -> GSErrCode {
        for (const GS::ObjectState& elementWithMoveVector : elementsWithMoveVectors) {
            const GS::ObjectState* elementId = elementWithMoveVector.Get ("elementId");
            if (elementId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
                continue;
            }

            const GS::ObjectState* moveVector = elementWithMoveVector.Get ("moveVector");
            if (moveVector == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "moveVector is missing"));
                continue;
            }

            const API_Guid elemGuid = GetGuidFromObjectState (*elementId);

            bool copy = false;
            elementWithMoveVector.Get ("copy", copy);

            const GSErrCode err = MoveElement (elemGuid,
                                               Get3DCoordinateFromObjectState (*moveVector),
                                               copy);
            if (err != NoError) {
                const GS::UniString errorMsg = GS::UniString::Printf ("Failed to move element with guid %T!", APIGuidToString (elemGuid).ToPrintf ());
                executionResults (CreateFailedExecutionResult (err, errorMsg));
            } else {
                executionResults (CreateSuccessfulExecutionResult ());
            }
        }

        return NoError;
    });

    return response;
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
                "$ref": "#/GDLParameterList"
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

static void AddValueInteger (GS::ObjectState& gdlParameterDetails,
                             const API_AddParType& actParam)
{
    if (actParam.typeMod == API_ParSimple) {
        gdlParameterDetails.Add ("value", static_cast<Int32> (actParam.value.real));
    } else {
        const auto& arrayValueItemAdder = gdlParameterDetails.AddList<Int32> ("value");
        Int32 arrayIndex = 0;
        for (Int32 i1 = 1; i1 <= actParam.dim1; i1++) {
            for (Int32 i2 = 1; i2 <= actParam.dim2; i2++) {
                arrayValueItemAdder (static_cast<Int32> (((double*) *actParam.value.array)[arrayIndex++]));
            }
        }
    }
}

static void AddValueDouble (GS::ObjectState& gdlParameterDetails,
                            const API_AddParType& actParam)
{
    if (actParam.typeMod == API_ParSimple) {
        gdlParameterDetails.Add ("value", actParam.value.real);
    } else {
        const auto& arrayValueItemAdder = gdlParameterDetails.AddList<double> ("value");
        Int32 arrayIndex = 0;
        for (Int32 i1 = 1; i1 <= actParam.dim1; i1++) {
            for (Int32 i2 = 1; i2 <= actParam.dim2; i2++) {
                arrayValueItemAdder (((double*) *actParam.value.array)[arrayIndex++]);
            }
        }
    }
}

template<typename T>
static void AddValueTrueFalseOptions (GS::ObjectState& gdlParameterDetails,
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
                arrayValueItemAdder (static_cast<Int32> (((double*) *actParam.value.array)[arrayIndex++]) == 0 ? optionFalse : optionTrue);
            }
        }
    }
}

static void AddValueOnOff (GS::ObjectState& gdlParameterDetails,
                           const API_AddParType& actParam)
{
    AddValueTrueFalseOptions (gdlParameterDetails, actParam, GS::String ("On"), GS::String ("Off"));
}

static void AddValueBool (GS::ObjectState& gdlParameterDetails,
                          const API_AddParType& actParam)
{
    AddValueTrueFalseOptions (gdlParameterDetails, actParam, true, false);
}

static void AddValueString (GS::ObjectState& gdlParameterDetails,
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

static void SetParamValueInteger (API_ChangeParamType& changeParam,
                                  const GS::ObjectState& parameterDetails)
{
    Int32 value;
    parameterDetails.Get (ParameterValueFieldName, value);
    changeParam.realValue = value;
}

static void SetParamValueDouble (API_ChangeParamType& changeParam,
                                 const GS::ObjectState& parameterDetails)
{
    double value;
    parameterDetails.Get (ParameterValueFieldName, value);
    changeParam.realValue = value;
}

static void SetParamValueOnOff (API_ChangeParamType& changeParam,
                                const GS::ObjectState& parameterDetails)
{
    GS::String value;
    parameterDetails.Get (ParameterValueFieldName, value);
    changeParam.realValue = (value == "Off" ? 0 : 1);
}

static void SetParamValueBool (API_ChangeParamType& changeParam,
                               const GS::ObjectState& parameterDetails)
{
    bool value;
    parameterDetails.Get (ParameterValueFieldName, value);
    changeParam.realValue = (value ? 0 : 1);
}

static void SetParamValueString (API_ChangeParamType& changeParam,
                                 const GS::ObjectState& parameterDetails)
{
    GS::UniString value;
    parameterDetails.Get (ParameterValueFieldName, value);

    constexpr USize MaxStrValueLength = 512;

    static GS::uchar_t strValuePtr[MaxStrValueLength];
    GS::ucscpy (strValuePtr, value.ToUStr (0, GS::Min (value.GetLength (), MaxStrValueLength)).Get ());

    changeParam.uStrValue = strValuePtr;
}

GS::ObjectState	GetGDLParametersOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& elemGdlParameterListAdder = response.AddList<GS::ObjectState> ("gdlParametersOfElements");

    API_Guid elemGuid;
    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            elemGdlParameterListAdder (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        elemGuid = GetGuidFromObjectState (*elementId);

        API_ParamOwnerType paramOwner = {};
        paramOwner.libInd = 0;
#ifdef ServerMainVers_2600
        paramOwner.type = API_ObjectID;
#else
        paramOwner.typeID = API_ObjectID;
#endif
        paramOwner.guid = elemGuid;

        API_ElementMemo memo = {};
        const GSErrCode err = ACAPI_Element_GetMemo (elemGuid, &memo, APIMemoMask_AddPars);
        if (err != NoError) {
            const GS::UniString errorMsg = GS::UniString::Printf ("Failed to get parameters of element with guid %T!", APIGuidToString (elemGuid).ToPrintf ());
            elemGdlParameterListAdder (CreateErrorResponse (err, errorMsg));
        }

        const GSSize nParams = BMGetHandleSize ((GSHandle) memo.params) / sizeof (API_AddParType);
        GS::ObjectState gdlParameters;
        const auto& parameterListAdder = gdlParameters.AddList<GS::ObjectState> ("parameters");
        for (GSIndex ii = 0; ii < nParams; ++ii) {
            const API_AddParType& actParam = (*memo.params)[ii];

            if (actParam.typeID == APIParT_Separator) {
                continue;
            }

            GS::ObjectState gdlParameterDetails;
            gdlParameterDetails.Add ("name", actParam.name);
            gdlParameterDetails.Add ("index", actParam.index);
            gdlParameterDetails.Add ("type", ConvertAddParIDToString (actParam.typeID));
            if (actParam.typeMod == API_ParArray) {
                gdlParameterDetails.Add ("dimension1", actParam.dim1);
                gdlParameterDetails.Add ("dimension2", actParam.dim2);
            }

            switch (actParam.typeID) {
                case APIParT_Integer:
                case APIParT_PenCol:
                case APIParT_LineTyp:
                case APIParT_Mater:
                case APIParT_FillPat:
                case APIParT_BuildingMaterial:
                case APIParT_Profile:
                    AddValueInteger (gdlParameterDetails, actParam);
                    break;
                case APIParT_ColRGB:
                case APIParT_Intens:
                case APIParT_Length:
                case APIParT_RealNum:
                case APIParT_Angle:
                    AddValueDouble (gdlParameterDetails, actParam);
                    break;
                case APIParT_LightSw:
                    AddValueOnOff (gdlParameterDetails, actParam);
                    break;
                case APIParT_Boolean:
                    AddValueBool (gdlParameterDetails, actParam);
                    break;
                case APIParT_CString:
                case APIParT_Title:
                    AddValueString (gdlParameterDetails, actParam);
                    break;
                default:
                case APIParT_Dictionary:
                    // Not supported by the Archicad API yet
                    break;
            }

            parameterListAdder (gdlParameterDetails);
        }
        elemGdlParameterListAdder (gdlParameters);
        ACAPI_DisposeAddParHdl (&memo.params);
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
                        "$ref": "#/GDLParameterList"
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

GS::Optional<GS::UniString> SetGDLParametersOfElementsCommand::GetResponseSchema () const
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

GS::ObjectState	SetGDLParametersOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elementsWithGDLParameters;
    parameters.Get ("elementsWithGDLParameters", elementsWithGDLParameters);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Set GDL Parameters of Elements", [&]() -> GSErrCode {
        for (const GS::ObjectState& elementWithGDLParameters : elementsWithGDLParameters) {
            GSErrCode err = NoError;
            GS::UniString errMessage;
            const GS::ObjectState* elementId = elementWithGDLParameters.Get ("elementId");
            if (elementId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
                continue;
            }

            const API_Guid elemGuid = GetGuidFromObjectState (*elementId);
            API_ParamOwnerType paramOwner = {};
            paramOwner.libInd = 0;
#ifdef ServerMainVers_2600
            paramOwner.type = API_ObjectID;
#else
            paramOwner.typeID = API_ObjectID;
#endif
            paramOwner.guid = elemGuid;

            GS::Array<GS::ObjectState> elemGdlParameters;
            elementWithGDLParameters.Get ("gdlParameters", elemGdlParameters);

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

                    for (const GS::ObjectState& elemGdlParametersItem : elemGdlParameters) {
                        GS::Array<GS::ObjectState> parameters;
                        elemGdlParametersItem.Get ("parameters", parameters);

                        API_ChangeParamType changeParam = {};
                        for (const GS::ObjectState& parameter : parameters) {
                            GS::String parameterName;
                            parameter.Get ("name", parameterName);

                            if (!gdlParametersTypeDictionary.ContainsKey (parameterName)) {
                                errMessage = GS::UniString::Printf ("Invalid input: %s is not a GDL parameter of element %T", parameterName.ToCStr (), APIGuidToString (elemGuid).ToPrintf ());
                                err = APIERR_BADPARS;
                                break;
                            }

                            CHTruncate (parameterName.ToCStr (), changeParam.name, sizeof (changeParam.name));
                            if (parameter.Contains ("index1")) {
                                parameter.Get ("index1", changeParam.ind1);
                                if (parameter.Contains ("index2")) {
                                    parameter.Get ("index2", changeParam.ind2);
                                }
                            }

                            switch (gdlParametersTypeDictionary[parameterName]) {
                                case APIParT_Integer:
                                case APIParT_PenCol:
                                case APIParT_LineTyp:
                                case APIParT_Mater:
                                case APIParT_FillPat:
                                case APIParT_BuildingMaterial:
                                case APIParT_Profile:
                                    SetParamValueInteger (changeParam, parameter);
                                    break;
                                case APIParT_ColRGB:
                                case APIParT_Intens:
                                case APIParT_Length:
                                case APIParT_RealNum:
                                case APIParT_Angle:
                                    SetParamValueDouble (changeParam, parameter);
                                    break;
                                case APIParT_LightSw:
                                    SetParamValueOnOff (changeParam, parameter);
                                    break;
                                case APIParT_Boolean:
                                    SetParamValueBool (changeParam, parameter);
                                    break;
                                case APIParT_CString:
                                case APIParT_Title:
                                    SetParamValueString (changeParam, parameter);
                                    break;
                                default:
                                case APIParT_Dictionary:
                                    // Not supported by the Archicad API yet
                                    break;
                            }

                            err = ACAPI_LibraryPart_ChangeAParameter (&changeParam);
                            if (err != NoError) {
                                errMessage = GS::UniString::Printf ("Failed to change parameter %s of element with guid %T", parameterName.ToCStr (), APIGuidToString (elemGuid).ToPrintf ());
                                break;
                            }

                            ACAPI_DisposeAddParHdl (&getParams.params);
                            ACAPI_LibraryPart_GetActParameters (&getParams);
                        }
                    }

                    if (err == NoError) {
                        API_Element	element = {};
                        element.header.guid = elemGuid;

                        err = ACAPI_Element_Get (&element);
                        if (err == NoError) {
                            API_Element 	mask = {};
                            API_ElementMemo memo = {};

                            ACAPI_ELEMENT_MASK_CLEAR (mask);
                            switch (GetElemTypeId (element.header)) {
                                case API_ObjectID:
                                    element.object.xRatio = getParams.a;
                                    element.object.yRatio = getParams.b;
                                    ACAPI_ELEMENT_MASK_SET (mask, API_ObjectType, xRatio);
                                    ACAPI_ELEMENT_MASK_SET (mask, API_ObjectType, yRatio);
                                    break;
                                case API_WindowID:
                                case API_DoorID:
                                    element.window.openingBase.width = getParams.a;
                                    element.window.openingBase.height = getParams.b;
                                    ACAPI_ELEMENT_MASK_SET (mask, API_WindowType, openingBase.width);
                                    ACAPI_ELEMENT_MASK_SET (mask, API_WindowType, openingBase.height);
                                    break;
                                case API_SkylightID:
                                    element.skylight.openingBase.width = getParams.a;
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
                }
                ACAPI_LibraryPart_CloseParameters ();
                ACAPI_DisposeAddParHdl (&getParams.params);
            }

            if (err != NoError) {
                if (errMessage.IsEmpty ()) {
                    executionResults (CreateFailedExecutionResult (err, GS::UniString::Printf ("Failed to change parameters of element with guid %T", APIGuidToString (elemGuid).ToPrintf ())));
                } else {
                    executionResults (CreateFailedExecutionResult (err, errMessage));
                }
            } else {
                executionResults (CreateSuccessfulExecutionResult ());
            }
        }

        return NoError;
    });

    return response;
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
    return R"({
        "$ref": "#/ExecutionResult"
    })";
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
        return CreateSuccessfulExecutionResult ();
    }

    GS::Array<GS::Array<GS::Int32>> highlightedColors;
    parameters.Get ("highlightedColors", highlightedColors);

    if (highlightedColors.GetSize () != elements.GetSize ()) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "The size of 'elements' array and 'highlightedColors' array does not match.");
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

    return CreateSuccessfulExecutionResult ();
}

#else

GS::ObjectState HighlightElementsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    return CreateFailedExecutionResult (APIERR_GENERAL, GetName () + " command is not supported for this AC version.");
}

#endif