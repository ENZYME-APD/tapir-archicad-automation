#include "ElementCommands.hpp"

#include "ElementCreationCommands.hpp"
#include "MigrationHelper.hpp"
#include "GSUnID.hpp"
#include "Plane.hpp"
#include "CoordTypedef.hpp"
#include "ModelEdge.hpp"
#include "ModelMeshBody.hpp"
#include "NativeImage.hpp"
#include "MemoryOChannel32.hpp"
#include "Base64Converter.hpp"
#ifdef ServerMainVers_2800
#include "ACAPI/ZoneBoundaryQuery.hpp"
#include "ACAPI/ZoneLabelingSettings.hpp"
#endif

#include <algorithm>
#include <cmath>

// The pivot polygon of a multi-plane roof lives in the memo's additional polygon
// (APIMemoMask_AdditionalPolygon); the first sub-polygon is the outline. Arcs are
// not carried: a curved pivot edge comes back as its end points (schema says so).
static void AddPivotPolygonFromMemo (const API_Guid& guid, GS::ObjectState& os)
{
    API_ElementMemo memo = {};
    if (ACAPI_Element_GetMemo (guid, &memo, APIMemoMask_AdditionalPolygon) != NoError || memo.additionalPolyCoords == nullptr) {
        ACAPI_DisposeElemMemoHdls (&memo);
        return;
    }
    const GSSize nCoords = BMhGetSize (reinterpret_cast<GSHandle> (memo.additionalPolyCoords)) / sizeof (API_Coord);
    GSIndex last = nCoords - 1;
    if (memo.additionalPolyPends != nullptr && BMhGetSize (reinterpret_cast<GSHandle> (memo.additionalPolyPends)) / sizeof (Int32) > 1) {
        last = (*memo.additionalPolyPends)[1];
    }
    const auto& coords = os.AddList<GS::ObjectState> ("pivotPolygonOutline");
    for (GSIndex i = 1; i <= last && i < nCoords; ++i) {
        coords (Create2DCoordinateObjectState ((*memo.additionalPolyCoords)[i]));
    }
    ACAPI_DisposeElemMemoHdls (&memo);
}

// The baseline of a Stair is a polyline, not a polygon: it lives in the memo's stairBaseLine
// (the very handle CreateStairsCommand::SetTypeSpecificParameters fills in to place one),
// 1-based like the other memo polygons, with no closing vertex. Read with APIMemoMask_All,
// the way CalculateStairBounds and GetSubelementsOfHierarchicalElements already read the
// Stair's memo in this file. The arcs come back with the same 0-based indices as the
// polygon arcs of the other element types.
// On a placed stair the coords are relative to the stair's basePlane.basePoint (a stair
// created at (110, 0) reads its baseline from (0, 0), #1561), so they are moved by origin
// to report them in the same coordinates as CreateStairs takes them.
static void AddStairBaseLineFromMemo (const API_Guid& guid, const API_Coord& origin, GS::ObjectState& os)
{
    const auto& coords = os.AddList<GS::ObjectState> ("baseLinePoints");

    API_ElementMemo memo = {};
    const GS::OnExit guard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
    if (ACAPI_Element_GetMemo (guid, &memo, APIMemoMask_All) != NoError || memo.stairBaseLine.coords == nullptr) {
        return;
    }
    const auto& baseLine = memo.stairBaseLine;

    const GSSize nCoords = BMhGetSize (reinterpret_cast<GSHandle> (baseLine.coords)) / sizeof (API_Coord);
    GSIndex last = nCoords - 1;
    if (baseLine.pends != nullptr && BMhGetSize (reinterpret_cast<GSHandle> (baseLine.pends)) / sizeof (Int32) > 1) {
        last = (*baseLine.pends)[1];
    }
    for (GSIndex i = 1; i <= last && i < nCoords; ++i) {
        const API_Coord& coord = (*baseLine.coords)[i];
        coords (Create2DCoordinateObjectState ({ coord.x + origin.x, coord.y + origin.y }));
    }

    if (baseLine.parcs == nullptr) {
        return;
    }
    const GSSize nArcs = BMhGetSize (reinterpret_cast<GSHandle> (baseLine.parcs)) / sizeof (API_PolyArc);
    if (nArcs == 0) {
        return;
    }
    const auto& arcs = os.AddList<GS::ObjectState> ("baseLineArcs");
    for (GSIndex i = 0; i < nArcs; ++i) {
        arcs (CreatePolyArcObjectState ((*baseLine.parcs)[i]));
    }
}


// Shared "line-family settings" fields present on Line/PolyLine/Arc/Circle/Spline
// (API_LineType/API_PolyLineType/API_ArcType/API_SplineType all share this exact shape).
// NOTE: penWeight is deliberately NOT exposed here (GET or SET) - confirmed live that a raw
// penWeight override does not reliably take visible effect in Archicad; line thickness is
// controlled by which pen (linePenIndex) is assigned, since each pen has its own weight defined
// in the project's pen table. Exposing a non-functional field would be misleading.
static void AddLineFamilySettingsDetails (GS::ObjectState& os, const API_ExtendedPenType& linePen, API_AttributeIndex ltypeInd, bool roomSeparator)
{
    os.Add ("roomSeparator", roomSeparator);
    os.Add ("linePenIndex", linePen.penIndex);
    os.Add ("lineTypeId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_LinetypeID, ltypeInd)));
}

// Returns true if any field was actually set (caller still needs to set its own type's mask bits).
static bool SetLineFamilySettingsFields (const GS::ObjectState& typeSpecificDetails, API_ExtendedPenType& linePen, API_AttributeIndex& ltypeInd, bool& roomSeparator,
                                         bool& penChanged, bool& ltypeChanged, bool& roomSepChanged)
{
    short penIndex = 0;
    if (typeSpecificDetails.Get ("linePenIndex", penIndex)) {
        linePen.penIndex = penIndex;
        linePen.colorOverridePenIndex = 0;
        penChanged = true;
    }
    const GS::ObjectState* lineTypeId = typeSpecificDetails.Get ("lineTypeId");
    if (lineTypeId != nullptr) {
        ltypeInd = GetAttributeIndexFromGuid (API_LinetypeID, GetGuidFromObjectState (*lineTypeId));
        ltypeChanged = true;
    }
    if (typeSpecificDetails.Get ("roomSeparator", roomSeparator)) {
        roomSepChanged = true;
    }
    return penChanged || ltypeChanged || roomSepChanged;
}

struct API_RoomUpdateParams {
    bool keepStampPos;
    bool undoTopTrim;
    bool undoBotTrim;
    bool filler_1[5];

    API_RoomUpdateParams() : keepStampPos(true), undoTopTrim(false), undoBotTrim(false)
    {}
};

typedef enum {
    APIInternal_UpdateRoomsID    = 'UPDR',
    APIInternal_PostCommandIdID  = 'ESPC',
} API_InternalID;

extern "C" {
    GSErrCode ACAPI_Internal (API_InternalID code, void* par1 = nullptr, void* par2 = nullptr, void* par3 = nullptr);
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

static GS::UniString DrawingNumberingTypeToString (API_NumberingTypeValues numberingType)
{
    switch (numberingType) {
        case APINumbering_ByViewId:   return "ByViewId";
        case APINumbering_CustomNum:  return "CustomNumber";
        default:
        case APINumbering_ByLayout:   return "ByLayout";
    }
}

static API_NumberingTypeValues DrawingNumberingTypeFromString (const GS::UniString& str)
{
    if (str == "ByViewId")
        return APINumbering_ByViewId;
    if (str == "CustomNumber")
        return APINumbering_CustomNum;
    return APINumbering_ByLayout;
}

static GS::UniString StructureTypeToString (API_ModelElemStructureType structureType)
{
    switch (structureType) {
        case API_BasicStructure:
            return "Basic";
        case API_CompositeStructure:
            return "Composite";
        case API_ProfileStructure:
            return "Profile";
        default:
            return "Basic";
    }
}

// An object that belongs to another element, such as a zone stamp or a part of a stair or railing.
// The listing commands leave these out unless the IncludeSubElemObjects filter asks for them.
static bool IsSubElemObject (const API_Guid& elemGuid)
{
    API_Element elem = {};
    elem.header.guid = elemGuid;
    if (ACAPI_Element_GetHeader (&elem.header) != NoError || GetElemTypeId (elem.header) != API_ObjectID) {
        return false;
    }
    return ACAPI_Element_Get (&elem) != NoError || elem.object.owner != APINULLGuid;
}

template <typename ListProxyType>
static GSErrCode GetElementsFromCurrentDatabase (const GS::ObjectState& parameters, ListProxyType& elementsListProxy)
{
    API_ElemTypeID elemType = API_ZombieElemID;
    GS::UniString elementTypeStr;
    if (parameters.Get ("elementType", elementTypeStr)) {
        elemType = GetElementTypeFromNonLocalizedName (elementTypeStr);
    }

    bool includeSubElemObjects = false;
    API_ElemFilterFlags filterFlags = APIFilt_None;
    GS::Array<GS::UniString> filters;
    if (parameters.Get ("filters", filters)) {
        for (const GS::UniString& filter : filters) {
            if (filter == "IncludeSubElemObjects") {
                includeSubElemObjects = true;
            } else {
                filterFlags |= ConvertFilterStringToFlag (filter);
            }
        }
    }

    GS::Array<API_Guid> elemList;
    GSErrCode err = ACAPI_Element_GetElemList (elemType, &elemList, filterFlags);
    if (err != NoError) {
        return err;
    }

    // Only a listing of Objects or of every type (GetAllElements) can contain sub-element objects.
    const bool skipSubElemObjects = !includeSubElemObjects && (elemType == API_ObjectID || elemType == API_ZombieElemID);
    for (const API_Guid& elemGuid : elemList) {
        const API_Guid parentGuid = GetParentElemOfSectElem (elemGuid);
        if (skipSubElemObjects && IsSubElemObject (parentGuid)) {
            continue;
        }
        elementsListProxy (CreateElementIdObjectState (parentGuid));
    }
    return NoError;
}

// The elements drawn in a section/elevation/interior elevation database, together with the
// owner element each of them was generated from. Every other listing command converts a
// section element to its owner (GetParentElemOfSectElem above), so this is the only way to
// obtain a raw API_SectElemID guid - which is what CreateAssociativeDimensionsOnSection
// requires for its sectionElementId (#509).
template <typename ListProxyType>
static GSErrCode GetSectionElementsFromCurrentDatabase (ListProxyType& sectionElementsListProxy)
{
    GS::Array<API_Guid> elemList;
    GSErrCode err = ACAPI_Element_GetElemList (API_SectElemID, &elemList);
    if (err != NoError) {
        return err;
    }

    for (const API_Guid& elemGuid : elemList) {
        API_Element element = {};
        element.header.guid = elemGuid;
        if (ACAPI_Element_Get (&element) != NoError) {
            continue;
        }

        GS::ObjectState sectionElement;
        sectionElement.Add ("sectionElementId", CreateGuidObjectState (elemGuid));
        sectionElement.Add ("ownerElementId", CreateGuidObjectState (element.sectElem.parentGuid));

        // The owner lives in the model, not in this database, so its header is not always
        // readable while the section database is the current one - the type is reported only
        // when it resolves, rather than guessed.
        API_Elem_Head ownerHead = {};
        ownerHead.guid = element.sectElem.parentGuid;
        if (ACAPI_Element_GetHeader (&ownerHead) == NoError) {
            sectionElement.Add ("ownerElementType", GetElementTypeNonLocalizedName (GetElemTypeId (ownerHead)));
        }

        sectionElementsListProxy (sectionElement);
    }

    return NoError;
}

GetSectionElementsCommand::GetSectionElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetSectionElementsCommand::GetName () const
{
    return "GetSectionElements";
}

GS::Optional<GS::UniString> GetSectionElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "databases": {
                "$ref": "#/Databases",
                "description": "The section, elevation or interior elevation databases to list the section elements of. If omitted, the current database is used."
            }
        },
        "additionalProperties": false,
        "required": []
    })";
}

GS::Optional<GS::UniString> GetSectionElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "sectionElements": {
                "type": "array",
                "description": "The elements drawn in the given databases, each with the owner element it was generated from.",
                "items": {
                    "type": "object",
                    "properties": {
                        "sectionElementId": {
                            "$ref": "#/ElementId",
                            "description": "The identifier of the section element itself, accepted by CreateAssociativeDimensionsOnSection as sectionElementId."
                        },
                        "ownerElementId": {
                            "$ref": "#/ElementId",
                            "description": "The identifier of the owner element the section element was generated from - this is what every other listing command returns."
                        },
                        "ownerElementType": {
                            "$ref": "#/ElementType",
                            "description": "The type of the owner element. Only present when the owner's header is readable from the section database."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "sectionElementId",
                        "ownerElementId"
                    ]
                }
            },
            "executionResultForDatabases": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "sectionElements"
        ]
    })";
}

GS::ObjectState GetSectionElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::ObjectState response;
    const auto& sectionElements = response.AddList<GS::ObjectState> ("sectionElements");

    GS::Array<GS::ObjectState> databases;
    if (!parameters.Get ("databases", databases) || databases.IsEmpty ()) {
        const GSErrCode err = GetSectionElementsFromCurrentDatabase (sectionElements);
        if (err != NoError) {
            return CreateErrorResponse (err, "Failed to list the section elements of the current database.");
        }
        return response;
    }

    const auto& executionResultForDatabases = response.AddList<GS::ObjectState> ("executionResultForDatabases");
    const GS::Array<API_Guid> databaseIds = databases.Transform<API_Guid> (GetGuidFromDatabaseArrayItem);

    auto action = [&]() -> GSErrCode {
        return GetSectionElementsFromCurrentDatabase (sectionElements);
    };
    auto actionSuccess = [&]() -> void {
        executionResultForDatabases (CreateSuccessfulExecutionResult ());
    };
    auto actionFailure = [&](GSErrCode err, const GS::UniString& errMsg) -> void {
        executionResultForDatabases (CreateFailedExecutionResult (err, errMsg));
    };

    const GSErrCode err = ExecuteActionForEachDatabase (databaseIds, action, actionSuccess, actionFailure);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to retrieve the starting database or to switch back to it after execution.");
    }

    return response;
}

// Walks the current database with ACAPI_DrawOrder_DrawOrderGetNext, starting from an empty
// header (null guid, zombie type) and feeding back the header it returned each time. The walk
// ends at the first call that fails, which is how the function reports that no element is left.
// The visited set stops it should a database ever hand back an element twice.
template <typename ListProxyType>
static GSErrCode GetElementsInDrawOrderFromCurrentDatabase (const GS::ObjectState& parameters, ListProxyType& elementsListProxy)
{
    API_ElemTypeID elemType = API_ZombieElemID;
    GS::UniString elementTypeStr;
    if (parameters.Get ("elementType", elementTypeStr)) {
        elemType = GetElementTypeFromNonLocalizedName (elementTypeStr);
    }

    bool includeSubElemObjects = false;
    API_ElemFilterFlags filterFlags = APIFilt_None;
    GS::Array<GS::UniString> filters;
    if (parameters.Get ("filters", filters)) {
        for (const GS::UniString& filter : filters) {
            if (filter == "IncludeSubElemObjects") {
                includeSubElemObjects = true;
            } else {
                filterFlags |= ConvertFilterStringToFlag (filter);
            }
        }
    }

    GS::HashSet<API_Guid> visitedGuids;
    GS::HashSet<API_Guid> reportedGuids;
    API_Elem_Head elemHead = {};
    while (ACAPI_DrawOrder_DrawOrderGetNext (filterFlags, &elemHead) == NoError) {
        if (elemHead.guid == APINULLGuid || visitedGuids.Contains (elemHead.guid)) {
            break;
        }
        visitedGuids.Add (elemHead.guid);

        const API_Guid elemGuid = GetParentElemOfSectElem (elemHead.guid);
        API_Elem_Head reportedHead = {};
        reportedHead.guid = elemGuid;
        if (ACAPI_Element_GetHeader (&reportedHead) != NoError) {
            reportedHead = elemHead;
        }
        const API_ElemTypeID reportedType = GetElemTypeId (reportedHead);
        if (elemType != API_ZombieElemID && reportedType != elemType) {
            continue;
        }
        if (!includeSubElemObjects && reportedType == API_ObjectID && IsSubElemObject (elemGuid)) {
            continue;
        }
        if (reportedGuids.Contains (elemGuid)) {
            continue;
        }
        reportedGuids.Add (elemGuid);
        elementsListProxy (CreateElementIdObjectState (elemGuid));
    }

    return NoError;
}

GetElementsInDrawOrderCommand::GetElementsInDrawOrderCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetElementsInDrawOrderCommand::GetName () const
{
    return "GetElementsInDrawOrder";
}

GS::Optional<GS::UniString> GetElementsInDrawOrderCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementType": {
                "$ref": "#/ElementType",
                "description": "Optional. Only elements of this type are returned, still in drawing order. If omitted, every element is returned."
            },
            "filters": {
                "type": "array",
                "description": "Optional. The filters an element must pass to be returned, the same as for GetElementsByType.",
                "items": {
                    "$ref": "#/ElementFilter"
                },
                "minItems": 1
            },
            "databases": {
                "$ref": "#/Databases",
                "description": "The databases to list the elements of, each in its own drawing order. If omitted, the current database is used."
            }
        },
        "additionalProperties": false,
        "required": []
    })";
}

GS::Optional<GS::UniString> GetElementsInDrawOrderCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ElementsWithExecutionResultsOrError"
    })";
}

GS::ObjectState GetElementsInDrawOrderCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString elementTypeStr;
    if (parameters.Get ("elementType", elementTypeStr)) {
        if (GetElementTypeFromNonLocalizedName (elementTypeStr) == API_ZombieElemID) {
            return CreateErrorResponse (APIERR_BADPARS,
                GS::UniString::Printf ("Invalid elementType '%T'.", elementTypeStr.ToPrintf ()));
        }
    }

    GS::ObjectState response;
    const auto& elements = response.AddList<GS::ObjectState> ("elements");

    GS::Array<GS::ObjectState> databases;
    if (!parameters.Get ("databases", databases) || databases.IsEmpty ()) {
        const GSErrCode err = GetElementsInDrawOrderFromCurrentDatabase (parameters, elements);
        if (err != NoError) {
            return CreateErrorResponse (err, "Failed to list the elements of the current database in drawing order.");
        }
        return response;
    }

    const auto& executionResultForDatabases = response.AddList<GS::ObjectState> ("executionResultForDatabases");
    const GS::Array<API_Guid> databaseIds = databases.Transform<API_Guid> (GetGuidFromDatabaseArrayItem);

    auto action = [&]() -> GSErrCode {
        return GetElementsInDrawOrderFromCurrentDatabase (parameters, elements);
    };
    auto actionSuccess = [&]() -> void {
        executionResultForDatabases (CreateSuccessfulExecutionResult ());
    };
    auto actionFailure = [&](GSErrCode err, const GS::UniString& errMsg) -> void {
        executionResultForDatabases (CreateFailedExecutionResult (err, errMsg));
    };

    const GSErrCode err = ExecuteActionForEachDatabase (databaseIds, action, actionSuccess, actionFailure);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to retrieve the starting database or to switch back to it after execution.");
    }

    return response;
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

GS::Optional<GS::UniString> GetElementsByTypeCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ElementsWithExecutionResultsOrError"
    })";
}

GS::ObjectState GetElementsByTypeCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString elementTypeStr;
    if (parameters.Get ("elementType", elementTypeStr)) {
        if (GetElementTypeFromNonLocalizedName (elementTypeStr) == API_ZombieElemID) {
            return CreateErrorResponse (APIERR_BADPARS,
                GS::UniString::Printf ("Invalid elementType '%T'.", elementTypeStr.ToPrintf ()));
        }
    }

    GS::ObjectState response;
    const auto& elements = response.AddList<GS::ObjectState> ("elements");

    GS::Array<GS::ObjectState> databases;
    bool databasesParameterExists = parameters.Get ("databases", databases);
    if (!databasesParameterExists || databases.IsEmpty ()) {
        GetElementsFromCurrentDatabase (parameters, elements);
    }
    else {
        const auto& executionResultForDatabases = response.AddList<GS::ObjectState> ("executionResultForDatabases");

        const GS::Array<API_Guid> databaseIds = databases.Transform<API_Guid> (GetGuidFromDatabaseArrayItem);

        auto action = [&]() -> GSErrCode {
            return GetElementsFromCurrentDatabase (parameters, elements);
        };
        auto actionSuccess = [&]() -> void {
            executionResultForDatabases (CreateSuccessfulExecutionResult ());
        };
        auto actionFailure = [&](GSErrCode err, const GS::UniString& errMsg) -> void {
            executionResultForDatabases (CreateFailedExecutionResult (err, errMsg));
        };

        GSErrCode err = ExecuteActionForEachDatabase (databaseIds, action,  actionSuccess, actionFailure);
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

GetElementTypeNamesCommand::GetElementTypeNamesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetElementTypeNamesCommand::GetName () const
{
    return "GetElementTypeNames";
}

GS::Optional<GS::UniString> GetElementTypeNamesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementTypes": {
                "type": "array",
                "description": "The element types to get the localized names of.",
                "items": {
                    "$ref": "#/ElementType"
                },
                "minItems": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "elementTypes"
        ]
    })";
}

GS::Optional<GS::UniString> GetElementTypeNamesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "names": {
                "type": "array",
                "description": "The localized name of each element type, aligned with the input, or an error.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "elementType": {
                                    "$ref": "#/ElementType"
                                },
                                "name": {
                                    "type": "string",
                                    "description": "The name of the element type in the language of the running Archicad (for example \"Wand\" for Wall in German Archicad)."
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "elementType",
                                "name"
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
            "names"
        ]
    })";
}

GS::ObjectState GetElementTypeNamesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::Array<GS::UniString> elementTypes;
    parameters.Get ("elementTypes", elementTypes);

    GS::ObjectState response;
    const auto& names = response.AddList<GS::ObjectState> ("names");

    for (const GS::UniString& elementTypeStr : elementTypes) {
        const API_ElemTypeID typeId = GetElementTypeFromNonLocalizedName (elementTypeStr);
        if (typeId == API_ZombieElemID) {
            names (CreateErrorResponse (APIERR_BADPARS,
                GS::UniString::Printf ("Invalid elementType '%T'.", elementTypeStr.ToPrintf ())));
            continue;
        }

        GS::UniString name;
        const GSErrCode err = ACAPI_Element_GetElemTypeName (API_ElemType (typeId), name);
        if (err != NoError) {
            names (CreateErrorResponse (err,
                GS::UniString::Printf ("Failed to get the name of elementType '%T'.", elementTypeStr.ToPrintf ())));
            continue;
        }

        names (GS::ObjectState ("elementType", elementTypeStr, "name", name));
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
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
            },
            "fields": {
                "type": "array",
                "description": "Optional filter for the fields to return for each element. When omitted, every field is returned. Fields not listed are not computed at all, so listing only what you need skips in particular the floorPlanPolygons extraction, which regenerates each element's 2D drawing primitives and can dominate the execution time of batch reads.",
                "items": {
                    "$ref": "#/ElementDetailsField"
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

GS::Optional<GS::UniString> GetDetailsOfElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "detailsOfElements": {
                "type": "array",
                "description": "One entry per input element, in the same order as the input elements array. An element that cannot be read gets an error entry in its place, so the position of an entry tells which input element it belongs to.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "description": "Details of an element. When the optional fields filter is given in the input, only the requested fields are present; the required list below applies to unfiltered requests.",
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
                                "hotlinkId": {
                                    "$ref": "#/ElementId",
                                    "description": "The hotlink instance this element belongs to. Present only for elements that came in through a placed hotlink; such elements are read-only."
                                },
                                "details": {
                                    "$ref": "#/TypeSpecificDetails"
                                },
                                "floorPlanPolygons": {
                                    "type": "array",
                                    "description": "Cut-fill polygons as drawn on the floor plan (wall joins resolved by ArchiCAD). Only collected for construction element types whose floor plan symbol comes from a real cut through the element - walls, columns, beams and their segments, slabs, roofs, shells, meshes, morphs, curtain walls and their segments/panels/frames, stairs and their risers/treads/structures. Absent for every other type, when the element has no cut fill, or when the floor plan database is not accessible.",
                                    "items": {
                                        "type": "object",
                                        "properties": {
                                            "coordinates": {
                                                "type": "array",
                                                "items": {
                                                    "$ref": "#/Coordinate2D"
                                                }
                                            }
                                        }
                                    }
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "type",
                                "id",
                                "floorIndex",
                                "layerIndex",
                                "drawIndex",
                                "details"
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
            "detailsOfElements"
        ]
    })";
}

// ── Floor plan polygon infrastructure (used by GetDetailsOfElements) ─────────

struct FloorPlanCollectorContext {
    GS::Array<GS::Array<API_Coord>> polys;
    bool inCutFill = false;
};

static thread_local FloorPlanCollectorContext* tl_floorPlanCtx = nullptr;

static GSErrCode CollectCutFillPolygons (const API_PrimElement* primElem,
                                          const void* par1, const void* par2, const void* /*par3*/)
{
    if (tl_floorPlanCtx == nullptr)
        return NoError;

    switch (primElem->header.typeID) {
        case API_PrimCtrl_HatchBorderBegID: {
            const auto* border = static_cast<const API_PrimHatchBorder*> (par1);
            tl_floorPlanCtx->inCutFill = (border != nullptr && border->determination == APIHatch_CutFills);
            break;
        }
        case API_PrimCtrl_HatchBorderEndID:
            tl_floorPlanCtx->inCutFill = false;
            break;
        case API_PrimPolyID: {
            if (!tl_floorPlanCtx->inCutFill)
                break;
            const Int32 nCoords = primElem->poly.nCoords;
            if (nCoords < 3)
                break;
            const auto* coords = static_cast<const API_Coord*> (par1);
            const auto* pends  = static_cast<const Int32*> (par2);
            // par1 is only guaranteed to carry the coordinate array for the primitives this
            // callback is meant to see - never dereference it unchecked, a poly primitive that
            // arrives without one would otherwise take the whole application down with us.
            if (coords == nullptr)
                break;
            Int32 start = 1;
            const Int32 nSubPolys = primElem->poly.nSubPolys;
            for (Int32 sub = 1; sub <= nSubPolys; ++sub) {
                // pends is 1 based and its last entry is nCoords; clamp anyway so a subpoly
                // count that disagrees with the coordinate count cannot read past the array.
                Int32 end = (pends != nullptr) ? pends[sub] : nCoords;
                if (end > nCoords)
                    end = nCoords;
                GS::Array<API_Coord> ring;
                for (Int32 i = start; i <= end; ++i)
                    ring.Push (coords[i]);
                if (ring.GetSize () >= 3)
                    tl_floorPlanCtx->polys.Push (ring);
                start = end + 1;
            }
            break;
        }
        default:
            break;
    }
    return NoError;
}

// Which element types are handed to the shape primitive decomposition below.
//
// GetDetailsOfElements used to call it for every element it was asked about, whatever the
// type - including types it has no branch for at all and types Tapir cannot even name
// (GetElementTypeNonLocalizedName returns "Unknown" for them, and GetElementsByType cannot
// ask for them). Only elements the floor plan actually cuts through have a cut fill to
// collect, so restricting the call to those loses nothing and keeps element types the
// decomposition was never exercised against out of it.
//
// This is the list of construction element types whose floor plan symbol comes from a real
// cut through the element. Deliberately not in it: types drawn by a GDL 2D script (Object,
// Lamp, Window, Door, Skylight) and every 2D, annotation, viewpoint and marker type - a GDL
// script can emit a cut fill, so an object shaped like a column may stop reporting polygons
// here; that is the intended trade against feeding arbitrary elements to the decomposition.
static bool CanHaveFloorPlanCutFill (API_ElemTypeID typeID)
{
    switch (typeID) {
        case API_WallID:
        case API_ColumnID:
        case API_ColumnSegmentID:
        case API_BeamID:
        case API_BeamSegmentID:
        case API_SlabID:
        case API_RoofID:
        case API_ShellID:
        case API_MeshID:
        case API_MorphID:
        case API_CurtainWallID:
        case API_CurtainWallSegmentID:
        case API_CurtainWallPanelID:
        case API_CurtainWallFrameID:
        case API_StairID:
        case API_StairStructureID:
        case API_RiserID:
        case API_TreadID:
            return true;
        default:
            return false;
    }
}

static void AddFloorPlanPolygonsIfAvailable (const API_Guid& guid, GS::ObjectState& os)
{
    FloorPlanCollectorContext ctx;
    tl_floorPlanCtx = &ctx;
    const GS::OnExit ctxGuard ([&] () { tl_floorPlanCtx = nullptr; });

    API_Elem_Head elemHead = {};
    elemHead.guid = guid;

    API_ShapePrimsParams params = {};
    params.dontClip   = true;
    params.allStories = true;
    params.polygon    = nullptr;

#if defined(ServerMainVers_2700)
    const GSErrCode fpErr = ACAPI_DrawingPrimitive_ShapePrimsExt (elemHead, CollectCutFillPolygons, &params);
#else
    const GSErrCode fpErr = ACAPI_Element_ShapePrimsExt (elemHead, CollectCutFillPolygons, &params);
#endif
    if (fpErr != NoError)
        return;
    if (ctx.polys.IsEmpty ())
        return;

    const auto& polygonsOS = os.AddList<GS::ObjectState> ("floorPlanPolygons");
    for (const GS::Array<API_Coord>& poly : ctx.polys) {
        GS::ObjectState polyOS;
        const auto& coordsOS = polyOS.AddList<GS::ObjectState> ("coordinates");
        for (const API_Coord& c : poly)
            coordsOS (Create2DCoordinateObjectState (c));
        polygonsOS (polyOS);
    }
}

static GS::ObjectState CreateColorObjectState (const API_RGBColor& color)
{
    return GS::ObjectState ("red", color.f_red, "green", color.f_green, "blue", color.f_blue);
}

static void AddLibPartBasedElementDetails (GS::ObjectState& os, const Int32 libInd, const API_Guid& owner, API_ElemTypeID ownerType = API_ZombieElemID)
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
        if (ownerType == API_ZombieElemID) {
            API_Elem_Head elemHead = {};
            elemHead.guid = owner;
            ACAPI_Element_GetHeader (&elemHead);
            ownerType = GetElemTypeId (elemHead);
        }
        os.Add ("ownerElementType", GetElementTypeNonLocalizedName (ownerType));
    }
}

// The placed size in metres. xRatio/yRatio hold it directly only with useXYFixSize on; otherwise
// they hold it divided by the library part's own A/B (as documented). Like the other library part
// reads here, a failed lookup leaves the stored values as they are.
static API_Coord GetObjectPlanSize (const API_Element& elem)
{
    API_Coord size = { elem.object.xRatio, elem.object.yRatio };
    if (elem.object.useXYFixSize) {
        return size;
    }
    double libraryPartA = 0.0;
    double libraryPartB = 0.0;
    Int32 addParNum = 0;
    API_AddParType** addPars = nullptr;
    const GSErrCode err = ACAPI_LibraryPart_GetParams (elem.object.libInd, &libraryPartA, &libraryPartB, &addParNum, &addPars);
    if (addPars != nullptr) {
        ACAPI_DisposeAddParHdl (&addPars);
    }
    if (err == NoError) {
        if (libraryPartA > 1e-6) {
            size.x *= libraryPartA;
        }
        if (libraryPartB > 1e-6) {
            size.y *= libraryPartB;
        }
    }
    return size;
}

// Reads every non-reserved field of API_ObjectType (shared verbatim as API_LampType) into
// `typeSpecificDetails`, in the shape the "ObjectDetails" schema definition
// (CommonSchemaDefinitions.json) expects - the same shape CreateObjects/CreateLamps/ModifyObjects/
// ModifyLamps accept back. lightColor/lightIsOn are Lamp-only (elem.object.lightColor/lightIsOn
// exist on the shared struct but are only meaningful for API_LampID). lookId is intentionally
// never exposed - it's an internal 2D-draw "same look" dedup id, not meant to be read or set
// externally. Per the SDK's own remarks, per-story visibility (showRelAbove/showRelBelow) and
// linkToSettings.newCreationMode were "not extended" for Object/Lamp the way they were for Morph -
// still exposed here for schema symmetry, but Archicad may silently no-op them on write.
static void AddObjectLampDetails (GS::ObjectState& typeSpecificDetails, const API_Element& elem, const Stories& stories)
{
    const bool isLamp = GetElemTypeId (elem.header) == API_LampID;
#ifdef ServerMainVers_2600
    auto ownerType = elem.object.ownerType;
#else
    auto ownerType = elem.object.ownerID;
#endif
    AddLibPartBasedElementDetails (typeSpecificDetails, elem.object.libInd, elem.object.owner, GetElemTypeId (ownerType));
    typeSpecificDetails.Add ("origin", Create3DCoordinateObjectState ({elem.object.pos.x, elem.object.pos.y, GetZPos (elem.header.floorInd, elem.object.level, stories)}));

    double zDimension = 0.0;
    API_ElementMemo objectMemo = {};
    const GS::OnExit objectMemoGuard ([&objectMemo] () { ACAPI_DisposeElemMemoHdls (&objectMemo); });
    ACAPI_Element_GetMemo (elem.header.guid, &objectMemo, APIMemoMask_AddPars);
    const GSSize nParams = BMGetHandleSize ((GSHandle) objectMemo.params) / sizeof (API_AddParType);
    for (GSIndex ii = 0; ii < nParams; ++ii) {
        API_AddParType& actParam = (*objectMemo.params)[ii];

        const GS::String name (actParam.name);
        if (name == "ZZYZX") {
            zDimension = actParam.value.real;
            break;
        }
    }
    const API_Coord planSize = GetObjectPlanSize (elem);
    typeSpecificDetails.Add ("dimensions", Create3DCoordinateObjectState ({planSize.x, planSize.y, zDimension}));
    typeSpecificDetails.Add ("angle", elem.object.angle);

    typeSpecificDetails.Add ("pen", (Int32) elem.object.pen);
    if (elem.object.ltypeInd != APIInvalidAttributeIndex) {
        typeSpecificDetails.Add ("lineTypeId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_LinetypeID, elem.object.ltypeInd)));
    }
    if (elem.object.mat != APIInvalidAttributeIndex) {
        typeSpecificDetails.Add ("surfaceId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_MaterialID, elem.object.mat)));
    }
    if (elem.object.sectFill != APIInvalidAttributeIndex) {
        typeSpecificDetails.Add ("sectionFillId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_FilltypeID, elem.object.sectFill)));
    }
    typeSpecificDetails.Add ("sectionFillPen", (Int32) elem.object.sectFillPen);
    typeSpecificDetails.Add ("sectionFillBackgroundPen", (Int32) elem.object.sectBGPen);
    typeSpecificDetails.Add ("sectionContourPen", (Int32) elem.object.sectContPen);
    typeSpecificDetails.Add ("useObjectPens", elem.object.useObjPens);
    typeSpecificDetails.Add ("useObjectLineTypes", elem.object.useObjLtypes);
    typeSpecificDetails.Add ("useObjectMaterials", elem.object.useObjMaterials);
    typeSpecificDetails.Add ("useObjectSectionAttributes", elem.object.useObjSectAttrs);
    typeSpecificDetails.Add ("reflected", elem.object.reflected);
    typeSpecificDetails.Add ("useFixSize", elem.object.useXYFixSize);
    typeSpecificDetails.Add ("fixPoint", (Int32) elem.object.fixPoint);
    typeSpecificDetails.Add ("offset", Create2DCoordinateObjectState (elem.object.offset));
    typeSpecificDetails.Add ("useFixedAngle", elem.object.fixedAngle != 0);
    typeSpecificDetails.Add ("isAutoOnStoryVisibility", elem.object.isAutoOnStoryVisibility);

    if (isLamp) {
        typeSpecificDetails.Add ("lightColor", CreateColorObjectState (elem.object.lightColor));
        typeSpecificDetails.Add ("lightIsOn", elem.object.lightIsOn);
    }

    {
        GS::ObjectState visibilityOS;
        visibilityOS.Add ("showOnHome", elem.object.visibility.showOnHome);
        visibilityOS.Add ("showAllAbove", elem.object.visibility.showAllAbove);
        visibilityOS.Add ("showAllBelow", elem.object.visibility.showAllBelow);
        visibilityOS.Add ("showRelAbove", (Int32) elem.object.visibility.showRelAbove);
        visibilityOS.Add ("showRelBelow", (Int32) elem.object.visibility.showRelBelow);
        typeSpecificDetails.Add ("visibility", visibilityOS);
    }
    {
        GS::ObjectState linkOS;
        linkOS.Add ("homeStoryDifference", (Int32) elem.object.linkToSettings.homeStoryDifference);
        linkOS.Add ("newCreationMode", elem.object.linkToSettings.newCreationMode);
        typeSpecificDetails.Add ("linkToSettings", linkOS);
    }
}

static std::vector<API_Coord> GetCWPanelSurfaceCoords (const API_Guid& cwPanelGuid)
{
    std::vector<PolygonData> polygonData = GetPolygonsFromMemoCoords (cwPanelGuid);
    if (polygonData.empty ()) {
        return {};
    }

    return polygonData[0].coords;
}

static Geometry::Point2d GridMeshVertexToPoint2d (const API_GridMeshVertex& vertex)
{
    return Geometry::Point2d (vertex.surfaceParam.x, vertex.surfaceParam.y);
}

static const API_GridMeshEdge& GetGridMeshEdge (const API_GridMesh& gridMesh, const API_GridEdgeInfo& edgeInfo)
{
    return edgeInfo.mainAxis
        ? gridMesh.meshEdgesMainAxis[edgeInfo.id]
        : gridMesh.meshEdgesSecondaryAxis[edgeInfo.id];
}

static std::pair<API_Coord, API_Coord> GetFrameSurfaceParamCoords (const API_CWFrameType& cwFrame, const API_GridMesh& ownerGridMesh)
{
    const API_Coord& begRel = cwFrame.begRel;
    const API_Coord& endRel = cwFrame.endRel;
    const API_GridMeshPolygon& gridMeshPolygon = ownerGridMesh.meshPolygons[cwFrame.cellID];
    std::vector<API_GridElemID> polygonCoordsIDs;
    const API_GridMeshEdge& edgeX = GetGridMeshEdge(ownerGridMesh, gridMeshPolygon.edges[0]);
    const API_GridMeshEdge& edgeY = GetGridMeshEdge(ownerGridMesh, gridMeshPolygon.edges[1]);
    const Geometry::Point2d vX1 = GridMeshVertexToPoint2d(ownerGridMesh.meshVertices[edgeX.begID]);
    const Geometry::Point2d vX2 = GridMeshVertexToPoint2d(ownerGridMesh.meshVertices[edgeX.endID]);
    const Geometry::Point2d vY1 = GridMeshVertexToPoint2d(ownerGridMesh.meshVertices[edgeY.begID]);
    const Geometry::Point2d vY2 = GridMeshVertexToPoint2d(ownerGridMesh.meshVertices[edgeY.endID]);
    const Geometry::Point2d& cellOrigo = vX1;
    const Geometry::Vector2d vX = vX2 - vX1;
    const Geometry::Vector2d vY = vY2 - vY1;
    return {API_Coord {cellOrigo.x + vX.x * begRel.x + vY.x * begRel.x, cellOrigo.y + vX.y * begRel.y + vY.y * begRel.y},
            API_Coord {cellOrigo.x + vX.x * endRel.x + vY.x * endRel.x, cellOrigo.y + vX.y * endRel.y + vY.y * endRel.y}};
}

static const API_CWFrameType* FindNextFrameOfPanel (std::vector<const API_CWFrameType*>& framePtrs, std::vector<API_Coord3D>& polygonCoords)
{
    for (auto it = framePtrs.begin (); it != framePtrs.end (); ++it) {
        const API_CWFrameType* framePtr = *it;
        if (IsSame3DCoordinate (polygonCoords.back (), framePtr->begC)) {
            polygonCoords.push_back (framePtr->endC);
            framePtrs.erase (it);
            return framePtr;
        } else if (IsSame3DCoordinate (polygonCoords.back (), framePtr->endC)) {
            polygonCoords.push_back (framePtr->begC);
            framePtrs.erase (it);
            return framePtr;
        }
    }
    return nullptr;
}

using CWSegmentGridCellID = std::pair<UInt32, API_GridElemID>;

GS::ObjectState GetDetailsOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::Array<GS::UniString> fields;
    const bool filterFields = parameters.Get ("fields", fields) && !fields.IsEmpty ();
    const auto isFieldRequested = [&] (const char* fieldName) {
        return !filterFields || fields.Contains (GS::UniString (fieldName));
    };

    GS::ObjectState response;
    const auto& detailsOfElements = response.AddList<GS::ObjectState> ("detailsOfElements");

    const Stories stories = GetStories ();

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
            detailsOfElements (CreateErrorResponse (err, "Failed to get the details of element (element guid: \"" + APIGuidToString (elem.header.guid) + "\")"));
            continue;
        }

        GS::ObjectState detailsOfElement;
        const API_ElemTypeID typeID = GetElemTypeId (elem.header);

        if (isFieldRequested ("type")) {
            detailsOfElement.Add ("type", GetElementTypeNonLocalizedName (typeID));
        }
        if (isFieldRequested ("floorIndex")) {
            detailsOfElement.Add ("floorIndex", elem.header.floorInd);
        }
        if (isFieldRequested ("layerIndex")) {
            detailsOfElement.Add ("layerIndex", GetAttributeIndex (elem.header.layer));
        }
        if (isFieldRequested ("drawIndex")) {
            detailsOfElement.Add ("drawIndex", static_cast<short> (elem.header.drwIndex));
        }
        if (isFieldRequested ("hotlinkId") && elem.header.hotlinkGuid != APINULLGuid) {
            detailsOfElement.Add ("hotlinkId", CreateGuidObjectState (elem.header.hotlinkGuid));
        }

        if (isFieldRequested ("id")) {
            API_ElementMemo memo = {};
            const GS::OnExit guard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
            ACAPI_Element_GetMemo (elem.header.guid, &memo, APIMemoMask_ElemInfoString);

            detailsOfElement.Add ("id", memo.elemInfoString != nullptr ? *memo.elemInfoString : GS::EmptyUniString);
        }

        if (!isFieldRequested ("details")) {
            if (isFieldRequested ("floorPlanPolygons") && CanHaveFloorPlanCutFill (typeID)) {
                AddFloorPlanPolygonsIfAvailable (elem.header.guid, detailsOfElement);
            }
            detailsOfElements (detailsOfElement);
            continue;
        }

        GS::ObjectState typeSpecificDetails;

        switch (typeID) {
            case API_WallID:
                switch (elem.wall.type) {
                    case APIWtyp_Normal:
                        typeSpecificDetails.Add ("geometryType", "Straight");
                        typeSpecificDetails.Add ("arcAngle", elem.wall.angle);
                        break;
                    case APIWtyp_Trapez:
                        typeSpecificDetails.Add ("geometryType", "Trapezoid");
                        break;
                    case APIWtyp_Poly:
                        {
                            typeSpecificDetails.Add ("geometryType", "Polygonal");
                            AddPolygonFromMemoCoords (elem.header.guid, typeSpecificDetails, "polygonOutline", "polygonArcs");
                            break;
                        }
                }
                typeSpecificDetails.Add ("structureType", StructureTypeToString (elem.wall.modelElemStructureType));
                typeSpecificDetails.Add ("zCoordinate", GetZPos (elem.header.floorInd, elem.wall.bottomOffset, stories));
                typeSpecificDetails.Add ("begCoordinate", Create2DCoordinateObjectState (elem.wall.begC));
                typeSpecificDetails.Add ("endCoordinate", Create2DCoordinateObjectState (elem.wall.endC));
                typeSpecificDetails.Add ("height", elem.wall.height);
                typeSpecificDetails.Add ("bottomOffset", elem.wall.bottomOffset);
                typeSpecificDetails.Add ("offset", elem.wall.offset);
                typeSpecificDetails.Add ("flipped", elem.wall.flipped);
                if (elem.wall.type == APIWtyp_Poly) {
                    typeSpecificDetails.Add ("begThickness", 0);
                    typeSpecificDetails.Add ("endThickness", 0);
                } else {
                    typeSpecificDetails.Add ("begThickness", elem.wall.thickness);
                    typeSpecificDetails.Add ("endThickness", elem.wall.thickness1);
                }
                if (StructureTypeToString (elem.wall.modelElemStructureType) == "Composite") {
                    typeSpecificDetails.Add ("compositeId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_CompWallID, elem.wall.composite)));
                } else if (StructureTypeToString (elem.wall.modelElemStructureType) == "Basic") {
                    typeSpecificDetails.Add ("buildingMaterialId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_BuildingMaterialID, elem.wall.buildingMaterial)));
                } else if (StructureTypeToString (elem.wall.modelElemStructureType) == "Profile") {
                    typeSpecificDetails.Add ("profileId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_ProfileID, elem.wall.profileAttr)));
                }
                typeSpecificDetails.Add ("referenceLineLocation", WallReferenceLineLocationToString (elem.wall.referenceLineLocation));
                {
                    const char* profileTypeStr = "Normal";
                    if (elem.wall.profileType == APISect_Slanted) {
                        profileTypeStr = "Slanted";
                    } else if (elem.wall.profileType == APISect_Trapez) {
                        profileTypeStr = "Trapez";
                    } else if (elem.wall.profileType == APISect_Poly) {
                        profileTypeStr = "Poly";
                    }
                    typeSpecificDetails.Add ("profileType", profileTypeStr);
                }
                typeSpecificDetails.Add ("slantAlpha", elem.wall.slantAlpha);
                typeSpecificDetails.Add ("slantBeta", elem.wall.slantBeta);
                typeSpecificDetails.Add ("topOffset", elem.wall.topOffset);
                typeSpecificDetails.Add ("relativeTopStory", elem.wall.relativeTopStory);
                typeSpecificDetails.Add ("zoneRel", ZoneRelToString (elem.wall.zoneRel));
                typeSpecificDetails.Add ("visibility", CreateStoryVisibilityObjectState (elem.wall.visibility));
                typeSpecificDetails.Add ("isAutoOnStoryVisibility", elem.wall.isAutoOnStoryVisibility);
                typeSpecificDetails.Add ("referenceMaterial", CreateOverriddenMaterialObjectState (elem.wall.refMat));
                typeSpecificDetails.Add ("oppositeMaterial", CreateOverriddenMaterialObjectState (elem.wall.oppMat));
                typeSpecificDetails.Add ("sideMaterial", CreateOverriddenMaterialObjectState (elem.wall.sidMat));
#ifdef ServerMainVers_2700
                typeSpecificDetails.Add ("cutFillPen", CreateOverriddenPenObjectState (elem.wall.cutFillPen));
                typeSpecificDetails.Add ("cutFillBackgroundPen", CreateOverriddenPenObjectState (elem.wall.cutFillBackgroundPen));
#else
                typeSpecificDetails.Add ("cutFillPen", GS::ObjectState ("overridden", false));
                typeSpecificDetails.Add ("cutFillBackgroundPen", GS::ObjectState ("overridden", false));
#endif
                typeSpecificDetails.Add ("contourPen", (Int32) elem.wall.contPen);
                typeSpecificDetails.Add ("contourPen3D", (Int32) elem.wall.contPen3D);
                if (elem.wall.contLtype != APIInvalidAttributeIndex) {
                    typeSpecificDetails.Add ("contourLineTypeId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_LinetypeID, elem.wall.contLtype)));
                }
                if (elem.wall.belowViewLineType != APIInvalidAttributeIndex) {
                    typeSpecificDetails.Add ("belowViewLineTypeId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_LinetypeID, elem.wall.belowViewLineType)));
                }
                typeSpecificDetails.Add ("aboveViewLinePen", (Int32) elem.wall.aboveViewLinePen);
                if (elem.wall.aboveViewLineType != APIInvalidAttributeIndex) {
                    typeSpecificDetails.Add ("aboveViewLineTypeId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_LinetypeID, elem.wall.aboveViewLineType)));
                }
                typeSpecificDetails.Add ("displayOption", ElemDisplayOptionToString (elem.wall.displayOption));
                typeSpecificDetails.Add ("viewDepthLimitation", ViewDepthLimitationToString (elem.wall.viewDepthLimitation));
                typeSpecificDetails.Add ("connectionPriority", (Int32) elem.wall.wallConnPriority);
                typeSpecificDetails.Add ("useCompositePriority", elem.wall.useCompositePriority);
                typeSpecificDetails.Add ("junctionSequence", elem.wall.sequence);
                {
                    GS::ObjectState linkOS;
                    linkOS.Add ("homeStoryDifference", (Int32) elem.wall.linkToSettings.homeStoryDifference);
                    linkOS.Add ("newCreationMode", elem.wall.linkToSettings.newCreationMode);
                    typeSpecificDetails.Add ("linkToSettings", linkOS);
                }
                typeSpecificDetails.Add ("inheritEndSurface", elem.wall.inheritEndSurface);
                typeSpecificDetails.Add ("alignTexture", elem.wall.alignTexture);
                typeSpecificDetails.Add ("offsetFromOutside", elem.wall.offsetFromOutside);
                typeSpecificDetails.Add ("polyCanChange", elem.wall.polyCanChange);
                typeSpecificDetails.Add ("materialsChained", elem.wall.materialsChained);
                typeSpecificDetails.Add ("logHeight", elem.wall.logHeight);
                typeSpecificDetails.Add ("hasWindow", elem.wall.hasWindow);
                typeSpecificDetails.Add ("hasDoor", elem.wall.hasDoor);
                break;

            case API_BeamID: {
                typeSpecificDetails.Add ("zCoordinate", GetZPos (elem.header.floorInd, elem.beam.level, stories));
                typeSpecificDetails.Add ("begCoordinate", Create2DCoordinateObjectState (elem.beam.begC));
                typeSpecificDetails.Add ("endCoordinate", Create2DCoordinateObjectState (elem.beam.endC));
                typeSpecificDetails.Add ("level", elem.beam.level);
                typeSpecificDetails.Add ("offset", elem.beam.offset);
                typeSpecificDetails.Add ("slantAngle", elem.beam.slantAngle);
                typeSpecificDetails.Add ("arcAngle", elem.beam.curveAngle);
                typeSpecificDetails.Add ("verticalCurveHeight", elem.beam.verticalCurveHeight);
                const char* beamShapeStr = "Straight";
                if (elem.beam.beamShape == API_HorizontallyCurvedBeam) {
                    beamShapeStr = "HorizontallyCurved";
                } else if (elem.beam.beamShape == API_VerticallyCurvedBeam) {
                    beamShapeStr = "VerticallyCurved";
                }
                typeSpecificDetails.Add ("beamShape", beamShapeStr);
                typeSpecificDetails.Add ("isSlanted", elem.beam.isSlanted);
                typeSpecificDetails.Add ("isFlipped", elem.beam.isFlipped);
                typeSpecificDetails.Add ("profileAngle", elem.beam.profileAngle);
                typeSpecificDetails.Add ("anchorPoint", AnchorIdToString (static_cast<API_AnchorID> (elem.beam.anchorPoint)));
#ifdef ServerMainVers_2700
                typeSpecificDetails.Add ("cutFillPen", CreateOverriddenPenObjectState (elem.beam.cutFillPen));
                typeSpecificDetails.Add ("cutFillBackgroundPen", CreateOverriddenPenObjectState (elem.beam.cutFillBackgroundPen));
#else
                typeSpecificDetails.Add ("cutFillPen", GS::ObjectState ("overridden", false));
                typeSpecificDetails.Add ("cutFillBackgroundPen", GS::ObjectState ("overridden", false));
#endif
                typeSpecificDetails.Add ("coverFill", CreateCoverFillObjectState (
                    elem.beam.useCoverFill, elem.beam.useCoverFillFromSurface, elem.beam.coverFillOrientationComesFrom3D,
                    elem.beam.coverFillType, elem.beam.coverFillForegroundPen, elem.beam.coverFillBackgroundPen,
                    elem.beam.coverFillTransformationType, elem.beam.coverFillTransformation));
                AddBeamHolesFromMemo (elem.header.guid, typeSpecificDetails, "holes");
                AddBeamSectionFromMemo (elem.header.guid, typeSpecificDetails);
            } break;

            case API_BeamSegmentID: {
                const API_AssemblySegmentData& segment = elem.beamSegment.assemblySegmentData;
                if (elem.beamSegment.owner != APINULLGuid) {
                    typeSpecificDetails.Add ("ownerElementId", CreateGuidObjectState (elem.beamSegment.owner));
                }
                typeSpecificDetails.Add ("structureType", StructureTypeToString (segment.modelElemStructureType));
                typeSpecificDetails.Add ("width", segment.nominalWidth);
                typeSpecificDetails.Add ("height", segment.nominalHeight);
                typeSpecificDetails.Add ("isWidthAndHeightLinked", segment.isWidthAndHeightLinked);
                typeSpecificDetails.Add ("circleBased", segment.circleBased);
                if (segment.modelElemStructureType == API_BasicStructure) {
                    typeSpecificDetails.Add ("buildingMaterialId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_BuildingMaterialID, segment.buildingMaterial)));
                } else if (segment.modelElemStructureType == API_ProfileStructure) {
                    typeSpecificDetails.Add ("profileId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_ProfileID, segment.profileAttr)));
                }
            } break;

            case API_SlabID:
                typeSpecificDetails.Add ("structureType", StructureTypeToString (elem.slab.modelElemStructureType));
                typeSpecificDetails.Add ("thickness", elem.slab.thickness);
                typeSpecificDetails.Add ("level", elem.slab.level);
                typeSpecificDetails.Add ("offsetFromTop", elem.slab.offsetFromTop);
                typeSpecificDetails.Add ("zCoordinate", GetZPos (elem.header.floorInd, elem.slab.level, stories));
                typeSpecificDetails.Add ("referencePlaneLocation", SlabReferencePlaneLocationToString (elem.slab.referencePlaneLocation));
                if (StructureTypeToString (elem.slab.modelElemStructureType) == "Composite") {
                    typeSpecificDetails.Add ("compositeId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_CompWallID, elem.slab.composite)));
                } else {
                    typeSpecificDetails.Add ("buildingMaterialId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_BuildingMaterialID, elem.slab.buildingMaterial)));
                }
                typeSpecificDetails.Add ("topMaterial", CreateOverriddenMaterialObjectState (elem.slab.topMat));
                typeSpecificDetails.Add ("sideMaterial", CreateOverriddenMaterialObjectState (elem.slab.sideMat));
                typeSpecificDetails.Add ("bottomMaterial", CreateOverriddenMaterialObjectState (elem.slab.botMat));
#ifdef ServerMainVers_2700
                typeSpecificDetails.Add ("cutFillPen", CreateOverriddenPenObjectState (elem.slab.cutFillPen));
                typeSpecificDetails.Add ("cutFillBackgroundPen", CreateOverriddenPenObjectState (elem.slab.cutFillBackgroundPen));
#else
                typeSpecificDetails.Add ("cutFillPen", GS::ObjectState ("overridden", false));
                typeSpecificDetails.Add ("cutFillBackgroundPen", GS::ObjectState ("overridden", false));
#endif
                {
                    GS::ObjectState floorFillOs;
                    floorFillOs.Add ("use", elem.slab.useFloorFill);
                    floorFillOs.Add ("foregroundPen", elem.slab.floorFillPen);
                    floorFillOs.Add ("backgroundPen", elem.slab.floorFillBGPen);
                    floorFillOs.Add ("fillId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_FilltypeID, elem.slab.floorFillInd)));
                    floorFillOs.Add ("use3DHatching", elem.slab.use3DHatching);
                    floorFillOs.Add ("orientation", CreateHatchOrientationObjectState (elem.slab.hatchOrientation));
                    typeSpecificDetails.Add ("floorFill", floorFillOs);
                }
                AddPolygonWithHolesFromMemoCoords (elem.header.guid, typeSpecificDetails, "polygonOutline", "polygonArcs", "holes", "polygonOutline", "polygonArcs");
                break;

            case API_ZoneID:
                typeSpecificDetails.Add ("name", GS::UniString (elem.zone.roomName));
                typeSpecificDetails.Add ("numberStr", GS::UniString (elem.zone.roomNoStr));
                typeSpecificDetails.Add ("categoryAttributeId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_ZoneCatID, elem.zone.catInd)));
                typeSpecificDetails.Add ("stampPosition", Create2DCoordinateObjectState (elem.zone.pos));
                typeSpecificDetails.Add ("stampAngle", elem.zone.stampAngle);
                typeSpecificDetails.Add ("fixedStampAngle", elem.zone.fixedAngle);
                typeSpecificDetails.Add ("isManual", elem.zone.manual);
                if (!elem.zone.manual) {
                    // The point an automatic zone was placed from (CreateZones geometry.referencePosition);
                    // the stamp is often moved away from it, so it is the only way to recreate the zone as it was.
                    typeSpecificDetails.Add ("referencePosition", Create2DCoordinateObjectState (elem.zone.refPos));
                }
                typeSpecificDetails.Add ("zCoordinate", GetZPos (elem.header.floorInd, elem.zone.roomBaseLev, stories));
                AddPolygonWithHolesFromMemoCoords (elem.header.guid, typeSpecificDetails, "polygonOutline", "polygonArcs", "holes", "polygonOutline", "polygonArcs");
                break;

            case API_ColumnID:
                typeSpecificDetails.Add ("origin", Create2DCoordinateObjectState (elem.column.origoPos));
                typeSpecificDetails.Add ("coreAnchor", AnchorIdToString (static_cast<API_AnchorID> (elem.column.coreAnchor)));
                typeSpecificDetails.Add ("zCoordinate", GetZPos (elem.header.floorInd, elem.column.bottomOffset, stories));
                typeSpecificDetails.Add ("height", elem.column.height);
                typeSpecificDetails.Add ("bottomOffset", elem.column.bottomOffset);
                typeSpecificDetails.Add ("axisRotationAngle", elem.column.axisRotationAngle);
                typeSpecificDetails.Add ("isSlanted", elem.column.isSlanted);
                typeSpecificDetails.Add ("slantAngle", elem.column.slantAngle);
                typeSpecificDetails.Add ("slantDirectionAngle", elem.column.slantDirectionAngle);
                typeSpecificDetails.Add ("isFlipped", elem.column.isFlipped);
                typeSpecificDetails.Add ("wrapping", elem.column.wrapping);
                typeSpecificDetails.Add ("topOffset", elem.column.topOffset);
                typeSpecificDetails.Add ("relativeTopStory", elem.column.relativeTopStory);
#ifdef ServerMainVers_2700
                typeSpecificDetails.Add ("cutFillPen", CreateOverriddenPenObjectState (elem.column.cutFillPen));
                typeSpecificDetails.Add ("cutFillBackgroundPen", CreateOverriddenPenObjectState (elem.column.cutFillBackgroundPen));
#else
                typeSpecificDetails.Add ("cutFillPen", GS::ObjectState ("overridden", false));
                typeSpecificDetails.Add ("cutFillBackgroundPen", GS::ObjectState ("overridden", false));
#endif
                typeSpecificDetails.Add ("coverFill", CreateCoverFillObjectState (
                    elem.column.useCoverFill, elem.column.useCoverFillFromSurface, elem.column.coverFillOrientationComesFrom3D,
                    elem.column.coverFillType, elem.column.coverFillForegroundPen, elem.column.coverFillBackgroundPen,
                    elem.column.coverFillTransformationType, elem.column.coverFillTransformation));
                AddColumnSectionFromMemo (elem.header.guid, typeSpecificDetails);
                break;

            case API_ColumnSegmentID: {
                const API_AssemblySegmentData& segment = elem.columnSegment.assemblySegmentData;
                if (elem.columnSegment.owner != APINULLGuid) {
                    typeSpecificDetails.Add ("ownerElementId", CreateGuidObjectState (elem.columnSegment.owner));
                }
                typeSpecificDetails.Add ("structureType", StructureTypeToString (segment.modelElemStructureType));
                typeSpecificDetails.Add ("width", segment.nominalWidth);
                typeSpecificDetails.Add ("depth", segment.nominalHeight);
                typeSpecificDetails.Add ("isWidthAndHeightLinked", segment.isWidthAndHeightLinked);
                typeSpecificDetails.Add ("circleBased", segment.circleBased);
                if (segment.modelElemStructureType == API_BasicStructure) {
                    typeSpecificDetails.Add ("buildingMaterialId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_BuildingMaterialID, segment.buildingMaterial)));
                } else if (segment.modelElemStructureType == API_ProfileStructure) {
                    typeSpecificDetails.Add ("profileId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_ProfileID, segment.profileAttr)));
                }
            } break;

            case API_DoorID:
            case API_WindowID:
                AddLibPartBasedElementDetails (typeSpecificDetails, elem.window.openingBase.libInd, elem.window.owner);
                typeSpecificDetails.Add ("width", elem.window.openingBase.width);
                typeSpecificDetails.Add ("height", elem.window.openingBase.height);
                typeSpecificDetails.Add ("sillHeight", elem.window.lower);
                typeSpecificDetails.Add ("centerOffset", elem.window.objLoc);
                typeSpecificDetails.Add ("reflected", elem.window.openingBase.reflected);
                typeSpecificDetails.Add ("refSide", elem.window.openingBase.refSide);
                typeSpecificDetails.Add ("oSide", elem.window.openingBase.oSide);
                // The same two members ModifyWindows/ModifyDoors write (#581), so a script can read
                // its own write back (#895).
                typeSpecificDetails.Add ("reveal", elem.window.reveal);
                typeSpecificDetails.Add ("revealDepthOffset", elem.window.revealDepthOffset);
                break;

            case API_LabelID: {
                AddLibPartBasedElementDetails (typeSpecificDetails, ((elem.label.labelClass == APILblClass_Symbol) ? elem.label.u.symbol.libInd : -1), elem.label.parent, GetElemTypeId (elem.label.parentType));
                typeSpecificDetails.Add ("begCoordinate", Create2DCoordinateObjectState (elem.label.begC));
                typeSpecificDetails.Add ("midCoordinate", Create2DCoordinateObjectState (elem.label.midC));
                typeSpecificDetails.Add ("endCoordinate", Create2DCoordinateObjectState (elem.label.endC));
                typeSpecificDetails.Add ("hasLeaderLine", elem.label.hasLeaderLine);

                typeSpecificDetails.Add ("labelClass", elem.label.labelClass == APILblClass_Symbol ? "Symbol" : "Text");

                GS::ObjectState leaderLineOS;
                TextLabelDetails::AddLabelLeaderLineDetails (leaderLineOS, elem.label);
                typeSpecificDetails.Add ("leaderLine", leaderLineOS);

                if (elem.label.labelClass == APILblClass_Text) {
                    GS::ObjectState styleOS;
                    TextLabelDetails::AddTextStyleDetails (styleOS, elem.label.u.text, true);
                    typeSpecificDetails.Add ("style", styleOS);
                    if (TextLabelDetails::AddTextContent (typeSpecificDetails, elem.header.guid) != NoError) {
                        // The fields are required by the schema; an unreadable memo reads as empty content.
                        typeSpecificDetails.Add ("text", GS::EmptyUniString);
                        typeSpecificDetails.Add ("paragraphCount", 0);
                    }
                } else {
                    GS::ObjectState symbolStyleOS;
                    TextLabelDetails::AddLabelSymbolStyleDetails (symbolStyleOS, elem.label);
                    typeSpecificDetails.Add ("symbolStyle", symbolStyleOS);
                }
                break;
            }

            case API_TextID: {
                // The flat fields are the ones SetDetailsOfElements takes back (position, angle, height,
                // justification) and the Grasshopper TextDetails type reads; "style" carries the full
                // style state and AddTextContent adds "text", "paragraphCount" and the styled "runs".
                typeSpecificDetails.Add ("position", Create2DCoordinateObjectState (elem.text.loc));
                typeSpecificDetails.Add ("angle", elem.text.angle);
                typeSpecificDetails.Add ("height", elem.text.size);
                typeSpecificDetails.Add ("pen", (Int32) elem.text.pen);
                typeSpecificDetails.Add ("justification", JustificationToString (static_cast<API_JustID> (elem.text.just)));
                typeSpecificDetails.Add ("zCoordinate", GetZPos (elem.header.floorInd, 0, stories));
                GS::ObjectState styleOS;
                TextLabelDetails::AddTextStyleDetails (styleOS, elem.text, true);
                typeSpecificDetails.Add ("style", styleOS);
                if (TextLabelDetails::AddTextContent (typeSpecificDetails, elem.header.guid) != NoError) {
                    // The fields are required by the schema; an unreadable memo reads as empty content.
                    typeSpecificDetails.Add ("text", GS::EmptyUniString);
                    typeSpecificDetails.Add ("paragraphCount", 0);
                }
                break;
            }

            case API_LevelDimensionID: {
                // "level" is the value the dimension shows: the parent's height at "position"
                // for an associative one, the floor level or the static value otherwise.
                typeSpecificDetails.Add ("position", Create2DCoordinateObjectState (elem.levelDimension.loc));
                typeSpecificDetails.Add ("level", elem.levelDimension.level);
                typeSpecificDetails.Add ("staticLevel", elem.levelDimension.staticLevel);
                const API_ElemTypeID parentTypeId = GetElemTypeId (elem.levelDimension.parentType);
                if (parentTypeId != API_ZombieElemID && elem.levelDimension.parentGuid != APINULLGuid) {
                    typeSpecificDetails.Add ("ownerElementId", CreateGuidObjectState (elem.levelDimension.parentGuid));
                    typeSpecificDetails.Add ("ownerElementType", GetElementTypeNonLocalizedName (parentTypeId));
                }
                typeSpecificDetails.Add ("dimForm", (Int32) elem.levelDimension.dimForm);
                typeSpecificDetails.Add ("needPlus", elem.levelDimension.needPlus);
                typeSpecificDetails.Add ("pen", (Int32) elem.levelDimension.pen);
                typeSpecificDetails.Add ("textHeight", elem.levelDimension.note1.noteSize);
                typeSpecificDetails.Add ("textPen", (Int32) elem.levelDimension.note1.notePen);
                typeSpecificDetails.Add ("markerSize", elem.levelDimension.markerSize);
                typeSpecificDetails.Add ("angle", elem.levelDimension.angle);
                typeSpecificDetails.Add ("origin", DimensionOriginToString (elem.levelDimension.origin));
                break;
            }

            case API_ObjectID:
            case API_LampID:
                AddObjectLampDetails (typeSpecificDetails, elem, stories);
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
                AddPolygonFromMemoCoords (elem.header.guid, typeSpecificDetails, "clipPolygon");
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

            case API_PolyLineID: {
                AddPolygonFromMemoCoords (elem.header.guid, typeSpecificDetails, "coordinates", "arcs");
                AddLineFamilySettingsDetails (typeSpecificDetails, elem.polyLine.linePen, elem.polyLine.ltypeInd, elem.polyLine.roomSeparator);
                typeSpecificDetails.Add ("zCoordinate", GetZPos (elem.header.floorInd, 0, stories));
            } break;

            case API_HatchID: {
                AddPolygonWithHolesFromMemoCoords (elem.header.guid, typeSpecificDetails, "coordinates", "arcs", "holes", "polygonOutline", "polygonArcs");
                typeSpecificDetails.Add ("contourPenIndex", elem.hatch.contPen.penIndex);
                typeSpecificDetails.Add ("fillPenIndex", elem.hatch.fillPen.penIndex);
                typeSpecificDetails.Add ("fillBackgroundPenIndex", elem.hatch.fillBGPen);
                // The RGB colors are only reported when they override the pens, so their absence means the pens are in effect.
                if ((elem.hatch.hatchFlags & APIHatch_HasFgRGBColor) != 0) {
                    typeSpecificDetails.Add ("foregroundColor", CreateColorObjectState (elem.hatch.foregroundRGB));
                }
                if ((elem.hatch.hatchFlags & APIHatch_HasBkgRGBColor) != 0) {
                    typeSpecificDetails.Add ("backgroundColor", CreateColorObjectState (elem.hatch.backgroundRGB));
                }
                typeSpecificDetails.Add ("fillId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_FilltypeID, elem.hatch.fillInd)));
                typeSpecificDetails.Add ("buildingMaterialId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_BuildingMaterialID, elem.hatch.buildingMaterial)));
                typeSpecificDetails.Add ("roomSpecial", (Int32) elem.hatch.roomSpecial);
                typeSpecificDetails.Add ("showArea", elem.hatch.showArea);
                typeSpecificDetails.Add ("zCoordinate", GetZPos (elem.header.floorInd, 0, stories));
            } break;

            case API_LineID: {
                typeSpecificDetails.Add ("begCoordinate", Create2DCoordinateObjectState (elem.line.begC));
                typeSpecificDetails.Add ("endCoordinate", Create2DCoordinateObjectState (elem.line.endC));
                AddLineFamilySettingsDetails (typeSpecificDetails, elem.line.linePen, elem.line.ltypeInd, elem.line.roomSeparator);
                typeSpecificDetails.Add ("zCoordinate", GetZPos (elem.header.floorInd, 0, stories));
            } break;

            case API_ArcID:
            case API_CircleID: {
                typeSpecificDetails.Add ("origin", Create2DCoordinateObjectState (elem.arc.origC));
                typeSpecificDetails.Add ("radius", elem.arc.r);
                typeSpecificDetails.Add ("angle", elem.arc.angle);
                typeSpecificDetails.Add ("ratio", elem.arc.ratio);
                if (typeID == API_ArcID) {
                    typeSpecificDetails.Add ("begAngle", elem.arc.begAng);
                    typeSpecificDetails.Add ("endAngle", elem.arc.endAng);
                }
                typeSpecificDetails.Add ("reflected", elem.arc.reflected);
                AddLineFamilySettingsDetails (typeSpecificDetails, elem.arc.linePen, elem.arc.ltypeInd, elem.arc.roomSeparator);
                typeSpecificDetails.Add ("zCoordinate", GetZPos (elem.header.floorInd, 0, stories));
            } break;

            case API_HotspotID: {
                typeSpecificDetails.Add ("position", Create2DCoordinateObjectState (elem.hotspot.pos));
                typeSpecificDetails.Add ("height", elem.hotspot.height);
                typeSpecificDetails.Add ("penIndex", elem.hotspot.pen);
            } break;

            case API_SplineID: {
                // Spline has no poly/nCoords summary field (unlike Line/PolyLine/Hatch) - the
                // point count is implicit in the coords memo handle's byte size. Index 0 is a
                // real point here (not a dummy sentinel), and no closing duplicate is stored even
                // when closed=true (per the ACAPI_SplineType reference docs).
                API_ElementMemo splineMemo = {};
                if (ACAPI_Element_GetMemo (elem.header.guid, &splineMemo, APIMemoMask_Polygon) == NoError && splineMemo.coords != nullptr) {
                    const Int32 nPoints = BMGetHandleSize (reinterpret_cast<GSHandle> (splineMemo.coords)) / sizeof (API_Coord);
                    const auto& coords = typeSpecificDetails.AddList<GS::ObjectState> ("coordinates");
                    for (Int32 i = 0; i < nPoints; ++i) {
                        coords (Create2DCoordinateObjectState ((*splineMemo.coords)[i]));
                    }
                }
                ACAPI_DisposeElemMemoHdls (&splineMemo);
                typeSpecificDetails.Add ("closed", elem.spline.closed);
                AddLineFamilySettingsDetails (typeSpecificDetails, elem.spline.linePen, elem.spline.ltypeInd, elem.spline.roomSeparator);
                typeSpecificDetails.Add ("zCoordinate", GetZPos (elem.header.floorInd, 0, stories));
            } break;

            case API_CurtainWallID: {
                typeSpecificDetails.Add ("height", elem.curtainWall.height);
                double actualHeight = 0.0;
                if (ACAPI_CurtainWall_GetCWHeight (&elem.header.guid, &actualHeight) == NoError) {
                    typeSpecificDetails.Add ("actualHeight", actualHeight);
                }
                typeSpecificDetails.Add ("angle", elem.curtainWall.angle);
                typeSpecificDetails.Add ("flipped", elem.curtainWall.flipped);
            } break;

            case API_CurtainWallSegmentID: {
                typeSpecificDetails.Add ("begCoordinate", Create3DCoordinateObjectState (elem.cwSegment.begC));
                typeSpecificDetails.Add ("endCoordinate", Create3DCoordinateObjectState (elem.cwSegment.endC));
                typeSpecificDetails.Add ("extrusionVector", Create3DCoordinateObjectState (elem.cwSegment.extrusion));
                typeSpecificDetails.Add ("gridOrigin", Create3DCoordinateObjectState (elem.cwSegment.gridOrigin));
                typeSpecificDetails.Add ("gridAngle", elem.cwSegment.gridAngle);
                if (elem.cwSegment.segmentType == API_CWSegmentTypeID::APICWSeT_Arc) {
                    typeSpecificDetails.Add ("arcOrigin", Create3DCoordinateObjectState (elem.cwSegment.arcOrigin));
                    typeSpecificDetails.Add ("isNegativeArc", elem.cwSegment.negArc);
                }
            } break;

            case API_CurtainWallPanelID: {
                API_Element ownerCW = {};
                ownerCW.header.guid = elem.cwPanel.owner;
                ACAPI_Element_Get(&ownerCW);

                API_ElementMemo cwMemo = {};
                const GS::OnExit cwMemoGuard ([&cwMemo] () { ACAPI_DisposeElemMemoHdls (&cwMemo); });
                ACAPI_Element_GetMemo(ownerCW.header.guid, &cwMemo, APIMemoMask_CWallSegments | APIMemoMask_CWallFrames);
                const API_Guid ownerCWSegment = cwMemo.cWallSegments[elem.cwPanel.segmentID].head.guid;

                API_ElementMemo cwSegmentMemo = {};
                const GS::OnExit cwSegmentMemoGuard ([&cwSegmentMemo] () { ACAPI_DisposeElemMemoHdls (&cwSegmentMemo); });
                ACAPI_Element_GetMemo(ownerCWSegment, &cwSegmentMemo, APIMemoMask_CWSegGridMesh);
                const API_GridMesh& ownerGridMesh = *cwSegmentMemo.cWSegGridMesh;

                API_ElementMemo cwPanelMemo = {};
                const GS::OnExit cwPanelMemoGuard ([&cwPanelMemo] () { ACAPI_DisposeElemMemoHdls (&cwPanelMemo); });
                ACAPI_Element_GetMemo(elem.header.guid, &cwPanelMemo, APIMemoMask_CWallPanels);
                const GS::HashTable<API_Guid, GS::Array<API_GridElemID>>& cWallPanelGridIDTable = *cwPanelMemo.cWallPanelGridIDTable;

                const GS::Array<API_GridElemID>& gridIDs = cWallPanelGridIDTable[elem.header.guid];
                GS::Array<API_GridElemID> gridMeshPolygonIDs = gridIDs;
                for (const API_GridElemID& polygonID : gridIDs) {
                    const API_GridMeshPolygon& gridMeshPolygon = ownerGridMesh.meshPolygons[polygonID];
                    gridMeshPolygonIDs.Append (gridMeshPolygon.neighbourIDs[API_GridMeshDirection::API_GridMeshRight]);
                    gridMeshPolygonIDs.Append (gridMeshPolygon.neighbourIDs[API_GridMeshDirection::API_GridMeshUpper]);
                }

                const std::vector<API_Coord> panelSurfaceCoords = GetCWPanelSurfaceCoords (elem.header.guid);

                std::vector<const API_CWFrameType*> framePtrs;
                {
                    std::vector<const API_CWFrameType*> cornerFramesOfNextSegment;
                    for (UIndex i = 0; i < ownerCW.curtainWall.nFrames; ++i) {
                        const API_CWFrameType& frame = cwMemo.cWallFrames[i];
                        if (frame.segmentID == elem.cwPanel.segmentID) {
                            if (gridMeshPolygonIDs.Contains (frame.cellID)) {
                                for (GSIndex i = 0; i < panelSurfaceCoords.size () - 1; ++i) {
                                    const API_Coord& c1 = panelSurfaceCoords[i];
                                    const API_Coord& c2 = panelSurfaceCoords[i + 1];
                                    auto frameSurfaceCoords = GetFrameSurfaceParamCoords (frame, ownerGridMesh);

                                    if ((IsSame2DCoordinate (frameSurfaceCoords.first, c1) && IsSame2DCoordinate (frameSurfaceCoords.second, c2)) ||
                                        (IsSame2DCoordinate (frameSurfaceCoords.first, c2) && IsSame2DCoordinate (frameSurfaceCoords.second, c1))) {
                                        framePtrs.push_back (&frame);
                                        break;
                                    }
                                }
                                if (framePtrs.size () >= elem.cwPanel.edgesNum) {
                                    break;
                                }
                            }
                        } else if (frame.classID == APICWFrameClass_Corner && frame.segmentID == elem.cwPanel.segmentID + 1) {
                            cornerFramesOfNextSegment.push_back (&frame);
                        }
                    }
                    if (framePtrs.size () < elem.cwPanel.edgesNum) {
                        framePtrs.insert (framePtrs.end (), cornerFramesOfNextSegment.begin (), cornerFramesOfNextSegment.end ());
                    }
                }

                const auto& polygonCoordinates = typeSpecificDetails.AddList<GS::ObjectState> ("polygonCoordinates");
                const auto& frames = typeSpecificDetails.AddList<GS::ObjectState> ("frames");
                if (!framePtrs.empty ()) {
                    std::vector<API_Coord3D> polygonCoords = { framePtrs[0]->begC, framePtrs[0]->endC };
                    frames (CreateElementIdObjectState (framePtrs[0]->head.guid));
                    framePtrs.erase (framePtrs.begin ());
                    while (polygonCoords.size() != (elem.cwPanel.edgesNum + 1) && !framePtrs.empty ()) {
                        auto* framePtr = FindNextFrameOfPanel (framePtrs, polygonCoords);
                        if (framePtr == nullptr) {
                            break;
                        }
                        frames (CreateElementIdObjectState (framePtr->head.guid));
                    }
                    for (const auto& c : polygonCoords) {
                        polygonCoordinates (Create3DCoordinateObjectState (c));
                    }
                }
                typeSpecificDetails.Add ("isHidden", elem.cwPanel.hidden);
                bool isDegenerate = false;
                if (ACAPI_CurtainWall_IsCWPanelDegenerate (&elem.header.guid, &isDegenerate) == NoError) {
                    typeSpecificDetails.Add ("isDegenerate", isDegenerate);
                }
                typeSpecificDetails.Add ("segmentIndex", elem.cwPanel.segmentID);
                typeSpecificDetails.Add ("className", GS::UniString (elem.cwPanel.className));
            } break;

            case API_CurtainWallFrameID: {
                typeSpecificDetails.Add ("begCoordinate", Create3DCoordinateObjectState (elem.cwFrame.begC));
                typeSpecificDetails.Add ("endCoordinate", Create3DCoordinateObjectState (elem.cwFrame.endC));
                typeSpecificDetails.Add ("orientationVector", Create3DCoordinateObjectState (elem.cwFrame.orientation));
                typeSpecificDetails.Add ("panelConnectionHole", GS::ObjectState ("d", elem.cwFrame.d, "w", elem.cwFrame.w));
                typeSpecificDetails.Add ("frameContour", GS::ObjectState ("a1", elem.cwFrame.a1, "a2", elem.cwFrame.a2, "b1", elem.cwFrame.b1, "b2", elem.cwFrame.b2));
                typeSpecificDetails.Add ("segmentIndex", elem.cwFrame.segmentID);
                typeSpecificDetails.Add ("className", GS::UniString (elem.cwFrame.className));
                typeSpecificDetails.Add ("type",
                    elem.cwFrame.classID == APICWFrameClass_Merged ? "Deleted" :
                    elem.cwFrame.classID == APICWFrameClass_Boundary ? "Boundary" :
                    elem.cwFrame.classID == APICWFrameClass_Corner ? "Corner" :
                    elem.cwFrame.classID == APICWFrameClass_Division ? "Division" : "Custom");
            } break;

            case API_MeshID: {
                typeSpecificDetails.Add ("level", elem.mesh.level);
                if (elem.mesh.skirt == 3) {
                    typeSpecificDetails.Add ("skirtType", "SurfaceOnlyWithoutSkirt");
                } else if (elem.mesh.skirt == 2) {
                    typeSpecificDetails.Add ("skirtType", "WithSkirt");
                } else {
                    typeSpecificDetails.Add ("skirtType", "SolidBodyWithSkirt");
                }
                typeSpecificDetails.Add ("skirtLevel", elem.mesh.skirtLevel);
                if (elem.mesh.smoothRidges == APIRidge_AllSharp) {
                    typeSpecificDetails.Add ("ridges", GS::UniString ("AllSharp"));
                } else if (elem.mesh.smoothRidges == APIRidge_AllSmooth) {
                    typeSpecificDetails.Add ("ridges", GS::UniString ("AllSmooth"));
                } else {
                    typeSpecificDetails.Add ("ridges", GS::UniString ("UserDefined"));
                }
                typeSpecificDetails.Add ("showLines",    elem.mesh.showLines != 0);
                typeSpecificDetails.Add ("contourPen",   (Int32)elem.mesh.contPen);
                typeSpecificDetails.Add ("levelPen",     (Int32)elem.mesh.levelPen);
                typeSpecificDetails.Add ("lineTypeIndex", GetAttributeIndex (elem.mesh.ltypeInd));
                constexpr bool includeZCoords = true;
                AddPolygonWithHolesFromMemoCoords (elem.header.guid, typeSpecificDetails, "polygonCoordinates", "polygonArcs", "holes", "polygonCoordinates", "polygonArcs", includeZCoords);
                if (elem.mesh.levelLines.nSubLines > 0) {
                    API_ElementMemo memo = {};
                    const GS::OnExit guard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
                    if (ACAPI_Element_GetMemo (elem.header.guid, &memo, APIMemoMask_MeshLevel) == NoError && memo.meshLevelCoords != nullptr && memo.meshLevelEnds != nullptr) {
                        const auto& sublines = typeSpecificDetails.AddList<GS::ObjectState> ("sublines");
                        const GSSize nSublines = BMhGetSize (reinterpret_cast<GSHandle> (memo.meshLevelEnds)) / sizeof (Int32);
                        Int32 iCoord = 0;
                        for (Int32 i = 0; i < nSublines; ++i) {
                            GS::ObjectState subline;
                            const auto& coordinates = subline.AddList<GS::ObjectState> ("coordinates");

                            const Int32 nCoords = (*memo.meshLevelEnds)[i];
                            for (; iCoord < nCoords; ++iCoord) {
                                const API_MeshLevelCoord& coord = (*memo.meshLevelCoords)[iCoord];
                                coordinates (Create3DCoordinateObjectState (coord.c));
                            }
                            sublines (subline);
                        }
                    }
                }
            } break;

            case API_DrawingID: {
                typeSpecificDetails.Add ("pos",           Create2DCoordinateObjectState (elem.drawing.pos));
                typeSpecificDetails.Add ("angle",         elem.drawing.angle);
                typeSpecificDetails.Add ("ratio",         elem.drawing.ratio);
                typeSpecificDetails.Add ("drawingScale",  elem.drawing.drawingScale);
                typeSpecificDetails.Add ("modelOffset",   Create2DCoordinateObjectState (elem.drawing.modelOffset));
                typeSpecificDetails.Add ("isCutWithFrame", elem.drawing.isCutWithFrame);
                // Not the source view: once the Drawing exists Archicad reports its own link id
                // here, which navigator commands do not accept (#1480).
                typeSpecificDetails.Add ("navigatorItemId", CreateGuidObjectState (elem.drawing.drawingGuid));
                typeSpecificDetails.Add ("bounds", GS::ObjectState (
                    "xMin", elem.drawing.bounds.xMin,
                    "yMin", elem.drawing.bounds.yMin,
                    "xMax", elem.drawing.bounds.xMax,
                    "yMax", elem.drawing.bounds.yMax));
                if (elem.drawing.isCutWithFrame) {
                    AddPolygonFromMemoCoords (elem.header.guid, typeSpecificDetails, "clipPolygon");
                }
                typeSpecificDetails.Add ("nameType",      DrawingNameTypeToString (elem.drawing.nameType));
                if (elem.drawing.nameType == APIName_CustomName) {
                    typeSpecificDetails.Add ("customName", GS::UniString (elem.drawing.name));
                }
                typeSpecificDetails.Add ("numberingType",  DrawingNumberingTypeToString (elem.drawing.numberingType));
                if (elem.drawing.numberingType == APINumbering_CustomNum) {
                    typeSpecificDetails.Add ("customNumber", GS::UniString (elem.drawing.customNumber));
                }
                typeSpecificDetails.Add ("isInNumbering", elem.drawing.isInNumbering);
                typeSpecificDetails.Add ("hasBorderLine",       elem.drawing.hasBorderLine);
                typeSpecificDetails.Add ("borderLineTypeIndex", GetAttributeIndex (elem.drawing.borderLineType));
                typeSpecificDetails.Add ("borderPenIndex",      (Int32)elem.drawing.borderPen);
                typeSpecificDetails.Add ("borderSize",          elem.drawing.borderSize);
                // Library part index of the title (API_DrawingTitle::libInd) - a negative/invalid
                // index here means no title object is instantiated at all (nothing to show); setting
                // it to a valid index (e.g. copied from another drawing) is what makes Archicad
                // create the title's own placed element.
                typeSpecificDetails.Add ("titleLibraryPartIndex", elem.drawing.title.libInd);
                if (elem.drawing.title.guid != APINULLGuid) {
                    // The drawing title is itself a placed, GDL-parameterized library part
                    // element (API_DrawingTitle::guid, "object based elements from Archicad 10")
                    // - its own settings (font/pen/size aside) are reachable like any other
                    // element via this id, e.g. with Get/SetGDLParametersOfElements.
                    typeSpecificDetails.Add ("titleElementId", CreateGuidObjectState (elem.drawing.title.guid));
                }
            } break;

            case API_MorphID:
                AddMorphBodyFromMemo (elem, typeSpecificDetails);
                break;

            case API_RoofID: {
                const API_ShellBaseType& base = elem.roof.shellBase;
                typeSpecificDetails.Add ("roofClass", elem.roof.roofClass == API_PolyRoofID ? "MultiPlane" : "SinglePlane");
                typeSpecificDetails.Add ("structureType", StructureTypeToString (base.modelElemStructureType));
                typeSpecificDetails.Add ("thickness", base.thickness);
                typeSpecificDetails.Add ("level", base.level);
                typeSpecificDetails.Add ("zCoordinate", GetZPos (elem.header.floorInd, base.level, stories));
                if (StructureTypeToString (base.modelElemStructureType) == "Composite") {
                    typeSpecificDetails.Add ("compositeId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_CompWallID, base.composite)));
                } else {
                    typeSpecificDetails.Add ("buildingMaterialId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_BuildingMaterialID, base.buildingMaterial)));
                }
                if (elem.roof.roofClass == API_PlaneRoofID) {
                    typeSpecificDetails.Add ("angle", elem.roof.u.planeRoof.angle);
                    GS::ObjectState pivotLine;
                    pivotLine.Add ("begin", Create2DCoordinateObjectState (elem.roof.u.planeRoof.baseLine.c1));
                    pivotLine.Add ("end", Create2DCoordinateObjectState (elem.roof.u.planeRoof.baseLine.c2));
                    typeSpecificDetails.Add ("pivotLine", pivotLine);
                } else {
                    typeSpecificDetails.Add ("eavesOverhang", elem.roof.u.polyRoof.eavesOverHang);
                    const auto& levels = typeSpecificDetails.AddList<GS::ObjectState> ("levels");
                    for (short i = 0; i < elem.roof.u.polyRoof.levelNum && i < 16; ++i) {
                        GS::ObjectState level;
                        level.Add ("height", elem.roof.u.polyRoof.levelData[i].levelHeight);
                        level.Add ("angle", elem.roof.u.polyRoof.levelData[i].levelAngle);
                        levels (level);
                    }
                    AddPivotPolygonFromMemo (elem.header.guid, typeSpecificDetails);
                }
                // the roof's own polygon: the plane roof's outline, the multi-plane roof's contour
                AddPolygonWithHolesFromMemoCoords (elem.header.guid, typeSpecificDetails, "polygonOutline", "polygonArcs", "holes", "polygonOutline", "polygonArcs");
            } break;

            case API_HotlinkID: {
                API_Coord3D origin;
                double rotationAngle;
                bool mirrored;
                DecomposeHotlinkTransformation (elem.hotlink.transformation, origin, rotationAngle, mirrored);
                typeSpecificDetails.Add ("hotlinkType", elem.hotlink.type == APIHotlink_XRef ? "XRef" : "Module");
                typeSpecificDetails.Add ("hotlinkNodeId", CreateGuidObjectState (elem.hotlink.hotlinkNodeGuid));
                typeSpecificDetails.Add ("origin", Create3DCoordinateObjectState (origin));
                typeSpecificDetails.Add ("rotationAngle", rotationAngle);
                typeSpecificDetails.Add ("mirrored", mirrored);
                typeSpecificDetails.Add ("floorDifference", elem.hotlink.floorDifference);
                typeSpecificDetails.Add ("skipNested", elem.hotlink.skipNested);
                typeSpecificDetails.Add ("suspendFixAngle", elem.hotlink.suspendFixAngle);
                typeSpecificDetails.Add ("ignoreTopFloorLinks", elem.hotlink.ignoreTopFloorLinks);
                typeSpecificDetails.Add ("relinkWallOpenings", elem.hotlink.relinkWallOpenings);
                typeSpecificDetails.Add ("adjustLevelDiffs", elem.hotlink.adjustLevelDiffs);
            } break;

            case API_StairID: {
                // The fields CreateStairs writes, read back (#823). The finishes and the
                // structure are modeled by the Stair's subelements (risers, treads and
                // structures, see GetSubelementsOfHierarchicalElements), not by the Stair
                // element itself. On a placed stair basePlane.basePoint is in model
                // coordinates: its z is the absolute elevation of the base, not the offset
                // from the home story CreateStairs writes (a stair created on the 6.4 story
                // reads 6.4, and read 12.8 as zCoordinate when the story level was added
                // again, #1561), and its x and y are the origin of the baseline's coords.
                const API_Coord3D& basePoint = elem.stair.basePlane.basePoint;
                typeSpecificDetails.Add ("stepNum", static_cast<Int32> (elem.stair.stepNum));
                typeSpecificDetails.Add ("riserHeight", elem.stair.riserHeight);
                typeSpecificDetails.Add ("treadDepth", elem.stair.treadDepth);
                typeSpecificDetails.Add ("flightWidth", elem.stair.flightWidth);
                typeSpecificDetails.Add ("totalHeight", elem.stair.totalHeight);
                typeSpecificDetails.Add ("level", basePoint.z - GetZPos (elem.header.floorInd, 0.0, stories));
                typeSpecificDetails.Add ("zCoordinate", basePoint.z);
                typeSpecificDetails.Add ("finishVisible", elem.stair.finishVisible);
                AddStairBaseLineFromMemo (elem.header.guid, { basePoint.x, basePoint.y }, typeSpecificDetails);
            } break;

            default:
                typeSpecificDetails.Add ("error", "Not yet supported element type");
                break;
        }

        detailsOfElement.Add ("details", typeSpecificDetails);
        if (isFieldRequested ("floorPlanPolygons") && CanHaveFloorPlanCutFill (typeID)) {
            AddFloorPlanPolygonsIfAvailable (elem.header.guid, detailsOfElement);
        }

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
                                "id": {
                                    "type": "string",
                                    "description": "The Element ID (the ID field of the element's Settings dialog, shown in schedules and labels). The same string GetDetailsOfElements returns as id."
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
                                "typeSpecificDetails": {
                                    "$ref": "#/TypeSpecificSettings"
                                }
                            },
                            "additionalProperties": false,
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

GS::Optional<GS::UniString> SetDetailsOfElementsCommand::GetRawResponseSchema () const
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

// ============================================================
// drawIndex repositioning — implementation notes
// ============================================================
//
// WHY NOT ACAPI_Element_Change WITH drwIndex MASK
// ------------------------------------------------
// ACAPI_Element_Change silently ignores the drwIndex field even when the
// mask bit is set (confirmed Graphisoft bug, AC27). The only working API
// is ACAPI_Grouping_Tool with the following commands:
//   APITool_BringForward  (+1, reliable)
//   APITool_SendBackward  (-1, reliable)
//   APITool_ResetOrder    (→ class default, unreliable for some types — see below)
//
// DEAD ENDS (do not reintroduce)
// --------------------------------
//   APITool_BringToFront : unreliable on Wall and its sub-elements; causes
//                          unexpected level jumps. Removed from all strategies.
//   APITool_SendToBack   : does NOT reliably reach level 1. In sparse files
//                          it stops at 2 or 3. Removed from all strategies.
//
// STRATEGIES FOR NORMAL ELEMENTS
// --------------------------------
// Target clamped to [1, 14]. Two strategies, cheapest wins:
//   Direct  : |target - current| × BringForward or SendBackward
//   ViaReset: 1 ResetOrder + |target - classDefault| steps
//             (disabled for Morph, Railing, PolyLine — ResetOrder is a no-op
//              for these types; a simulated default from GetDefaultDrwIndex is
//              used instead if they need to reach their default level)
//
// CLASS DEFAULTS (per ArchiCAD documentation, levels 1-4 and 11-14 are void)
//   5  : Drawing
//   6  : Hatch, Zone
//   7  : Wall, Slab, Roof, Shell, Morph, Mesh, Column, CurtainWall, Stair,
//         Railing, Door, Window, Skylight, Opening
//   8  : Lamp, Object
//   9  : Line, Arc, Spline, PolyLine, Beam, Hotspot, ChangeMarker, Detail,
//         Worksheet
//   10 : Dimension (all types), Label, Text, CutPlane, Elevation,
//         InteriorElevation
//
// OPENINGS (Door, Window, Skylight) — special case
// --------------------------------------------------
// ArchiCAD enforces a hard minimum of 7 for elements embedded in a host
// (wall or roof/shell). Levels 1-6 are unreachable regardless of strategy.
//
// Host types by opening type:
//   Door   → Wall
//   Window → Wall
//   Skylight → Roof or Shell
//
// Levels 8-14 require indirect host manipulation:
//   1. Step host UP  (target - hostOriginal) × BringForward  → opening follows
//   2. Step host DOWN the same number of times × SendBackward → host returns,
//      opening stays elevated (the two are decoupled once host passes the
//      opening's natural level)
//
// Reset (drawIndex = 0) on an elevated opening is a no-op via direct
// ResetOrder. It must be applied to the HOST instead, which resets all its
// openings to level 7.
//
// Reverse host lookup (opening → host) uses the owner guid stored directly on
// the opening element (API_WindowType::owner for door/window, API_SkylightType::owner
// for skylight) via ACAPI_Element_Get — no need to scan hosts.
//
// BATCH ORDERING CONSTRAINT
// --------------------------
// Openings are processed before all other elements within the same batch.
// This ensures the host manipulation reads the host's pre-batch position, not
// a position already shifted by another element in the same call.
//
// KNOWN LIMITATIONS
// -----------------
// • Door/Window/Skylight: levels 1-6 are unreachable (ArchiCAD constraint).
// • An opening cannot be below its host's current level. If the host wall/roof
//   is at level 9, the opening cannot reach levels 7 or 8.
// • Multiple openings in the same host wall/roof with different targets in a
//   single batch: all will end up at the same level (the last one processed),
//   because moving the host affects all its openings simultaneously.
// • Opening (API_OpeningID, generic void): completely immovable. All
//   Grouping_Tool calls are no-ops. ArchiCAD displays level 0 for these
//   elements (outside the 1-14 ordering system); the API may report 7
//   (class default) but the actual display order cannot be changed. Likely
//   due to multi-host nature (can span Wall, Slab, Roof, Shell, Beam, Column).
// • Morph, Railing, PolyLine: ResetOrder is a no-op; reset (drawIndex = 0)
//   is simulated by stepping from current position to the class default.
// ============================================================

static bool IsOpeningType (const API_Elem_Head& head)
{
    switch (GetElemTypeId (head)) {
        case API_DoorID:
        case API_WindowID:
        case API_SkylightID: return true;
        default:             return false;
    }
}

static bool IsResetOrderReliable (const API_Elem_Head& head)
{
    switch (GetElemTypeId (head)) {
        case API_MorphID:
        case API_RailingID:
        case API_PolyLineID: return false;
        default:             return true;
    }
}

static Int32 GetDefaultDrwIndex (const API_Elem_Head& head)
{
    switch (GetElemTypeId (head)) {
        case API_DrawingID:                                                      return 5;
        case API_HatchID:     case API_ZoneID:                                   return 6;
        case API_WallID:      case API_SlabID:      case API_RoofID:
        case API_ShellID:     case API_MorphID:     case API_MeshID:
        case API_ColumnID:    case API_CurtainWallID: case API_StairID:
        case API_RailingID:   case API_DoorID:      case API_WindowID:
        case API_SkylightID:  case API_OpeningID:                                return 7;
        case API_LampID:      case API_ObjectID:                                 return 8;
        case API_LineID:      case API_ArcID:       case API_SplineID:
        case API_PolyLineID:  case API_BeamID:      case API_HotspotID:
        case API_ChangeMarkerID: case API_DetailID: case API_WorksheetID:        return 9;
        case API_DimensionID: case API_RadialDimensionID:
        case API_LevelDimensionID: case API_AngleDimensionID:
        case API_LabelID:     case API_TextID:      case API_CutPlaneID:
        case API_ElevationID: case API_InteriorElevationID:                      return 10;
        default:                                                                  return 7;
    }
}

static void ApplyDrawIndexStrategy (GS::Array<API_Guid>& guids, const API_Elem_Head& head, Int32 target, Int32 current)
{
    target = GS::Max (1, GS::Min (target, 14));
    const Int32 defaultLevel = GetDefaultDrwIndex (head);
    const Int32 directCost   = GS::Abs (target - current);
    const Int32 viaResetCost = IsResetOrderReliable (head)
                             ? 1 + GS::Abs (target - defaultLevel)
                             : INT_MAX;

    if (viaResetCost < directCost) {
        ACAPI_Grouping_Tool (guids, APITool_ResetOrder, nullptr);
        const API_ToolCmdID step = (target > defaultLevel) ? APITool_BringForward : APITool_SendBackward;
        for (Int32 i = 0; i < GS::Abs (target - defaultLevel); ++i)
            ACAPI_Grouping_Tool (guids, step, nullptr);
    } else {
        const API_ToolCmdID step = (target > current) ? APITool_BringForward : APITool_SendBackward;
        for (Int32 i = 0; i < directCost; ++i)
            ACAPI_Grouping_Tool (guids, step, nullptr);
    }
}

static GS::HashTable<API_Guid, API_Guid> BuildOpeningToHostMap (const GS::HashSet<API_Guid>& targetGuids)
{
    GS::HashTable<API_Guid, API_Guid> result;
    for (const API_Guid& guid : targetGuids) {
        API_Element elem = {};
        elem.header.guid = guid;
        if (ACAPI_Element_Get (&elem) != NoError)
            continue;

        switch (GetElemTypeId (elem.header)) {
            case API_DoorID:
            case API_WindowID:   result.Add (guid, elem.window.owner);   break;
            case API_SkylightID: result.Add (guid, elem.skylight.owner); break;
            default: break;
        }
    }
    return result;
}

// JSON has no separate integer type, so clients routinely send 2.0 where an index is
// expected. GS::ObjectState::Get with an integer target rejects such a value, which used to
// drop the field without any error (#530). This accepts a floating point value as well and
// floors it - std::floor, not truncation, so a negative index (stories below 0) rounds the
// same way as a positive one.
template <typename IntType>
static bool GetIndexValue (const GS::ObjectState& os, const char* fieldName, IntType& target)
{
    if (os.Get (fieldName, target)) {
        return true;
    }
    double doubleValue = 0.0;
    if (os.Get (fieldName, doubleValue)) {
        target = static_cast<IntType> (std::floor (doubleValue));
        return true;
    }
    return false;
}

GS::ObjectState SetDetailsOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elementsWithDetails;
    parameters.Get ("elementsWithDetails", elementsWithDetails);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    // Pre-scan: identify openings (Door/Window/Skylight) that need host manipulation:
    //   - target > 7  : elevate via host (step up, step back)
    //   - target == 0 : reset via host ResetOrder (direct ResetOrder on opening doesn't work when elevated)
    GS::HashSet<API_Guid> openingsNeedingHost;
    for (const GS::ObjectState& ewd : elementsWithDetails) {
        const GS::ObjectState* eid = ewd.Get ("elementId");
        const GS::ObjectState* det = ewd.Get ("details");
        if (eid == nullptr || det == nullptr) continue;
        short tgt = -1;
        GetIndexValue (*det, "drawIndex", tgt);
        if (tgt > 0 && tgt <= 7) continue;   // 1-7: no host needed
        API_Element hdr = {};
        hdr.header.guid = GetGuidFromObjectState (*eid);
        if (ACAPI_Element_GetHeader (&hdr.header) != NoError) continue;
        if (IsOpeningType (hdr.header))
            openingsNeedingHost.Add (hdr.header.guid);
    }
    const GS::HashTable<API_Guid, API_Guid> openingToHost = BuildOpeningToHostMap (openingsNeedingHost);

    // Process openings first so host manipulation starts from the host's natural position,
    // not a position already shifted by another element in the same batch.
    GS::Array<GS::ObjectState> sortedElements;
    for (const GS::ObjectState& ewd : elementsWithDetails) {
        const GS::ObjectState* eid = ewd.Get ("elementId");
        if (eid != nullptr && openingsNeedingHost.Contains (GetGuidFromObjectState (*eid)))
            sortedElements.Push (ewd);
    }
    for (const GS::ObjectState& ewd : elementsWithDetails) {
        const GS::ObjectState* eid = ewd.Get ("elementId");
        if (eid == nullptr || !openingsNeedingHost.Contains (GetGuidFromObjectState (*eid)))
            sortedElements.Push (ewd);
    }

    ACAPI_CallUndoableCommand ("SetDetailsOfElementsCommand", [&]() {
        for (const GS::ObjectState& elementWithDetails : sortedElements) {
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
            bool hasElementChanges = false;
            if (GetIndexValue (*details, "floorIndex", elem.header.floorInd)) {
                ACAPI_ELEMENT_MASK_SET (mask, API_Elem_Head, floorInd);
                hasElementChanges = true;
            }
            Int32 layerIndex;
            if (GetIndexValue (*details, "layerIndex", layerIndex)) {
                elem.header.layer = ACAPI_CreateAttributeIndex (layerIndex);
                ACAPI_ELEMENT_MASK_SET (mask, API_Elem_Head, layer);
                hasElementChanges = true;
            }
            short drwIndexTarget = -1;
            GetIndexValue (*details, "drawIndex", drwIndexTarget);
            GS::Optional<double> requestedDrawingScale;

            API_ElementMemo clipMemo = {};
            UInt64 memoMask = 0;
            bool hasMemoChanges = false;

            const GS::ObjectState* typeSpecificDetails = details->Get ("typeSpecificDetails");
            if (typeSpecificDetails != nullptr) {
                switch (GetElemTypeId (elem.header)) {
                    case API_WallID: {
                        const GS::ObjectState* begCoordinate = typeSpecificDetails->Get ("begCoordinate");
                        if (begCoordinate != nullptr) {
                            elem.wall.begC = Get2DCoordinateFromObjectState (*begCoordinate);
                            ACAPI_ELEMENT_MASK_SET (mask, API_WallType, begC);
                        }
                        const GS::ObjectState* endCoordinate = typeSpecificDetails->Get ("endCoordinate");
                        if (endCoordinate != nullptr) {
                            elem.wall.endC = Get2DCoordinateFromObjectState (*endCoordinate);
                            ACAPI_ELEMENT_MASK_SET (mask, API_WallType, endC);
                        }
                        if (typeSpecificDetails->Get ("height", elem.wall.height)) {
                            ACAPI_ELEMENT_MASK_SET (mask, API_WallType, height);
                        }
                        if (typeSpecificDetails->Get ("offset", elem.wall.offset)) {
                            ACAPI_ELEMENT_MASK_SET (mask, API_WallType, offset);
                        }

                        switch (elem.wall.type) {
                            case APIWtyp_Trapez: {
                                if (typeSpecificDetails->Get ("begThickness", elem.wall.thickness)) {
                                    ACAPI_ELEMENT_MASK_SET (mask, API_WallType, thickness);
                                }
                                if (typeSpecificDetails->Get ("endThickness", elem.wall.thickness1)) {
                                    ACAPI_ELEMENT_MASK_SET (mask, API_WallType, thickness1);
                                }
                            } break;
                            default:
                            break;
                        }
                    } break;
                    case API_ZoneID: {
                        const GS::ObjectState* stampPosition = typeSpecificDetails->Get ("stampPosition");
                        if (stampPosition != nullptr) {
                            elem.zone.pos = Get2DCoordinateFromObjectState (*stampPosition);
                            ACAPI_ELEMENT_MASK_SET (mask, API_ZoneType, pos);
                        }
                        if (typeSpecificDetails->Get ("stampAngle", elem.zone.stampAngle)) {
                            ACAPI_ELEMENT_MASK_SET (mask, API_ZoneType, stampAngle);
                        }
                        if (typeSpecificDetails->Get ("fixedStampAngle", elem.zone.fixedAngle)) {
                            ACAPI_ELEMENT_MASK_SET (mask, API_ZoneType, fixedAngle);
                        }
                        // Only an automatic zone is found around its reference point; a manual
                        // zone has its outline instead, so the field is ignored for it.
                        const GS::ObjectState* referencePosition = typeSpecificDetails->Get ("referencePosition");
                        if (referencePosition != nullptr && !elem.zone.manual) {
                            elem.zone.refPos = Get2DCoordinateFromObjectState (*referencePosition);
                            ACAPI_ELEMENT_MASK_SET (mask, API_ZoneType, refPos);
                        }
                    } break;
                    case API_LineID: {
                        const GS::ObjectState* begCoordinate = typeSpecificDetails->Get ("begCoordinate");
                        if (begCoordinate != nullptr) {
                            elem.line.begC = Get2DCoordinateFromObjectState (*begCoordinate);
                            ACAPI_ELEMENT_MASK_SET (mask, API_LineType, begC);
                        }
                        const GS::ObjectState* endCoordinate = typeSpecificDetails->Get ("endCoordinate");
                        if (endCoordinate != nullptr) {
                            elem.line.endC = Get2DCoordinateFromObjectState (*endCoordinate);
                            ACAPI_ELEMENT_MASK_SET (mask, API_LineType, endC);
                        }
                        bool penChanged = false, ltypeChanged = false, roomSepChanged = false;
                        SetLineFamilySettingsFields (*typeSpecificDetails, elem.line.linePen, elem.line.ltypeInd, elem.line.roomSeparator, penChanged, ltypeChanged, roomSepChanged);
                        if (penChanged) ACAPI_ELEMENT_MASK_SET (mask, API_LineType, linePen);
                        if (ltypeChanged) ACAPI_ELEMENT_MASK_SET (mask, API_LineType, ltypeInd);
                        if (roomSepChanged) ACAPI_ELEMENT_MASK_SET (mask, API_LineType, roomSeparator);
                    } break;
                    case API_ArcID:
                    case API_CircleID: {
                        const GS::ObjectState* origin = typeSpecificDetails->Get ("origin");
                        if (origin != nullptr) {
                            elem.arc.origC = Get2DCoordinateFromObjectState (*origin);
                            ACAPI_ELEMENT_MASK_SET (mask, API_ArcType, origC);
                        }
                        if (typeSpecificDetails->Get ("radius", elem.arc.r)) {
                            ACAPI_ELEMENT_MASK_SET (mask, API_ArcType, r);
                        }
                        if (typeSpecificDetails->Get ("angle", elem.arc.angle)) {
                            ACAPI_ELEMENT_MASK_SET (mask, API_ArcType, angle);
                        }
                        if (typeSpecificDetails->Get ("ratio", elem.arc.ratio)) {
                            ACAPI_ELEMENT_MASK_SET (mask, API_ArcType, ratio);
                        }
                        if (GetElemTypeId (elem.header) == API_ArcID) {
                            if (typeSpecificDetails->Get ("begAngle", elem.arc.begAng)) {
                                ACAPI_ELEMENT_MASK_SET (mask, API_ArcType, begAng);
                            }
                            if (typeSpecificDetails->Get ("endAngle", elem.arc.endAng)) {
                                ACAPI_ELEMENT_MASK_SET (mask, API_ArcType, endAng);
                            }
                        }
                        if (typeSpecificDetails->Get ("reflected", elem.arc.reflected)) {
                            ACAPI_ELEMENT_MASK_SET (mask, API_ArcType, reflected);
                        }
                        bool penChanged = false, ltypeChanged = false, roomSepChanged = false;
                        SetLineFamilySettingsFields (*typeSpecificDetails, elem.arc.linePen, elem.arc.ltypeInd, elem.arc.roomSeparator, penChanged, ltypeChanged, roomSepChanged);
                        if (penChanged) ACAPI_ELEMENT_MASK_SET (mask, API_ArcType, linePen);
                        if (ltypeChanged) ACAPI_ELEMENT_MASK_SET (mask, API_ArcType, ltypeInd);
                        if (roomSepChanged) ACAPI_ELEMENT_MASK_SET (mask, API_ArcType, roomSeparator);
                    } break;
                    case API_HotspotID: {
                        const GS::ObjectState* position = typeSpecificDetails->Get ("position");
                        if (position != nullptr) {
                            elem.hotspot.pos = Get2DCoordinateFromObjectState (*position);
                            ACAPI_ELEMENT_MASK_SET (mask, API_HotspotType, pos);
                        }
                        if (typeSpecificDetails->Get ("height", elem.hotspot.height)) {
                            ACAPI_ELEMENT_MASK_SET (mask, API_HotspotType, height);
                        }
                        short hotspotPen = 0;
                        if (typeSpecificDetails->Get ("penIndex", hotspotPen)) {
                            elem.hotspot.pen = hotspotPen;
                            ACAPI_ELEMENT_MASK_SET (mask, API_HotspotType, pen);
                        }
                    } break;
                    case API_SplineID: {
                        // Geometry (coordinates/closed) cannot be modified via ACAPI_Element_Change
                        // for Spline (documented ACAPI limitation) - only these settings fields.
                        bool penChanged = false, ltypeChanged = false, roomSepChanged = false;
                        SetLineFamilySettingsFields (*typeSpecificDetails, elem.spline.linePen, elem.spline.ltypeInd, elem.spline.roomSeparator, penChanged, ltypeChanged, roomSepChanged);
                        if (penChanged) ACAPI_ELEMENT_MASK_SET (mask, API_SplineType, linePen);
                        if (ltypeChanged) ACAPI_ELEMENT_MASK_SET (mask, API_SplineType, ltypeInd);
                        if (roomSepChanged) ACAPI_ELEMENT_MASK_SET (mask, API_SplineType, roomSeparator);
                    } break;
                    case API_DrawingID: {
                        GS::Array<GS::ObjectState> clipCoords;
                        if (typeSpecificDetails->Get ("clipPolygon", clipCoords) && clipCoords.GetSize () >= 3) {
                            Int32 nUnique = (Int32) clipCoords.GetSize ();
                            if (nUnique > 1) {
                                API_Coord first = Get2DCoordinateFromObjectState (clipCoords.GetFirst ());
                                API_Coord last  = Get2DCoordinateFromObjectState (clipCoords.GetLast ());
                                if (first.x == last.x && first.y == last.y)
                                    --nUnique;
                            }
                            elem.drawing.isCutWithFrame    = true;
                            elem.drawing.poly.nSubPolys    = 1;
                            elem.drawing.poly.nCoords      = nUnique + 1; // +1 for closing vertex
                            elem.drawing.poly.nArcs        = 0;
                            ACAPI_ELEMENT_MASK_SET (mask, API_DrawingType, isCutWithFrame);
                            ACAPI_ELEMENT_MASK_SET (mask, API_DrawingType, poly);
                        } else if (typeSpecificDetails->Get ("isCutWithFrame", elem.drawing.isCutWithFrame)) {
                            // No clipPolygon supplied: only the flag itself is changed. false clears
                            // cropping and restores the full extent (#651); true re-enables whatever
                            // clip polygon is still stored on the Drawing.
                            ACAPI_ELEMENT_MASK_SET (mask, API_DrawingType, isCutWithFrame);
                        }
                        if (typeSpecificDetails->Get ("drawingScale", elem.drawing.drawingScale)) {
                            ACAPI_ELEMENT_MASK_SET (mask, API_DrawingType, drawingScale);
                            requestedDrawingScale = elem.drawing.drawingScale;
                        }
                        if (typeSpecificDetails->Get ("ratio", elem.drawing.ratio)) {
                            ACAPI_ELEMENT_MASK_SET (mask, API_DrawingType, ratio);
                        }
                        if (typeSpecificDetails->Get ("angle", elem.drawing.angle)) {
                            ACAPI_ELEMENT_MASK_SET (mask, API_DrawingType, angle);
                        }
                        const GS::ObjectState* posState = typeSpecificDetails->Get ("pos");
                        if (posState != nullptr) {
                            elem.drawing.pos = Get2DCoordinateFromObjectState (*posState);
                            ACAPI_ELEMENT_MASK_SET (mask, API_DrawingType, pos);
                        }
                        const GS::ObjectState* modelOffsetState = typeSpecificDetails->Get ("modelOffset");
                        if (modelOffsetState != nullptr) {
                            elem.drawing.modelOffset = Get2DCoordinateFromObjectState (*modelOffsetState);
                            ACAPI_ELEMENT_MASK_SET (mask, API_DrawingType, modelOffset);
                        }
                        // drawingGuid (the Drawing's source link) is intentionally not settable here:
                        // confirmed live that ACAPI_Element_Change silently discards any change to it
                        // after creation - it can only be set once, at CreateDrawings time.
                        GS::UniString nameTypeStr;
                        if (typeSpecificDetails->Get ("nameType", nameTypeStr)) {
                            elem.drawing.nameType = DrawingNameTypeFromString (nameTypeStr);
                            ACAPI_ELEMENT_MASK_SET (mask, API_DrawingType, nameType);
                        }
                        if (SetCharProperty (typeSpecificDetails, "customName", elem.drawing.name)) {
                            ACAPI_ELEMENT_MASK_SET (mask, API_DrawingType, name);
                        }
                        GS::UniString numberingTypeStr;
                        if (typeSpecificDetails->Get ("numberingType", numberingTypeStr)) {
                            elem.drawing.numberingType = DrawingNumberingTypeFromString (numberingTypeStr);
                            ACAPI_ELEMENT_MASK_SET (mask, API_DrawingType, numberingType);
                        }
                        if (SetCharProperty (typeSpecificDetails, "customNumber", elem.drawing.customNumber)) {
                            ACAPI_ELEMENT_MASK_SET (mask, API_DrawingType, customNumber);
                        }
                        if (typeSpecificDetails->Get ("isInNumbering", elem.drawing.isInNumbering)) {
                            ACAPI_ELEMENT_MASK_SET (mask, API_DrawingType, isInNumbering);
                        }
                        if (typeSpecificDetails->Get ("hasBorderLine", elem.drawing.hasBorderLine)) {
                            ACAPI_ELEMENT_MASK_SET (mask, API_DrawingType, hasBorderLine);
                        }
                        Int32 borderLineTypeIndex = 0;
                        if (typeSpecificDetails->Get ("borderLineTypeIndex", borderLineTypeIndex) && borderLineTypeIndex > 0) {
                            elem.drawing.borderLineType = ACAPI_CreateAttributeIndex (borderLineTypeIndex);
                            ACAPI_ELEMENT_MASK_SET (mask, API_DrawingType, borderLineType);
                        }
                        short borderPen = 0;
                        if (typeSpecificDetails->Get ("borderPenIndex", borderPen)) {
                            elem.drawing.borderPen = borderPen;
                            ACAPI_ELEMENT_MASK_SET (mask, API_DrawingType, borderPen);
                        }
                        if (typeSpecificDetails->Get ("borderSize", elem.drawing.borderSize)) {
                            ACAPI_ELEMENT_MASK_SET (mask, API_DrawingType, borderSize);
                        }
                        if (typeSpecificDetails->Get ("titleLibraryPartIndex", elem.drawing.title.libInd)) {
                            ACAPI_ELEMENT_MASK_SET (mask, API_DrawingType, title.libInd);
                        }
                    } break;
                    case API_TextID: {
                        const GS::ObjectState* position = typeSpecificDetails->Get ("position");
                        if (position != nullptr) {
                            elem.text.loc = Get2DCoordinateFromObjectState (*position);
                            ACAPI_ELEMENT_MASK_SET (mask, API_TextType, loc);
                        }
                        if (typeSpecificDetails->Get ("angle", elem.text.angle)) {
                            ACAPI_ELEMENT_MASK_SET (mask, API_TextType, angle);
                        }
                        const bool heightChanged = typeSpecificDetails->Get ("height", elem.text.size);
                        if (heightChanged) {
                            ACAPI_ELEMENT_MASK_SET (mask, API_TextType, size);
                        }
                        GS::UniString justification;
                        if (typeSpecificDetails->Get ("justification", justification)) {
                            elem.text.just = ParseJustificationString (justification);
                            ACAPI_ELEMENT_MASK_SET (mask, API_TextType, just);
                        }
                        GS::UniString text;
                        if (typeSpecificDetails->Get ("text", text)) {
                            // Same content handling as CreateTexts: the whole textContent is replaced
                            // and the paragraphs memo is rebuilt as a single paragraph/run, so any
                            // per-run formatting of the old content is dropped and the element becomes
                            // auto-width (nonBreaking) - mask exactly the API_TextType fields the helper sets.
                            // A new height given alongside is carried into the rebuilt run via elem.text.size.
                            SetTextContentAndParagraphs (clipMemo, elem.text, text);
                            ACAPI_ELEMENT_MASK_SET (mask, API_TextType, nLine);
                            ACAPI_ELEMENT_MASK_SET (mask, API_TextType, useEolPos);
#ifndef ServerMainVers_2800
                            ACAPI_ELEMENT_MASK_SET (mask, API_TextType, charCode); // the content is UTF-16 (see ApplyTextContent)
#endif
                            ACAPI_ELEMENT_MASK_SET (mask, API_TextType, nonBreaking);
                            ACAPI_ELEMENT_MASK_SET (mask, API_TextType, width);
                            ACAPI_ELEMENT_MASK_SET (mask, API_TextType, height);
                            memoMask = TextContentMemoMask;
                            hasMemoChanges = true;
                        } else if (heightChanged) {
                            // A multistyle Text (which every UI-placed or Tapir-created Text is) takes
                            // its character height from the per-run sizes in the paragraphs memo, not
                            // from API_TextType::size, so masking size alone is silently ignored (see
                            // ModifyTexts). Rebuild the content from its read-back with the new height
                            // merged into every run; the runs' own pen/font/faces/effects are kept.
                            GS::ObjectState heightStyle;
                            heightStyle.Add ("height", elem.text.size);
                            GS::ObjectState contentParams;
                            auto contentError = TextLabelDetails::ReadContentForStyleOnlyModify (elem.header.guid, heightStyle, contentParams);
                            if (!contentError.HasValue ()) {
                                contentError = TextLabelDetails::ApplyTextContent (clipMemo, elem.text, contentParams);
                            }
                            if (contentError.HasValue ()) {
                                ACAPI_DisposeElemMemoHdls (&clipMemo);
                                executionResults (CreateFailedExecutionResult (*contentError));
                                continue;
                            }
                            ACAPI_ELEMENT_MASK_SET (mask, API_TextType, nLine);
                            ACAPI_ELEMENT_MASK_SET (mask, API_TextType, useEolPos);
#ifndef ServerMainVers_2800
                            ACAPI_ELEMENT_MASK_SET (mask, API_TextType, charCode); // the content is UTF-16 (see ApplyTextContent)
#endif
                            ACAPI_ELEMENT_MASK_SET (mask, API_TextType, nonBreaking);
                            ACAPI_ELEMENT_MASK_SET (mask, API_TextType, width);
                            ACAPI_ELEMENT_MASK_SET (mask, API_TextType, height);
                            memoMask = TextContentMemoMask;
                            hasMemoChanges = true;
                        }
                    } break;
                    case API_LabelID: {
                        GS::UniString text;
                        if (elem.label.labelClass == APILblClass_Text && typeSpecificDetails->Get ("text", text)) {
                            // Same content handling as CreateLabels (see the API_TextID case above).
                            SetTextContentAndParagraphs (clipMemo, elem.label.u.text, text);
                            ACAPI_ELEMENT_MASK_SET (mask, API_LabelType, u.text.nLine);
                            ACAPI_ELEMENT_MASK_SET (mask, API_LabelType, u.text.useEolPos);
#ifndef ServerMainVers_2800
                            ACAPI_ELEMENT_MASK_SET (mask, API_LabelType, u.text.charCode); // the content is UTF-16 (see ApplyTextContent)
#endif
                            ACAPI_ELEMENT_MASK_SET (mask, API_LabelType, u.text.nonBreaking);
                            ACAPI_ELEMENT_MASK_SET (mask, API_LabelType, u.text.width);
                            ACAPI_ELEMENT_MASK_SET (mask, API_LabelType, u.text.height);
                            memoMask = TextContentMemoMask;
                            hasMemoChanges = true;
                        }
                    } break;
                    default:
                    break;
                }
                hasElementChanges = true;
            }

            if (typeSpecificDetails != nullptr && GetElemTypeId (elem.header) == API_DrawingID) {
                GS::Array<GS::ObjectState> clipCoords;
                if (typeSpecificDetails->Get ("clipPolygon", clipCoords) && clipCoords.GetSize () >= 3) {
                    // Drop explicit closing vertex if caller already duplicated first point at the end
                    if (clipCoords.GetSize () > 1) {
                        API_Coord first = Get2DCoordinateFromObjectState (clipCoords.GetFirst ());
                        API_Coord last  = Get2DCoordinateFromObjectState (clipCoords.GetLast ());
                        if (first.x == last.x && first.y == last.y)
                            clipCoords.Pop ();
                    }
                    const Int32 nUnique = clipCoords.GetSize ();
                    // AC polygon convention: coords[1..nUnique] = vertices, coords[nUnique+1] = closing duplicate of coords[1]
                    // Handle size = nUnique+2 (index 0 unused, 1..nUnique vertices, nUnique+1 closing)
                    clipMemo.coords = reinterpret_cast<API_Coord**> (BMAllocateHandle ((nUnique + 2) * sizeof (API_Coord), ALLOCATE_CLEAR, 0));
                    clipMemo.pends  = reinterpret_cast<Int32**>     (BMAllocateHandle (2 * sizeof (Int32), ALLOCATE_CLEAR, 0));
                    if (clipMemo.coords != nullptr && clipMemo.pends != nullptr) {
                        for (Int32 i = 0; i < nUnique; ++i)
                            (*clipMemo.coords)[i + 1] = Get2DCoordinateFromObjectState (clipCoords[i]);
                        (*clipMemo.coords)[nUnique + 1] = (*clipMemo.coords)[1]; // closing vertex
                        (*clipMemo.pends)[1] = nUnique + 1;
                        memoMask = APIMemoMask_Polygon;
                        hasMemoChanges = true;
                    }
                }
            } else if (typeSpecificDetails != nullptr && GetElemTypeId (elem.header) == API_PolyLineID) {
                // vertexIDs[0] must hold the MAX vertex ID used across the shape (not be left at
                // 0/uninitialized) - confirmed via a real ACAPI_Element_Create usage example
                // (Graphisoft community forum). This was the missing piece in every earlier
                // from-scratch attempt, which is why they all failed with APIERR_BADPOLY even for
                // an identity write - nothing to do with mask bits, GetMemo-seeding, or the
                // coords[0]=(-1,0) sentinel (which was already correct).
                GS::Array<GS::ObjectState> coordinates;
                if (typeSpecificDetails->Get ("coordinates", coordinates) && coordinates.GetSize () >= 2) {
                    GS::Array<GS::ObjectState> arcs;
                    typeSpecificDetails->Get ("arcs", arcs);
                    const GS::Array<API_PolyArc> polyArcs = GetPolyArcs (arcs, 1);

                    const Int32 nCoords = (Int32) coordinates.GetSize ();
                    clipMemo.coords    = reinterpret_cast<API_Coord**>   (BMAllocateHandle ((nCoords + 1) * sizeof (API_Coord), ALLOCATE_CLEAR, 0));
                    clipMemo.pends     = reinterpret_cast<Int32**>       (BMAllocateHandle (2 * sizeof (Int32), ALLOCATE_CLEAR, 0));
                    clipMemo.parcs     = reinterpret_cast<API_PolyArc**> (BMAllocateHandle (polyArcs.GetSize () * sizeof (API_PolyArc), ALLOCATE_CLEAR, 0));
                    clipMemo.vertexIDs = reinterpret_cast<UInt32**>      (BMAllocateHandle ((nCoords + 1) * sizeof (UInt32), ALLOCATE_CLEAR, 0));
                    if (clipMemo.coords != nullptr && clipMemo.pends != nullptr && clipMemo.vertexIDs != nullptr) {
                        (*clipMemo.coords)[0] = { -1.0, 0.0 }; // open-polyline dummy coord
                        (*clipMemo.vertexIDs)[0] = (UInt32) nCoords; // max vertex ID used
                        for (Int32 i = 0; i < nCoords; ++i) {
                            (*clipMemo.coords)[i + 1] = Get2DCoordinateFromObjectState (coordinates[i]);
                            (*clipMemo.vertexIDs)[i + 1] = (UInt32) (i + 1);
                        }
                        (*clipMemo.pends)[0] = 0;
                        (*clipMemo.pends)[1] = nCoords;
                        if (clipMemo.parcs != nullptr) {
                            Int32 iArc = 0;
                            for (const API_PolyArc& a : polyArcs)
                                (*clipMemo.parcs)[iArc++] = a;
                        }
                        elem.polyLine.poly.nCoords   = nCoords;
                        elem.polyLine.poly.nSubPolys = 1;
                        elem.polyLine.poly.nArcs     = (Int32) polyArcs.GetSize ();
                        ACAPI_ELEMENT_MASK_SET (mask, API_PolyLineType, poly);
                        memoMask = APIMemoMask_Polygon;
                        hasMemoChanges = true;
                        hasElementChanges = true;
                    }
                }
                {
                    bool penChanged = false, ltypeChanged = false, roomSepChanged = false;
                    if (SetLineFamilySettingsFields (*typeSpecificDetails, elem.polyLine.linePen, elem.polyLine.ltypeInd, elem.polyLine.roomSeparator, penChanged, ltypeChanged, roomSepChanged)) {
                        if (penChanged) ACAPI_ELEMENT_MASK_SET (mask, API_PolyLineType, linePen);
                        if (ltypeChanged) ACAPI_ELEMENT_MASK_SET (mask, API_PolyLineType, ltypeInd);
                        if (roomSepChanged) ACAPI_ELEMENT_MASK_SET (mask, API_PolyLineType, roomSeparator);
                        hasElementChanges = true;
                    }
                }
            } else if (typeSpecificDetails != nullptr && GetElemTypeId (elem.header) == API_HatchID) {
                // Hatch is a CLOSED polygon (unlike PolyLine): coords[0] dummy is (0,0) not
                // (-1,0), and every contour (the main outline, plus each hole) needs its own
                // explicit closing vertex duplicating that contour's first point, whose vertexID
                // also duplicates that first point's ID. Vertex IDs restart at 1 for EACH contour
                // (not continued sequentially across contours) - confirmed live: global numbering
                // across contours reproducibly failed with APIERR_BADPOLY for a hatch with holes,
                // per-contour numbering works.
                GS::Array<GS::ObjectState> coordinates;
                if (typeSpecificDetails->Get ("coordinates", coordinates) && coordinates.GetSize () >= 3) {
                    GS::Array<GS::ObjectState> arcs;
                    typeSpecificDetails->Get ("arcs", arcs);
                    GS::Array<GS::ObjectState> holes;
                    typeSpecificDetails->Get ("holes", holes);
                    auto holesError = ValidateHoles (holes);
                    if (holesError.HasValue ()) {
                        executionResults (CreateFailedExecutionResult (APIERR_BADPARS, holesError.Get ()));
                        continue;
                    }

                    // Each contour: N unique coords -> N+1 stored coords (with closing duplicate).
                    Int32 totalUnique = (Int32) coordinates.GetSize ();
                    Int32 totalCoords = totalUnique + 1;
                    Int32 totalArcs = (Int32) arcs.GetSize ();
                    GS::Array<GS::Array<GS::ObjectState>> holeCoordsList;
                    GS::Array<GS::Array<GS::ObjectState>> holeArcsList;
                    for (const GS::ObjectState& hole : holes) {
                        GS::Array<GS::ObjectState> holeCoords, holeArcs;
                        if (GetHoleGeometry (hole, holeCoords, holeArcs) && holeCoords.GetSize () >= 3) {
                            totalUnique += (Int32) holeCoords.GetSize ();
                            totalCoords += (Int32) holeCoords.GetSize () + 1;
                            totalArcs += (Int32) holeArcs.GetSize ();
                            holeCoordsList.Push (holeCoords);
                            holeArcsList.Push (holeArcs);
                        }
                    }
                    const Int32 nSubPolys = 1 + (Int32) holeCoordsList.GetSize ();

                    clipMemo.coords    = reinterpret_cast<API_Coord**>   (BMAllocateHandle ((totalCoords + 1) * sizeof (API_Coord), ALLOCATE_CLEAR, 0));
                    clipMemo.pends     = reinterpret_cast<Int32**>       (BMAllocateHandle ((nSubPolys + 1) * sizeof (Int32), ALLOCATE_CLEAR, 0));
                    clipMemo.parcs     = reinterpret_cast<API_PolyArc**> (BMAllocateHandle (totalArcs * sizeof (API_PolyArc), ALLOCATE_CLEAR, 0));
                    clipMemo.vertexIDs = reinterpret_cast<UInt32**>      (BMAllocateHandle ((totalCoords + 1) * sizeof (UInt32), ALLOCATE_CLEAR, 0));
                    if (clipMemo.coords != nullptr && clipMemo.pends != nullptr && clipMemo.vertexIDs != nullptr) {
                        Int32 maxContourUnique = (Int32) coordinates.GetSize ();
                        for (const auto& hc : holeCoordsList) {
                            maxContourUnique = GS::Max (maxContourUnique, (Int32) hc.GetSize ());
                        }
                        (*clipMemo.coords)[0] = { 0.0, 0.0 };
                        (*clipMemo.vertexIDs)[0] = (UInt32) maxContourUnique;
                        (*clipMemo.pends)[0] = 0;

                        Int32 iCoord = 0;
                        Int32 iArc = 0;
                        Int32 subPolyIndex = 1;

                        auto writeContour = [&] (const GS::Array<GS::ObjectState>& contourCoords, const GS::Array<GS::ObjectState>& contourArcs) {
                            const Int32 firstIndex = iCoord + 1;
                            const Int32 nUniqueLocal = (Int32) contourCoords.GetSize ();
                            for (Int32 i = 0; i < nUniqueLocal; ++i) {
                                ++iCoord;
                                (*clipMemo.coords)[iCoord] = Get2DCoordinateFromObjectState (contourCoords[i]);
                                (*clipMemo.vertexIDs)[iCoord] = (UInt32) (i + 1); // per-contour, restarts at 1
                            }
                            ++iCoord;
                            (*clipMemo.coords)[iCoord] = (*clipMemo.coords)[firstIndex]; // closing duplicate
                            (*clipMemo.vertexIDs)[iCoord] = (*clipMemo.vertexIDs)[firstIndex];
                            (*clipMemo.pends)[subPolyIndex++] = iCoord;

                            if (clipMemo.parcs != nullptr) {
                                const GS::Array<API_PolyArc> localArcs = GetPolyArcs (contourArcs, firstIndex);
                                for (const API_PolyArc& a : localArcs)
                                    (*clipMemo.parcs)[iArc++] = a;
                            }
                        };

                        writeContour (coordinates, arcs);
                        for (Int32 i = 0; i < (Int32) holeCoordsList.GetSize (); ++i) {
                            writeContour (holeCoordsList[i], holeArcsList[i]);
                        }

                        elem.hatch.poly.nCoords   = totalCoords;
                        elem.hatch.poly.nSubPolys = nSubPolys;
                        elem.hatch.poly.nArcs     = totalArcs;
                        ACAPI_ELEMENT_MASK_SET (mask, API_HatchType, poly);
                        memoMask = APIMemoMask_Polygon;
                        hasMemoChanges = true;
                        hasElementChanges = true;
                    }
                }

                short contourPenIndex = 0;
                if (typeSpecificDetails->Get ("contourPenIndex", contourPenIndex)) {
                    elem.hatch.contPen.penIndex = contourPenIndex;
                    elem.hatch.contPen.colorOverridePenIndex = 0;
                    ACAPI_ELEMENT_MASK_SET (mask, API_HatchType, contPen);
                    hasElementChanges = true;
                }
                // A given pen index clears the matching RGB override so the pen actually shows;
                // a given color sets the override and wins over the pen.
                short fillPenIndex = 0;
                if (typeSpecificDetails->Get ("fillPenIndex", fillPenIndex)) {
                    elem.hatch.fillPen.penIndex = fillPenIndex;
                    elem.hatch.fillPen.colorOverridePenIndex = 0;
                    elem.hatch.hatchFlags &= ~APIHatch_HasFgRGBColor;
                    ACAPI_ELEMENT_MASK_SET (mask, API_HatchType, fillPen);
                    ACAPI_ELEMENT_MASK_SET (mask, API_HatchType, hatchFlags);
                    hasElementChanges = true;
                }
                if (typeSpecificDetails->Get ("fillBackgroundPenIndex", elem.hatch.fillBGPen)) {
                    elem.hatch.hatchFlags &= ~APIHatch_HasBkgRGBColor;
                    ACAPI_ELEMENT_MASK_SET (mask, API_HatchType, fillBGPen);
                    ACAPI_ELEMENT_MASK_SET (mask, API_HatchType, hatchFlags);
                    hasElementChanges = true;
                }
                if (GetColor (*typeSpecificDetails, "foregroundColor", elem.hatch.foregroundRGB)) {
                    elem.hatch.hatchFlags |= APIHatch_HasFgRGBColor;
                    ACAPI_ELEMENT_MASK_SET (mask, API_HatchType, foregroundRGB);
                    ACAPI_ELEMENT_MASK_SET (mask, API_HatchType, hatchFlags);
                    hasElementChanges = true;
                }
                if (GetColor (*typeSpecificDetails, "backgroundColor", elem.hatch.backgroundRGB)) {
                    elem.hatch.hatchFlags |= APIHatch_HasBkgRGBColor;
                    ACAPI_ELEMENT_MASK_SET (mask, API_HatchType, backgroundRGB);
                    ACAPI_ELEMENT_MASK_SET (mask, API_HatchType, hatchFlags);
                    hasElementChanges = true;
                }
                // The hatch type decides which of fillInd and buildingMaterial Archicad uses, so a given
                // attribute alone also switches the type. GetDetailsOfElements reports both, so when both are
                // given the type is kept and the details read can be written back unchanged.
                const GS::ObjectState* fillId = typeSpecificDetails->Get ("fillId");
                const GS::ObjectState* buildingMaterialId = typeSpecificDetails->Get ("buildingMaterialId");
                if (fillId != nullptr) {
                    elem.hatch.fillInd = GetAttributeIndexFromGuid (API_FilltypeID, GetGuidFromObjectState (*fillId));
                    ACAPI_ELEMENT_MASK_SET (mask, API_HatchType, fillInd);
                    if (buildingMaterialId == nullptr) {
                        elem.hatch.hatchType = API_FillHatch;
                        ACAPI_ELEMENT_MASK_SET (mask, API_HatchType, hatchType);
                    }
                    hasElementChanges = true;
                }
                if (buildingMaterialId != nullptr) {
                    elem.hatch.buildingMaterial = GetAttributeIndexFromGuid (API_BuildingMaterialID, GetGuidFromObjectState (*buildingMaterialId));
                    ACAPI_ELEMENT_MASK_SET (mask, API_HatchType, buildingMaterial);
                    if (fillId == nullptr) {
                        elem.hatch.hatchType = API_BuildingMaterialHatch;
                        ACAPI_ELEMENT_MASK_SET (mask, API_HatchType, hatchType);
                    }
                    hasElementChanges = true;
                }
                Int32 roomSpecial = 0;
                if (typeSpecificDetails->Get ("roomSpecial", roomSpecial)) {
                    elem.hatch.roomSpecial = (char) roomSpecial;
                    ACAPI_ELEMENT_MASK_SET (mask, API_HatchType, roomSpecial);
                    hasElementChanges = true;
                }
                if (typeSpecificDetails->Get ("showArea", elem.hatch.showArea)) {
                    ACAPI_ELEMENT_MASK_SET (mask, API_HatchType, showArea);
                    hasElementChanges = true;
                }
            }

            // The Element ID is a memo part of its own (elemInfoString), written separately
            // from the element change below so it never depends on the type specific mask.
            // It is written first, before anything else of the element is changed, so a
            // failed id write leaves the element untouched.
            GS::UniString id;
            const bool hasId = details->Get ("id", id);
            if (hasId) {
                err = ChangeElementId (elem.header.guid, id);
                if (err != NoError) {
                    ACAPI_DisposeElemMemoHdls (&clipMemo);
                    executionResults (CreateFailedExecutionResult (err, "Failed to set the id of the element, no other details were changed"));
                    continue;
                }
            }

            constexpr bool withDel = true;
            if (hasElementChanges || hasMemoChanges) {
                err = ACAPI_Element_Change (&elem, &mask,
                                            hasMemoChanges ? &clipMemo : nullptr,
                                            memoMask, withDel);
                ACAPI_DisposeElemMemoHdls (&clipMemo);
                if (err != NoError) {
                    executionResults (CreateFailedExecutionResult (err, hasId ? "Failed to change element, only its id was set" : "Failed to change element"));
                    continue;
                }
            }

            if (drwIndexTarget >= 0) {
                GS::Array<API_Guid> guids = { elem.header.guid };

                if (IsOpeningType (elem.header) && drwIndexTarget > 7) {
                    // Openings (Door/Window/Skylight) can only reach levels above 7 by
                    // temporarily elevating their host wall/roof step by step, then
                    // bringing it back the same way (BringToFront is unreliable on walls).
                    const API_Guid* hostGuidPtr = openingToHost.GetPtr (elem.header.guid);
                    if (hostGuidPtr != nullptr) {
                        API_Element hostElem = {};
                        hostElem.header.guid = *hostGuidPtr;
                        if (ACAPI_Element_Get (&hostElem) == NoError) {
                            const Int32 hostOriginal = (Int32) hostElem.header.drwIndex;
                            const Int32 target       = GS::Max (8, GS::Min ((Int32) drwIndexTarget, 14));
                            const Int32 steps        = target - hostOriginal;
                            GS::Array<API_Guid> hostGuids = { *hostGuidPtr };
                            if (steps > 0) {
                                // Step host up — opening follows
                                for (Int32 i = 0; i < steps; ++i)
                                    ACAPI_Grouping_Tool (hostGuids, APITool_BringForward, nullptr);
                                // Step host back down — opening stays elevated
                                for (Int32 i = 0; i < steps; ++i)
                                    ACAPI_Grouping_Tool (hostGuids, APITool_SendBackward, nullptr);
                            }
                        }
                    }
                } else if (drwIndexTarget == 0 && IsOpeningType (elem.header)) {
                    // Reset elevated opening via its host (direct ResetOrder on opening doesn't work)
                    const API_Guid* hostGuidPtr = openingToHost.GetPtr (elem.header.guid);
                    if (hostGuidPtr != nullptr) {
                        GS::Array<API_Guid> hostGuids = { *hostGuidPtr };
                        ACAPI_Grouping_Tool (hostGuids, APITool_ResetOrder, nullptr);
                    }
                } else if (drwIndexTarget == 0 && IsResetOrderReliable (elem.header)) {
                    ACAPI_Grouping_Tool (guids, APITool_ResetOrder, nullptr);
                } else if (drwIndexTarget != 0 || !IsResetOrderReliable (elem.header)) {
                    const short resolvedTarget = (drwIndexTarget == 0) ? (short) GetDefaultDrwIndex (elem.header) : drwIndexTarget;
                    ApplyDrawIndexStrategy (guids, elem.header, (Int32) resolvedTarget, (Int32) elem.header.drwIndex);
                }
            }

            // Reported live (#1476): ACAPI_Element_Change can return NoError on a placed
            // Drawing and still keep its old drawingScale, while the rest of the change
            // (ratio, name, ...) is applied. Read it back so an ignored scale is not
            // reported as a success.
            if (requestedDrawingScale.HasValue ()) {
                API_Element changedElem = {};
                changedElem.header.guid = elem.header.guid;
                if (ACAPI_Element_Get (&changedElem) == NoError &&
                    GS::Abs (changedElem.drawing.drawingScale - *requestedDrawingScale) > 1e-9 * GS::Max (1.0, GS::Abs (*requestedDrawingScale))) {
                    executionResults (CreateFailedExecutionResult (APIERR_GENERAL, GS::UniString::Printf (
                        "Archicad did not apply drawingScale: it stayed %g instead of %g. The other details given were applied.",
                        changedElem.drawing.drawingScale, *requestedDrawingScale)));
                    continue;
                }
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

GS::Optional<GS::UniString> GetSelectedElementsCommand::GetRawResponseSchema () const
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
        elementsList (CreateElementIdObjectState (GetParentElemOfSectElem (selectedNeig.guid)));
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

GS::Optional<GS::UniString> ChangeSelectionOfElementsCommand::GetRawResponseSchema () const
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

GS::Optional<GS::UniString> GetSubelementsOfHierarchicalElementsCommand::GetRawResponseSchema () const
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
                        },
                        "externalSubelements": {
                            "$ref": "#/Elements",
                            "description": "The subelements of an external hierarchical element (for example an MEP element or an element of another add-on). Available from Archicad 28."
                        }
                    },
                    "additionalProperties": false,
                    "required": []
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

#ifdef ServerMainVers_2800
        API_Elem_Head header = {};
        header.guid = elemGuid;
        if (ACAPI_Element_GetHeader (&header) == NoError && GetElemTypeId (header) == API_ExternalElemID) {
            std::vector<API_SubElement> externalSubelements;
            const GS::OnExit externalGuard ([&externalSubelements] () {
                for (API_SubElement& externalSubelement : externalSubelements) {
                    ACAPI_DisposeElemMemoHdls (&externalSubelement.memo);
                }
            });
            const GSErrCode err = ACAPI_Element_GetMemo_ExternalHierarchical (elemGuid, externalSubelements);
            if (err != NoError) {
                subelementsOfHierarchicalElements (CreateErrorResponse (err, "Failed to get the subelements"));
                continue;
            }

            GS::ObjectState subelements;
            if (!externalSubelements.empty ()) {
                const auto& externalSubelementIds = subelements.AddList<GS::ObjectState> ("externalSubelements");
                for (const API_SubElement& externalSubelement : externalSubelements) {
                    externalSubelementIds (CreateElementIdObjectState (externalSubelement.subElem.header.guid));
                }
            }
            subelementsOfHierarchicalElements (subelements);
            continue;
        }
#endif

        API_ElementMemo memo = {};
        const GS::OnExit guard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
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

#undef AddSubelements

        subelementsOfHierarchicalElements (subelements);
    }

    return response;
}

EditHierarchicalElementCommand::EditHierarchicalElementCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String EditHierarchicalElementCommand::GetName () const
{
    return "EditHierarchicalElement";
}

GS::Optional<GS::UniString> EditHierarchicalElementCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementId": {
                "$ref": "#/ElementId",
                "description": "The hierarchical element (Curtain Wall, Stair, Railing, Beam or Column) to open in hierarchical editing mode."
            }
        },
        "additionalProperties": false,
        "required": [
            "elementId"
        ]
    })";
}

GS::Optional<GS::UniString> EditHierarchicalElementCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState EditHierarchicalElementCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#if defined (ServerMainVers_2700)
    const GS::ObjectState* elementId = parameters.Get ("elementId");
    if (elementId == nullptr) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing");
    }

    const API_Guid elemGuid = GetGuidFromObjectState (*elementId);
    if (elemGuid == APINULLGuid) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "elementId is corrupt or missing");
    }

    // A complete operation: it must not run inside ACAPI_CallUndoableCommand, so it is called directly.
    const GSErrCode err = ACAPI_HierarchicalEditing_EditHierarchicalElem (&elemGuid);
    if (err == APIERR_REFUSEDCMD) {
        return CreateFailedExecutionResult (err, "Archicad refused to enter the hierarchical editing mode; it may already be in an editing mode. Finish it first with FinishHierarchicalEditing.");
    }
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to open the element in hierarchical editing mode; it must be a Curtain Wall, Stair, Railing, Beam or Column.");
    }

    return CreateSuccessfulExecutionResult ();
#else
    UNUSED_PARAMETER (parameters);
    return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "EditHierarchicalElement requires Archicad 27 or later.");
#endif
}

FinishHierarchicalEditingCommand::FinishHierarchicalEditingCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String FinishHierarchicalEditingCommand::GetName () const
{
    return "FinishHierarchicalEditing";
}

GS::Optional<GS::UniString> FinishHierarchicalEditingCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "accept": {
                "type": "boolean",
                "description": "True to accept the changes made in the hierarchical editing mode (OK), false to discard them (Cancel)."
            }
        },
        "additionalProperties": false,
        "required": [
            "accept"
        ]
    })";
}

GS::Optional<GS::UniString> FinishHierarchicalEditingCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState FinishHierarchicalEditingCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#if defined (ServerMainVers_2700)
    // Required, never defaulted: the changes are only discarded when the caller asks for it.
    bool accept = false;
    if (!parameters.Get ("accept", accept)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Missing parameter: accept.");
    }

    // Complete operations: they must not run inside ACAPI_CallUndoableCommand, so they are called directly.
    const GSErrCode err = accept
        ? ACAPI_HierarchicalEditing_OkHierarchicalElem ()
        : ACAPI_HierarchicalEditing_CancelHierarchicalElem ();
    if (err == APIERR_REFUSEDCMD) {
        return CreateFailedExecutionResult (err, "Archicad is not in a hierarchical editing mode that can be finished.");
    }
    if (err != NoError) {
        return CreateFailedExecutionResult (err, accept ? "Failed to accept the hierarchical editing." : "Failed to cancel the hierarchical editing.");
    }

    return CreateSuccessfulExecutionResult ();
#else
    UNUSED_PARAMETER (parameters);
    return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "FinishHierarchicalEditing requires Archicad 27 or later.");
#endif
}

GetHierarchicalElementOwnersCommand::GetHierarchicalElementOwnersCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetHierarchicalElementOwnersCommand::GetName () const
{
    return "GetHierarchicalElementOwners";
}

GS::Optional<GS::UniString> GetHierarchicalElementOwnersCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements",
                "description": "The elements whose owners are looked up, e.g. Curtain Wall frames or panels, Stair treads or Railing posts."
            },
            "ownerMode": {
                "type": "string",
                "enum": ["root", "parent"],
                "description": "root (default): the root of the hierarchical element tree, e.g. the Curtain Wall, Stair or Railing. parent: the immediate parent, which may itself be a subelement."
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> GetHierarchicalElementOwnersCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "owners": {
                "type": "array",
                "description": "The owner of each element, aligned with the input.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "ownerElementId": {
                                    "$ref": "#/ElementId",
                                    "description": "The owner element. For a main element (e.g. a Curtain Wall) or an element that is not part of a hierarchical element, Archicad returns the input element itself."
                                },
                                "hierarchicalElementType": {
                                    "type": "string",
                                    "enum": ["MainElement", "ChildElement", "SingleElement"],
                                    "description": "The classification of the input element (not of its owner): MainElement is the main element of a hierarchical element (e.g. a Curtain Wall, Stair or Railing), ChildElement is a subelement of one (e.g. a frame or a tread), SingleElement is not part of a hierarchical element."
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "ownerElementId",
                                "hierarchicalElementType"
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
            "owners"
        ]
    })";
}

static GS::Optional<GS::UniString> GetHierarchicalElemTypeName (API_HierarchicalElemType hierarchicalElemType)
{
    switch (hierarchicalElemType) {
        case API_MainElemInMultipleElem:    return GS::UniString ("MainElement");
        case API_ChildElemInMultipleElem:   return GS::UniString ("ChildElement");
        case API_SingleElem:                return GS::UniString ("SingleElement");
        default:                            return {};
    }
}

GS::ObjectState GetHierarchicalElementOwnersCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    API_HierarchicalOwnerType ownerType = API_RootHierarchicalOwner;
    GS::UniString ownerModeStr;
    if (parameters.Get ("ownerMode", ownerModeStr)) {
        if (ownerModeStr == "parent") {
            ownerType = API_ParentHierarchicalOwner;
        } else if (ownerModeStr != "root") {
            return CreateErrorResponse (APIERR_BADPARS,
                GS::UniString::Printf ("Invalid ownerMode '%T'.", ownerModeStr.ToPrintf ()));
        }
    }

    GS::ObjectState response;
    const auto& owners = response.AddList<GS::ObjectState> ("owners");

    for (const GS::ObjectState& elementOS : elements) {
        const GS::ObjectState* elementId = elementOS.Get ("elementId");
        if (elementId == nullptr) {
            owners (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        const API_Guid elemGuid = GetGuidFromObjectState (*elementId);

        API_HierarchicalElemType hierarchicalElemType = API_UnknownElemType;
        API_Guid ownerGuid = APINULLGuid;
        const GSErrCode err = ACAPI_HierarchicalEditing_GetHierarchicalElementOwner (&elemGuid, &ownerType, &hierarchicalElemType, &ownerGuid);
        if (err != NoError) {
            owners (CreateErrorResponse (err, "Failed to get the owner of the element"));
            continue;
        }

        const GS::Optional<GS::UniString> hierarchicalElemTypeName = GetHierarchicalElemTypeName (hierarchicalElemType);
        if (!hierarchicalElemTypeName.HasValue () || ownerGuid == APINULLGuid) {
            owners (CreateErrorResponse (APIERR_BADID, "Failed to get the owner of the element"));
            continue;
        }

        GS::ObjectState ownerOS;
        ownerOS.Add ("ownerElementId", CreateGuidObjectState (ownerGuid));
        ownerOS.Add ("hierarchicalElementType", *hierarchicalElemTypeName);
        owners (ownerOS);
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
                "$ref": "#/Elements",
                "description": "The owner (host) elements whose connected elements are collected, e.g. Walls, Curtain Walls, Stairs or Railings."
            },
            "connectedElementType": {
                "$ref": "#/ElementType",
                "description": "The type of the connected elements to collect, e.g. Window or Door for a Wall owner, or a subelement type of a Curtain Wall, Stair or Railing owner."
            }
        },
        "additionalProperties": false,
        "required": [
            "elements",
            "connectedElementType"
        ]
    })";
}

GS::Optional<GS::UniString> GetConnectedElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ConnectedElementsOrError"
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
        if (elemType == API_ZombieElemID) {
            return CreateErrorResponse (APIERR_BADPARS,
                GS::UniString::Printf ("Invalid connectedElementType '%T'.", elementTypeStr.ToPrintf ()));
        }
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

GetRelationsOfElementsCommand::GetRelationsOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetRelationsOfElementsCommand::GetName () const
{
    return "GetRelationsOfElements";
}

GS::Optional<GS::UniString> GetRelationsOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            },
            "otherElementType": {
                "$ref": "#/ElementType",
                "description": "Optional filter: only relations to elements of this type are returned."
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> GetRelationsOfElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "relations": {
                "type": "array",
                "description": "Type-specific relations of each element, aligned with the input.",
                "items": {
                    "$ref": "#/ElementRelationsOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "relations"
        ]
    })";
}

#ifdef ServerMainVers_2600

static GS::ObjectState ConvertConnectionGuidItem (const API_ConnectionGuidItem& item)
{
    GS::ObjectState itemOS;
    itemOS.Add ("elementId", CreateGuidObjectState (item.guid));
    itemOS.Add ("connectedWithBeginPoint", item.conWithBeg);
    return itemOS;
}

static void AddConnectionList (GS::ObjectState& os, const char* fieldName, const API_ConnectionGuidItem* const* itemsHandle, Int32 itemCount)
{
    const auto& list = os.AddList<GS::ObjectState> (fieldName);
    if (itemsHandle == nullptr || *itemsHandle == nullptr) {
        return;
    }
    for (Int32 i = 0; i < itemCount; ++i) {
        list (ConvertConnectionGuidItem ((*itemsHandle)[i]));
    }
}

static void AddConnectionList (GS::ObjectState& os, const char* fieldName, const GS::Array<API_ConnectionGuidItem>* items)
{
    const auto& list = os.AddList<GS::ObjectState> (fieldName);
    if (items == nullptr) {
        return;
    }
    for (const API_ConnectionGuidItem& item : *items) {
        list (ConvertConnectionGuidItem (item));
    }
}

static GS::ObjectState ConvertZoneBoundaryPart (const API_WallPart& part, bool addRoomEdgeIndex)
{
    GS::ObjectState partOS;
    partOS.Add ("elementId", CreateGuidObjectState (part.guid));
    if (addRoomEdgeIndex) {
        partOS.Add ("roomEdgeIndex", part.roomEdge);
    }
    partOS.Add ("begDistance", part.tBeg);
    partOS.Add ("endDistance", part.tEnd);
    return partOS;
}

static GS::ObjectState GetWallRelations (const API_Guid& guid, const API_ElemType& otherType)
{
    API_WallRelation relation = {};
    const GSErrCode err = ACAPI_Element_GetRelations (guid, otherType, &relation);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the relations of the element.");
    }

    GS::ObjectState connections;
    AddConnectionList (connections, "connectedToBeginPoint", relation.conBeg, relation.nConBeg);
    AddConnectionList (connections, "connectedToEndPoint", relation.conEnd, relation.nConEnd);
    AddConnectionList (connections, "connectedWithReferenceLineToEndPoints", relation.conRef, relation.nConRef);
    AddConnectionList (connections, "connectedToReferenceLine", relation.con, relation.nCon);
    AddConnectionList (connections, "crossingReferenceLine", relation.conX, relation.nConX);
    ACAPI_DisposeWallRelationHdls (&relation);

    return GS::ObjectState ("wallConnections", connections);
}

static GS::ObjectState GetBeamRelations (const API_Guid& guid, const API_ElemType& otherType)
{
    API_BeamRelation relation = {};
    const GSErrCode err = ACAPI_Element_GetRelations (guid, otherType, &relation);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the relations of the element.");
    }

    GS::ObjectState connections;
    AddConnectionList (connections, "connectedToBeginPoint", relation.conBeg);
    AddConnectionList (connections, "connectedToEndPoint", relation.conEnd);
    AddConnectionList (connections, "connectedWithReferenceLineToEndPoints", relation.conRef);
    AddConnectionList (connections, "connectedToReferenceLine", relation.con);
    AddConnectionList (connections, "crossingReferenceLine", relation.conX);
    ACAPI_DisposeBeamRelationHdls (&relation);

    return GS::ObjectState ("beamConnections", connections);
}

static GS::ObjectState GetBeamSegmentRelations (const API_Guid& guid, const API_ElemType& otherType)
{
    API_BeamSegmentRelation relation = {};
    const GSErrCode err = ACAPI_Element_GetRelations (guid, otherType, &relation);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the relations of the element.");
    }

    GS::ObjectState connections;
    AddConnectionList (connections, "connectedToBeginPoint", relation.conBeg);
    AddConnectionList (connections, "connectedToEndPoint", relation.conEnd);
    AddConnectionList (connections, "connectedWithReferenceLineToEndPoints", relation.conRef);
    AddConnectionList (connections, "connectedToReferenceLine", relation.con);
    AddConnectionList (connections, "crossingReferenceLine", relation.conX);
    ACAPI_DisposeBeamSegmentRelationHdls (&relation);

    return GS::ObjectState ("beamSegmentConnections", connections);
}

static GS::ObjectState GetZoneRelations (const API_Guid& guid, const API_ElemType& otherType)
{
    API_RoomRelation relation = {};
    const GSErrCode err = ACAPI_Element_GetRelations (guid, otherType, &relation);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the relations of the element.");
    }

    GS::ObjectState zoneRelations;
    const auto& elementsGroupedByType = zoneRelations.AddList<GS::ObjectState> ("elementsGroupedByType");
    relation.elementsGroupedByType.Enumerate ([&] (const API_ElemType& elemType, const GS::Array<API_Guid>& elemGuids) {
        GS::ObjectState elementsOfTypeOS;
        elementsOfTypeOS.Add ("elementType", GetElementTypeNonLocalizedName (elemType.typeID));
        const auto& elementsOS = elementsOfTypeOS.AddList<GS::ObjectState> ("elements");
        for (const API_Guid& elemGuid : elemGuids) {
            elementsOS (CreateElementIdObjectState (elemGuid));
        }
        elementsGroupedByType (elementsOfTypeOS);
    });

    const auto& wallParts = zoneRelations.AddList<GS::ObjectState> ("wallParts");
    for (const API_WallPart& part : relation.wallPart) {
        wallParts (ConvertZoneBoundaryPart (part, true));
    }

    const auto& beamParts = zoneRelations.AddList<GS::ObjectState> ("beamParts");
    for (const API_BeamPart& part : relation.beamPart) {
        GS::ObjectState partOS;
        partOS.Add ("elementId", CreateGuidObjectState (part.guid));
        partOS.Add ("begDistance", part.tBeg);
        partOS.Add ("endDistance", part.tEnd);
        beamParts (partOS);
    }

    const auto& curtainWallSegmentParts = zoneRelations.AddList<GS::ObjectState> ("curtainWallSegmentParts");
    for (const API_CWSegmentPart& part : relation.cwSegmentPart) {
        curtainWallSegmentParts (ConvertZoneBoundaryPart (part, true));
    }

    ACAPI_DisposeRoomRelationHdls (&relation);

    return GS::ObjectState ("zoneRelations", zoneRelations);
}

static GS::ObjectState GetOpeningRelations (const API_Guid& guid, const API_ElemType& otherType)
{
    API_CWPanelRelation relation = {};
    const GSErrCode err = ACAPI_Element_GetRelations (guid, otherType, &relation);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the relations of the element.");
    }

    GS::ObjectState openingRelations;
    if (relation.fromRoom != APINULLGuid) {
        openingRelations.Add ("fromRoom", CreateGuidObjectState (relation.fromRoom));
    }
    if (relation.toRoom != APINULLGuid) {
        openingRelations.Add ("toRoom", CreateGuidObjectState (relation.toRoom));
    }

    return GS::ObjectState ("openingRelations", openingRelations);
}

static GS::ObjectState GetRoofOrShellRelations (const API_Guid& guid, const API_ElemType& otherType)
{
    API_RoofRelation relation = {};
    // The rooms array is provided by the caller; if the API replaces the
    // pointer with its own allocation, that one is released below.
    GS::Array<API_Guid> roomGuids;
    relation.rooms = &roomGuids;
    const GSErrCode err = ACAPI_Element_GetRelations (guid, otherType, &relation);
    if (err != NoError) {
        if (relation.rooms != nullptr && relation.rooms != &roomGuids) {
            delete relation.rooms;
        }
        return CreateErrorResponse (err, "Failed to get the relations of the element.");
    }

    GS::ObjectState roofOrShellRelations;
    const auto& connectedRooms = roofOrShellRelations.AddList<GS::ObjectState> ("connectedRooms");
    if (relation.rooms != nullptr) {
        for (const API_Guid& roomGuid : *relation.rooms) {
            connectedRooms (CreateElementIdObjectState (roomGuid));
        }
    }
    if (relation.rooms != nullptr && relation.rooms != &roomGuids) {
        delete relation.rooms;
    }

    return GS::ObjectState ("roofOrShellRelations", roofOrShellRelations);
}

#endif

GS::ObjectState GetRelationsOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2600
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    API_ElemType otherType = API_ZombieElemID;
    GS::UniString otherTypeStr;
    if (parameters.Get ("otherElementType", otherTypeStr)) {
        otherType = GetElementTypeFromNonLocalizedName (otherTypeStr);
        if (otherType == API_ZombieElemID) {
            return CreateErrorResponse (APIERR_BADPARS,
                GS::UniString::Printf ("Invalid otherElementType '%T'.", otherTypeStr.ToPrintf ()));
        }
    }

    GS::ObjectState response;
    const auto& relations = response.AddList<GS::ObjectState> ("relations");

    for (const GS::ObjectState& elementOS : elements) {
        const GS::ObjectState* elementId = elementOS.Get ("elementId");
        if (elementId == nullptr) {
            relations (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        API_Elem_Head elemHead = {};
        elemHead.guid = GetGuidFromObjectState (*elementId);
        if (ACAPI_Element_GetHeader (&elemHead) != NoError) {
            relations (CreateErrorResponse (APIERR_BADID, "Failed to find the element."));
            continue;
        }

        switch (elemHead.type.typeID) {
            case API_WallID:
                relations (GetWallRelations (elemHead.guid, otherType));
                break;
            case API_BeamID:
                relations (GetBeamRelations (elemHead.guid, otherType));
                break;
            case API_BeamSegmentID:
                relations (GetBeamSegmentRelations (elemHead.guid, otherType));
                break;
            case API_ZoneID:
                relations (GetZoneRelations (elemHead.guid, otherType));
                break;
            case API_CurtainWallPanelID:
            case API_SkylightID:
            case API_WindowID:
            case API_DoorID:
                relations (GetOpeningRelations (elemHead.guid, otherType));
                break;
            case API_RoofID:
            case API_ShellID:
                relations (GetRoofOrShellRelations (elemHead.guid, otherType));
                break;
            default:
                relations (CreateErrorResponse (APIERR_BADPARS,
                    GS::UniString::Printf ("Element type '%T' is not supported. Supported types: Wall, Beam, BeamSegment, Zone, CurtainWallPanel, Skylight, Window, Door, Roof, Shell.",
                        GetElementTypeNonLocalizedName (elemHead.type.typeID).ToPrintf ())));
                break;
        }
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "GetRelationsOfElements requires Archicad 26 or newer.");
#endif
}

// ---------------------------------------------------------------------------
// Element links (LinkElements, UnlinkElements, GetLinksOfElements)
// ---------------------------------------------------------------------------

// Reads the two ends of a link item and checks that both elements exist.
// Returns false with a FailedExecutionResult in failedResult otherwise.
static bool GetElementLinkEnds (const GS::ObjectState& linkOS, API_Guid& fromGuid, API_Guid& toGuid, GS::ObjectState& failedResult)
{
    GS::ObjectState fromIdOS, toIdOS;
    if (!linkOS.Get ("fromElementId", fromIdOS) || !linkOS.Get ("toElementId", toIdOS)) {
        failedResult = CreateFailedExecutionResult (APIERR_BADPARS, "fromElementId or toElementId is missing.");
        return false;
    }

    fromGuid = GetGuidFromObjectState (fromIdOS);
    toGuid = GetGuidFromObjectState (toIdOS);

    API_Elem_Head fromHead = {};
    fromHead.guid = fromGuid;
    if (ACAPI_Element_GetHeader (&fromHead) != NoError) {
        failedResult = CreateFailedExecutionResult (APIERR_BADID, "Failed to find the element given as fromElementId.");
        return false;
    }

    API_Elem_Head toHead = {};
    toHead.guid = toGuid;
    if (ACAPI_Element_GetHeader (&toHead) != NoError) {
        failedResult = CreateFailedExecutionResult (APIERR_BADID, "Failed to find the element given as toElementId.");
        return false;
    }

    return true;
}

LinkElementsCommand::LinkElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String LinkElementsCommand::GetName () const
{
    return "LinkElements";
}

GS::Optional<GS::UniString> LinkElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "links": {
                "type": "array",
                "description": "The links to create, each from one element to another.",
                "items": {
                    "$ref": "#/ElementLinkData"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "links"
        ]
    })";
}

GS::Optional<GS::UniString> LinkElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults",
                "description": "One result per link, aligned with the input."
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    })";
}

GS::ObjectState LinkElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> links;
    parameters.Get ("links", links);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("LinkElements", [&] () -> GSErrCode {
        for (const GS::ObjectState& linkOS : links) {
            API_Guid fromGuid = APINULLGuid;
            API_Guid toGuid = APINULLGuid;
            GS::ObjectState failedResult;
            if (!GetElementLinkEnds (linkOS, fromGuid, toGuid, failedResult)) {
                executionResults (failedResult);
                continue;
            }

            Int32 flags = 0;
            linkOS.Get ("flags", flags);

            const GSErrCode err = ACAPI_ElementLink_Link (fromGuid, toGuid, static_cast<GSFlags> (flags));
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to link the elements."));
            } else {
                executionResults (CreateSuccessfulExecutionResult ());
            }
        }
        return NoError;
    });

    return response;
}

UnlinkElementsCommand::UnlinkElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String UnlinkElementsCommand::GetName () const
{
    return "UnlinkElements";
}

GS::Optional<GS::UniString> UnlinkElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "links": {
                "type": "array",
                "description": "The links to remove, each from one element to another.",
                "items": {
                    "$ref": "#/ElementLinkReference"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "links"
        ]
    })";
}

GS::Optional<GS::UniString> UnlinkElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults",
                "description": "One result per link, aligned with the input."
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    })";
}

GS::ObjectState UnlinkElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> links;
    parameters.Get ("links", links);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("UnlinkElements", [&] () -> GSErrCode {
        for (const GS::ObjectState& linkOS : links) {
            API_Guid fromGuid = APINULLGuid;
            API_Guid toGuid = APINULLGuid;
            GS::ObjectState failedResult;
            if (!GetElementLinkEnds (linkOS, fromGuid, toGuid, failedResult)) {
                executionResults (failedResult);
                continue;
            }

            const GSErrCode err = ACAPI_ElementLink_Unlink (fromGuid, toGuid);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to unlink the elements."));
            } else {
                executionResults (CreateSuccessfulExecutionResult ());
            }
        }
        return NoError;
    });

    return response;
}

GetLinksOfElementsCommand::GetLinksOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetLinksOfElementsCommand::GetName () const
{
    return "GetLinksOfElements";
}

GS::Optional<GS::UniString> GetLinksOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements",
                "description": "The elements whose outgoing links are returned."
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> GetLinksOfElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "linksOfElements": {
                "type": "array",
                "description": "The links starting from each element, aligned with the input.",
                "items": {
                    "$ref": "#/ElementLinksOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "linksOfElements"
        ]
    })";
}

GS::ObjectState GetLinksOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& linksOfElements = response.AddList<GS::ObjectState> ("linksOfElements");

    for (const GS::ObjectState& elementOS : elements) {
        API_Elem_Head elemHead = {};
        elemHead.guid = GetGuidFromElementsArrayItem (elementOS);
        if (ACAPI_Element_GetHeader (&elemHead) != NoError) {
            linksOfElements (CreateErrorResponse (APIERR_BADID, "Failed to find the element."));
            continue;
        }

        GS::Array<API_Guid> linkedGuids;
        const GSErrCode err = ACAPI_ElementLink_GetLinks (elemHead.guid, &linkedGuids);
        if (err != NoError) {
            linksOfElements (CreateErrorResponse (err, "Failed to get the links of the element."));
            continue;
        }

        GS::ObjectState elementLinksOS;
        const auto& linkList = elementLinksOS.AddList<GS::ObjectState> ("links");
        for (const API_Guid& linkedGuid : linkedGuids) {
            GSFlags flags = 0;
            ACAPI_ElementLink_GetLinkFlags (elemHead.guid, linkedGuid, &flags);

            GS::ObjectState linkOS;
            linkOS.Add ("elementId", CreateGuidObjectState (linkedGuid));
            linkOS.Add ("flags", static_cast<Int32> (flags));
            linkList (linkOS);
        }
        linksOfElements (elementLinksOS);
    }

    return response;
}

GetZoneBoundariesCommand::GetZoneBoundariesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetZoneBoundariesCommand::GetName () const
{
    return "GetZoneBoundaries";
}

GS::Optional<GS::UniString> GetZoneBoundariesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "zoneElementId": {
                "$ref": "#/ElementId",
                "description": "The identifier of a single Zone. Prefer the zones array: querying many Zones in one call is much faster than one call per Zone."
            },
            "zones": {
                "$ref": "#/Elements",
                "description": "A list of Zones. Only one of zoneElementId and zones can be given."
            }
        },
        "additionalProperties": false,
        "required": [
        ]
    })";
}

GS::Optional<GS::UniString> GetZoneBoundariesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "oneOf": [
            {
                "$ref": "#/ZoneBoundariesOrError"
            },
            {
                "$ref": "#/ZoneBoundariesOfZonesWrapper"
            }
        ]
    })";
}

#ifdef ServerMainVers_2800

static GS::ObjectState GetBoundariesOfZone (ACAPI::ZoneBoundaryQuery& query, const API_Guid& zoneGuid)
{
    const ACAPI::Result<std::vector<ACAPI::ZoneBoundary>> boundaries = query.GetZoneBoundaries (zoneGuid);

    if (boundaries.IsErr ()) {
        return CreateErrorResponse (boundaries.UnwrapErr ().kind, "Failed to get zone boundary");
    }

    GS::ObjectState zoneBoundariesOS;
    const auto& zoneBoundaries = zoneBoundariesOS.AddList<GS::ObjectState> ("zoneBoundaries");

    for (const ACAPI::ZoneBoundary& boundary : boundaries.Unwrap ()) {
        GS::ObjectState boundaryOS;
        boundaryOS.Add ("connectedElementId", CreateGuidObjectState (boundary.GetElemId ()));
        boundaryOS.Add ("isExternal", boundary.IsExternal ());
        boundaryOS.Add ("neighbouringZoneElementId", CreateGuidObjectState (boundary.GetNeighbouringZoneId ()));
        boundaryOS.Add ("area", boundary.GetArea ());

        const auto& polygonOutline = boundaryOS.AddList<GS::ObjectState> ("polygonOutline");
        const ModelerAPI::MeshBody& body = boundary.GetBody ();
        const ModelerAPI::Polygon& poly = boundary.GetPolygon ();
        {
            ModelerAPI::Edge edge;
            ModelerAPI::Vertex vertex;
            for (Int32 edgeIdx = 1; edgeIdx <= poly.GetEdgeCount (); ++edgeIdx) {

                const Int32 edgeIndex = poly.GetEdgeIndex (edgeIdx);

                if (edgeIndex == 0) {
                    body.GetVertex (edge.GetVertexIndex2 (), &vertex);
                    polygonOutline (Create3DCoordinateObjectState (*reinterpret_cast<API_Coord3D*> (&vertex)));
                    break;
                }
                
                body.GetEdge (edgeIndex, &edge);
                body.GetVertex (edge.GetVertexIndex1 (), &vertex);

                polygonOutline (Create3DCoordinateObjectState (*reinterpret_cast<API_Coord3D*> (&vertex)));

                if (edgeIdx == poly.GetEdgeCount ()) {
                    body.GetVertex (edge.GetVertexIndex2 (), &vertex);
                    polygonOutline (Create3DCoordinateObjectState (*reinterpret_cast<API_Coord3D*> (&vertex)));
                }
            }
        }

        zoneBoundaries (boundaryOS);
    }

    return zoneBoundariesOS;
}

#endif

GS::ObjectState GetZoneBoundariesCommand::Execute (
    const GS::ObjectState& parameters,
#ifdef ServerMainVers_2800
    GS::ProcessControl& processControl) const
#else
    GS::ProcessControl& /*processControl*/) const
#endif
{
    const GS::ObjectState* zoneElementId = parameters.Get ("zoneElementId");
    GS::Array<GS::ObjectState> zones;
    const bool zonesGiven = parameters.Get ("zones", zones);

    if (zoneElementId == nullptr && !zonesGiven) {
        return CreateErrorResponse (APIERR_BADPARS, "One of zoneElementId and zones is required");
    }

    if (zoneElementId != nullptr && zonesGiven) {
        return CreateErrorResponse (APIERR_BADPARS, "Only one of zoneElementId and zones can be given");
    }

#ifdef ServerMainVers_2800
    ACAPI::ZoneBoundaryQuery query = ACAPI::CreateZoneBoundaryQuery ();

    ACAPI::Result updateResult = query.Modify (
        [&] (ACAPI::ZoneBoundaryQuery::Modifier& modifier) -> GSErrCode {
            ACAPI::Result<void> result = modifier.Update (processControl);
            return result.IsOk () ? NoError : result.UnwrapErr ().kind;
        }
    );

    if (updateResult.IsErr ()) {
        return CreateErrorResponse (updateResult.UnwrapErr ().kind, "Failed to execute zone boundary query");
    }

    if (zoneElementId != nullptr) {
        return GetBoundariesOfZone (query, GetGuidFromObjectState (*zoneElementId));
    }

    GS::ObjectState response;
    const auto& zoneBoundariesOfZones = response.AddList<GS::ObjectState> ("zoneBoundariesOfZones");

    for (const GS::ObjectState& zone : zones) {
        const GS::ObjectState* elementId = zone.Get ("elementId");
        if (elementId == nullptr) {
            zoneBoundariesOfZones (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        zoneBoundariesOfZones (GetBoundariesOfZone (query, GetGuidFromObjectState (*elementId)));
    }

    return response;
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command is only supported in ArchiCAD 28 or later.");
#endif
}

GetZoneReductionsCommand::GetZoneReductionsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetZoneReductionsCommand::GetName () const
{
    return "GetZoneReductions";
}

GS::Optional<GS::UniString> GetZoneReductionsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements",
                "description": "A list of Zones."
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> GetZoneReductionsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "reductionsOfZones": {
                "type": "array",
                "description": "The area reductions of each Zone, aligned with the input array of elements, or an error.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "reductions": {
                                    "type": "array",
                                    "description": "The reduction polygons of the Zone, as its Zone Stamp's area calculation uses them: the parts of the Zone polygon covered by Walls, Columns and reducing Fills, and the parts under a low ceiling (for example under a sloping Roof), each with the percentage it is reduced by and its area. Empty when nothing reduces the Zone's area.",
                                    "items": {
                                        "type": "object",
                                        "properties": {
                                            "type": {
                                                "type": "string",
                                                "description": "What reduces the area in the polygon, as the DevKit's APIRoomReduction type of the polygon: Wall, Column, Hatch (a Fill set to reduce the Zone area), Gable (a low ceiling, for example the part under a sloping Roof below the low ceiling height), or Rest (the remaining polygon of the Zone). Unknown for a value this version of Tapir does not name.",
                                                "enum": [
                                                    "Rest",
                                                    "Wall",
                                                    "Column",
                                                    "Hatch",
                                                    "Gable",
                                                    "Unknown"
                                                ]
                                            },
                                            "percent": {
                                                "type": "number",
                                                "description": "The percentage by which the area of the polygon reduces the Zone area, as the DevKit reports it."
                                            },
                                            "area": {
                                                "type": "number",
                                                "description": "The area of the polygon in square meters."
                                            },
                                            "polygonOutline": {
                                                "type": "array",
                                                "description": "The 2D coordinates of the outline of the polygon in meters; the last coordinate repeats the first.",
                                                "items": {
                                                    "$ref": "#/Coordinate2D"
                                                }
                                            },
                                            "polygonArcs": {
                                                "type": "array",
                                                "description": "The arcs of the outline, indices into polygonOutline. Absent when the outline has no arc.",
                                                "items": {
                                                    "$ref": "#/PolyArc"
                                                }
                                            },
                                            "holes": {
                                                "$ref": "#/Holes2D"
                                            }
                                        },
                                        "additionalProperties": false,
                                        "required": [
                                            "type",
                                            "percent",
                                            "area",
                                            "polygonOutline",
                                            "holes"
                                        ]
                                    }
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "reductions"
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
            "reductionsOfZones"
        ]
    })";
}

// API_RoomReductionPolyType::type is a short holding an APIRoomReduction_* value.
static GS::UniString RoomReductionPolyTypeToString (short type)
{
    switch (type) {
        case APIRoomReduction_Rest:   return "Rest";
        case APIRoomReduction_Wall:   return "Wall";
        case APIRoomReduction_Column: return "Column";
        case APIRoomReduction_Hatch:  return "Hatch";
        case APIRoomReduction_Gable:  return "Gable";
        default:                      return "Unknown";
    }
}

// ACAPI_Element_RoomReductions (ACAPI_Database with APIDb_RoomReductionsID before Archicad 27,
// see MigrationHelper.hpp) hands every reduction polygon to a plain callback without a user
// data parameter, so the list under construction is reached through a thread local pointer, the
// same way the floor plan primitive collector above does it.
static thread_local GS::Array<GS::ObjectState>* tl_roomReductions = nullptr;

// Adds the coordinates startIndex..endIndex (1 based, inclusive) of a reduction polygon as
// polygonOutline and the arcs between them as polygonArcs (only when there is one), with the arc
// indices rebased to that subpolygon the same way GetPolygonsFromMemoCoords does.
static void AddRoomReductionSubPolygon (GS::ObjectState& os, const API_RoomReductionPolyType& roomReduction, Int32 startIndex, Int32 endIndex)
{
    const auto& coords = os.AddList<GS::ObjectState> ("polygonOutline");
    for (Int32 iCoord = startIndex; iCoord <= endIndex; ++iCoord) {
        coords (Create2DCoordinateObjectState ((*roomReduction.coords)[iCoord]));
    }

    GS::Array<GS::ObjectState> arcsOfSubPoly;
    const Int32 nArcs = roomReduction.arcs == nullptr ? 0 : roomReduction.nArcs;
    for (Int32 iArc = 0; iArc < nArcs; ++iArc) {
        API_PolyArc arc = (*roomReduction.arcs)[iArc];
        if (arc.begIndex < startIndex || arc.endIndex > endIndex) {
            continue;
        }
        arc.begIndex -= startIndex - 1;
        arc.endIndex -= startIndex - 1;
        arcsOfSubPoly.Push (CreatePolyArcObjectState (arc));
    }
    if (arcsOfSubPoly.IsEmpty ()) {
        return;
    }

    const auto& arcs = os.AddList<GS::ObjectState> ("polygonArcs");
    for (const GS::ObjectState& arcOS : arcsOfSubPoly) {
        arcs (arcOS);
    }
}

// The polygon of a reduction uses the memo polygon convention: coords[0] is unused, the
// coordinates of subpolygon k run from subPolys[k - 1] + 1 to subPolys[k] (both 1 based), the
// first subpolygon is the outline and the rest are holes, every subpolygon repeats its first
// coordinate as its last one, and the arcs index into the whole coordinate array.
static void CollectRoomReduction (const API_RoomReductionPolyType* roomReduction)
{
    if (roomReduction == nullptr) {
        return;
    }
    // The handles of the polygon are the callback's to dispose of (API_RoomReductionPolyType).
    const GS::OnExit polygonGuard ([roomReduction] () {
        API_Coord** coords = roomReduction->coords;
        Int32** subPolys = roomReduction->subPolys;
        API_PolyArc** arcs = roomReduction->arcs;
        BMKillHandle (reinterpret_cast<GSHandle*> (&coords));
        BMKillHandle (reinterpret_cast<GSHandle*> (&subPolys));
        BMKillHandle (reinterpret_cast<GSHandle*> (&arcs));
    });
    if (tl_roomReductions == nullptr) {
        return;
    }

    GS::ObjectState reductionOS;
    reductionOS.Add ("type", RoomReductionPolyTypeToString (roomReduction->type));
    reductionOS.Add ("percent", roomReduction->percent);
    reductionOS.Add ("area", roomReduction->area);

    const auto& holes = reductionOS.AddList<GS::ObjectState> ("holes");

    const Int32 nCoords = roomReduction->coords == nullptr ? 0 : roomReduction->nCoords;
    // Without subpolygon end indices the whole coordinate array is the outline.
    const Int32 nSubPolys = roomReduction->subPolys == nullptr ? 1 : roomReduction->nSubPolys;

    bool outlineAdded = false;
    Int32 startIndex = 1;
    for (Int32 iSubPoly = 1; iSubPoly <= nSubPolys && startIndex <= nCoords; ++iSubPoly) {
        // Clamp so a subpolygon count that disagrees with the coordinate count cannot read past the array.
        Int32 endIndex = roomReduction->subPolys == nullptr ? nCoords : (*roomReduction->subPolys)[iSubPoly];
        if (endIndex > nCoords) {
            endIndex = nCoords;
        }
        if (endIndex < startIndex) {
            break;
        }

        if (!outlineAdded) {
            AddRoomReductionSubPolygon (reductionOS, *roomReduction, startIndex, endIndex);
            outlineAdded = true;
        } else {
            GS::ObjectState holeOS;
            AddRoomReductionSubPolygon (holeOS, *roomReduction, startIndex, endIndex);
            holes (holeOS);
        }

        startIndex = endIndex + 1;
    }

    if (!outlineAdded) {
        reductionOS.AddList<GS::ObjectState> ("polygonOutline");
    }

    tl_roomReductions->Push (reductionOS);
}

static GS::ObjectState GetReductionsOfZone (const API_Guid& zoneGuid)
{
    API_Elem_Head elemHead = {};
    elemHead.guid = zoneGuid;
    GSErrCode err = ACAPI_Element_GetHeader (&elemHead);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to find element in Archicad");
    }

    if (GetElemTypeId (elemHead) != API_ZoneID) {
        return CreateErrorResponse (APIERR_BADELEMENTTYPE, "Element is not a Zone");
    }

    GS::Array<GS::ObjectState> reductions;
    tl_roomReductions = &reductions;
    const GS::OnExit guard ([] () { tl_roomReductions = nullptr; });

    err = ACAPI_Element_RoomReductions (&elemHead.guid, CollectRoomReduction);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the area reductions of the Zone");
    }

    GS::ObjectState reductionsOfZone;
    const auto& reductionsList = reductionsOfZone.AddList<GS::ObjectState> ("reductions");
    for (const GS::ObjectState& reduction : reductions) {
        reductionsList (reduction);
    }
    return reductionsOfZone;
}

GS::ObjectState GetZoneReductionsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& reductionsOfZones = response.AddList<GS::ObjectState> ("reductionsOfZones");

    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            reductionsOfZones (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        reductionsOfZones (GetReductionsOfZone (GetGuidFromObjectState (*elementId)));
    }

    return response;
}

UpdateZonesCommand::UpdateZonesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String UpdateZonesCommand::GetName () const
{
    return "UpdateZones";
}

GS::Optional<GS::UniString> UpdateZonesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "keepStampPosition": {
                "type": "boolean",
                "description": "Keep the position of the Zone Stamps. The default is true."
            },
            "undoTopTrim": {
                "type": "boolean",
                "description": "Undo the trimming of the top of the Zones. The default is false."
            },
            "undoBottomTrim": {
                "type": "boolean",
                "description": "Undo the trimming of the bottom of the Zones. The default is false."
            },
            "zones": {
                "$ref": "#/Elements",
                "description": "Optional list of the Zones to update, on any story. They are selected instead of the current selection and stay selected: Archicad updates the selected Zones right after the command has returned, not during it, and clears the selection when it is done. So do not change the selection until then, and read the new geometry with a later command. Every Zone is checked first; if one cannot be updated (not a Zone, listed twice, hidden or locked layer, not editable), nothing is selected or updated. An empty list updates nothing. Without the list the current selection is updated, or every Zone when nothing is selected."
            }
        },
        "additionalProperties": false,
        "required": [
        ]
    })";
}

GS::Optional<GS::UniString> UpdateZonesCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

static GS::Optional<GS::ObjectState> GetFailedExecutionResultIfNotEditable (const API_Guid& elemGuid, const char* operation);

// APIInternal_UpdateRoomsID updates the Zones that are selected when Archicad gets to it, which is
// after the command has returned (measured on Archicad 29: about 0.3 s later, whether it is called
// inside an undoable command, outside one, or from the main event loop). With an empty selection it
// updates every Zone (#894). So the listed Zones are checked, made the selection and left selected;
// Archicad clears the selection after the update. Putting the previous selection back here would make
// the deferred update work on that selection instead (#900).
static GS::Optional<GS::ObjectState> SelectZonesToUpdate (const GS::Array<GS::ObjectState>& zones)
{
    GS::Array<API_Neig> neigs;
    GS::HashSet<API_Guid> listed;
    GS::UniString errors;
    for (const GS::ObjectState& zone : zones) {
        const GS::ObjectState* elementId = zone.Get ("elementId");
        if (elementId == nullptr) {
            errors += "An item has no elementId. ";
            continue;
        }
        const API_Guid guid = GetGuidFromObjectState (*elementId);
        const GS::UniString guidString = APIGuidToString (guid);
        API_Elem_Head head = {};
        head.guid = guid;
        if (ACAPI_Element_GetHeader (&head) != NoError) {
            errors += guidString + ": the element does not exist. ";
            continue;
        }
        if (GetElemTypeId (head) != API_ZoneID) {
            errors += guidString + ": the element is not a Zone. ";
            continue;
        }
        if (listed.Contains (guid)) {
            errors += guidString + ": the Zone is listed twice. ";
            continue;
        }
        listed.Add (guid);
        const GS::Optional<GS::ObjectState> notEditable = GetFailedExecutionResultIfNotEditable (guid, "updated");
        if (notEditable.HasValue ()) {
            GS::UniString message;
            const GS::ObjectState* error = notEditable->Get ("error");
            if (error != nullptr) {
                error->Get ("message", message);
            }
            errors += guidString + ": " + message + " ";
            continue;
        }
        neigs.Push (API_Neig (guid));
    }
    if (!errors.IsEmpty ()) {
        errors.Trim ();
        return CreateFailedExecutionResult (APIERR_BADPARS, "No Zone was selected or updated. " + errors);
    }

    GSErrCode err = ACAPI_Selection_DeselectAll ();
    if (err == NoError) {
        err = ACAPI_Selection_Select (neigs, true);
    }
    // The update works on whatever is selected, so it may only go ahead when the selection is exactly
    // the listed Zones.
    API_SelectionInfo selectionInfo = {};
    GS::Array<API_Neig> selected;
    if (err == NoError) {
        err = ACAPI_Selection_Get (&selectionInfo, &selected, false);
        BMKillHandle ((GSHandle*) &selectionInfo.marquee.coords);
    }
    bool exact = err == NoError && selected.GetSize () == neigs.GetSize ();
    for (UIndex i = 0; exact && i < selected.GetSize (); ++i) {
        exact = listed.Contains (selected[i].guid);
    }
    if (!exact) {
        ACAPI_Selection_DeselectAll ();
        return CreateFailedExecutionResult (err != NoError ? err : APIERR_GENERAL, "Failed to select exactly the listed Zones, so none was updated.");
    }
    return GS::NoValue;
}

GS::ObjectState UpdateZonesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    API_RoomUpdateParams roomUpdateParams;
    parameters.Get ("keepStampPosition", roomUpdateParams.keepStampPos);
    parameters.Get ("undoTopTrim", roomUpdateParams.undoTopTrim);
    parameters.Get ("undoBottomTrim", roomUpdateParams.undoBotTrim);

    GS::Array<GS::ObjectState> zones;
    if (parameters.Get ("zones", zones)) {
        if (zones.IsEmpty ()) {
            return CreateSuccessfulExecutionResult ();
        }
        const GS::Optional<GS::ObjectState> selectError = SelectZonesToUpdate (zones);
        if (selectError.HasValue ()) {
            return selectError.Get ();
        }
    }

    GSErrCode err = NoError;

    ACAPI_CallUndoableCommand ("UpdateZonesCommand", [&]() {
        err = ACAPI_Internal (APIInternal_UpdateRoomsID, &roomUpdateParams);

        return err;
    });

    return err == NoError
        ? CreateSuccessfulExecutionResult ()
        : CreateFailedExecutionResult (err, "Failed to update zones.");
}

SetZoneLabelingSettingsCommand::SetZoneLabelingSettingsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetZoneLabelingSettingsCommand::GetName () const
{
    return "SetZoneLabelingSettings";
}

GS::Optional<GS::UniString> SetZoneLabelingSettingsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "automaticLabeling": {
                "type": "boolean",
                "description": "Turns the automatic labeling of Zones (placing a Label on each new Zone) on or off."
            },
            "favoriteName": {
                "type": "string",
                "description": "The name of the Label Favorite that is placed on each new Zone by the automatic labeling."
            }
        },
        "additionalProperties": false,
        "minProperties": 1
    })";
}

GS::Optional<GS::UniString> SetZoneLabelingSettingsCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState SetZoneLabelingSettingsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    bool automaticLabeling = false;
    const bool hasAutomaticLabeling = parameters.Get ("automaticLabeling", automaticLabeling);
    GS::UniString favoriteName;
    const bool hasFavoriteName = parameters.Get ("favoriteName", favoriteName);
    if (!hasAutomaticLabeling && !hasFavoriteName) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Neither automaticLabeling nor favoriteName is given.");
    }

#ifdef ServerMainVers_2800
    ACAPI::ZoneLabelingSettings settings = ACAPI::CreateZoneLabelingSettings ();

    GS::UniString errorMessage = "Failed to modify the zone labeling settings.";
    ACAPI::Result<void> modifyResult = settings.Modify (
        [&] (ACAPI::ZoneLabelingSettings::Modifier& modifier) -> ACAPI::Result<void> {
            if (hasFavoriteName) {
                ACAPI::Result<void> favoriteResult = modifier.SetSelectedFavorite (favoriteName);
                if (favoriteResult.IsErr ()) {
                    switch (favoriteResult.UnwrapErr ().kind) {
                        case APIERR_GENERAL:
                            errorMessage = "Failed to select the Favorite '" + favoriteName + "': there are no Favorites in the project.";
                            break;
                        case APIERR_BADNAME:
                            errorMessage = "Failed to select the Favorite '" + favoriteName + "': there is no Favorite with this name.";
                            break;
                        case APIERR_BADELEMENTTYPE:
                            errorMessage = "Failed to select the Favorite '" + favoriteName + "': it is not a Label Favorite.";
                            break;
                        default:
                            errorMessage = "Failed to select the Favorite '" + favoriteName + "'.";
                            break;
                    }
                    return favoriteResult;
                }
            }
            if (hasAutomaticLabeling) {
                modifier.SetAutomaticLabeling (automaticLabeling);
            }
            return ACAPI::Ok ();
        }
    );

    return modifyResult.IsOk ()
        ? CreateSuccessfulExecutionResult ()
        : CreateFailedExecutionResult (modifyResult.UnwrapErr ().kind, errorMessage);
#else
    return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "This command is only supported in Archicad 28 or later.");
#endif
}

GetZoneLabelingSettingsCommand::GetZoneLabelingSettingsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetZoneLabelingSettingsCommand::GetName () const
{
    return "GetZoneLabelingSettings";
}

GS::Optional<GS::UniString> GetZoneLabelingSettingsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "isAutomaticLabelingOn": {
                "type": "boolean",
                "description": "Whether the automatic labeling of Zones (placing a Label on each new Zone) is on."
            },
            "selectedFavoriteName": {
                "type": "string",
                "description": "The name of the Label Favorite that is placed on each new Zone. Missing when no Favorite is selected."
            }
        },
        "additionalProperties": false,
        "required": [
            "isAutomaticLabelingOn"
        ]
    })";
}

GS::ObjectState GetZoneLabelingSettingsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    ACAPI::ZoneLabelingSettings settings = ACAPI::CreateZoneLabelingSettings ();

    GS::ObjectState response;
    response.Add ("isAutomaticLabelingOn", settings.IsAutomaticLabelingOn ());

    ACAPI::Result<GS::UniString> favoriteName = settings.GetSelectedFavoriteName ();
    if (favoriteName.IsOk ()) {
        response.Add ("selectedFavoriteName", favoriteName.Unwrap ());
    }

    return response;
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command is only supported in Archicad 28 or later.");
#endif
}

GetCollisionsCommand::GetCollisionsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetCollisionsCommand::GetName () const
{
    return "GetCollisions";
}

GS::Optional<GS::UniString> GetCollisionsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementsGroup1": {
                "$ref": "#/Elements"
            },
            "elementsGroup2": {
                "$ref": "#/Elements"
            },
            "settings": {
                "type": "object",
                "properties": {
                    "volumeTolerance": {
                        "type": "number",
                        "description": "Intersection body volume greater then this value will be considered as a collision. Default value is 0.001."
                    },
                    "performSurfaceCheck": {
                        "type": "boolean",
                        "description": "Enables surface collision check. If disabled the surfaceTolerance value will be ignored. By default it's false."
                    },
                    "surfaceTolerance": {
                        "type": "number",
                        "description": "Intersection body surface area greater then this value will be considered as a collision. Default value is 0.001."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "volumeTolerance",
                    "performSurfaceCheck",
                    "surfaceTolerance"
                ]
            }
        },
        "additionalProperties": false,
        "required": [
            "elementsGroup1",
            "elementsGroup2"
        ]
    })";
}

GS::Optional<GS::UniString> GetCollisionsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "collisions": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId1": {
                            "$ref": "#/ElementId"
                        },
                        "elementId2": {
                            "$ref": "#/ElementId"
                        },
                        "hasBodyCollision": {
                            "type": "boolean"
                        },
                        "hasClearenceCollision": {
                            "type": "boolean"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId1",
                        "elementId2",
                        "hasBodyCollision",
                        "hasClearenceCollision"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "collisions"
        ]
    })";
}

GS::ObjectState GetCollisionsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elementsGroup1;
    parameters.Get ("elementsGroup1", elementsGroup1);
    GS::Array<GS::ObjectState> elementsGroup2;
    parameters.Get ("elementsGroup2", elementsGroup2);

    API_CollisionDetectionSettings collisionSettings = {};
    collisionSettings.volumeTolerance = 0.001;
    collisionSettings.performSurfaceCheck = false;
    collisionSettings.surfaceTolerance = 0.001;
    GS::ObjectState settings;
    if (parameters.Get ("settings", settings)) {
        settings.Get ("volumeTolerance", collisionSettings.volumeTolerance);
        settings.Get ("performSurfaceCheck", collisionSettings.performSurfaceCheck);
        settings.Get ("surfaceTolerance", collisionSettings.surfaceTolerance);
    }

    const GS::Array<API_Guid> elemIds1 = elementsGroup1.Transform<API_Guid> (GetGuidFromElementsArrayItem);
    const GS::Array<API_Guid> elemIds2 = elementsGroup2.Transform<API_Guid> (GetGuidFromElementsArrayItem);
    GS::Array<GS::Pair<API_CollisionElem, API_CollisionElem>> resultArray;
    GSErrCode err = ACAPI_Element_GetCollisions (elemIds1, elemIds2, resultArray, collisionSettings);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to perform collision detection.");
    }

    GS::ObjectState response;
    const auto& collisions = response.AddList<GS::ObjectState> ("collisions");

    for (const auto& collisionElement : resultArray) {
        collisions (GS::ObjectState (
            "elementId1", CreateGuidObjectState (collisionElement.first.collidedElemGuid),
            "elementId2", CreateGuidObjectState (collisionElement.second.collidedElemGuid),
            "hasBodyCollision", collisionElement.first.hasBodyCollision,
#ifdef ServerMainVers_3000
            "hasClearanceCollision", collisionElement.second.hasClearanceCollision));
#else
            "hasClearenceCollision", collisionElement.second.hasClearenceCollision));
#endif
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

GS::Optional<GS::UniString> MoveElementsCommand::GetRawResponseSchema () const
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

// ACAPI_Element_Edit also reports NoError and leaves the element untouched when
// the element lives in a 2D database (worksheet, detail, layout, ...) that is
// not the current one (#1473). Returns the name of that database, or nothing
// when the element is in the current database, on the floor plan (model
// elements are edited from the 3D, section, ... windows too, so they are left
// as they were), or when its database cannot be determined.
static GS::Optional<GS::UniString> GetOtherDatabaseOfElement (const API_Guid& elemGuid)
{
    API_DatabaseInfo currentDb = {};
    if (ACAPI_Database_GetCurrentDatabase (&currentDb) != NoError) {
        return GS::NoValue;
    }

    API_DatabaseInfo elemDb = {};
    if (ACAPI_Database_GetContainingDatabase (&elemGuid, &elemDb) != NoError) {
        return GS::NoValue;
    }

    // Fills in the type and the name of the database from its id, as
    // GetContainingDatabases does.
    if (ACAPI_Window_GetDatabaseInfo (&elemDb) != NoError) {
        return GS::NoValue;
    }

    if (elemDb.typeID == APIWind_FloorPlanID) {
        return GS::NoValue;
    }

    const DatabaseIdResolver& resolver = DatabaseIdResolver::Instance ();
    if (resolver.GetIdOfDatabase (elemDb) == resolver.GetIdOfDatabase (currentDb)) {
        return GS::NoValue;
    }

    return GS::UniString (elemDb.name);
}

// ACAPI_Element_Edit reports NoError for an element it is not allowed to edit
// (hidden or locked layer, locked element, teamwork access, another database)
// and leaves it untouched, so the element is checked before the edit. Returns the failed
// execution result for an element that cannot be edited, and nothing when the
// edit may go ahead. operation is the past participle of the edit ("moved").
static GS::Optional<GS::ObjectState> GetFailedExecutionResultIfNotEditable (const API_Guid& elemGuid, const char* operation)
{
    API_Elem_Head elemHead = {};
    if (!LoadElementHeaderByGuid (elemGuid, elemHead)) {
        return CreateFailedExecutionResult (APIERR_BADID, "Element does not exist.");
    }

    API_Attribute layerAttr = {};
    layerAttr.header.typeID = API_LayerID;
    layerAttr.header.index = elemHead.layer;
    if (ACAPI_Attribute_Get (&layerAttr) == NoError) {
        if ((layerAttr.header.flags & APILay_Locked) != 0) {
            return CreateFailedExecutionResult (APIERR_LOCKEDLAY, GS::UniString::Printf ("The element was not %s, because its layer is locked.", operation));
        }
        if ((layerAttr.header.flags & APILay_Hidden) != 0) {
            return CreateFailedExecutionResult (APIERR_HIDDENLAY, GS::UniString::Printf ("The element was not %s, because its layer is hidden.", operation));
        }
    }

    if (!ACAPI_Element_Filter (elemGuid, APIFilt_IsEditable)) {
        return CreateFailedExecutionResult (APIERR_GENERAL, GS::UniString::Printf ("The element was not %s, because it is not editable.", operation));
    }

    const GS::Optional<GS::UniString> otherDatabase = GetOtherDatabaseOfElement (elemGuid);
    if (otherDatabase.HasValue ()) {
        return CreateFailedExecutionResult (APIERR_BADDATABASE,
            GS::UniString::Printf ("The element was not %s, because it is in another database (%T) than the current one. Activate its window first (see GetContainingDatabases and ChangeWindow).",
                operation, otherDatabase->ToPrintf ()));
    }

    return GS::NoValue;
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

            const GS::Optional<GS::ObjectState> notEditableResult = GetFailedExecutionResultIfNotEditable (elemGuid, "moved");
            if (notEditableResult.HasValue ()) {
                executionResults (*notEditableResult);
                continue;
            }

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

static GSErrCode RotateElement (const API_Guid& elemGuid, const API_Coord& beginPoint, const API_Coord& endPoint, const API_Coord& origin, bool withCopy)
{
    GS::Array<API_Neig> elementsToEdit = { API_Neig (elemGuid) };

    API_EditPars editPars = {};
    editPars.typeID = APIEdit_Rotate;
    editPars.begC = API_Coord3D { beginPoint.x, beginPoint.y, 0.0 };
    editPars.endC = API_Coord3D { endPoint.x, endPoint.y, 0.0 };
    editPars.origC = origin;
    editPars.withDelete = !withCopy;

    return ACAPI_Element_Edit (&elementsToEdit, editPars);
}

RotateElementsCommand::RotateElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String RotateElementsCommand::GetName () const
{
    return "RotateElements";
}

GS::Optional<GS::UniString> RotateElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementsWithRotations": {
                "type": "array",
                "description": "The elements with rotation settings.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "rotation": {
                            "type": "object",
                            "description": "Rotation parameters for an element.",
                            "properties": {
                                "beginPoint": {
                                    "$ref": "#/Coordinate2D",
                                    "description": "Starting point of the rotation arc."
                                },
                                "endPoint": {
                                    "$ref": "#/Coordinate2D",
                                    "description": "End point of the rotation arc."
                                },
                                "origin": {
                                    "$ref": "#/Coordinate2D",
                                    "description": "Center of rotation."
                                }
                            },
                            "additionalProperties": false,
                            "required": ["beginPoint", "endPoint", "origin"]
                        },
                        "copy": {
                            "type": "boolean",
                            "description": "Optional parameter. If true, a copy of the element is rotated. By default it's false."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["elementId", "rotation"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["elementsWithRotations"]
    })";
}

GS::Optional<GS::UniString> RotateElementsCommand::GetRawResponseSchema () const
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

GS::ObjectState RotateElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elementsWithRotations;
    parameters.Get ("elementsWithRotations", elementsWithRotations);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Rotate Elements", [&]() -> GSErrCode {
        for (const GS::ObjectState& elementWithRotation : elementsWithRotations) {
            const GS::ObjectState* elementId = elementWithRotation.Get ("elementId");
            if (elementId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
                continue;
            }

            const GS::ObjectState* rotation = elementWithRotation.Get ("rotation");
            if (rotation == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "rotation is missing"));
                continue;
            }

            const GS::ObjectState* beginPoint = rotation->Get ("beginPoint");
            const GS::ObjectState* endPoint = rotation->Get ("endPoint");
            const GS::ObjectState* origin = rotation->Get ("origin");
            if (beginPoint == nullptr || endPoint == nullptr || origin == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "rotation beginPoint, endPoint, or origin is missing"));
                continue;
            }

            const API_Guid elemGuid = GetGuidFromObjectState (*elementId);

            bool copy = false;
            elementWithRotation.Get ("copy", copy);

            const GS::Optional<GS::ObjectState> notEditableResult = GetFailedExecutionResultIfNotEditable (elemGuid, "rotated");
            if (notEditableResult.HasValue ()) {
                executionResults (*notEditableResult);
                continue;
            }

            const GSErrCode err = RotateElement (elemGuid,
                                                Get2DCoordinateFromObjectState (*beginPoint),
                                                Get2DCoordinateFromObjectState (*endPoint),
                                                Get2DCoordinateFromObjectState (*origin),
                                                copy);
            if (err != NoError) {
                const GS::UniString errorMsg = GS::UniString::Printf ("Failed to rotate element with guid %T!", APIGuidToString (elemGuid).ToPrintf ());
                executionResults (CreateFailedExecutionResult (err, errorMsg));
            } else {
                executionResults (CreateSuccessfulExecutionResult ());
            }
        }

        return NoError;
    });

    return response;
}

// The mirror axis is the line through beginPoint and endPoint, given in begC
// and endC like the two points of the rotation arc in RotateElement.
static GSErrCode MirrorElement (const API_Guid& elemGuid, const API_Coord& beginPoint, const API_Coord& endPoint, bool withCopy)
{
    GS::Array<API_Neig> elementsToEdit = { API_Neig (elemGuid) };

    API_EditPars editPars = {};
    editPars.typeID = APIEdit_Mirror;
    editPars.begC = API_Coord3D { beginPoint.x, beginPoint.y, 0.0 };
    editPars.endC = API_Coord3D { endPoint.x, endPoint.y, 0.0 };
    editPars.withDelete = !withCopy;

    return ACAPI_Element_Edit (&elementsToEdit, editPars);
}

MirrorElementsCommand::MirrorElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String MirrorElementsCommand::GetName () const
{
    return "MirrorElements";
}

GS::Optional<GS::UniString> MirrorElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementsWithMirrorAxes": {
                "type": "array",
                "description": "The elements with mirror axes.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "mirrorAxis": {
                            "type": "object",
                            "description": "The mirror axis: the line through two points on the floor plan.",
                            "properties": {
                                "beginPoint": {
                                    "$ref": "#/Coordinate2D",
                                    "description": "First point of the mirror axis."
                                },
                                "endPoint": {
                                    "$ref": "#/Coordinate2D",
                                    "description": "Second point of the mirror axis. Must differ from beginPoint."
                                }
                            },
                            "additionalProperties": false,
                            "required": ["beginPoint", "endPoint"]
                        },
                        "copy": {
                            "type": "boolean",
                            "description": "Optional parameter. If true, a mirrored copy of the element is created and the original is kept. By default it's false."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["elementId", "mirrorAxis"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["elementsWithMirrorAxes"]
    })";
}

GS::Optional<GS::UniString> MirrorElementsCommand::GetRawResponseSchema () const
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

GS::ObjectState MirrorElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elementsWithMirrorAxes;
    parameters.Get ("elementsWithMirrorAxes", elementsWithMirrorAxes);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Mirror Elements", [&]() -> GSErrCode {
        for (const GS::ObjectState& elementWithMirrorAxis : elementsWithMirrorAxes) {
            const GS::ObjectState* elementId = elementWithMirrorAxis.Get ("elementId");
            if (elementId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
                continue;
            }

            const GS::ObjectState* mirrorAxis = elementWithMirrorAxis.Get ("mirrorAxis");
            if (mirrorAxis == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "mirrorAxis is missing"));
                continue;
            }

            const GS::ObjectState* beginPoint = mirrorAxis->Get ("beginPoint");
            const GS::ObjectState* endPoint = mirrorAxis->Get ("endPoint");
            if (beginPoint == nullptr || endPoint == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "mirrorAxis beginPoint or endPoint is missing"));
                continue;
            }

            const API_Coord axisBegin = Get2DCoordinateFromObjectState (*beginPoint);
            const API_Coord axisEnd = Get2DCoordinateFromObjectState (*endPoint);
            if (IsSame2DCoordinate (axisBegin, axisEnd)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "mirrorAxis beginPoint and endPoint are the same point, so they do not define an axis"));
                continue;
            }

            const API_Guid elemGuid = GetGuidFromObjectState (*elementId);

            bool copy = false;
            elementWithMirrorAxis.Get ("copy", copy);

            const GS::Optional<GS::ObjectState> notEditableResult = GetFailedExecutionResultIfNotEditable (elemGuid, "mirrored");
            if (notEditableResult.HasValue ()) {
                executionResults (*notEditableResult);
                continue;
            }

            const GSErrCode err = MirrorElement (elemGuid, axisBegin, axisEnd, copy);
            if (err != NoError) {
                const GS::UniString errorMsg = GS::UniString::Printf ("Failed to mirror element with guid %T!", APIGuidToString (elemGuid).ToPrintf ());
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
                "description": "Optional. Archicad's own flag filters. An element is kept only when it fulfills all of them.",
                "items": {
                    "$ref": "#/ElementFilter"
                },
                "minItems": 1
            },
            "layerIndices": {
                "type": "array",
                "description": "Optional. An element is kept only when it is on one of these layers (the layerIndex field of GetDetailsOfElements).",
                "items": {
                    "type": "integer"
                },
                "minItems": 1
            },
            "floorIndices": {
                "type": "array",
                "description": "Optional. An element is kept only when its home story is one of these (the floorIndex field of GetDetailsOfElements).",
                "items": {
                    "type": "integer"
                },
                "minItems": 1
            },
            "elementTypes": {
                "type": "array",
                "description": "Optional. An element is kept only when its type is one of these.",
                "items": {
                    "$ref": "#/ElementType"
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

GS::Optional<GS::UniString> FilterElementsCommand::GetRawResponseSchema () const
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
    if (!filters.IsEmpty () && filterFlags == APIFilt_None) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing filters!");
    }

    GS::Array<Int32> layerIndices;
    parameters.Get ("layerIndices", layerIndices);

    GS::Array<Int32> floorIndices;
    parameters.Get ("floorIndices", floorIndices);

    GS::Array<GS::UniString> elementTypeStrs;
    parameters.Get ("elementTypes", elementTypeStrs);
    GS::Array<API_ElemTypeID> elementTypes;
    for (const GS::UniString& elementTypeStr : elementTypeStrs) {
        const API_ElemTypeID elementType = GetElementTypeFromNonLocalizedName (elementTypeStr);
        if (elementType == API_ZombieElemID) {
            return CreateErrorResponse (APIERR_BADPARS,
                GS::UniString::Printf ("Invalid elementType '%T'.", elementTypeStr.ToPrintf ()));
        }
        elementTypes.Push (elementType);
    }

    // The header based criteria (layer, home story, type) are checked only when at least one of them is given.
    const bool checkHeader = !layerIndices.IsEmpty () || !floorIndices.IsEmpty () || !elementTypes.IsEmpty ();
    if (filterFlags == APIFilt_None && !checkHeader) {
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
        if (filterFlags != APIFilt_None && !ACAPI_Element_Filter (elemGuid, filterFlags)) {
            continue;
        }

        if (checkHeader) {
            API_Elem_Head elemHead = {};
            elemHead.guid = elemGuid;
            if (ACAPI_Element_GetHeader (&elemHead) != NoError) {
                continue;
            }
            if (!layerIndices.IsEmpty () && !layerIndices.Contains (GetAttributeIndex (elemHead.layer))) {
                continue;
            }
            if (!floorIndices.IsEmpty () && !floorIndices.Contains (elemHead.floorInd)) {
                continue;
            }
            if (!elementTypes.IsEmpty () && !elementTypes.Contains (GetElemTypeId (elemHead))) {
                continue;
            }
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

GS::Optional<GS::UniString> HighlightElementsCommand::GetRawResponseSchema () const
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


static void UpdateGlobalBoundsWithPoint (API_Box3D& globalBounds, const API_Coord3D& pt)
{
    if (pt.x < globalBounds.xMin) globalBounds.xMin = pt.x;
    if (pt.x > globalBounds.xMax) globalBounds.xMax = pt.x;
    if (pt.y < globalBounds.yMin) globalBounds.yMin = pt.y;
    if (pt.y > globalBounds.yMax) globalBounds.yMax = pt.y;
    if (pt.z < globalBounds.zMin) globalBounds.zMin = pt.z;
    if (pt.z > globalBounds.zMax) globalBounds.zMax = pt.z;
}

static void InitializeEmptyBounds (API_Box3D& bounds)
{
    bounds.xMin = bounds.yMin = bounds.zMin = 1e30;
    bounds.xMax = bounds.yMax = bounds.zMax = -1e30;
}

static bool IsEmptyBounds (const API_Box3D& bounds)
{
    return bounds.xMin > bounds.xMax || bounds.yMin > bounds.yMax || bounds.zMin > bounds.zMax;
}

// Extends bounds with the solid 3D bodies of the element. foundSolidBody is only ever set to
// true here, so the same accumulator can be run over several elements in a row.
static GSErrCode AccumulateSolidBodyBounds (const API_Elem_Head& elemHead, API_Box3D& bounds, bool& foundSolidBody)
{
    API_ElemInfo3D info3D = {};
    GSErrCode err = ACAPI_ModelAccess_Get3DInfo (elemHead, &info3D);
    if (err != NoError) {
        return err;
    }

    for (Int32 iBody = info3D.fbody; iBody <= info3D.lbody; ++iBody) {
        API_Component3D bodyComp = {};
        bodyComp.header.typeID = API_BodyID;
        bodyComp.header.index = iBody;

        if (ACAPI_ModelAccess_GetComponent (&bodyComp) != NoError) continue;

        if (bodyComp.body.nPgon == 0) { // Skip non-solid bodies
            continue;
        }

        foundSolidBody = true;

        // body.xmin..zmax is the body's bounding box in world coordinates: measured on a
        // live model it equals the min/max of the body's vertices *after* they are
        // transformed by body.tranmat. Applying tranmat to it again adds the placement a
        // second time, which is what made a stair report twice its height (#563). It went
        // unnoticed for Roofs and Zones only because their tranmat is the identity.
        UpdateGlobalBoundsWithPoint (bounds, API_Coord3D { bodyComp.body.xmin, bodyComp.body.ymin, bodyComp.body.zmin });
        UpdateGlobalBoundsWithPoint (bounds, API_Coord3D { bodyComp.body.xmax, bodyComp.body.ymax, bodyComp.body.zmax });
    }

    return NoError;
}

static GSErrCode CalculateSolidBodyBounds (const API_Elem_Head& elemHead, API_Box3D& outBounds)
{
    InitializeEmptyBounds (outBounds);

    bool foundSolidBody = false;
    GSErrCode err = AccumulateSolidBodyBounds (elemHead, outBounds, foundSolidBody);
    if (err != NoError) {
        return err;
    }

    if (!foundSolidBody) {
        return APIERR_GENERAL;
    }

    return NoError;
}

template<typename APIElemType>
static void AccumulateSubelementBounds (APIElemType* subelemArray, API_Box3D& bounds, bool& foundSolidBody)
{
    if (subelemArray == nullptr) {
        return;
    }

    const GSSize nSubelements = BMGetPtrSize (reinterpret_cast<GSPtr>(subelemArray)) / sizeof (APIElemType);
    for (GSIndex i = 0; i < nSubelements; ++i) {
        AccumulateSolidBodyBounds (subelemArray[i].head, bounds, foundSolidBody);
    }
}

// A Stair carries its 3D geometry in its subelements (risers, treads and structures), so neither
// ACAPI_Element_CalcBounds nor the 3D model of the Stair element itself gives back the vertical
// extent of the flight - both answer with zMin == zMax == 0 (#563). The bounds are the union of
// the solid bodies of the Stair and of all of its subelements.
static GSErrCode CalculateStairBounds (const API_Elem_Head& stairElemHead, API_Box3D& outBounds)
{
    InitializeEmptyBounds (outBounds);

    bool foundSolidBody = false;
    AccumulateSolidBodyBounds (stairElemHead, outBounds, foundSolidBody);

    API_ElementMemo memo = {};
    const GS::OnExit guard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
    if (ACAPI_Element_GetMemo (stairElemHead.guid, &memo, APIMemoMask_All) == NoError) {
        AccumulateSubelementBounds (memo.stairRisers, outBounds, foundSolidBody);
        AccumulateSubelementBounds (memo.stairTreads, outBounds, foundSolidBody);
        AccumulateSubelementBounds (memo.stairStructures, outBounds, foundSolidBody);
    }

    if (!foundSolidBody) {
        return APIERR_GENERAL;
    }

    return NoError;
}

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

GS::Optional<GS::UniString> Get3DBoundingBoxesCommand::GetRawResponseSchema () const
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
        GSErrCode err = ACAPI_Element_GetHeader (&elemHead);
        if (err != NoError) {
            boundingBoxes3D (CreateErrorResponse (err, "Failed to find element in Archicad"));
            continue;
        }
        const API_ElemTypeID typeID = GetElemTypeId (elemHead);

        API_Box3D box3D = {};
        if (typeID == API_RoofID || typeID == API_ZoneID || typeID == API_SlabID) {
            // The Slab is routed here to avoid a crash, not for precision: for a Slab,
            // ACAPI_Element_CalcBounds goes through Archicad's 2D bound calculation, which
            // dereferences the current window's draw environment - null when the slab is not
            // drawn in that window (another story is displayed, its layer is hidden, or a
            // non-floor-plan window is active), so Archicad dies with a SIGSEGV instead of
            // returning an error (#686). Deliberately no CalcBounds fallback for the Slab:
            // the situations that leave it without a solid body are the very ones in which
            // CalcBounds crashes.
            err = CalculateSolidBodyBounds (elemHead, box3D);
        } else if (typeID == API_StairID) {
            err = CalculateStairBounds (elemHead, box3D);
            if (err != NoError) {
                // The Stair has no solid body at all - for example it is filtered out of the 3D
                // model - so fall back to the old answer instead of failing the whole element.
                err = ACAPI_Element_CalcBounds (&elemHead, &box3D);
            }
        } else if (typeID == API_RiserID || typeID == API_TreadID || typeID == API_StairStructureID) {
            // A riser, tread or structure of a Stair carries the 3D body CalculateStairBounds
            // unions for the Stair, so it is read from its solid bodies the same way (#823);
            // ACAPI_Element_CalcBounds stays the fallback when it has none.
            err = CalculateSolidBodyBounds (elemHead, box3D);
            if (err != NoError) {
                err = ACAPI_Element_CalcBounds (&elemHead, &box3D);
            }
        } else {
            err = ACAPI_Element_CalcBounds (&elemHead, &box3D);
        }
        if (err != NoError) {
            boundingBoxes3D (CreateErrorResponse (err, "Failed to get the 3D bounding box"));
            continue;
        }
        if (IsEmptyBounds (box3D)) {
            // ACAPI_Element_CalcBounds succeeds with an empty, inverted box (min 1e35, max -1e35)
            // for an element that has no 3D geometry of its own, e.g. a Stair tread whose 3D model
            // cannot be read (#1567). That is no bounding box, so it is reported as an error.
            boundingBoxes3D (CreateErrorResponse (APIERR_GENERAL, "The element has no 3D geometry to get the bounding box of"));
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

GetLibPartTransformationsCommand::GetLibPartTransformationsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetLibPartTransformationsCommand::GetName () const
{
    return "GetLibPartTransformations";
}

GS::Optional<GS::UniString> GetLibPartTransformationsCommand::GetInputParametersSchema () const
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

GS::Optional<GS::UniString> GetLibPartTransformationsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "transformations": {
                "type": "array",
                "description": "The placement transformation of each element, aligned with the input, or an error.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "transformation": {
                                    "type": "object",
                                    "description": "The transformation chain of the library part instance: placement at location, rotation by angle, then scaling by xRatio and yRatio in the rotated coordinate system.",
                                    "properties": {
                                        "location": {
                                            "$ref": "#/Coordinate2D",
                                            "description": "The location of the referenced library part on the floor plan."
                                        },
                                        "angle": {
                                            "type": "number",
                                            "description": "The rotation angle in radians."
                                        },
                                        "xRatio": {
                                            "type": "number",
                                            "description": "The scaling along the rotated x axis."
                                        },
                                        "yRatio": {
                                            "type": "number",
                                            "description": "The scaling along the rotated y axis."
                                        }
                                    },
                                    "additionalProperties": false,
                                    "required": [
                                        "location",
                                        "angle",
                                        "xRatio",
                                        "yRatio"
                                    ]
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "transformation"
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
            "transformations"
        ]
    })";
}

GS::ObjectState GetLibPartTransformationsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& transformations = response.AddList<GS::ObjectState> ("transformations");

    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            transformations (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        API_Elem_Head elemHead = {};
        elemHead.guid = GetGuidFromObjectState (*elementId);
        GSErrCode err = ACAPI_Element_GetHeader (&elemHead);
        if (err != NoError) {
            transformations (CreateErrorResponse (err, "Failed to find element in Archicad"));
            continue;
        }

        // ACAPI_Element_GetSyTran is documented for these types only, so the others are
        // rejected here instead of being passed to it.
        const API_ElemTypeID typeID = GetElemTypeId (elemHead);
        if (typeID != API_WindowID && typeID != API_DoorID && typeID != API_ObjectID &&
            typeID != API_LampID && typeID != API_ZoneID) {
            transformations (CreateErrorResponse (APIERR_BADPARS, "Invalid element type, only Window, Door, Object, Lamp and Zone are supported"));
            continue;
        }

        API_SyTran syTran = {};
        err = ACAPI_Element_GetSyTran (&elemHead, &syTran);
        if (err != NoError) {
            transformations (CreateErrorResponse (err, "Failed to get the transformation of the element"));
            continue;
        }

        GS::ObjectState transformation ("location", GS::ObjectState ("x", syTran.loc.x, "y", syTran.loc.y),
                                        "angle", syTran.angle,
                                        "xRatio", syTran.xRatio,
                                        "yRatio", syTran.yRatio);
        transformations (GS::ObjectState ("transformation", transformation));
    }

    return response;
}

ConvertRevealOffsetsCommand::ConvertRevealOffsetsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ConvertRevealOffsetsCommand::GetName () const
{
    return "ConvertRevealOffsets";
}

GS::Optional<GS::UniString> ConvertRevealOffsetsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "openings": {
                "type": "array",
                "description": "Windows and doors placed in walls, each with exactly one of coreOffset or sideOffset.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId",
                            "description": "A window or door placed in a wall."
                        },
                        "coreOffset": {
                            "type": "number",
                            "description": "Reveal depth offset measured from the core skin of the owner wall, to convert to the wall side reference."
                        },
                        "sideOffset": {
                            "type": "number",
                            "description": "Reveal depth offset measured from the side of the owner wall, to convert to the core skin reference."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "openings"
        ]
    })";
}

GS::Optional<GS::UniString> ConvertRevealOffsetsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "revealOffsets": {
                "type": "array",
                "description": "The reveal depth offset of each opening in both references, aligned with the input, or an error.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "coreOffset": {
                                    "type": "number",
                                    "description": "Reveal depth offset measured from the core skin of the owner wall."
                                },
                                "sideOffset": {
                                    "type": "number",
                                    "description": "Reveal depth offset measured from the side of the owner wall."
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "coreOffset",
                                "sideOffset"
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
            "revealOffsets"
        ]
    })";
}

GS::ObjectState ConvertRevealOffsetsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::Array<GS::ObjectState> openings;
    parameters.Get ("openings", openings);

    GS::ObjectState response;
    const auto& revealOffsets = response.AddList<GS::ObjectState> ("revealOffsets");

    for (const GS::ObjectState& opening : openings) {
        const GS::ObjectState* elementId = opening.Get ("elementId");
        if (elementId == nullptr) {
            revealOffsets (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        const bool hasCoreOffset = opening.Contains ("coreOffset");
        const bool hasSideOffset = opening.Contains ("sideOffset");
        if (hasCoreOffset == hasSideOffset) {
            revealOffsets (CreateErrorResponse (APIERR_BADPARS, "Exactly one of coreOffset or sideOffset must be given"));
            continue;
        }

        API_Elem_Head elemHead = {};
        elemHead.guid = GetGuidFromObjectState (*elementId);
        GSErrCode err = ACAPI_Element_GetHeader (&elemHead);
        if (err != NoError) {
            revealOffsets (CreateErrorResponse (err, "Failed to find element in Archicad"));
            continue;
        }
        const API_ElemTypeID typeID = GetElemTypeId (elemHead);
        if (typeID != API_WindowID && typeID != API_DoorID) {
            revealOffsets (CreateErrorResponse (APIERR_BADPARS, "The element is not a window or a door"));
            continue;
        }

        double coreOffset = 0.0;
        double sideOffset = 0.0;
        if (hasCoreOffset) {
            opening.Get ("coreOffset", coreOffset);
            err = ACAPI_Element_CalcCoreOffsetToSideOffset (&elemHead.guid, &coreOffset, &sideOffset);
        } else {
            opening.Get ("sideOffset", sideOffset);
            err = ACAPI_Element_CalcSideOffsetToCoreOffset (&elemHead.guid, &sideOffset, &coreOffset);
        }
        if (err != NoError) {
            revealOffsets (CreateErrorResponse (err, "Failed to convert the reveal depth offset; the element must be a window or door placed in a wall"));
            continue;
        }

        revealOffsets (GS::ObjectState ("coreOffset", coreOffset, "sideOffset", sideOffset));
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
}

// Body-local vertex -> world coordinates with the body's placement. API_Tranmat is a 3x4 matrix
// stored row by row (tmx[3], tmx[7] and tmx[11] are the translation), the same layout the Morph
// code in ExtendedElementCommands.cpp reads and writes.
static API_Coord3D TransformBodyVertex (const API_Tranmat& tranmat, double x, double y, double z)
{
    const double* tmx = tranmat.tmx;
    return API_Coord3D {
        tmx[0] * x + tmx[1] * y + tmx[2] * z + tmx[3],
        tmx[4] * x + tmx[5] * y + tmx[6] * z + tmx[7],
        tmx[8] * x + tmx[9] * y + tmx[10] * z + tmx[11]
    };
}

// Adds the solid bodies of the element's own 3D model to `bodies`, in the shape the "Body3D"
// schema definition describes: world-coordinate vertices and the faces as 0-based vertex loops.
// The model is walked through the component API like AccumulateSolidBodyBounds above: once a
// body has been fetched, the vertex, edge, polygon-edge and polygon indices are 1-based within
// that body. A polygon lists its edges in fpedg..lpedg as signed edge indices - the sign is the
// direction the edge is traversed in, so the loop continues with vert1 of a positive and vert2 of
// a negative entry - and a 0 entry separates the outer contour from the holes that follow it.
static GSErrCode CollectSolidBodies (const API_Elem_Head& elemHead, GS::Array<GS::ObjectState>& bodies)
{
    API_ElemInfo3D info3D = {};
    GSErrCode err = ACAPI_ModelAccess_Get3DInfo (elemHead, &info3D);
    if (err != NoError) {
        return err;
    }

    for (Int32 iBody = info3D.fbody; iBody <= info3D.lbody; ++iBody) {
        API_Component3D component = {};
        component.header.typeID = API_BodyID;
        component.header.index = iBody;
        if (ACAPI_ModelAccess_GetComponent (&component) != NoError) {
            continue;
        }
        if (component.body.nPgon == 0) { // Skip non-solid bodies
            continue;
        }

        // `component` is reused for every other component type below, so keep the body's data.
        const API_BodyType body = component.body;

        GS::ObjectState bodyOS;
        const auto& vertices = bodyOS.AddList<GS::ObjectState> ("vertices");
        for (Int32 iVert = 1; iVert <= body.nVert; ++iVert) {
            component.header.typeID = API_VertID;
            component.header.index = iVert;
            API_Coord3D vertex = { 0.0, 0.0, 0.0 };
            if (ACAPI_ModelAccess_GetComponent (&component) == NoError) {
                vertex = TransformBodyVertex (body.tranmat, component.vert.x, component.vert.y, component.vert.z);
            }
            // Written even when the read failed, so the polygons' indices keep pointing at the
            // right vertices.
            vertices (Create3DCoordinateObjectState (vertex));
        }

        const auto& polygons = bodyOS.AddList<GS::ObjectState> ("polygons");
        for (Int32 iPgon = 1; iPgon <= body.nPgon; ++iPgon) {
            component.header.typeID = API_PgonID;
            component.header.index = iPgon;
            if (ACAPI_ModelAccess_GetComponent (&component) != NoError) {
                continue;
            }
            const API_PgonType pgon = component.pgon;

            GS::Array<GS::Array<Int32>> contours;
            contours.Push (GS::Array<Int32> ());
            for (Int32 iPedg = pgon.fpedg; iPedg <= pgon.lpedg; ++iPedg) {
                component.header.typeID = API_PedgID;
                component.header.index = iPedg;
                if (ACAPI_ModelAccess_GetComponent (&component) != NoError) {
                    continue;
                }
                const Int32 pedg = component.pedg.pedg;
                if (pedg == 0) {
                    contours.Push (GS::Array<Int32> ());
                    continue;
                }
                component.header.typeID = API_EdgeID;
                component.header.index = pedg > 0 ? pedg : -pedg;
                if (ACAPI_ModelAccess_GetComponent (&component) != NoError) {
                    continue;
                }
                contours.GetLast ().Push ((pedg > 0 ? component.edge.vert1 : component.edge.vert2) - 1);
            }
            if (contours[0].GetSize () < 3) {
                continue;
            }

            GS::ObjectState polygonOS;
            {
                const auto& vertexIds = polygonOS.AddList<Int32> ("vertexIds");
                for (Int32 vertexId : contours[0]) {
                    vertexIds (vertexId);
                }
            }
            if (contours.GetSize () > 1) {
                const auto& holes = polygonOS.AddList<GS::ObjectState> ("holes");
                for (UIndex h = 1; h < contours.GetSize (); ++h) {
                    if (contours[h].GetSize () < 3) {
                        continue;
                    }
                    GS::ObjectState hole;
                    const auto& holeVertexIds = hole.AddList<Int32> ("vertexIds");
                    for (Int32 vertexId : contours[h]) {
                        holeVertexIds (vertexId);
                    }
                    holes (hole);
                }
            }

            // The face's surface: the entry of the model's material table the polygon refers to
            // carries the surface attribute it was made from. A material defined by a GDL script
            // is not a project attribute, so it has no guid and the field is left out.
            if (pgon.iumat > 0) {
                component.header.typeID = API_UmatID;
                component.header.index = pgon.iumat;
                if (ACAPI_ModelAccess_GetComponent (&component) == NoError) {
                    const API_Guid surfaceGuid = GetAttributeGuidFromIndex (API_MaterialID, component.umat.mater.head.index);
                    if (surfaceGuid != APINULLGuid) {
                        polygonOS.Add ("surfaceId", CreateGuidObjectState (surfaceGuid));
                    }
                }
            }

            polygons (polygonOS);
        }

        bodies.Push (bodyOS);
    }

    return NoError;
}

template<typename APIElemType>
static void CollectSubelementBodies (APIElemType* subelemArray, GS::Array<GS::ObjectState>& bodies)
{
    if (subelemArray == nullptr) {
        return;
    }

    const GSSize nSubelements = BMGetPtrSize (reinterpret_cast<GSPtr>(subelemArray)) / sizeof (APIElemType);
    for (GSIndex i = 0; i < nSubelements; ++i) {
        CollectSolidBodies (subelemArray[i].head, bodies);
    }
}

// A Stair carries its 3D geometry in its subelements (see CalculateStairBounds above), and so do
// a Railing and a Curtain Wall, so their subelements' bodies are collected too. The 3D model of a
// Column or a Beam belongs to its segments, so theirs are collected when the element itself has
// no body, which keeps a body from being answered twice. Returns whether the element is one of those.
static bool CollectBodiesOfSubelements (const API_Elem_Head& elemHead, GS::Array<GS::ObjectState>& bodies)
{
    const API_ElemTypeID typeID = GetElemTypeId (elemHead);
    const bool isSegmented = typeID == API_ColumnID || typeID == API_BeamID;
    if (typeID != API_StairID && typeID != API_RailingID && typeID != API_CurtainWallID && !isSegmented) {
        return false;
    }
    if (isSegmented && !bodies.IsEmpty ()) {
        return true;
    }

    API_ElementMemo memo = {};
    const GS::OnExit guard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
    if (ACAPI_Element_GetMemo (elemHead.guid, &memo, APIMemoMask_All) != NoError) {
        return true;
    }

    CollectSubelementBodies (memo.stairRisers, bodies);
    CollectSubelementBodies (memo.stairTreads, bodies);
    CollectSubelementBodies (memo.stairStructures, bodies);

    CollectSubelementBodies (memo.railingPosts, bodies);
    CollectSubelementBodies (memo.railingInnerPosts, bodies);
    CollectSubelementBodies (memo.railingBalusters, bodies);
    CollectSubelementBodies (memo.railingPanels, bodies);
    CollectSubelementBodies (memo.railingRails, bodies);
    CollectSubelementBodies (memo.railingToprails, bodies);
    CollectSubelementBodies (memo.railingHandrails, bodies);
    CollectSubelementBodies (memo.railingRailEnds, bodies);
    CollectSubelementBodies (memo.railingToprailEnds, bodies);
    CollectSubelementBodies (memo.railingHandrailEnds, bodies);
    CollectSubelementBodies (memo.railingRailConnections, bodies);
    CollectSubelementBodies (memo.railingToprailConnections, bodies);
    CollectSubelementBodies (memo.railingHandrailConnections, bodies);

    CollectSubelementBodies (memo.cWallFrames, bodies);
    CollectSubelementBodies (memo.cWallPanels, bodies);
    CollectSubelementBodies (memo.cWallJunctions, bodies);
    CollectSubelementBodies (memo.cWallAccessories, bodies);

    CollectSubelementBodies (memo.columnSegments, bodies);
    CollectSubelementBodies (memo.beamSegments, bodies);

    return true;
}

Get3DGeometryOfElementsCommand::Get3DGeometryOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String Get3DGeometryOfElementsCommand::GetName () const
{
    return "Get3DGeometryOfElements";
}

GS::Optional<GS::UniString> Get3DGeometryOfElementsCommand::GetInputParametersSchema () const
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

GS::Optional<GS::UniString> Get3DGeometryOfElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "geometries3D": {
                "type": "array",
                "description": "The 3D geometry of each element, aligned with the input: the solid bodies of its 3D model, or an error.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "bodies": {
                                    "type": "array",
                                    "description": "The solid bodies of the element's 3D model, including those of its subelements for a Stair, Railing or Curtain Wall, and of its segments for a Column or Beam. Empty when the element has no solid 3D model (for example a 2D element, or one filtered out of the 3D view).",
                                    "items": {
                                        "$ref": "#/Body3D"
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
            "geometries3D"
        ]
    })";
}

GS::ObjectState Get3DGeometryOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& geometries3D = response.AddList<GS::ObjectState> ("geometries3D");

    // ACAPI_ModelAccess_Get3DInfo answers APIERR_BADDATABASE (-2130313110) when the current
    // database is a 3D Document (and likewise a section, elevation, layout, ...), while it works
    // from the floor plan and the 3D window (#1482). So switch the CURRENT DATABASE - not the
    // visible window - to the floor plan for the reads in those cases, as CreateWindows and
    // CreateDoors do (see SwitchCurrentDatabaseToFloorPlan in ExtendedElementCommands.cpp), and
    // put the previous one back on every exit path.
    API_DatabaseInfo previousDatabase = {};
    bool databaseSwitched = false;
    const GS::OnExit restoreDatabase ([&]() {
        if (databaseSwitched) {
            ACAPI_Database_ChangeCurrentDatabase (&previousDatabase);
        }
    });
    if (ACAPI_Database_GetCurrentDatabase (&previousDatabase) == NoError &&
        previousDatabase.typeID != APIWind_FloorPlanID &&
        previousDatabase.typeID != APIWind_3DModelID)
    {
        API_DatabaseInfo floorPlanDatabase = {};
        floorPlanDatabase.typeID = APIWind_FloorPlanID;
        if (ACAPI_Window_GetDatabaseInfo (&floorPlanDatabase) == NoError &&
            ACAPI_Database_ChangeCurrentDatabase (&floorPlanDatabase) == NoError)
        {
            databaseSwitched = true;
        }
    }

    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            geometries3D (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        API_Elem_Head elemHead = {};
        elemHead.guid = GetGuidFromObjectState (*elementId);
        GSErrCode err = ACAPI_Element_GetHeader (&elemHead);
        if (err != NoError) {
            geometries3D (CreateErrorResponse (err, "Failed to find element in Archicad"));
            continue;
        }

        GS::Array<GS::ObjectState> bodies;
        err = CollectSolidBodies (elemHead, bodies);
        if (CollectBodiesOfSubelements (elemHead, bodies)) {
            // A hierarchical element may have no 3D model of its own (a Stair answers Get3DInfo
            // with nothing, see CalculateStairBounds); its subelements' bodies are the answer.
            err = NoError;
        }
        if (err != NoError) {
            geometries3D (CreateErrorResponse (err, "Failed to get the 3D model of the element"));
            continue;
        }

        GS::ObjectState geometry;
        const auto& bodiesList = geometry.AddList<GS::ObjectState> ("bodies");
        for (const GS::ObjectState& body : bodies) {
            bodiesList (body);
        }
        geometries3D (geometry);
    }

    return response;
}

GetVisibleSkinStatusCommand::GetVisibleSkinStatusCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetVisibleSkinStatusCommand::GetName () const
{
    return "GetVisibleSkinStatus";
}

GS::Optional<GS::UniString> GetVisibleSkinStatusCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            },
            "structureDisplay": {
                "type": "string",
                "description": "The Partial Structure Display mode to check the elements in. The current window is not changed.",
                "enum": ["EntireStructure", "CoreOnly", "WithoutFinishes", "StructureOnly"]
            }
        },
        "additionalProperties": false,
        "required": [
            "elements",
            "structureDisplay"
        ]
    })";
}

GS::Optional<GS::UniString> GetVisibleSkinStatusCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "visibleSkinStatuses": {
                "type": "array",
                "description": "Whether each element has a visible skin in the given Partial Structure Display mode, aligned with the input, or an error.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "hasVisibleSkin": {
                                    "type": "boolean",
                                    "description": "True when the element has at least one skin shown in the given Partial Structure Display mode."
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "hasVisibleSkin"
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
            "visibleSkinStatuses"
        ]
    })";
}

GS::ObjectState GetVisibleSkinStatusCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString structureDisplayStr;
    parameters.Get ("structureDisplay", structureDisplayStr);

    API_StructureDisplay structureDisplay = API_EntireStructure;
    if      (structureDisplayStr == "EntireStructure")  structureDisplay = API_EntireStructure;
    else if (structureDisplayStr == "CoreOnly")         structureDisplay = API_CoreOnly;
    else if (structureDisplayStr == "WithoutFinishes")  structureDisplay = API_WithoutFinishes;
    else if (structureDisplayStr == "StructureOnly")    structureDisplay = API_StructureOnly;
    else {
        return CreateErrorResponse (APIERR_BADPARS, "structureDisplay must be one of EntireStructure, CoreOnly, WithoutFinishes, StructureOnly.");
    }

    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& visibleSkinStatuses = response.AddList<GS::ObjectState> ("visibleSkinStatuses");

    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            visibleSkinStatuses (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        API_Elem_Head elemHead = {};
        elemHead.guid = GetGuidFromObjectState (*elementId);
        GSErrCode err = ACAPI_Element_GetHeader (&elemHead);
        if (err != NoError) {
            visibleSkinStatuses (CreateErrorResponse (err, "Failed to find element in Archicad"));
            continue;
        }

        bool hasVisibleSkin = false;
        err = ACAPI_Element_ElemHasVisibleSkin (&elemHead.guid, &structureDisplay, &hasVisibleSkin);
        if (err != NoError) {
            visibleSkinStatuses (CreateErrorResponse (err, "Failed to check the visible skins of the element"));
            continue;
        }

        visibleSkinStatuses (GS::ObjectState ("hasVisibleSkin", hasVisibleSkin));
    }

    return response;
}

GetElementAtCoordinateCommand::GetElementAtCoordinateCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetElementAtCoordinateCommand::GetName () const
{
    return "GetElementAtCoordinate";
}

GS::Optional<GS::UniString> GetElementAtCoordinateCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementType": {
                "$ref": "#/ElementType",
                "description": "The type of the element to look for. Only polygonal element types can be searched, for example Slab, Roof, Mesh, Zone or Hatch."
            },
            "coordinate": {
                "$ref": "#/Coordinate2D",
                "description": "The point to search at, in the project's coordinates, on the floor plan of the current database."
            },
            "z": {
                "type": "number",
                "description": "Optional. The elevation of the search point, handed to Archicad together with the coordinate. Default is 0; without z a point 1 mm higher is also tried, so an element whose top is at 0 (for example a slab at the story level) is found."
            },
            "filters": {
                "type": "array",
                "description": "Optional. Only an element passing every given filter is found (the same values as FilterElements accepts).",
                "items": {
                    "$ref": "#/ElementFilter"
                },
                "minItems": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "elementType",
            "coordinate"
        ]
    })";
}

GS::Optional<GS::UniString> GetElementAtCoordinateCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "found": {
                "type": "boolean",
                "description": "True when an element of the given type was found at the coordinate."
            },
            "elementId": {
                "$ref": "#/ElementId",
                "description": "The identifier of the found element. Absent when nothing was found."
            }
        },
        "additionalProperties": false,
        "required": [
            "found"
        ]
    })";
}

GS::ObjectState GetElementAtCoordinateCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString elementTypeStr;
    parameters.Get ("elementType", elementTypeStr);
    const API_ElemTypeID typeID = GetElementTypeFromNonLocalizedName (elementTypeStr);
    if (typeID == API_ZombieElemID) {
        return CreateErrorResponse (APIERR_BADPARS,
            GS::UniString::Printf ("Invalid elementType '%T'.", elementTypeStr.ToPrintf ()));
    }

    const GS::ObjectState* coordinate = parameters.Get ("coordinate");
    if (coordinate == nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, "coordinate is missing");
    }

    // The same search Archicad runs for a click on the floor plan: the element of the given
    // type at the point, restricted by the filter flags (none by default).
    API_ElemSearchPars searchPars = {};
#ifdef ServerMainVers_2600
    searchPars.type = typeID;
#else
    searchPars.typeID = typeID;
#endif
    searchPars.loc = Get2DCoordinateFromObjectState (*coordinate);
    searchPars.z = 0.0;
    const bool zGiven = parameters.Get ("z", searchPars.z);

    GS::Array<GS::UniString> filters;
    if (parameters.Get ("filters", filters)) {
        API_ElemFilterFlags filterFlags = APIFilt_None;
        for (const GS::UniString& filter : filters) {
            filterFlags |= ConvertFilterStringToFlag (filter);
        }
        searchPars.filterBits = static_cast<decltype (searchPars.filterBits)> (filterFlags);
    }

    API_Guid elemGuid = APINULLGuid;
    GSErrCode err = ACAPI_Element_SearchElementByCoord (&searchPars, &elemGuid);
    if (err == APIERR_BADPARS && !zGiven) {
        // Archicad 27 and later miss an element whose top is exactly at the default elevation
        // (a slab with its top at the story level), so the search is repeated slightly above it.
        searchPars.z = 0.001;
        elemGuid = APINULLGuid;
        err = ACAPI_Element_SearchElementByCoord (&searchPars, &elemGuid);
    }
    if (err == APIERR_REFUSEDPAR) {
        return CreateErrorResponse (err, "Only polygonal element types can be searched by coordinate (for example Slab, Roof, Mesh, Zone or Hatch).");
    }
    // Both pointers are valid here, so APIERR_BADPARS is Archicad's way of saying that there
    // is no element at the point.
    if (err != NoError && err != APIERR_BADPARS) {
        return CreateErrorResponse (err, "Failed to search for an element at the coordinate.");
    }

    GS::ObjectState response;
    const bool found = (err == NoError && elemGuid != APINULLGuid);
    response.Add ("found", found);
    if (found) {
        response.Add ("elementId", CreateGuidObjectState (elemGuid));
    }
    return response;
}

DeleteElementsCommand::DeleteElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteElementsCommand::GetName () const
{
    return "DeleteElements";
}

GS::Optional<GS::UniString> DeleteElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements",
                "description": "The elements to delete. The zones among them are deleted before all the others, so an automatic zone deleted together with its bounding elements is gone before its boundary is. Deleting the boundary of an automatic zone without the zone itself can open Archicad's modal 'No closed polygon found around the clicked point' dialog, which blocks the command until someone clicks it in Archicad."
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteElementsCommand::GetRawResponseSchema () const
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

GS::ObjectState DeleteElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    const GS::Array<API_Guid> elemGuids = elements.Transform<API_Guid> (GetGuidFromElementsArrayItem);

    // A guid that is not in the project must not reach ACAPI_Element_Delete:
    // listed after an existing element it makes the whole call fail, the
    // undoable command rolls back and that existing element stays in the
    // project (#749). Such guids are left out of the calls and reported as
    // APIERR_BADID below.
    //
    // The zones of the call are deleted before everything else (#901). An automatic
    // zone is recomputed from its bounding elements when one of them is deleted, and
    // when the room is no longer closed Archicad opens its modal "No closed polygon
    // found around the clicked point" dialog - which blocks this command, and every
    // later one, until someone clicks it in Archicad. One ACAPI_Element_Delete call
    // with the walls listed before their zone opened that dialog, while deleting the
    // zone first in a call of its own did not; so the zones get a call of their own,
    // before the rest, within the same undoable command.
    GS::Array<bool> existedBeforeDelete;
    GS::Array<API_Guid> zoneGuids;
    GS::Array<API_Guid> otherGuids;
    for (const API_Guid& elemGuid : elemGuids) {
        API_Elem_Head elemHead = {};
        const bool exists = LoadElementHeaderByGuid (elemGuid, elemHead);
        existedBeforeDelete.Push (exists);
        if (!exists) {
            continue;
        }
        if (GetElemTypeId (elemHead) == API_ZoneID) {
            zoneGuids.Push (elemGuid);
        } else {
            otherGuids.Push (elemGuid);
        }
    }

    GSErrCode deleteErr = NoError;
    if (!zoneGuids.IsEmpty () || !otherGuids.IsEmpty ()) {
        ACAPI_CallUndoableCommand ("DeleteElementsCommand", [&]() {
            if (!zoneGuids.IsEmpty ()) {
                deleteErr = ACAPI_Element_Delete (zoneGuids);
            }
            if (!otherGuids.IsEmpty ()) {
                const GSErrCode otherErr = ACAPI_Element_Delete (otherGuids);
                if (deleteErr == NoError) {
                    deleteErr = otherErr;
                }
            }

            return deleteErr;
        });
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    // ACAPI_Element_Delete reports NoError even when it skips elements it is
    // not allowed to delete (locked layer, locked element, teamwork access),
    // so success is decided per element by whether the element is really gone
    // afterwards. An element that existed and is gone is the asked-for outcome
    // however it got there: deleted by this call or deleted with its owner.
    for (UIndex i = 0; i < elemGuids.GetSize (); ++i) {
        if (!existedBeforeDelete[i]) {
            executionResults (CreateFailedExecutionResult (APIERR_BADID, "The element was not deleted, because it does not exist."));
            continue;
        }

        const API_Guid& elemGuid = elemGuids[i];
        API_Elem_Head elemHead = {};
        if (!LoadElementHeaderByGuid (elemGuid, elemHead)) {
            executionResults (CreateSuccessfulExecutionResult ());
            continue;
        }

        API_Attribute layerAttr = {};
        layerAttr.header.typeID = API_LayerID;
        layerAttr.header.index = elemHead.layer;
        if (ACAPI_Attribute_Get (&layerAttr) == NoError && (layerAttr.header.flags & APILay_Locked) != 0) {
            executionResults (CreateFailedExecutionResult (APIERR_LOCKEDLAY, "The element was not deleted, because its layer is locked."));
        } else if (!ACAPI_Element_Filter (elemGuid, APIFilt_IsEditable)) {
            executionResults (CreateFailedExecutionResult (APIERR_GENERAL, "The element was not deleted, because it is not editable."));
        } else {
            executionResults (CreateFailedExecutionResult (deleteErr != NoError ? deleteErr : APIERR_GENERAL, "Failed to delete the element."));
        }
    }

    return response;
}

DeleteSelectedElementsCommand::DeleteSelectedElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteSelectedElementsCommand::GetName () const
{
    return "DeleteSelectedElements";
}

GS::Optional<GS::UniString> DeleteSelectedElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState DeleteSelectedElementsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GSErrCode err = ACAPI_CallUndoableCommand ("DeleteSelectedElementsCommand", [&]() {
        return ACAPI_Element_DeleteSelected ();
    });

    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to delete the selected elements.");
    }

    return CreateSuccessfulExecutionResult ();
}

LockElementsCommand::LockElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String LockElementsCommand::GetName () const
{
    return "LockElements";
}

GS::Optional<GS::UniString> LockElementsCommand::GetInputParametersSchema () const
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

GS::Optional<GS::UniString> LockElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState LockElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GSErrCode err = NoError;

    ACAPI_CallUndoableCommand ("LockElementsCommand", [&]() {
        err = ACAPI_Grouping_Tool (elements.Transform<API_Guid> (GetGuidFromElementsArrayItem), APITool_Lock, nullptr);
        return err;
    });

    return err == NoError
        ? CreateSuccessfulExecutionResult ()
        : CreateFailedExecutionResult (err, "Failed to lock elements.");
}

UnlockElementsCommand::UnlockElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String UnlockElementsCommand::GetName () const
{
    return "UnlockElements";
}

GS::Optional<GS::UniString> UnlockElementsCommand::GetInputParametersSchema () const
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

GS::Optional<GS::UniString> UnlockElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState UnlockElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GSErrCode err = NoError;

    ACAPI_CallUndoableCommand ("UnlockElementsCommand", [&]() {
        err = ACAPI_Grouping_Tool (elements.Transform<API_Guid> (GetGuidFromElementsArrayItem), APITool_Unlock, nullptr);
        return err;
    });

    return err == NoError
        ? CreateSuccessfulExecutionResult ()
        : CreateFailedExecutionResult (err, "Failed to unlock elements.");
}

RefreshElementsCommand::RefreshElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String RefreshElementsCommand::GetName () const
{
    return "RefreshElements";
}

GS::Optional<GS::UniString> RefreshElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            },
            "force": {
                "type": "boolean",
                "description": "Forces the rebuild of GDL-based elements, which runs their parameter script. The default is false."
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> RefreshElementsCommand::GetRawResponseSchema () const
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

// ACAPI_Element_RefreshElement is a non-undoable data structure modifier, so,
// like the hotlink node functions in UpdateHotlinks, it is not wrapped in
// ACAPI_CallUndoableCommand.
GS::ObjectState RefreshElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    bool force = false;
    parameters.Get ("force", force);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    for (const GS::ObjectState& element : elements) {
        const API_Guid elemGuid = GetGuidFromElementsArrayItem (element);

        // The header is read from the current database, so an element of
        // another database (or a deleted one) is reported here instead of
        // being passed to the refresh with an incomplete header.
        API_Elem_Head elemHead = {};
        if (!LoadElementHeaderByGuid (elemGuid, elemHead)) {
            executionResults (CreateFailedExecutionResult (APIERR_BADID, "The element was not found in the current database (the floor plan, section or detail window that is open)."));
            continue;
        }

        const GSErrCode err = ACAPI_Element_RefreshElement (&elemHead, &force);
        if (err != NoError) {
            executionResults (CreateFailedExecutionResult (err, "Failed to refresh the element."));
            continue;
        }
        executionResults (CreateSuccessfulExecutionResult ());
    }

    return response;
}

GetElementPreviewImageCommand::GetElementPreviewImageCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetElementPreviewImageCommand::GetName () const
{
    return "GetElementPreviewImage";
}

GS::Optional<GS::UniString> GetElementPreviewImageCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementId": {
                "$ref": "#/ElementId"
            },
            "imageType": {
                "type": "string",
                "description": "The type of the preview image. Default is 3D.",
                "enum": ["2D", "Section", "3D"]
            },
            "format": {
                "type": "string",
                "description": "The image format. Default is png.",
                "enum": ["png", "jpg"]
            },
            "width": {
                "type": "integer",
                "description": "The width of the preview image in pixels. Default is 128."
            },
            "height": {
                "type": "integer",
                "description": "The height of the preview image in pixels. Default is 128."
            }
        },
        "additionalProperties": false,
        "required": [
            "elementId"
        ]
    })";
}

GS::Optional<GS::UniString> GetElementPreviewImageCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "previewImage": {
                "type": "string",
                "description": "The base64 encoded preview image."
            }
        },
        "additionalProperties": false,
        "required": [
            "previewImage"
        ]
    })";
}

GS::ObjectState GetElementPreviewImageCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    API_VisualOverriddenImage image = {};
    image.view = APIImage_Model3D;
    GS::UniString imageTypeStr;
    if (parameters.Get ("imageType", imageTypeStr)) {
        if (imageTypeStr == "2D") {
            image.view = APIImage_Model2D;
        } else if (imageTypeStr == "Section") {
            image.view = APIImage_Section;
        } else if (imageTypeStr == "3D") {
            image.view = APIImage_Model3D;
        } else {
            return CreateErrorResponse (APIERR_BADPARS, "Invalid imageType parameter.");
        }
    }

    NewDisplay::NativeImage::Encoding encoding = NewDisplay::NativeImage::Encoding::PNG;
    GS::UniString formatStr;
    if (parameters.Get ("format", formatStr)) {
        if (formatStr == "png") {
            encoding = NewDisplay::NativeImage::Encoding::PNG;
        } else if (formatStr == "jpg") {
            encoding = NewDisplay::NativeImage::Encoding::JPEG;
        } else {
            return CreateErrorResponse (APIERR_BADPARS, "Invalid format parameter.");
        }
    }

    UInt32 width = 128;
    UInt32 height = 128;
    parameters.Get ("width", width);
    parameters.Get ("height", height);

    NewDisplay::NativeImage nativeImage (width, height, 32, nullptr);
    image.nativeImagePtr = &nativeImage;
    GSErrCode err = ACAPI_GraphicalOverride_GetVisualOverriddenImage (GetGuidFromElementsArrayItem (parameters), &image);
    BMhFree (image.vectorImageHandle);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get element preview image.");
    }

    GS::MemoryOChannel32 memChannel (GS::MemoryOChannel32::BMAllocation);
    if (!nativeImage.Encode (memChannel, encoding)) {
        return CreateErrorResponse (APIERR_GENERAL, "Failed to encode element preview image.");
    }

    auto str = Base64Converter::Encode (memChannel.GetDestination (), memChannel.GetDataSize ());
    str.DeleteAll (GS::UniChar(char('\n')));
    return GS::ObjectState ("previewImage", str);
}

GetRoomImageCommand::GetRoomImageCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetRoomImageCommand::GetName () const
{
    return "GetRoomImage";
}

GS::Optional<GS::UniString> GetRoomImageCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "zoneId": {
                "$ref": "#/ElementId"
            },
            "format": {
                "type": "string",
                "description": "The image format. Default is png.",
                "enum": ["png", "jpg"]
            },
            "width": {
                "type": "integer",
                "description": "The width of the preview image in pixels. Default is 256."
            },
            "height": {
                "type": "integer",
                "description": "The height of the preview image in pixels. Default is 256."
            },
            "offset": {
                "type": "number",
                "description": "Offset of the clip polygon from the edge of the zone. Default is 0.001."
            },
            "scale": {
                "type": "number",
                "description": "Scale of the view (e.g. 0.005 for 1:200). Default is 0.005."
            },
            "backgroundColor": {
                "$ref": "#/ColorRGB",
                "description": "Background color of the generated image. Default is white (1.0, 1.0, 1.0)."
            }
        },
        "additionalProperties": false,
        "required": [
            "zoneId"
        ]
    })";
}

GS::Optional<GS::UniString> GetRoomImageCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "roomImage": {
                "type": "string",
                "description": "The base64 encoded room image."
            }
        },
        "additionalProperties": false,
        "required": [
            "roomImage"
        ]
    })";
}

GS::ObjectState GetRoomImageCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    API_RoomImage image = {};
    image.roomGuid = GetGuidFromArrayItem ("zoneId", parameters);
    image.viewType = APIImage_Model2D;

    NewDisplay::NativeImage::Encoding encoding = NewDisplay::NativeImage::Encoding::PNG;
    GS::UniString formatStr;
    if (parameters.Get ("format", formatStr)) {
        if (formatStr == "png") {
            encoding = NewDisplay::NativeImage::Encoding::PNG;
        } else if (formatStr == "jpg") {
            encoding = NewDisplay::NativeImage::Encoding::JPEG;
        } else {
            return CreateErrorResponse (APIERR_BADPARS, "Invalid format parameter.");
        }
    }

    UInt32 width = 256;
    UInt32 height = 256;
    parameters.Get ("width", width);
    parameters.Get ("height", height);

    image.offset = 0.001;
    parameters.Get ("offset", image.offset);

    image.scale = 0.005;
    parameters.Get ("scale", image.scale);

    image.backgroundColor = {1.0, 1.0, 1.0};
    GetColor(parameters, "backgroundColor", image.backgroundColor);

    NewDisplay::NativeImage nativeImage (width, height, 32, nullptr);
    image.nativeImagePtr = &nativeImage;
    GSErrCode err = ACAPI_Element_GetRoomImage (&image);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get room image.");
    }

    GS::MemoryOChannel32 memChannel (GS::MemoryOChannel32::BMAllocation);
    if (!nativeImage.Encode (memChannel, encoding)) {
        return CreateErrorResponse (APIERR_GENERAL, "Failed to encode room image.");
    }

    auto str = Base64Converter::Encode (memChannel.GetDestination (), memChannel.GetDataSize ());
    str.DeleteAll (GS::UniChar(char('\n')));
    return GS::ObjectState ("roomImage", str);
}

// TriangulatePolygonsOfElements

// ACAPI_Polygon_TriangulatePoly is the Archicad 27+ name (ACAPI_Goodies.h); Archicad 25/26 reach the
// same function through the untyped ACAPI_Goodies dispatcher with APIAny_TriangulatePolyID, the same
// split the TAPIR_Polygon_* wrappers in ExtendedElementCommands.cpp make.
static GSErrCode TAPIR_Polygon_TriangulatePoly (API_ElementMemo* memo, API_Coord*** coords)
{
#ifdef ServerMainVers_2700
    return ACAPI_Polygon_TriangulatePoly (memo, coords);
#else
    return ACAPI_Goodies (APIAny_TriangulatePolyID, memo, coords);
#endif
}

// The element types whose memo coords hold one plan polygon with holes, as GetDetailsOfElements
// reads it: the outline of a Slab, Zone, Hatch or Mesh, and the outline (single-plane) or contour
// (multi-plane) of a Roof.
static bool HasTriangulablePolygon (API_ElemTypeID typeId)
{
    switch (typeId) {
        case API_SlabID:
        case API_ZoneID:
        case API_HatchID:
        case API_MeshID:
        case API_RoofID:
            return true;
        default:
            return false;
    }
}

TriangulatePolygonsOfElementsCommand::TriangulatePolygonsOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String TriangulatePolygonsOfElementsCommand::GetName () const
{
    return "TriangulatePolygonsOfElements";
}

GS::Optional<GS::UniString> TriangulatePolygonsOfElementsCommand::GetInputParametersSchema () const
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

GS::Optional<GS::UniString> TriangulatePolygonsOfElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "triangulations": {
                "type": "array",
                "description": "The triangulation of the polygon of each element, aligned with the input: its triangles, or an error (for example for an element type that has no polygon).",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "triangles": {
                                    "type": "array",
                                    "description": "The triangles covering the polygon of the element, holes excluded, as Archicad triangulates it.",
                                    "items": {
                                        "type": "object",
                                        "properties": {
                                            "vertices": {
                                                "type": "array",
                                                "description": "The 2D coordinates of the three vertices of the triangle.",
                                                "items": {
                                                    "$ref": "#/Coordinate2D"
                                                },
                                                "minItems": 3,
                                                "maxItems": 3
                                            }
                                        },
                                        "additionalProperties": false,
                                        "required": [
                                            "vertices"
                                        ]
                                    }
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "triangles"
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
            "triangulations"
        ]
    })";
}

GS::ObjectState TriangulatePolygonsOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& triangulations = response.AddList<GS::ObjectState> ("triangulations");

    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            triangulations (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        API_Elem_Head elemHead = {};
        elemHead.guid = GetGuidFromObjectState (*elementId);
        GSErrCode err = ACAPI_Element_GetHeader (&elemHead);
        if (err != NoError) {
            triangulations (CreateErrorResponse (err, "Failed to find element in Archicad"));
            continue;
        }
        if (!HasTriangulablePolygon (GetElemTypeId (elemHead))) {
            triangulations (CreateErrorResponse (APIERR_BADPARS, "The element has no polygon to triangulate. Supported types: Slab, Zone, Hatch, Mesh, Roof."));
            continue;
        }

        API_ElementMemo memo = {};
        const GS::OnExit memoGuard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
        err = ACAPI_Element_GetMemo (elemHead.guid, &memo, APIMemoMask_Polygon);
        if (err != NoError || memo.coords == nullptr) {
            triangulations (CreateErrorResponse (err != NoError ? err : APIERR_GENERAL, "Failed to get the polygon of the element."));
            continue;
        }

        // The triangles come back in a new handle owned by the caller, three coordinates per
        // triangle; it is disposed with BMKillHandle like the other handles Archicad allocates.
        API_Coord** triangleCoords = nullptr;
        err = TAPIR_Polygon_TriangulatePoly (&memo, &triangleCoords);
        const GS::OnExit coordsGuard ([&triangleCoords] () {
            if (triangleCoords != nullptr) {
                BMKillHandle (reinterpret_cast<GSHandle*> (&triangleCoords));
            }
        });
        if (err != NoError) {
            triangulations (CreateErrorResponse (err, "Failed to triangulate the polygon of the element."));
            continue;
        }

        GS::ObjectState triangulation;
        const auto& triangles = triangulation.AddList<GS::ObjectState> ("triangles");
        if (triangleCoords != nullptr) {
            const GSSize nCoords = BMhGetSize (reinterpret_cast<GSHandle> (triangleCoords)) / sizeof (API_Coord);
            // Memo coordinate handles leave their item 0 unused; should this one do the same,
            // its size is one more than a multiple of three and the triangles start at item 1.
            const GSSize firstIndex = nCoords % 3 == 1 ? 1 : 0;
            for (GSSize i = firstIndex; i + 2 < nCoords; i += 3) {
                GS::ObjectState triangle;
                const auto& vertices = triangle.AddList<GS::ObjectState> ("vertices");
                for (GSSize j = 0; j < 3; ++j) {
                    vertices (Create2DCoordinateObjectState ((*triangleCoords)[i + j]));
                }
                triangles (triangle);
            }
        }
        triangulations (triangulation);
    }

    return response;
}

// Get3DModelCounts

Get3DModelCountsCommand::Get3DModelCountsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String Get3DModelCountsCommand::GetName () const
{
    return "Get3DModelCounts";
}

GS::Optional<GS::UniString> Get3DModelCountsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "counts": {
                "type": "array",
                "description": "The number of components of each type in the 3D model Archicad currently holds, one item per type, or an error for a type that could not be counted.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "componentType": {
                                    "type": "string",
                                    "description": "The component type: the API_3DTypeID enumerator name without its API_ prefix and ID suffix.",
                                    "enum": ["Body", "Pgon", "Pedg", "Edge", "Vert", "Vect", "Umat", "Lght"]
                                },
                                "count": {
                                    "type": "integer",
                                    "description": "The number of components of this type."
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "componentType",
                                "count"
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
            "counts"
        ]
    })";
}

GS::ObjectState Get3DModelCountsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    // Every API_3DTypeID except API_Zombie3DID, in the order of the enum.
    static const std::pair<API_3DTypeID, const char*> componentTypes[] = {
        { API_BodyID, "Body" },
        { API_PgonID, "Pgon" },
        { API_PedgID, "Pedg" },
        { API_EdgeID, "Edge" },
        { API_VertID, "Vert" },
        { API_VectID, "Vect" },
        { API_UmatID, "Umat" },
        { API_LghtID, "Lght" }
    };

    GS::ObjectState response;
    const auto& counts = response.AddList<GS::ObjectState> ("counts");

    for (const auto& componentType : componentTypes) {
        Int32 count = 0;
        const GSErrCode err = ACAPI_ModelAccess_GetNum (componentType.first, &count);
        if (err != NoError) {
            counts (CreateErrorResponse (err, GS::UniString::Printf ("Failed to count the %s components of the 3D model", componentType.second)));
            continue;
        }

        GS::ObjectState countOS;
        countOS.Add ("componentType", GS::UniString (componentType.second));
        countOS.Add ("count", count);
        counts (countOS);
    }

    return response;
#else
    return CreateErrorResponse (APIERR_GENERAL, "Get3DModelCounts is available from Archicad 27.");
#endif
}


// GetQuantitiesOfElements

GetQuantitiesOfElementsCommand::GetQuantitiesOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetQuantitiesOfElementsCommand::GetName () const
{
    return "GetQuantitiesOfElements";
}

GS::Optional<GS::UniString> GetQuantitiesOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            },
            "minOpeningSize": {
                "type": "number",
                "minimum": 0,
                "description": "Optional minimum opening size in square meters. Openings smaller than this are not subtracted when the conditional values (for example volumeConditional of a Wall) are calculated. Archicad only uses it for walls. Default value is 0, so every opening is subtracted."
            },
            "calculateInOneCall": {
                "type": "boolean",
                "description": "Optional. When true, Archicad calculates the quantities of all the supported elements in one call instead of one call per element, which can be faster for many elements. The output has the same format. If that one call fails, every supported element gets the same error. Default value is false."
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> GetQuantitiesOfElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "quantitiesOfElements": {
                "type": "array",
                "description": "The quantities of the elements, in the order of the given elements. Lengths are in meters, areas in square meters and volumes in cubic meters.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "type": {
                                    "$ref": "#/ElementType"
                                },
                                "quantities": {
                                    "$ref": "#/ElementQuantities"
                                },
                                "composites": {
                                    "type": "array",
                                    "description": "The quantities per building material, in the order Archicad reports them: one item for each skin of a composite or complex profile element, and a single item for a basic element. Empty for Zones and Morphs, for which Archicad reports no skins.",
                                    "items": {
                                        "type": "object",
                                        "properties": {
                                            "buildingMaterialId": {
                                                "$ref": "#/AttributeIdArrayItem"
                                            },
                                            "volume": {
                                                "type": "number",
                                                "description": "Net volume of the skin."
                                            },
                                            "projectedArea": {
                                                "type": "number",
                                                "description": "Projected area of the skin. Archicad reports 0 when the legacy intersection and surface calculation is turned on in the Project Preferences."
                                            },
                                            "isCore": {
                                                "type": "boolean",
                                                "description": "The skin is marked as core."
                                            },
                                            "isFinish": {
                                                "type": "boolean",
                                                "description": "The skin is marked as finish."
                                            },
                                            "subElementId": {
                                                "$ref": "#/ElementId",
                                                "description": "The sub-element (for example a column or beam segment) the skin belongs to. Present only when Archicad attaches the skin to a sub-element."
                                            }
                                        },
                                        "additionalProperties": false,
                                        "required": [
                                            "buildingMaterialId",
                                            "volume",
                                            "projectedArea",
                                            "isCore",
                                            "isFinish"
                                        ]
                                    }
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "type",
                                "quantities",
                                "composites"
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
            "quantitiesOfElements"
        ]
    })";
}

static bool AreQuantitiesSupported (API_ElemTypeID typeID)
{
    switch (typeID) {
        case API_WallID:
        case API_ColumnID:
        case API_BeamID:
        case API_SlabID:
        case API_RoofID:
        case API_ShellID:
        case API_MorphID:
        case API_ZoneID:
        case API_ColumnSegmentID:
        case API_BeamSegmentID:
            return true;
        default:
            return false;
    }
}

static void AddTypeSpecificQuantities (API_ElemTypeID typeID, const API_ElementQuantity& elementQuantity, GS::ObjectState& quantities)
{
    switch (typeID) {
        case API_WallID: {
            const auto& q = elementQuantity.wall;
            quantities.Add ("volume", q.volume);
            quantities.Add ("volumeConditional", q.volume_cond);
            quantities.Add ("grossVolume", q.grossVolume);
            quantities.Add ("referenceSideSurface", q.surface1);
            quantities.Add ("oppositeSideSurface", q.surface2);
            quantities.Add ("edgeSurface", q.surface3);
            quantities.Add ("referenceSideSurfaceConditional", q.surface1_cond);
            quantities.Add ("oppositeSideSurfaceConditional", q.surface2_cond);
            quantities.Add ("grossReferenceSideSurface", q.grossSurf1);
            quantities.Add ("grossOppositeSideSurface", q.grossSurf2);
            quantities.Add ("length", q.length);
            quantities.Add ("referenceSideLength", q.length12);
            quantities.Add ("oppositeSideLength", q.length34);
            quantities.Add ("centerLength", q.centerLength);
            quantities.Add ("referenceLineLength", q.refLineLength);
            quantities.Add ("minHeight", q.minHeight);
            quantities.Add ("maxHeight", q.maxHeight);
            quantities.Add ("area", q.area);
            quantities.Add ("perimeter", q.perimeter);
            quantities.Add ("windowsSurface", q.windowsSurf);
            quantities.Add ("doorsSurface", q.doorsSurf);
            quantities.Add ("emptyOpeningsSurface", q.emptyholesSurf);
            quantities.Add ("emptyOpeningsVolume", q.emptyHolesVolume);
            break;
        }
        case API_ColumnID: {
            const auto& q = elementQuantity.column;
            quantities.Add ("coreVolume", q.coreVolume);
            quantities.Add ("veneerVolume", q.veneVolume);
            quantities.Add ("coreGrossVolume", q.coreGrossVolume);
            quantities.Add ("veneerGrossVolume", q.veneGrossVolume);
            quantities.Add ("coreSurface", q.coreSurface);
            quantities.Add ("veneerSurface", q.veneSurface);
            quantities.Add ("coreGrossSurface", q.coreGrossSurf);
            quantities.Add ("veneerGrossSurface", q.veneGrossSurf);
            quantities.Add ("area", q.area);
            quantities.Add ("perimeter", q.perimeter);
            quantities.Add ("minHeight", q.minHeight);
            quantities.Add ("maxHeight", q.maxHeight);
            quantities.Add ("crossSectionAreaCutBegin", q.crossSectionAreaCutBegin);
            quantities.Add ("crossSectionAreaCutEnd", q.crossSectionAreaCutEnd);
            break;
        }
        case API_BeamID: {
            const auto& q = elementQuantity.beam;
            quantities.Add ("volume", q.volume);
            quantities.Add ("volumeConditional", q.volume_cond);
            quantities.Add ("holesVolume", q.holesVolume);
            quantities.Add ("area", q.area);
            quantities.Add ("leftLength", q.leftLength);
            quantities.Add ("rightLength", q.rightLength);
            quantities.Add ("topSurface", q.topSurface);
            quantities.Add ("bottomSurface", q.bottomSurface);
            quantities.Add ("leftSurface", q.edgeSurfaceLeft);
            quantities.Add ("rightSurface", q.edgeSurfaceRight);
            quantities.Add ("endSurface", q.edgeSurface);
            quantities.Add ("holesSurface", q.holesSurface);
            quantities.Add ("holesEdgeSurface", q.holesEdgeSurface);
            quantities.Add ("holesNumber", static_cast<Int32> (q.holesNumber));
            quantities.Add ("crossSectionAreaCutBegin", q.crossSectionAreaCutBegin);
            quantities.Add ("crossSectionAreaCutEnd", q.crossSectionAreaCutEnd);
            break;
        }
        case API_SlabID: {
            const auto& q = elementQuantity.slab;
            quantities.Add ("volume", q.volume);
            quantities.Add ("volumeConditional", q.volume_cond);
            quantities.Add ("grossVolume", q.grossVolume);
            quantities.Add ("topSurface", q.topSurface);
            quantities.Add ("bottomSurface", q.bottomSurface);
            quantities.Add ("edgeSurface", q.edgeSurface);
            quantities.Add ("topSurfaceConditional", q.topSurface_cond);
            quantities.Add ("bottomSurfaceConditional", q.bottomSurface_cond);
            quantities.Add ("grossTopSurface", q.grossTopSurf);
            quantities.Add ("grossBottomSurface", q.grossBotSurf);
            quantities.Add ("grossEdgeSurface", q.grossEdgeSurf);
            quantities.Add ("perimeter", q.perimeter);
            quantities.Add ("holesSurface", q.holesSurf);
            quantities.Add ("holesPerimeter", q.holesPrm);
            break;
        }
        case API_RoofID: {
            const auto& q = elementQuantity.roof;
            quantities.Add ("volume", q.volume);
            quantities.Add ("volumeConditional", q.volume_cond);
            quantities.Add ("grossVolume", q.grossVolume);
            quantities.Add ("topSurface", q.topSurface);
            quantities.Add ("bottomSurface", q.bottomSurface);
            quantities.Add ("edgeSurface", q.edgeSurface);
            quantities.Add ("topSurfaceConditional", q.topSurface_cond);
            quantities.Add ("bottomSurfaceConditional", q.bottomSurface_cond);
            quantities.Add ("grossTopSurface", q.grossTopSurf);
            quantities.Add ("grossBottomSurface", q.grossBotSurf);
            quantities.Add ("grossEdgeSurface", q.grossEdgeSurf);
            quantities.Add ("contourArea", q.contourArea);
            quantities.Add ("perimeter", q.perimeter);
            quantities.Add ("holesSurface", q.holesSurf);
            quantities.Add ("holesPerimeter", q.holesPrm);
            quantities.Add ("openingsSurface", q.openingSurf);
            quantities.Add ("ridgesLength", q.ridgesLength);
            quantities.Add ("valleysLength", q.valleysLength);
            quantities.Add ("hipsLength", q.hipsLength);
            quantities.Add ("gablesLength", q.gablesLength);
            quantities.Add ("eavesLength", q.eavesLength);
            quantities.Add ("peaksLength", q.peaksLength);
            break;
        }
        case API_ShellID: {
            const auto& q = elementQuantity.shell;
            quantities.Add ("volume", q.volume);
            quantities.Add ("volumeConditional", q.volume_cond);
            quantities.Add ("grossVolume", q.grossVolume);
            quantities.Add ("referenceSurface", q.referenceSurface);
            quantities.Add ("oppositeSurface", q.oppositeSurface);
            quantities.Add ("edgeSurface", q.edgeSurface);
            quantities.Add ("referenceSurfaceConditional", q.referenceSurface_cond);
            quantities.Add ("oppositeSurfaceConditional", q.oppositeSurface_cond);
            quantities.Add ("grossReferenceSurface", q.grossReferenceSurf);
            quantities.Add ("grossOppositeSurface", q.grossOppositeSurf);
            quantities.Add ("grossEdgeSurface", q.grossEdgeSurf);
            quantities.Add ("perimeter", q.perimeter);
            quantities.Add ("holesSurface", q.holesSurf);
            quantities.Add ("holesPerimeter", q.holesPrm);
            quantities.Add ("floorPlanArea", q.floorplanArea);
            break;
        }
        case API_MorphID: {
            const auto& q = elementQuantity.morph;
            quantities.Add ("volume", q.volume);
            quantities.Add ("surface", q.surface);
            quantities.Add ("floorPlanArea", q.floorPlanArea);
            quantities.Add ("floorPlanPerimeter", q.floorPlanPerimeter);
            quantities.Add ("height", q.wholeHeight);
            break;
        }
        case API_ZoneID: {
            const auto& q = elementQuantity.zone;
            quantities.Add ("area", q.area);
            quantities.Add ("netArea", q.netarea);
            quantities.Add ("calculatedArea", q.calcArea);
            quantities.Add ("perimeter", q.perimeter);
            quantities.Add ("netPerimeter", q.netperimeter);
            quantities.Add ("volume", q.volume);
            quantities.Add ("height", q.height);
            quantities.Add ("wallsPerimeter", q.wallsPrm);
            quantities.Add ("wallsSurface", q.wallsSurf);
            quantities.Add ("doorsSurface", q.doorsSurf);
            quantities.Add ("windowsSurface", q.windowsSurf);
            break;
        }
        case API_ColumnSegmentID: {
            const auto& q = elementQuantity.columnSegment;
            quantities.Add ("volume", q.volume);
            quantities.Add ("grossVolume", q.grossVolume);
            quantities.Add ("coreVolume", q.coreVolume);
            quantities.Add ("coreGrossVolume", q.coreGrossVolume);
            quantities.Add ("veneerVolume", q.veneerNetVolume);
            quantities.Add ("veneerGrossVolume", q.veneerGrossVolume);
            quantities.Add ("coreSurface", q.coreNetSurface);
            quantities.Add ("veneerSideSurface", q.veneerSideSurface);
            quantities.Add ("length", q.length);
            quantities.Add ("area", q.area);
            quantities.Add ("height", q.height);
            quantities.Add ("minHeight", q.minHeight);
            quantities.Add ("maxHeight", q.maxHeight);
            break;
        }
        case API_BeamSegmentID: {
            const auto& q = elementQuantity.beamSegment;
            quantities.Add ("volume", q.volume);
            quantities.Add ("holesVolume", q.holesVolume);
            quantities.Add ("length", q.length);
            quantities.Add ("leftLength", q.leftLength);
            quantities.Add ("rightLength", q.rightLength);
            quantities.Add ("topSurface", q.topSurface);
            quantities.Add ("bottomSurface", q.bottomSurface);
            quantities.Add ("leftSurface", q.leftSurface);
            quantities.Add ("rightSurface", q.rightSurface);
            quantities.Add ("endSurface", q.endSurface);
            quantities.Add ("holesSurface", q.holesSurface);
            break;
        }
        default:
            break;
    }
}

static GS::ObjectState CreateQuantitiesOfElement (API_ElemTypeID typeID, const API_ElementQuantity& elementQuantity, const GS::Array<API_CompositeQuantity>& composites)
{
    GS::ObjectState typeSpecificQuantities;
    AddTypeSpecificQuantities (typeID, elementQuantity, typeSpecificQuantities);

    GS::ObjectState quantitiesOfElement;
    quantitiesOfElement.Add ("type", GetElementTypeNonLocalizedName (typeID));
    quantitiesOfElement.Add ("quantities", typeSpecificQuantities);

    const auto& compositesList = quantitiesOfElement.AddList<GS::ObjectState> ("composites");
    for (const API_CompositeQuantity& composite : composites) {
        GS::ObjectState compositeOs;
        compositeOs.Add ("buildingMaterialId", CreateAttributeIdObjectState (GetAttributeGuidFromIndex (API_BuildingMaterialID, composite.buildMatIndices)));
        compositeOs.Add ("volume", composite.volumes);
        compositeOs.Add ("projectedArea", composite.projectedArea);
        compositeOs.Add ("isCore", (composite.flags & APICWallComp_Core) != 0);
        compositeOs.Add ("isFinish", (composite.flags & APICWallComp_Finish) != 0);
        if (composite.compositeId.subElementGuid != APINULLGuid) {
            compositeOs.Add ("subElementId", CreateGuidObjectState (composite.compositeId.subElementGuid));
        }
        compositesList (compositeOs);
    }

    return quantitiesOfElement;
}

GS::ObjectState GetQuantitiesOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    API_QuantityPar quantityParams = {};
    quantityParams.minOpeningSize = 0.0;
    parameters.Get ("minOpeningSize", quantityParams.minOpeningSize);

    bool calculateInOneCall = false;
    parameters.Get ("calculateInOneCall", calculateInOneCall);

    // Pass a full mask: Archicad crashes when the mask is nullptr, although the API documentation allows it.
    API_QuantitiesMask mask;
    ACAPI_ELEMENT_QUANTITIES_MASK_SETFULL (mask);

    // One item per input element, in the input order. With calculateInOneCall the supported elements
    // get a placeholder here, filled in after the loop from the result of the one call.
    GS::Array<GS::ObjectState> results;
    GS::Array<UIndex> resultIndicesInOneCall;
    GS::Array<API_Guid> guidsInOneCall;
    GS::Array<API_ElemTypeID> typeIDsInOneCall;

    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            results.Push (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        API_Elem_Head elemHead = {};
        elemHead.guid = GetGuidFromObjectState (*elementId);
        GSErrCode err = ACAPI_Element_GetHeader (&elemHead);
        if (err != NoError) {
            results.Push (CreateErrorResponse (err, "Failed to find element in Archicad"));
            continue;
        }

        // Reject unsupported types before Archicad calculates their quantities.
        const API_ElemTypeID typeID = GetElemTypeId (elemHead);
        if (!AreQuantitiesSupported (typeID)) {
            results.Push (CreateErrorResponse (APIERR_BADELEMENTTYPE,
                GS::UniString ("Quantities are not supported for this element type: ") + GetElementTypeNonLocalizedName (typeID)));
            continue;
        }

        if (calculateInOneCall) {
            resultIndicesInOneCall.Push (results.GetSize ());
            guidsInOneCall.Push (elemHead.guid);
            typeIDsInOneCall.Push (typeID);
            results.Push (GS::ObjectState ());
            continue;
        }

        API_ElementQuantity elementQuantity = {};
        GS::Array<API_CompositeQuantity> composites;
        API_Quantities quantities = {};
        quantities.elements = &elementQuantity;
        quantities.composites = &composites;

        err = ACAPI_Element_GetQuantities (elemHead.guid, &quantityParams, &quantities, &mask);
        if (err != NoError) {
            results.Push (CreateErrorResponse (err, "Failed to get the quantities of the element"));
            continue;
        }

        results.Push (CreateQuantitiesOfElement (typeID, elementQuantity, composites));
    }

    if (!guidsInOneCall.IsEmpty ()) {
        // Each element gets its own storage, like in the per element call above. All of it is pushed
        // before any address is taken, so no reallocation can invalidate the pointers.
        const UIndex count = guidsInOneCall.GetSize ();
        GS::Array<API_ElementQuantity> elementQuantities;
        GS::Array<GS::Array<API_CompositeQuantity>> compositesOfElements;
        for (UIndex i = 0; i < count; ++i) {
            elementQuantities.Push (API_ElementQuantity {});
            compositesOfElements.Push (GS::Array<API_CompositeQuantity> ());
        }

        GS::Array<API_Quantities> quantities;
        for (UIndex i = 0; i < count; ++i) {
            API_Quantities quantitiesOfElement = {};
            quantitiesOfElement.elements = &elementQuantities[i];
            quantitiesOfElement.composites = &compositesOfElements[i];
            quantities.Push (quantitiesOfElement);
        }

        GSErrCode err = ACAPI_Element_GetMoreQuantities (&guidsInOneCall, &quantityParams, &quantities, &mask);
        if (err == NoError && quantities.GetSize () != count) {
            err = APIERR_GENERAL;
        }

        const GS::Array<API_CompositeQuantity> noComposites;
        for (UIndex i = 0; i < count; ++i) {
            const UIndex resultIndex = resultIndicesInOneCall[i];
            if (err != NoError) {
                results[resultIndex] = CreateErrorResponse (err, "Failed to get the quantities of the elements");
                continue;
            }

            // Read through the pointers of the returned items rather than the storage directly,
            // so the result is still right if Archicad replaces the items.
            const API_Quantities& quantitiesOfElement = quantities[i];
            if (quantitiesOfElement.elements == nullptr) {
                results[resultIndex] = CreateErrorResponse (APIERR_GENERAL, "Failed to get the quantities of the element");
                continue;
            }

            results[resultIndex] = CreateQuantitiesOfElement (typeIDsInOneCall[i], *quantitiesOfElement.elements,
                quantitiesOfElement.composites != nullptr ? *quantitiesOfElement.composites : noComposites);
        }
    }

    GS::ObjectState response;
    const auto& quantitiesOfElements = response.AddList<GS::ObjectState> ("quantitiesOfElements");
    for (const GS::ObjectState& result : results) {
        quantitiesOfElements (result);
    }

    return response;
}


// GetSurfaceQuantitiesOfElements

GetSurfaceQuantitiesOfElementsCommand::GetSurfaceQuantitiesOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetSurfaceQuantitiesOfElementsCommand::GetName () const
{
    return "GetSurfaceQuantitiesOfElements";
}

GS::Optional<GS::UniString> GetSurfaceQuantitiesOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements",
                "description": "The elements to get the surface quantities of."
            },
            "coverElements": {
                "$ref": "#/Elements",
                "description": "Optional covering elements: the area where one of them overlaps a surface of an element is subtracted from that surface, so for example a slab on a wall takes its footprint off the top surface of the wall. When omitted, every Wall, Column, Beam, Slab, Roof, Shell, Morph, Mesh, Stair and Curtain Wall of the project is a covering element. Pass an empty array to get the whole surfaces with nothing subtracted. An element never covers itself."
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> GetSurfaceQuantitiesOfElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "surfaceQuantitiesOfElements": {
                "type": "array",
                "description": "The surface quantities of the elements, in the order of the given elements. Areas are in square meters.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "surfaces": {
                                    "type": "array",
                                    "description": "The surfaces of the element as Archicad reports them: one item per surface attribute and building material pair of the element, so an element has as many items as it has distinct pairs (a basic wall with three different surfaces reports three items). Empty when Archicad reports no surface for the element, for example for an element without a 3D model.",
                                    "items": {
                                        "type": "object",
                                        "properties": {
                                            "surfaceId": {
                                                "$ref": "#/AttributeIdArrayItem",
                                                "description": "The surface (material) attribute of the surface. Left out when the surface is not a project attribute, for example one defined by a GDL script."
                                            },
                                            "buildingMaterialId": {
                                                "$ref": "#/AttributeIdArrayItem",
                                                "description": "The building material of the part of the element the surface belongs to."
                                            },
                                            "exposedArea": {
                                                "type": "number",
                                                "description": "The area of the surface that is not covered by any of the covering elements, in square meters."
                                            }
                                        },
                                        "additionalProperties": false,
                                        "required": [
                                            "buildingMaterialId",
                                            "exposedArea"
                                        ]
                                    }
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "surfaces"
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
            "surfaceQuantitiesOfElements"
        ]
    })";
}

// The covering elements when the caller names none: every modelled construction element of the
// project, the ones that can hide a surface of another element.
static GS::Array<API_Guid> GetDefaultCoverElements ()
{
    static const API_ElemTypeID coverElemTypes[] = {
        API_WallID, API_ColumnID, API_BeamID, API_SlabID, API_RoofID,
        API_ShellID, API_MorphID, API_MeshID, API_StairID, API_CurtainWallID
    };

    GS::Array<API_Guid> coverElemGuids;
    for (API_ElemTypeID typeID : coverElemTypes) {
        GS::Array<API_Guid> elemList;
        if (ACAPI_Element_GetElemList (typeID, &elemList) != NoError) {
            continue;
        }
        for (const API_Guid& elemGuid : elemList) {
            coverElemGuids.Push (elemGuid);
        }
    }
    return coverElemGuids;
}

GS::ObjectState GetSurfaceQuantitiesOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::Array<API_Guid> coverElemGuids;
    GS::Array<GS::ObjectState> coverElements;
    if (parameters.Get ("coverElements", coverElements)) {
        for (const GS::ObjectState& coverElement : coverElements) {
            const GS::ObjectState* coverElementId = coverElement.Get ("elementId");
            if (coverElementId != nullptr) {
                coverElemGuids.Push (GetGuidFromObjectState (*coverElementId));
            }
        }
    } else {
        coverElemGuids = GetDefaultCoverElements ();
    }

    GS::ObjectState response;
    const auto& surfaceQuantitiesOfElements = response.AddList<GS::ObjectState> ("surfaceQuantitiesOfElements");

    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            surfaceQuantitiesOfElements (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        API_Elem_Head elemHead = {};
        elemHead.guid = GetGuidFromObjectState (*elementId);
        GSErrCode err = ACAPI_Element_GetHeader (&elemHead);
        if (err != NoError) {
            surfaceQuantitiesOfElements (CreateErrorResponse (err, "Failed to find element in Archicad"));
            continue;
        }

        // One call per element keeps the result aligned with the input and reports a failure
        // for that element only. The element is left out of its own covering elements.
        GS::Array<API_Guid> elemGuids = { elemHead.guid };
        GS::Array<API_Guid> coverElemGuidsOfElement;
        for (const API_Guid& coverElemGuid : coverElemGuids) {
            if (coverElemGuid != elemHead.guid) {
                coverElemGuidsOfElement.Push (coverElemGuid);
            }
        }

        // Archicad rejects a nullptr covering element array (APIERR_BADPARS), so an empty one
        // is passed for the whole surfaces.
        GS::Array<API_ElemPartSurfaceQuantity> surfaceQuantities;
        err = ACAPI_Element_GetSurfaceQuantities (&elemGuids, &coverElemGuidsOfElement, &surfaceQuantities);
        if (err != NoError) {
            surfaceQuantitiesOfElements (CreateErrorResponse (err, "Failed to get the surface quantities of the element"));
            continue;
        }

        GS::ObjectState surfaceQuantitiesOfElement;
        const auto& surfaces = surfaceQuantitiesOfElement.AddList<GS::ObjectState> ("surfaces");
        for (const API_ElemPartSurfaceQuantity& surfaceQuantity : surfaceQuantities) {
            GS::ObjectState surfaceOs;
            // A surface defined by a GDL script is not a project attribute, so it has no guid
            // and the field is left out, as in Get3DGeometryOfElements.
            const API_Guid surfaceGuid = GetAttributeGuidFromIndex (API_MaterialID, surfaceQuantity.materialIndex);
            if (surfaceGuid != APINULLGuid) {
                surfaceOs.Add ("surfaceId", CreateAttributeIdObjectState (surfaceGuid));
            }
            surfaceOs.Add ("buildingMaterialId", CreateAttributeIdObjectState (GetAttributeGuidFromIndex (API_BuildingMaterialID, surfaceQuantity.buildMatIdx)));
            surfaceOs.Add ("exposedArea", surfaceQuantity.exposedSurface);
            surfaces (surfaceOs);
        }

        surfaceQuantitiesOfElements (surfaceQuantitiesOfElement);
    }

    return response;
}
