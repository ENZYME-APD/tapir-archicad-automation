#include "ElementCommands.hpp"
#include "MigrationHelper.hpp"
#include "GSUnID.hpp"

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

template <typename ListProxyType>
static bool GetElementsFromCurrentDatabase (const GS::ObjectState& parameters, ListProxyType& elementsListProxy)
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
        return false;
    }

    for (const API_Guid& elemGuid : elemList) {
        elementsListProxy (CreateElementIdObjectState (elemGuid));
    }
    return true;
}

template <typename ListProxyType>
static GSErrCode ExecuteActionForEachDatabase (
    const std::function<bool ()>& action,
    const GS::Array<API_Guid>& databaseIds,
    ListProxyType& executionResultsListProxy
    )
{
    API_DatabaseInfo startingDatabase;
    GSErrCode err = ACAPI_Database_GetCurrentDatabase (&startingDatabase);
    if (err != NoError) {
        return err;
    }
    for (const API_Guid databaseId : databaseIds) {
        API_DatabaseInfo targetDbInfo = {};
        targetDbInfo.databaseUnId.elemSetId = databaseId;
        err = ACAPI_Window_GetDatabaseInfo (&targetDbInfo);
        if (err != NoError) {
            executionResultsListProxy (CreateFailedExecutionResult (err, "Failed to get database info"));
            continue;
        }
        err = ACAPI_Database_ChangeCurrentDatabase (&targetDbInfo);
        if (err != NoError) {
            executionResultsListProxy (CreateFailedExecutionResult (err, "Failed to switch to database"));
            continue;
        }
        bool success = action();
        if (success) {
            executionResultsListProxy (CreateSuccessfulExecutionResult ());
        }
        else {
            executionResultsListProxy (CreateFailedExecutionResult (err, "Failed to retrieve elements."));
        }
    }

    err = ACAPI_Database_ChangeCurrentDatabase (&startingDatabase);
    if (err != NoError) {
        return err;
    }
    return NoError;
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
            },
            "databases": {
                 "$ref": "#/Databases"
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
            },
            "executionResultForDatabases": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements",
            "executionResultForDatabases"
        ]
    })";
}

GS::ObjectState GetElementsByTypeCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{   
    GS::ObjectState response;
    const auto& elements = response.AddList<GS::ObjectState> ("elements");
    const auto& executionResultForDatabases = response.AddList<GS::ObjectState> ("executionResultForDatabases");

    GS::Array<GS::ObjectState> databases;
    bool databasesParameterExists = parameters.Get ("databases", databases);
    if (!databasesParameterExists) {
        GetElementsFromCurrentDatabase (parameters, elements);
        executionResultForDatabases (CreateSuccessfulExecutionResult());
    }
    else {
        const GS::Array<API_Guid> databaseIds = databases.Transform<API_Guid> (GetGuidFromDatabaseArrayItem);

        auto action = [&]() -> bool {
            return GetElementsFromCurrentDatabase (parameters, elements);
        };

        GSErrCode err = ExecuteActionForEachDatabase (action, databaseIds, executionResultForDatabases);
        if (err != NoError) {
            return CreateErrorResponse (err, "Failed to retrieve the starting database or to switch back to it after execution.");
        }
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
            },
            "databases": {
                 "$ref": "#/Databases"
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
                        "id": {
                            "type": "string"
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
                                    "title": "DetailWorksheetDetails",
                                    "properties": {
                                        "basePoint": {
                                            "$ref": "#/2DCoordinate",
                                            "description": "Coordinate of the base point"
                                        },
                                        "angle": {
                                            "type": "number",
                                            "description": "The rotation angle (radian) of the marker symbol"
                                        },
                                        "markerId": {
                                            "$ref": "#/ElementId",
                                            "description": "Guid of the marker symbol"
                                        },
                                        "detailName": {
                                            "type": "string",
                                            "description": "Name of the detail/worksheet"
                                        },
                                        "detailIdStr": {
                                            "type": "string",
                                            "description": "Reference ID of the detail/worksheet"
                                        },
                                        "isHorizontalMarker": {
                                            "type": "boolean",
                                            "description": "Marker symbol is always horizontal?"
                                        },
                                        "isWindowOpened": {
                                            "type": "boolean",
                                            "description": "Side (detail/worksheet) window is opened?"
                                        },
                                        "clipPolygon": {
                                            "type": "array",
                                            "description": "The clip polygon of the detail/worksheet",
                                            "items": {
                                                "$ref": "#/2DCoordinate"
                                            }
                                        },
                                        "linkData": {
                                            "type": "object",
                                            "description": "The marker link data",
                                            "properties": {
                                                "referredView": {
                                                    "$ref": "#/ElementId",
                                                    "description": "Guid of the referred view. Only if the marker refers to a view."
                                                },
                                                "referredDrawing": {
                                                    "$ref": "#/ElementId",
                                                    "description": "Guid of the referred drawing. Only if the marker refers to a drawing."
                                                },
                                                "referredPMViewPoint": {
                                                    "$ref": "#/ElementId",
                                                    "description": "Guid of the referred view point. Only if the marker refers to a view point."
                                                }
                                            },
                                            "required": []
                                        }
                                    },
                                    "required": [
                                        "basePoint",
                                        "angle",
                                        "markerId",
                                        "detailName",
                                        "detailIdStr",
                                        "isHorizontalMarker",
                                        "isWindowOpened",
                                        "clipPolygon",
                                        "linkData"
                                    ]
                                },
                                {
                                    "title": "LibPartBasedElementDetails",
                                    "properties": {
                                        "libPart": {
                                            "$ref": "#/LibPartDetails"
                                        },
                                        "ownerElementId": {
                                            "$ref": "#/ElementId"
                                        }
                                    },
                                    "required": [
                                        "libPart"
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
                        "id",
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

static void AddLibPartBasedElementDetails (GS::ObjectState& os, const API_Guid& owner, Int32 libInd)
{
    API_LibPart	lp = {};
    lp.index = libInd;
    ACAPI_LibraryPart_Get (&lp);
    os.Add ("libPart", GS::ObjectState (
        "name", GS::UniString (lp.docu_UName),
        "parentUnID", CreateGuidObjectState (GS::UnID (lp.parentUnID).GetMainGuid ()),
        "ownUnID", CreateGuidObjectState (GS::UnID (lp.ownUnID).GetMainGuid ())));

    if (owner != APINULLGuid) {
        os.Add ("ownerElementId", CreateGuidObjectState (owner));
    }
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

        API_ElementMemo memo = {};
        ACAPI_Element_GetMemo (elem.header.guid, &memo, APIMemoMask_ElemInfoString);

        detailsOfElement.Add ("id", memo.elemInfoString != nullptr ? *memo.elemInfoString : GS::EmptyUniString);

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
                            AddPolygonFromMemoCoords (typeSpecificDetails, "polygonOutline", elem.header.guid);
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

            case API_DoorID:
            case API_WindowID:
                AddLibPartBasedElementDetails (typeSpecificDetails, elem.window.owner, elem.window.openingBase.libInd);
                break;

            case API_LabelID:
                AddLibPartBasedElementDetails (typeSpecificDetails, elem.label.parent, elem.label.labelClass == APILblClass_Symbol ? elem.label.u.symbol.libInd : -1);
                break;

            case API_ObjectID:
            case API_LampID:
                AddLibPartBasedElementDetails (typeSpecificDetails, elem.object.owner, elem.object.libInd);
                break;

            case API_DetailID:
            case API_WorksheetID: {
                typeSpecificDetails.Add ("basePoint", Create2DCoordinateObjectState (elem.detail.pos));
                typeSpecificDetails.Add ("angle", elem.detail.angle);
                typeSpecificDetails.Add ("markerId", CreateGuidObjectState (elem.detail.markId));
                typeSpecificDetails.Add ("detailName", GS::UniString (elem.detail.detailName));
                typeSpecificDetails.Add ("detailIdStr", GS::UniString (elem.detail.detailIdStr));
                typeSpecificDetails.Add ("isHorizontalMarker", elem.detail.horizontalMarker);
                typeSpecificDetails.Add ("isWindowOpened", elem.detail.windOpened);
                AddPolygonFromMemoCoords (typeSpecificDetails, "clipPolygon", elem.header.guid);
                GS::ObjectState linkDataOS;
                switch (elem.detail.linkData.referringLevel) {
                    case API_ReferringLevel::ReferredToView:
                        linkDataOS.Add ("referredView", CreateGuidObjectState (elem.detail.linkData.referredView));
                        break;
                    case API_ReferringLevel::ReferredToDrawing:
                        linkDataOS.Add ("referredDrawing", CreateGuidObjectState (elem.detail.linkData.referredDrawing));
                        break;
                    case API_ReferringLevel::ReferredToViewPoint:
                        linkDataOS.Add ("referredPMViewPoint", CreateGuidObjectState (elem.detail.linkData.referredPMViewPoint));
                        break;
                    default:
                        break;
                }
                typeSpecificDetails.Add ("linkData", linkDataOS);
            } break;

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

GS::Optional<GS::UniString> GetSubelementsOfHierarchicalElementsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "subelements": {
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
            "subelements"
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
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& subelementsOfHierarchicalElements = response.AddList<GS::ObjectState> ("subelements");

    for (const GS::ObjectState& hierarchicalElement : elements) {
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

GetConnectedElementsCommand::GetConnectedElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetConnectedElementsCommand::GetName () const
{
    return "GetConnectedElements";
}

GS::Optional<GS::UniString> GetConnectedElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            },
            "connectedElementType": {
                "$ref": "#/ElementType"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements",
            "connectedElementType"
        ]
    })";
}

GS::Optional<GS::UniString> GetConnectedElementsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "connectedElements": {
                "type": "array",
                "items": {
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
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "connectedElements"
        ]
    })";
}

GS::ObjectState GetConnectedElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    API_ElemTypeID elemType = API_ZombieElemID;
    GS::UniString elementTypeStr;
    if (parameters.Get ("connectedElementType", elementTypeStr)) {
        elemType = GetElementTypeFromNonLocalizedName (elementTypeStr);
    }

    GS::ObjectState response;
    const auto& connectedElementsOfInputElements = response.AddList<GS::ObjectState> ("connectedElements");

    for (const GS::ObjectState& ownerElementOS : elements) {
        const GS::ObjectState* elementId = ownerElementOS.Get ("elementId");
        if (elementId == nullptr) {
            connectedElementsOfInputElements (CreateErrorResponse (APIERR_BADPARS, "elementId of owner element is missing"));
            continue;
        }

        const API_Guid ownerElemGuid = GetGuidFromObjectState (*elementId);

        GS::ObjectState elementsOS;
        const auto& elements = elementsOS.AddList<GS::ObjectState> ("elements");
        GS::Array<API_Guid> connectedElements;
        if (ACAPI_Grouping_GetConnectedElements (ownerElemGuid, elemType, &connectedElements) == NoError) {
            for (const API_Guid& elem : connectedElements) {
                elements (CreateElementIdObjectState (elem));
            }
        }

        connectedElementsOfInputElements (elementsOS);
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
    const auto& filteredElements = response.AddList<GS::ObjectState> ("elements");

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

Get3DBoundingBoxesCommand::Get3DBoundingBoxesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String Get3DBoundingBoxesCommand::GetName () const
{
    return "Get3DBoundingBoxes";
}

GS::Optional<GS::UniString> Get3DBoundingBoxesCommand::GetInputParametersSchema () const
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

GS::Optional<GS::UniString> Get3DBoundingBoxesCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
            "properties": {
            "boundingBoxes3D": {
                "$ref": "#/BoundingBoxes3D"
            }
        },
        "additionalProperties": false,
        "required": [
            "boundingBoxes3D"
        ]
    })";
}

GS::ObjectState Get3DBoundingBoxesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& boundingBoxes3D = response.AddList<GS::ObjectState> ("boundingBoxes3D");

    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            boundingBoxes3D (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        API_Elem_Head elemHead = {};
        elemHead.guid = GetGuidFromObjectState (*elementId);
        API_Box3D box3D = {};
        GSErrCode err = ACAPI_Element_CalcBounds (&elemHead, &box3D);
        if (err != NoError) {
            boundingBoxes3D (CreateErrorResponse (err, "Failed to get the 3D bounding box"));
            continue;
        }

        GS::ObjectState boundingBox3D ("xMin", box3D.xMin,
                                       "xMax", box3D.xMax,
                                       "yMin", box3D.yMin,
                                       "yMax", box3D.yMax,
                                       "zMin", box3D.zMin,
                                       "zMax", box3D.zMax);
        boundingBoxes3D (GS::ObjectState ("boundingBox3D", boundingBox3D));
    }

    return response;
}