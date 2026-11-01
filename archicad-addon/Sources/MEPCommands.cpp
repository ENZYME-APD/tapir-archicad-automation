#include "MEPCommands.hpp"
#include <cmath>

#ifdef ServerMainVers_2800
#include "ACAPI/Result.hpp"
#include "ACAPI/MEPAdapter.hpp"
#include "ACAPI/MEPTypes.hpp"
#include "ACAPI/MEPEnums.hpp"
#include "ACAPI/MEPElement.hpp"
#include "ACAPI/MEPPort.hpp"
#include "ACAPI/MEPPipingPort.hpp"
#include "ACAPI/MEPVentilationPort.hpp"
#include "ACAPI/MEPRoutingElement.hpp"
#include "ACAPI/MEPRoutingElementDefault.hpp"
#include "ACAPI/MEPRoutingSegment.hpp"
#include "ACAPI/MEPRoutingSegmentDefault.hpp"
#include "ACAPI/MEPRoutingNode.hpp"
#include "ACAPI/MEPTerminalDefault.hpp"
#include "ACAPI/MEPAccessoryDefault.hpp"
#include "ACAPI/MEPEquipmentDefault.hpp"
#include "ACAPI/MEPFittingDefault.hpp"
#include "ACAPI/MEPDistributionSystemsGraph.hpp"
#include "ACAPI/MEPDistributionSystem.hpp"
#include "ACAPI/MEPPipeSegmentPreferenceTable.hpp"
#include "ACAPI/MEPPipeSegmentPreferenceTableContainer.hpp"
#include "ACAPI/MEPDuctCircularSegmentPreferenceTable.hpp"
#include "ACAPI/MEPDuctSegmentPreferenceTableContainer.hpp"
#include "ACAPI/MEPDuctRectangularSegmentPreferenceTable.hpp"
#include "ACAPI/MEPCableCarrierSegmentPreferenceTable.hpp"
#include "ACAPI/MEPCableCarrierSegmentPreferenceTableContainer.hpp"
#include "ACAPI/MEPPipeElbowPreferenceTable.hpp"
#include "ACAPI/MEPPipeElbowPreferenceTableContainer.hpp"
#include "ACAPI/MEPPipeBranchPreferenceTable.hpp"
#include "ACAPI/MEPPipeBranchPreferenceTableContainer.hpp"
#include "ACAPI/MEPDuctElbowPreferenceTable.hpp"
#include "ACAPI/MEPDuctElbowPreferenceTableContainer.hpp"
#include "ACAPI/MEPDuctBranchPreferenceTable.hpp"
#include "ACAPI/MEPDuctBranchPreferenceTableContainer.hpp"
#include "ACAPI/MEPPipeReferenceSet.hpp"
#include "ACAPI/MEPDuctReferenceSet.hpp"
#include "ACAPI/MEPRigidSegment.hpp"
#include "ACAPI/MEPFlexibleSegment.hpp"
#include "ACAPI/MEPBranch.hpp"
#include "ACAPI/MEPTransition.hpp"
#ifdef ServerMainVers_2900
#include "ACAPI/MEPElbow.hpp"
#include "ACAPI/MEPPipeTransitionPreferenceTable.hpp"
#include "ACAPI/MEPPipeTransitionPreferenceTableContainer.hpp"
#include "ACAPI/MEPDuctTransitionPreferenceTable.hpp"
#include "ACAPI/MEPDuctTransitionPreferenceTableContainer.hpp"
#include "ACAPI/MEPPipeAngleSet.hpp"
#include "ACAPI/MEPDuctAngleSet.hpp"
#include "ACAPI/MEPSystemGroup.hpp"
#include "ACAPI/MEPPhysicalSystem.hpp"
#endif

#include <optional>
#include <utility>
#include <type_traits>
#include <utility>

using namespace ACAPI::MEP;
#elif defined (ServerMainVers_2700)
// Archicad 27 only runs ConnectMEPElements and CheckMEPPortConnectability.
#include "ACAPI/Result.hpp"
#include "ACAPI/MEPAdapter.hpp"
#include "ACAPI/MEPPort.hpp"
#include "ACAPI/MEPRoutingElement.hpp"

#include <optional>

using namespace ACAPI::MEP;
#endif

#ifdef ServerMainVers_3000
#include "ACAPI/MEPElectricalPanel.hpp"
#include "ACAPI/MEPElectricalCircuit.hpp"
#include "ACAPI/MEPElectricalDevice.hpp"
#include "ACAPI/MEPElectricalProjectSettings.hpp"
#include "ACAPI/MEPElectricalPanelDefault.hpp"
#include "ACAPI/MEPElectricalDeviceDefault.hpp"
#endif

#ifdef ServerMainVers_2800

static API_Guid MEPUniqueIDToAPIGuid (const ACAPI::MEP::UniqueID& id)
{
    return GSGuid2APIGuid (id.GetGuid ());
}

// Sets the layer of one element through the classic element API; the MEP API has no layer setter.
static GSErrCode SetLayerOfElement (const API_Guid& elemGuid, const API_AttributeIndex& layerIndex)
{
    API_Element element = {};
    element.header.guid = elemGuid;
    GSErrCode err = ACAPI_Element_Get (&element);
    if (err != NoError) {
        return err;
    }
    API_Element mask = {};
    ACAPI_ELEMENT_MASK_CLEAR (mask);
    ACAPI_ELEMENT_MASK_SET (mask, API_Elem_Head, layer);
    element.header.layer = layerIndex;
    return ACAPI_Element_Change (&element, &mask, nullptr, 0, true);
}

// The layer given for a new routing element has to exist and must not be locked, since an element
// on a locked layer cannot be changed. Returns the error response for a layer that cannot be used.
static GS::Optional<GS::ObjectState> GetErrorIfLayerNotUsable (Int32 layerIndex)
{
    API_Attribute layerAttr = {};
    layerAttr.header.typeID = API_LayerID;
    layerAttr.header.index = ACAPI_CreateAttributeIndex (layerIndex);
    if (ACAPI_Attribute_Get (&layerAttr) != NoError) {
        return CreateErrorResponse (APIERR_BADINDEX, "The 'layerIndex' parameter does not refer to an existing layer.");
    }
    if ((layerAttr.header.flags & APILay_Locked) != 0) {
        return CreateErrorResponse (APIERR_LOCKEDLAY, "The layer given by the 'layerIndex' parameter is locked.");
    }
    return GS::NoValue;
}

// Sets the layer of a routing element and of all its parts: routing segments, rigid segments,
// elbows (bends in Archicad 28) and transitions. Changing only the routing element did not
// always move the parts that carry the geometry. Runs inside the caller's undoable command. Every
// part is tried; the first error is returned.
static GSErrCode SetLayerOfRoutingElement (const ACAPI::MEP::UniqueID& routingElementId, const API_AttributeIndex& layerIndex)
{
    GSErrCode firstErr = NoError;
    const auto setLayer = [&] (const ACAPI::MEP::UniqueID& id) {
        const GSErrCode err = SetLayerOfElement (MEPUniqueIDToAPIGuid (id), layerIndex);
        if (firstErr == NoError) {
            firstErr = err;
        }
    };

    setLayer (routingElementId);
    auto routingElement = RoutingElement::Get (routingElementId);
    if (routingElement.IsErr ()) {
        return firstErr != NoError ? firstErr : routingElement.UnwrapErr ().kind;
    }
    for (const auto& segmentId : routingElement->GetRoutingSegmentIds ()) {
        setLayer (segmentId);
        auto segment = RoutingSegment::Get (segmentId);
        if (segment.IsOk ()) {
            for (const auto& rigidSegmentId : segment->GetRigidSegmentIds ()) {
                setLayer (rigidSegmentId);
            }
        }
    }
    for (const auto& nodeId : routingElement->GetRoutingNodeIds ()) {
        setLayer (nodeId);
        auto node = RoutingNode::Get (nodeId);
        if (node.IsOk ()) {
#ifdef ServerMainVers_2900
            for (const auto& elbowId : node->GetElbowIds ()) {
                setLayer (elbowId);
            }
#else
            // Archicad 28 calls the elbows bends (RoutingNode::GetBendIds).
            for (const auto& bendId : node->GetBendIds ()) {
                setLayer (bendId);
            }
#endif
            for (const auto& transitionId : node->GetTransitionIds ()) {
                setLayer (transitionId);
            }
        }
    }
    return firstErr;
}

static GS::UniString DomainToString (Domain domain)
{
    switch (domain) {
        case Domain::Ventilation:  return "Ventilation";
        case Domain::Piping:       return "Piping";
        case Domain::CableCarrier: return "CableCarrier";
        default:                   return "Unknown";
    }
}

static std::optional<Domain> DomainFromString (const GS::UniString& domain)
{
    if (domain == "Ventilation") {
        return Domain::Ventilation;
    }
    if (domain == "Piping") {
        return Domain::Piping;
    }
    if (domain == "CableCarrier") {
        return Domain::CableCarrier;
    }
    return std::nullopt;
}

// Calls a modifier function that returns either nothing or an ACAPI::Result, and gives back its error code, if any.
template <typename CallT>
static std::optional<GSErrCode> GetErrorOfCall (CallT&& call)
{
    if constexpr (std::is_void_v<decltype (call ())>) {
        call ();
        return std::nullopt;
    } else {
        auto result = call ();
        if (result.IsErr ()) {
            return result.UnwrapErr ().kind;
        }
        return std::nullopt;
    }
}

static GS::UniString ConnectorShapeToString (ConnectorShape shape)
{
    switch (shape) {
        case ConnectorShape::Rectangular: return "Rectangular";
        case ConnectorShape::Circular:    return "Circular";
        case ConnectorShape::Oval:        return "Oval";
        case ConnectorShape::UShape:      return "UShape";
        default:                          return "Unknown";
    }
}

static std::optional<ConnectorShape> ConnectorShapeFromString (const GS::UniString& shape)
{
    if (shape == "Rectangular") {
        return ConnectorShape::Rectangular;
    }
    if (shape == "Circular") {
        return ConnectorShape::Circular;
    }
    if (shape == "Oval") {
        return ConnectorShape::Oval;
    }
    if (shape == "UShape") {
        return ConnectorShape::UShape;
    }
    return std::nullopt;
}

// The MEP enums below go over JSON as the names of their values (MEPEnums.hpp, from Archicad 29
// MEPTypes.hpp).
template <typename EnumT>
struct MEPEnumName {
    EnumT       value;
    const char* name;
};

// Adds the name of the value; a value without a name is left out of the response.
template <typename EnumT, size_t N>
static void AddMEPEnumName (GS::ObjectState& os, const char* fieldName, const MEPEnumName<EnumT> (&names)[N], EnumT value)
{
    for (const MEPEnumName<EnumT>& entry : names) {
        if (entry.value == value) {
            os.Add (fieldName, GS::UniString (entry.name));
            return;
        }
    }
}

template <typename EnumT, size_t N>
static std::optional<EnumT> MEPEnumFromString (const MEPEnumName<EnumT> (&names)[N], const GS::UniString& name)
{
    for (const MEPEnumName<EnumT>& entry : names) {
        if (name == entry.name) {
            return entry.value;
        }
    }
    return std::nullopt;
}

static const MEPEnumName<ConnectionStyle> ConnectionStyleNames[] = {
    { ConnectionStyle::SimpleBody, "SimpleBody" },
    { ConnectionStyle::Flanged,    "Flanged" },
    { ConnectionStyle::Welded,     "Welded" }
};

static const MEPEnumName<PreferredTransitionPlacement> PreferredTransitionPlacementNames[] = {
    { PreferredTransitionPlacement::SmallToLarge, "SmallToLarge" },
    { PreferredTransitionPlacement::LargeToSmall, "LargeToSmall" }
};

static const MEPEnumName<FlowDirection> FlowDirectionNames[] = {
    { FlowDirection::FlowNotDefined, "FlowNotDefined" },
    { FlowDirection::FlowIn,         "FlowIn" },
    { FlowDirection::FlowOut,        "FlowOut" }
};

#ifdef ServerMainVers_2900
static const MEPEnumName<CutPlanePosition> CutPlanePositionNames[] = {
    { CutPlanePosition::AtUpperDisplayLimit, "AtUpperDisplayLimit" },
    { CutPlanePosition::ByViewSettings,      "ByViewSettings" }
};

static const MEPEnumName<FloorPlanDisplayOption> FloorPlanDisplayOptionNames[] = {
    { FloorPlanDisplayOption::Projected,             "Projected" },
    { FloorPlanDisplayOption::ProjectedWithOverhead, "ProjectedWithOverhead" },
    { FloorPlanDisplayOption::Symbolic,              "Symbolic" },
    { FloorPlanDisplayOption::ByMVO,                 "ByMVO" }
};

static const MEPEnumName<ShowProjection> ShowProjectionNames[] = {
    { ShowProjection::ToFloorPlanRange,       "ToFloorPlanRange" },
    { ShowProjection::ToAbsoluteDisplayLimit, "ToAbsoluteDisplayLimit" },
    { ShowProjection::EntireElement,          "EntireElement" }
};

static const MEPEnumName<OnStoryVisibility> OnStoryVisibilityNames[] = {
    { OnStoryVisibility::AllRelevantStories, "AllRelevantStories" },
    { OnStoryVisibility::HomeStoryOnly,      "HomeStoryOnly" }
};
#endif

// Classifies an MEP element based on its classID; empty for non-MEP classIDs.
static GS::UniString GetMEPElementTypeName (const API_Guid& classId)
{
    if (IsRoutingElement (classId)) {
        return "RoutingElement";
    }
    if (IsRigidSegment (classId)) {
        return "RigidSegment";
    }
#ifdef ServerMainVers_2900
    if (IsElbow (classId)) {
        return "Elbow";
    }
#else
    if (IsBend (classId)) {
        return "Elbow";
    }
#endif
    if (IsTransition (classId)) {
        return "Transition";
    }
    if (IsBranch (classId)) {
        return "Branch";
    }
    if (IsTerminal (classId)) {
        return "Terminal";
    }
    if (IsAccessory (classId)) {
        return "Accessory";
    }
    if (IsEquipment (classId)) {
        return "Equipment";
    }
    if (IsFitting (classId)) {
        return "Fitting";
    }
    if (IsFlexibleSegment (classId)) {
        return "FlexibleSegment";
    }
    if (IsTakeOff (classId)) {
        return "TakeOff";
    }
#ifdef ServerMainVers_3000
    if (IsElectricalPanel (classId)) {
        return "ElectricalPanel";
    }
    if (IsElectricalDevice (classId)) {
        return "ElectricalDevice";
    }
#endif
    return "";
}

static GS::UniString GetMEPElementDomainName (const API_Guid& classId)
{
    if (IsVentilation (classId)) {
        return "Ventilation";
    }
    if (IsPiping (classId)) {
        return "Piping";
    }
    if (IsCableCarrier (classId)) {
        return "CableCarrier";
    }
    return "";
}

static GS::ObjectState CreateMEPSystemIdObjectState (const API_AttributeIndex& attributeIndex)
{
    return CreateIdObjectState ("attributeId", GetAttributeGuidFromIndex (API_MEPSystemID, attributeIndex));
}

// The getters of the MEP element parts used by GetMEPElementDetails could not be checked against the
// DevKit when they were added, so these helpers accept a plain value, an ACAPI::Result or a
// std::optional of it. A value that cannot be read is left out of the response.
template <typename T>
static std::optional<T> ToOptional (const T& value)
{
    return value;
}

template <typename T>
static std::optional<T> ToOptional (const std::optional<T>& value)
{
    return value;
}

template <typename T>
static std::optional<T> ToOptional (ACAPI::Result<T>&& result)
{
    if (result.IsErr ()) {
        return std::nullopt;
    }
    return result.Unwrap ();
}

template <typename Value>
static void AddNumberIfAvailable (GS::ObjectState& os, const char* name, Value&& value)
{
    const auto number = ToOptional (std::forward<Value> (value));
    if (number.has_value ()) {
        os.Add (name, static_cast<double> (number.value ()));
    }
}

template <typename Value>
static void AddBoolIfAvailable (GS::ObjectState& os, const char* name, Value&& value)
{
    const auto boolValue = ToOptional (std::forward<Value> (value));
    if (boolValue.has_value ()) {
        os.Add (name, static_cast<bool> (boolValue.value ()));
    }
}

template <typename Value>
static void AddElementIdIfAvailable (GS::ObjectState& os, const char* name, Value&& value)
{
    const auto id = ToOptional (std::forward<Value> (value));
    if (id.has_value ()) {
        os.Add (name, CreateGuidObjectState (MEPUniqueIDToAPIGuid (id.value ())));
    }
}

template <typename Value>
static void AddGuidIfAvailable (GS::ObjectState& os, const char* name, Value&& value)
{
    const auto id = ToOptional (std::forward<Value> (value));
    if (id.has_value ()) {
        os.Add (name, APIGuidToString (MEPUniqueIDToAPIGuid (id.value ())));
    }
}

#endif

GetMEPElementsCommand::GetMEPElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetMEPElementsCommand::GetName () const
{
    return "GetMEPElements";
}

GS::Optional<GS::UniString> GetMEPElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementTypes": {
                "type": "array",
                "description": "Optional filter for the MEP element types.",
                "items": {
                    "$ref": "#/MEPElementType"
                }
            },
            "domains": {
                "type": "array",
                "description": "Optional filter for the MEP domains.",
                "items": {
                    "$ref": "#/MEPDomain"
                }
            }
        },
        "additionalProperties": false,
        "required": []
    })";
}

GS::Optional<GS::UniString> GetMEPElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "type": "array",
                "description": "The MEP elements.",
                "items": {
                    "$ref": "#/MEPElement"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::ObjectState GetMEPElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::Array<GS::UniString> elementTypes;
    parameters.Get ("elementTypes", elementTypes);
    GS::Array<GS::UniString> domains;
    parameters.Get ("domains", domains);

    GS::Array<API_Guid> elemList;
    ACAPI_Element_GetElemList (API_ExternalElemID, &elemList);
#ifdef ServerMainVers_3000
    // Electrical elements are only listed when asked for by their classID.
    for (const API_Guid& classId : { ElectricalPanelID, ElectricalDeviceID }) {
        GS::Array<API_Guid> electricalElemList;
        ACAPI_Element_GetElemList (API_ElemType (classId), &electricalElemList);
        for (const API_Guid& elemGuid : electricalElemList) {
            if (!elemList.Contains (elemGuid)) {
                elemList.Push (elemGuid);
            }
        }
    }
#endif

    GS::ObjectState response;
    const auto& elements = response.AddList<GS::ObjectState> ("elements");

    for (const API_Guid& elemGuid : elemList) {
        API_Elem_Head header = {};
        header.guid = elemGuid;
        if (ACAPI_Element_GetHeader (&header) != NoError) {
            continue;
        }

        const GS::UniString typeName = GetMEPElementTypeName (header.type.classID);
        if (typeName.IsEmpty ()) {
            continue;
        }
        const GS::UniString domainName = GetMEPElementDomainName (header.type.classID);

        if (!elementTypes.IsEmpty () && !elementTypes.Contains (typeName)) {
            continue;
        }
        if (!domains.IsEmpty () && !domains.Contains (domainName)) {
            continue;
        }

        GS::ObjectState element;
        element.Add ("elementId", CreateGuidObjectState (elemGuid));
        element.Add ("type", typeName);
        element.Add ("domain", domainName);
#ifndef ServerMainVers_2900
        // Archicad 29 removed GetLocalizedElemTypeName.
        element.Add ("localizedTypeName", GetLocalizedElemTypeName (header.type.classID));
#endif
        elements (element);
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

GetMEPRoutingElementsCommand::GetMEPRoutingElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetMEPRoutingElementsCommand::GetName () const
{
    return "GetMEPRoutingElements";
}

GS::Optional<GS::UniString> GetMEPRoutingElementsCommand::GetInputParametersSchema () const
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

GS::Optional<GS::UniString> GetMEPRoutingElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "routingElements": {
                "$ref": "#/MEPRoutingElementDetailsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "routingElements"
        ]
    })";
}

GS::ObjectState GetMEPRoutingElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::Array<GS::ObjectState> elements;
    if (!parameters.Get ("elements", elements)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'elements' parameter.");
    }

    GS::ObjectState response;
    const auto& routingElements = response.AddList<GS::ObjectState> ("routingElements");

    for (const GS::ObjectState& element : elements) {
        const API_Guid elemGuid = GetGuidFromElementsArrayItem (element);

        ACAPI::Result<RoutingElement> routingElement = RoutingElement::Get (Adapter::UniqueID (elemGuid));
        if (routingElement.IsErr ()) {
            routingElements (CreateErrorResponse (routingElement.UnwrapErr ().kind, "Failed to get the routing element."));
            continue;
        }

        GS::ObjectState routingElementOs;
        routingElementOs.Add ("domain", DomainToString (routingElement->GetDomain ()));
        routingElementOs.Add ("mepSystemId", CreateMEPSystemIdObjectState (routingElement->GetMEPSystem ()));
        routingElementOs.Add ("offsetFromHomeStory", routingElement->GetOffsetFromHomeStory ());
        ACAPI::Result<ACAPI::MEP::UniqueID> branchPreferenceTableId = routingElement->GetBranchPreferenceTableId ();
        if (branchPreferenceTableId.IsOk ()) {
            routingElementOs.Add ("branchPreferenceTableId", branchPreferenceTableId.Unwrap ().GetGuid ().ToUniString ());
        }
        const std::optional<ACAPI::MEP::UniqueID> portIdAtRouteBegin = routingElement->GetLogicallyConnectedPortIdAtRouteBegin ();
        if (portIdAtRouteBegin.has_value ()) {
            routingElementOs.Add ("logicallyConnectedPortIdAtRouteBegin", APIGuidToString (MEPUniqueIDToAPIGuid (portIdAtRouteBegin.value ())));
        }
        const std::optional<ACAPI::MEP::UniqueID> portIdAtRouteEnd = routingElement->GetLogicallyConnectedPortIdAtRouteEnd ();
        if (portIdAtRouteEnd.has_value ()) {
            routingElementOs.Add ("logicallyConnectedPortIdAtRouteEnd", APIGuidToString (MEPUniqueIDToAPIGuid (portIdAtRouteEnd.value ())));
        }
#ifdef ServerMainVers_2900
        AddMEPEnumName (routingElementOs, "cutPlanePosition", CutPlanePositionNames, routingElement->GetCutPlanePosition ());
        AddMEPEnumName (routingElementOs, "floorPlanDisplayOption", FloorPlanDisplayOptionNames, routingElement->GetFloorPlanDisplayOption ());
        AddMEPEnumName (routingElementOs, "showProjection", ShowProjectionNames, routingElement->GetShowProjection ());
        AddMEPEnumName (routingElementOs, "onStoryVisibility", OnStoryVisibilityNames, routingElement->GetOnStoryVisibility ());
#endif

        const auto& polyline = routingElementOs.AddList<GS::ObjectState> ("polyline");
        for (const API_Coord3D& coordinate : routingElement->GetPolyLine ()) {
            polyline (Create3DCoordinateObjectState (coordinate));
        }

        const auto& segments = routingElementOs.AddList<GS::ObjectState> ("segments");
        for (const ACAPI::MEP::UniqueID& segmentId : routingElement->GetRoutingSegmentIds ()) {
            ACAPI::Result<RoutingSegment> segment = RoutingSegment::Get (segmentId);
            if (segment.IsErr ()) {
                continue;
            }
            GS::ObjectState segmentOs;
            segmentOs.Add ("elementId", CreateGuidObjectState (MEPUniqueIDToAPIGuid (segmentId)));
            segmentOs.Add ("routingElementId", CreateGuidObjectState (MEPUniqueIDToAPIGuid (segment->GetRoutingElementId ())));
            segmentOs.Add ("crossSectionWidth", segment->GetCrossSectionWidth ());
            segmentOs.Add ("crossSectionHeight", segment->GetCrossSectionHeight ());
            segmentOs.Add ("crossSectionShape", ConnectorShapeToString (segment->GetCrossSectionShape ()));
            ACAPI::Result<uint32_t> referenceId = segment->GetCrossSectionReferenceId ();
            if (referenceId.IsOk ()) {
                segmentOs.Add ("crossSectionReferenceId", static_cast<Int32> (referenceId.Unwrap ()));
            }
            segmentOs.Add ("preferenceTableId", segment->GetPreferenceTableId ().GetGuid ().ToUniString ());
            segmentOs.Add ("beginNodeId", CreateGuidObjectState (MEPUniqueIDToAPIGuid (segment->GetBeginNodeId ())));
            segmentOs.Add ("endNodeId", CreateGuidObjectState (MEPUniqueIDToAPIGuid (segment->GetEndNodeId ())));
            segmentOs.Add ("anchor", AnchorIdToString (segment->GetAnchor ()));
            segmentOs.Add ("referenceLineOffsetX", segment->GetReferenceLineOffsetX ());
            segmentOs.Add ("referenceLineOffsetY", segment->GetReferenceLineOffsetY ());
            segmentOs.Add ("wallThickness", segment->GetWallThickness ());
            const std::optional<double> flangeSize = segment->GetFlangeSize ();
            if (flangeSize.has_value ()) {
                segmentOs.Add ("flangeSize", flangeSize.value ());
            }
            AddMEPEnumName (segmentOs, "connectionStyle", ConnectionStyleNames, segment->GetConnectionStyle ());
            const auto& rigidSegments = segmentOs.AddList<GS::ObjectState> ("rigidSegments");
            for (const ACAPI::MEP::UniqueID& rigidSegmentId : segment->GetRigidSegmentIds ()) {
                rigidSegments (CreateIdObjectState ("elementId", MEPUniqueIDToAPIGuid (rigidSegmentId)));
            }
            segments (segmentOs);
        }

        const auto& nodes = routingElementOs.AddList<GS::ObjectState> ("nodes");
        for (const ACAPI::MEP::UniqueID& nodeId : routingElement->GetRoutingNodeIds ()) {
            ACAPI::Result<RoutingNode> node = RoutingNode::Get (nodeId);
            if (node.IsErr ()) {
                continue;
            }
            GS::ObjectState nodeOs;
            nodeOs.Add ("elementId", CreateGuidObjectState (MEPUniqueIDToAPIGuid (nodeId)));
            nodeOs.Add ("routingElementId", CreateGuidObjectState (MEPUniqueIDToAPIGuid (node->GetRoutingElementId ())));
            nodeOs.Add ("position", Create3DCoordinateObjectState (node->GetPosition ()));
            const std::optional<ACAPI::MEP::UniqueID> incomingSegmentId = node->GetIncomingSegmentId ();
            if (incomingSegmentId.has_value ()) {
                nodeOs.Add ("incomingSegmentId", CreateGuidObjectState (MEPUniqueIDToAPIGuid (incomingSegmentId.value ())));
            }
            const std::optional<ACAPI::MEP::UniqueID> outgoingSegmentId = node->GetOutgoingSegmentId ();
            if (outgoingSegmentId.has_value ()) {
                nodeOs.Add ("outgoingSegmentId", CreateGuidObjectState (MEPUniqueIDToAPIGuid (outgoingSegmentId.value ())));
            }
            AddMEPEnumName (nodeOs, "preferredTransitionPlacement", PreferredTransitionPlacementNames, node->GetPreferredTransitionPlacement ());
            const auto& elbows = nodeOs.AddList<GS::ObjectState> ("elbows");
#ifdef ServerMainVers_2900
            for (const ACAPI::MEP::UniqueID& elbowId : node->GetElbowIds ()) {
#else
            // Archicad 28 calls the elbows bends (RoutingNode::GetBendIds).
            for (const ACAPI::MEP::UniqueID& elbowId : node->GetBendIds ()) {
#endif
                elbows (CreateIdObjectState ("elementId", MEPUniqueIDToAPIGuid (elbowId)));
            }
            const auto& transitions = nodeOs.AddList<GS::ObjectState> ("transitions");
            for (const ACAPI::MEP::UniqueID& transitionId : node->GetTransitionIds ()) {
                transitions (CreateIdObjectState ("elementId", MEPUniqueIDToAPIGuid (transitionId)));
            }
            nodes (nodeOs);
        }

        routingElements (routingElementOs);
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

GetMEPPortsCommand::GetMEPPortsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetMEPPortsCommand::GetName () const
{
    return "GetMEPPorts";
}

GS::Optional<GS::UniString> GetMEPPortsCommand::GetInputParametersSchema () const
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

GS::Optional<GS::UniString> GetMEPPortsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementPorts": {
                "$ref": "#/MEPElementPortsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elementPorts"
        ]
    })";
}

GS::ObjectState GetMEPPortsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::Array<GS::ObjectState> elements;
    if (!parameters.Get ("elements", elements)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'elements' parameter.");
    }

    GS::ObjectState response;
    const auto& elementPorts = response.AddList<GS::ObjectState> ("elementPorts");

    for (const GS::ObjectState& element : elements) {
        const API_Guid elemGuid = GetGuidFromElementsArrayItem (element);

        ACAPI::Result<Element> mepElement = Element::Get (Adapter::UniqueID (elemGuid));
        if (mepElement.IsErr ()) {
            elementPorts (CreateErrorResponse (mepElement.UnwrapErr ().kind, "Failed to get the MEP element."));
            continue;
        }
        if (!mepElement->Exists ()) {
            elementPorts (CreateErrorResponse (APIERR_BADID, "The MEP element was not found."));
            continue;
        }

        GS::ObjectState elementPortsOs;
        const auto& ports = elementPortsOs.AddList<GS::ObjectState> ("ports");

        for (const ACAPI::MEP::UniqueID& portId : mepElement->GetPortIDs ()) {
            ACAPI::Result<Port> port = Port::Get (portId);
            if (port.IsErr () || !port->Exists ()) {
                continue;
            }

            GS::ObjectState portOs;
            portOs.Add ("portId", APIGuidToString (MEPUniqueIDToAPIGuid (portId)));
            portOs.Add ("mepElementId", CreateGuidObjectState (MEPUniqueIDToAPIGuid (port->GetMEPElementId ())));
            portOs.Add ("name", port->GetName ());
            portOs.Add ("position", Create3DCoordinateObjectState (port->GetPosition ()));
            const Orientation orientation = port->GetOrientation ();
            portOs.Add ("direction", Create3DCoordinateObjectState (API_Coord3D { orientation.direction.x, orientation.direction.y, orientation.direction.z }));
            portOs.Add ("shape", ConnectorShapeToString (port->GetShape ()));
            portOs.Add ("width", port->GetWidth ());
            portOs.Add ("height", port->GetHeight ());
            const Domain portDomain = port->GetDomain ();
            portOs.Add ("domain", DomainToString (portDomain));
            portOs.Add ("mepSystemId", CreateMEPSystemIdObjectState (port->GetMEPSystem ()));
            portOs.Add ("isPhysicallyConnected", port->IsPhysicallyConnected ());
#ifdef ServerMainVers_3000
            AddMEPEnumName (portOs, "connectionStyle", ConnectionStyleNames, port->GetConnectionStyle ());
#endif

            // The settings of the domain specific port classes. Cable carrier ports have none.
            if (portDomain == Domain::Piping) {
                ACAPI::Result<PipingPort> pipingPort = PipingPort::Get (portId);
                if (pipingPort.IsOk ()) {
                    AddMEPEnumName (portOs, "flowDirection", FlowDirectionNames, pipingPort->GetFlowDirection ());
#ifdef ServerMainVers_3000
                    portOs.Add ("nominalSize", pipingPort->GetNominalSize ());
#endif
                }
            } else if (portDomain == Domain::Ventilation) {
                ACAPI::Result<VentilationPort> ventilationPort = VentilationPort::Get (portId);
                if (ventilationPort.IsOk ()) {
                    AddMEPEnumName (portOs, "flowDirection", FlowDirectionNames, ventilationPort->GetFlowDirection ());
                }
            }

            const std::optional<ACAPI::MEP::UniqueID> connectedPortId = port->GetConnectedPortId ();
            if (connectedPortId.has_value ()) {
                portOs.Add ("connectedPortId", APIGuidToString (MEPUniqueIDToAPIGuid (connectedPortId.value ())));
            }
            const std::optional<ACAPI::MEP::UniqueID> connectedElementId = port->GetConnectedMEPElementId ();
            if (connectedElementId.has_value ()) {
                portOs.Add ("connectedElementId", CreateGuidObjectState (MEPUniqueIDToAPIGuid (connectedElementId.value ())));
            }

            ports (portOs);
        }

        elementPorts (elementPortsOs);
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

GetMEPElementDetailsCommand::GetMEPElementDetailsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetMEPElementDetailsCommand::GetName () const
{
    return "GetMEPElementDetails";
}

GS::Optional<GS::UniString> GetMEPElementDetailsCommand::GetInputParametersSchema () const
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

GS::Optional<GS::UniString> GetMEPElementDetailsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementDetails": {
                "type": "array",
                "description": "The details of the given MEP elements, in the order of the input, or an error for an element that is not an MEP element, and in Archicad 28 for an elbow. A value that cannot be read is left out.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "elementId": {
                                    "$ref": "#/ElementId"
                                },
                                "type": {
                                    "$ref": "#/MEPElementType"
                                },
                                "domain": {
                                    "type": "string",
                                    "description": "The MEP domain (Ventilation, Piping or CableCarrier), empty if the element has none."
                                },
                                "insulationThickness": {
                                    "type": "number",
                                    "description": "The insulation thickness. RigidSegment, Elbow, Branch and Transition."
                                },
                                "routingSegmentId": {
                                    "$ref": "#/ElementId",
                                    "description": "The routing segment the rigid segment belongs to (see GetMEPRoutingElements). RigidSegment."
                                },
                                "routingNodeId": {
                                    "$ref": "#/ElementId",
                                    "description": "The routing node the elbow or transition belongs to (see GetMEPRoutingElements). Elbow and Transition."
                                },
                                "controlPoints": {
                                    "type": "array",
                                    "description": "The control points of the flexible segment. FlexibleSegment.",
                                    "items": {
                                        "$ref": "#/Coordinate3D"
                                    }
                                },
                                "factorRadius": {
                                    "type": "number",
                                    "description": "The radius factor of the elbow (Elbow::GetFactorRadius). Elbow, from Archicad 29."
                                },
                                "preferenceTableId": {
                                    "$ref": "#/Guid",
                                    "description": "The guid of the elbow or transition preference table (see GetMEPPreferenceTables). Elbow and Transition, from Archicad 29."
                                },
                                "branchAngle": {
                                    "type": "number",
                                    "description": "The angle of the branch, as returned by Branch::GetBranchAngle. Branch."
                                },
                                "branchLength": {
                                    "type": "number",
                                    "description": "The length of the branch. Branch."
                                },
                                "offsetY": {
                                    "type": "number",
                                    "description": "The Y offset of the transition (Transition::GetOffsetY). Transition."
                                },
                                "offsetZ": {
                                    "type": "number",
                                    "description": "The Z offset of the transition (Transition::GetOffsetZ). Transition."
                                },
                                "widerPortId": {
                                    "$ref": "#/Guid",
                                    "description": "The port at the wider end of the transition (see GetMEPPorts). Transition."
                                },
                                "narrowerPortId": {
                                    "$ref": "#/Guid",
                                    "description": "The port at the narrower end of the transition (see GetMEPPorts). Transition."
                                },
                                "isControlledByPreference": {
                                    "type": "boolean",
                                    "description": "Whether the transition is controlled by its preference table. Transition, from Archicad 29."
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "elementId",
                                "type",
                                "domain"
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
            "elementDetails"
        ]
    })";
}

GS::ObjectState GetMEPElementDetailsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::Array<GS::ObjectState> elements;
    if (!parameters.Get ("elements", elements)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'elements' parameter.");
    }

    GS::ObjectState response;
    const auto& elementDetails = response.AddList<GS::ObjectState> ("elementDetails");

    for (const GS::ObjectState& element : elements) {
        const API_Guid elemGuid = GetGuidFromElementsArrayItem (element);

        API_Elem_Head header = {};
        header.guid = elemGuid;
        if (ACAPI_Element_GetHeader (&header) != NoError) {
            elementDetails (CreateErrorResponse (APIERR_BADID, "Failed to get the element."));
            continue;
        }
        const GS::UniString typeName = GetMEPElementTypeName (header.type.classID);
        if (typeName.IsEmpty ()) {
            elementDetails (CreateErrorResponse (APIERR_BADPARS, "The element is not an MEP element."));
            continue;
        }

        const ACAPI::MEP::UniqueID elemId = Adapter::UniqueID (elemGuid);
        GS::ObjectState detailsOs;
        detailsOs.Add ("elementId", CreateGuidObjectState (elemGuid));
        detailsOs.Add ("type", typeName);
        detailsOs.Add ("domain", GetMEPElementDomainName (header.type.classID));

        // Only these types have data of their own in the MEP API; the routing element is covered by
        // GetMEPRoutingElements and the ports of every type by GetMEPPorts.
        if (typeName == "RigidSegment") {
            ACAPI::Result<ACAPI::MEP::RigidSegment> rigidSegment = ACAPI::MEP::RigidSegment::Get (elemId);
            if (rigidSegment.IsErr ()) {
                elementDetails (CreateErrorResponse (rigidSegment.UnwrapErr ().kind, "Failed to get the rigid segment."));
                continue;
            }
            AddNumberIfAvailable (detailsOs, "insulationThickness", rigidSegment->GetInsulationThickness ());
            AddElementIdIfAvailable (detailsOs, "routingSegmentId", rigidSegment->GetRoutingSegmentId ());
        } else if (typeName == "FlexibleSegment") {
            ACAPI::Result<ACAPI::MEP::FlexibleSegment> flexibleSegment = ACAPI::MEP::FlexibleSegment::Get (elemId);
            if (flexibleSegment.IsErr ()) {
                elementDetails (CreateErrorResponse (flexibleSegment.UnwrapErr ().kind, "Failed to get the flexible segment."));
                continue;
            }
            const auto controlPoints = ToOptional (flexibleSegment->GetControlPoints ());
            if (controlPoints.has_value ()) {
                const auto& controlPointList = detailsOs.AddList<GS::ObjectState> ("controlPoints");
                for (const auto& point : controlPoints.value ()) {
                    controlPointList (Create3DCoordinateObjectState (API_Coord3D { point.x, point.y, point.z }));
                }
            }
        } else if (typeName == "Elbow") {
#ifdef ServerMainVers_2900
            ACAPI::Result<ACAPI::MEP::Elbow> elbow = ACAPI::MEP::Elbow::Get (elemId);
            if (elbow.IsErr ()) {
                elementDetails (CreateErrorResponse (elbow.UnwrapErr ().kind, "Failed to get the elbow."));
                continue;
            }
            AddNumberIfAvailable (detailsOs, "factorRadius", elbow->GetFactorRadius ());
            AddNumberIfAvailable (detailsOs, "insulationThickness", elbow->GetInsulationThickness ());
            AddGuidIfAvailable (detailsOs, "preferenceTableId", elbow->GetPreferenceTable ());
            AddElementIdIfAvailable (detailsOs, "routingNodeId", elbow->GetRoutingNodeId ());
#else
            // Archicad 28 has no Elbow class; its elbows are bends (see GetMEPElementTypeName).
            elementDetails (CreateErrorResponse (APIERR_NOTSUPPORTED, "The details of elbows are available from Archicad 29."));
            continue;
#endif
        } else if (typeName == "Branch") {
            ACAPI::Result<ACAPI::MEP::Branch> branch = ACAPI::MEP::Branch::Get (elemId);
            if (branch.IsErr ()) {
                elementDetails (CreateErrorResponse (branch.UnwrapErr ().kind, "Failed to get the branch."));
                continue;
            }
            AddNumberIfAvailable (detailsOs, "branchAngle", branch->GetBranchAngle ());
            AddNumberIfAvailable (detailsOs, "branchLength", branch->GetBranchLength ());
            AddNumberIfAvailable (detailsOs, "insulationThickness", branch->GetInsulationThickness ());
        } else if (typeName == "Transition") {
            ACAPI::Result<ACAPI::MEP::Transition> transition = ACAPI::MEP::Transition::Get (elemId);
            if (transition.IsErr ()) {
                elementDetails (CreateErrorResponse (transition.UnwrapErr ().kind, "Failed to get the transition."));
                continue;
            }
            AddNumberIfAvailable (detailsOs, "insulationThickness", transition->GetInsulationThickness ());
            AddNumberIfAvailable (detailsOs, "offsetY", transition->GetOffsetY ());
            AddNumberIfAvailable (detailsOs, "offsetZ", transition->GetOffsetZ ());
            AddGuidIfAvailable (detailsOs, "widerPortId", transition->GetWiderPortID ());
            AddGuidIfAvailable (detailsOs, "narrowerPortId", transition->GetNarrowerPortID ());
            AddElementIdIfAvailable (detailsOs, "routingNodeId", transition->GetRoutingNodeId ());
#ifdef ServerMainVers_2900
            AddGuidIfAvailable (detailsOs, "preferenceTableId", transition->GetPreferenceTable ());
            AddBoolIfAvailable (detailsOs, "isControlledByPreference", transition->IsControlledByPreference ());
#endif
        }

        elementDetails (detailsOs);
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

GetMEPDistributionSystemsCommand::GetMEPDistributionSystemsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetMEPDistributionSystemsCommand::GetName () const
{
    return "GetMEPDistributionSystems";
}

GS::Optional<GS::UniString> GetMEPDistributionSystemsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "distributionSystems": {
                "type": "array",
                "description": "The distribution systems of the project.",
                "items": {
                    "$ref": "#/MEPDistributionSystem"
                }
            },
            "ports": {
                "type": "array",
                "description": "The ports in the distribution systems graph (see GetMEPPorts).",
                "items": {
                    "$ref": "#/Guid"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "distributionSystems"
        ]
    })";
}

GS::ObjectState GetMEPDistributionSystemsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    ACAPI::Result<DistributionSystemsGraph> graph = CreateDistributionSystemsGraph ();
    if (graph.IsErr ()) {
        return CreateErrorResponse (graph.UnwrapErr ().kind, "Failed to create the distribution systems graph.");
    }

    GS::ObjectState response;
    const auto& distributionSystems = response.AddList<GS::ObjectState> ("distributionSystems");

    for (const DistributionSystem& system : graph->GetSystems ()) {
        GS::ObjectState systemOs;

        ACAPI::Result<Domain> domain = system.GetMEPDomain ();
        systemOs.Add ("domain", domain.IsOk () ? DomainToString (domain.Unwrap ()) : GS::UniString ("Unknown"));

        ACAPI::Result<API_AttributeIndex> systemCategory = system.GetSystemCategory ();
        if (systemCategory.IsOk ()) {
            systemOs.Add ("mepSystemId", CreateMEPSystemIdObjectState (systemCategory.Unwrap ()));
        }

        const auto& elements = systemOs.AddList<GS::ObjectState> ("elements");
        ACAPI::Result<std::vector<ACAPI::MEP::UniqueID>> systemElements = system.GetElements ();
        if (systemElements.IsOk ()) {
            for (const ACAPI::MEP::UniqueID& elementId : systemElements.Unwrap ()) {
                elements (CreateElementIdObjectState (MEPUniqueIDToAPIGuid (elementId)));
            }
        }

        distributionSystems (systemOs);
    }

    const auto& ports = response.AddList<GS::UniString> ("ports");
    for (const ACAPI::MEP::UniqueID& portId : graph->GetPorts ()) {
        ports (APIGuidToString (MEPUniqueIDToAPIGuid (portId)));
    }

    return response;
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

CreateMEPRoutingElementsCommand::CreateMEPRoutingElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateMEPRoutingElementsCommand::GetName () const
{
    return "CreateMEPRoutingElements";
}

GS::Optional<GS::UniString> CreateMEPRoutingElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "routingElementsData": {
                "type": "array",
                "description": "Array of data to create MEP routing elements.",
                "items": {
                    "type": "object",
                    "properties": {
                        "domain": {
                            "$ref": "#/MEPDomain"
                        },
                        "nodeCoordinates": {
                            "type": "array",
                            "description": "The corner points of the route polyline.",
                            "items": {
                                "$ref": "#/Coordinate3D"
                            },
                            "minItems": 2
                        },
                        "crossSectionWidth": {
                            "type": "number",
                            "description": "Optional cross section width applied to all segments."
                        },
                        "crossSectionHeight": {
                            "type": "number",
                            "description": "Optional cross section height applied to all segments."
                        },
                        "crossSectionShape": {
                            "$ref": "#/MEPCrossSectionShape",
                            "description": "Optional; applied to all segments."
                        },
                        "crossSectionReferenceId": {
                            "type": "integer",
                            "description": "Optional cross section reference id of the segment preference table (used for circular cross sections). Resolved against preferenceTableId if given, otherwise against the table of the current routing tool default."
                        },
                        "preferenceTableId": {
                            "$ref": "#/Guid",
                            "description": "Optional guid of the segment preference table (see GetMEPPreferenceTables) applied to all segments. Not applicable to the CableCarrier domain."
                        },
                        "mepSystemId": {
                            "$ref": "#/AttributeId",
                            "description": "Optional MEP system attribute."
                        },
                        "layerIndex": {
                            "type": "integer",
                            "description": "Optional layer attribute index of the routing element and all its parts (routing segments, rigid segments, elbows and transitions). Only a positive index takes effect; otherwise the routing tool default layer is used. A layer that does not exist or is locked gives an error for the item, and nothing is placed."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "domain",
                        "nodeCoordinates"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "routingElementsData"
        ]
    })";
}

GS::Optional<GS::UniString> CreateMEPRoutingElementsCommand::GetRawResponseSchema () const
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

GS::ObjectState CreateMEPRoutingElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::Array<GS::ObjectState> routingElementsData;
    if (!parameters.Get ("routingElementsData", routingElementsData)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'routingElementsData' parameter.");
    }

    GS::ObjectState response;
    const auto& elements = response.AddList<GS::ObjectState> ("elements");

    ACAPI_CallUndoableCommand ("Create MEP Routing Elements", [&]() -> GSErrCode {
        for (const GS::ObjectState& routingElementData : routingElementsData) {
            GS::UniString domainStr;
            routingElementData.Get ("domain", domainStr);
            const std::optional<Domain> domain = DomainFromString (domainStr);
            if (!domain.has_value ()) {
                elements (CreateErrorResponse (APIERR_BADPARS, "Invalid 'domain' parameter."));
                continue;
            }

            GS::Array<GS::ObjectState> nodeCoordinates;
            if (!routingElementData.Get ("nodeCoordinates", nodeCoordinates) || nodeCoordinates.GetSize () < 2) {
                elements (CreateErrorResponse (APIERR_BADPARS, "The 'nodeCoordinates' parameter must contain at least 2 coordinates."));
                continue;
            }

            // The layer is checked before anything is placed, so a missing or locked layer does not
            // leave a route on the routing tool's default layer.
            Int32 layerIndex = 0;
            const bool hasLayerIndex = routingElementData.Get ("layerIndex", layerIndex) && layerIndex > 0;
            if (hasLayerIndex) {
                const GS::Optional<GS::ObjectState> layerError = GetErrorIfLayerNotUsable (layerIndex);
                if (layerError.HasValue ()) {
                    elements (layerError.Get ());
                    continue;
                }
            }

            // The MEP system is resolved before anything is placed, so an unknown one does not place a route without a system.
            const GS::ObjectState* mepSystemId = routingElementData.Get ("mepSystemId");
            const API_AttributeIndex mepSystem = mepSystemId != nullptr ? GetAttributeIndexFromGuid (API_MEPSystemID, GetGuidFromObjectState (*mepSystemId)) : ACAPI_CreateAttributeIndex (-1);
            if (mepSystemId != nullptr && mepSystem == ACAPI_CreateAttributeIndex (-1)) {
                elements (CreateErrorResponse (APIERR_BADID, "The 'mepSystemId' is not an MEP system attribute."));
                continue;
            }

            ACAPI::Result<RoutingElementDefault> routingElementDefault = CreateRoutingElementDefault (domain.value ());
            if (routingElementDefault.IsErr ()) {
                elements (CreateErrorResponse (routingElementDefault.UnwrapErr ().kind, "Failed to create the routing element default."));
                continue;
            }

            RoutingSegmentDefault segmentDefault = routingElementDefault->GetRoutingSegmentDefault ();
            double width = 0.0;
            double height = 0.0;
            GS::UniString shapeStr;
            Int32 referenceId = -1;
            GS::UniString preferenceTableIdStr;
            const bool hasWidth = routingElementData.Get ("crossSectionWidth", width);
            const bool hasHeight = routingElementData.Get ("crossSectionHeight", height);
            const bool hasShape = routingElementData.Get ("crossSectionShape", shapeStr);
            const bool hasReferenceId = routingElementData.Get ("crossSectionReferenceId", referenceId);
            const bool hasPreferenceTableId = routingElementData.Get ("preferenceTableId", preferenceTableIdStr);
            if (hasWidth || hasHeight || hasShape || hasReferenceId || hasPreferenceTableId) {
                segmentDefault.Modify ([&](RoutingSegmentDefault::Modifier& modifier) {
                    // The preference table is set before the reference id, because the reference id is
                    // resolved against the table the segment uses.
                    if (hasPreferenceTableId) {
                        modifier.SetPreferenceTableId (Adapter::UniqueID (APIGuidFromString (preferenceTableIdStr.ToCStr ())));
                    }
                    if (hasShape) {
                        const std::optional<ConnectorShape> shape = ConnectorShapeFromString (shapeStr);
                        if (shape.has_value ()) {
                            modifier.SetCrossSectionShape (shape.value ());
                        }
                    }
                    if (hasWidth) {
                        modifier.SetCrossSectionWidth (width);
                    }
                    if (hasHeight) {
                        modifier.SetCrossSectionHeight (height);
                    }
                    if (hasReferenceId && referenceId >= 0) {
                        modifier.SetCrossSectionReferenceId (static_cast<uint32_t> (referenceId));
                    }
                });
            }

            routingElementDefault->Modify ([&](RoutingElementDefault::Modifier& modifier) {
                modifier.SetRoutingSegmentDefault (segmentDefault);
                if (mepSystemId != nullptr) {
                    modifier.SetMEPSystem (mepSystem);
                }
            });

            std::vector<API_Coord3D> coordinates;
            for (const GS::ObjectState& nodeCoordinate : nodeCoordinates) {
                coordinates.push_back (Get3DCoordinateFromObjectState (nodeCoordinate));
            }

            ACAPI::Result<ACAPI::MEP::UniqueID> newElementId = routingElementDefault->Place (
                coordinates,
                std::map<UInt32, RoutingSegmentRectangularCrossSectionData> {},
                std::map<UInt32, RoutingSegmentCircularCrossSectionData> {});
            if (newElementId.IsErr ()) {
                elements (CreateErrorResponse (newElementId.UnwrapErr ().kind, "Failed to place the routing element."));
                continue;
            }
            const ACAPI::MEP::UniqueID placedId = newElementId.Unwrap ();

            if (hasLayerIndex) {
                const GSErrCode layerErr = SetLayerOfRoutingElement (placedId, ACAPI_CreateAttributeIndex (layerIndex));
                if (layerErr != NoError) {
                    // The route stays placed, so its guid is given in the message to let the caller find it.
                    elements (CreateErrorResponse (layerErr, GS::UniString::Printf (
                        "The routing element with guid %T was placed, but the layer of it or of some of its parts could not be set.",
                        APIGuidToString (MEPUniqueIDToAPIGuid (placedId)).ToPrintf ())));
                    continue;
                }
            }

            elements (CreateElementIdObjectState (MEPUniqueIDToAPIGuid (placedId)));
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

CreateMEPElementsCommand::CreateMEPElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateMEPElementsCommand::GetName () const
{
    return "CreateMEPElements";
}

GS::Optional<GS::UniString> CreateMEPElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementsData": {
                "type": "array",
                "description": "Array of data to create MEP elements.",
                "items": {
                    "type": "object",
                    "properties": {
                        "type": {
                            "$ref": "#/MEPComponentType"
                        },
                        "domain": {
                            "$ref": "#/MEPDomain",
                            "description": "Required for all types except Equipment."
                        },
                        "position": {
                            "$ref": "#/Coordinate3D"
                        },
                        "orientationDirection": {
                            "$ref": "#/Coordinate3D",
                            "description": "Optional direction vector of the orientation. Defaults to (1, 0, 0)."
                        },
                        "orientationRotation": {
                            "$ref": "#/Coordinate3D",
                            "description": "Optional rotation vector of the orientation. Defaults to (0, 1, 0)."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "type",
                        "position"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementsData"
        ]
    })";
}

GS::Optional<GS::UniString> CreateMEPElementsCommand::GetRawResponseSchema () const
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

GS::ObjectState CreateMEPElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::Array<GS::ObjectState> elementsData;
    if (!parameters.Get ("elementsData", elementsData)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'elementsData' parameter.");
    }

    GS::ObjectState response;
    const auto& elements = response.AddList<GS::ObjectState> ("elements");

    ACAPI_CallUndoableCommand ("Create MEP Elements", [&]() -> GSErrCode {
        for (const GS::ObjectState& elementData : elementsData) {
            GS::UniString typeStr;
            elementData.Get ("type", typeStr);

            const GS::ObjectState* position = elementData.Get ("position");
            if (position == nullptr) {
                elements (CreateErrorResponse (APIERR_BADPARS, "Missing 'position' parameter."));
                continue;
            }

            GS::UniString domainStr;
            elementData.Get ("domain", domainStr);
            const std::optional<Domain> domain = DomainFromString (domainStr);
            if (typeStr != "Equipment" && !domain.has_value ()) {
                elements (CreateErrorResponse (APIERR_BADPARS, "The 'domain' parameter is required for this element type."));
                continue;
            }

            Orientation orientation = { { 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 } };
            const GS::ObjectState* orientationDirection = elementData.Get ("orientationDirection");
            if (orientationDirection != nullptr) {
                const API_Coord3D direction = Get3DCoordinateFromObjectState (*orientationDirection);
                orientation.direction = { direction.x, direction.y, direction.z };
            }
            const GS::ObjectState* orientationRotation = elementData.Get ("orientationRotation");
            if (orientationRotation != nullptr) {
                const API_Coord3D rotation = Get3DCoordinateFromObjectState (*orientationRotation);
                orientation.rotation = { rotation.x, rotation.y, rotation.z };
            }

            const API_Coord3D coordinate = Get3DCoordinateFromObjectState (*position);

            ACAPI::Result<ACAPI::MEP::UniqueID> newElementId = [&]() -> ACAPI::Result<ACAPI::MEP::UniqueID> {
                if (typeStr == "Terminal") {
                    ACAPI::Result<TerminalDefault> terminalDefault = CreateTerminalDefault (domain.value ());
                    if (terminalDefault.IsErr ()) {
                        return { terminalDefault.UnwrapErr (), terminalDefault.Token () };
                    }
                    return terminalDefault->Place (coordinate, orientation);
                }
                if (typeStr == "Accessory") {
                    ACAPI::Result<AccessoryDefault> accessoryDefault = CreateAccessoryDefault (domain.value ());
                    if (accessoryDefault.IsErr ()) {
                        return { accessoryDefault.UnwrapErr (), accessoryDefault.Token () };
                    }
                    return accessoryDefault->Place (coordinate, orientation);
                }
                if (typeStr == "Fitting") {
                    ACAPI::Result<FittingDefault> fittingDefault = CreateFittingDefault (domain.value ());
                    if (fittingDefault.IsErr ()) {
                        return { fittingDefault.UnwrapErr (), fittingDefault.Token () };
                    }
                    return fittingDefault->Place (coordinate, orientation);
                }
                ACAPI::Result<EquipmentDefault> equipmentDefault = CreateEquipmentDefault ();
                if (equipmentDefault.IsErr ()) {
                    return { equipmentDefault.UnwrapErr (), equipmentDefault.Token () };
                }
                return equipmentDefault->Place (coordinate, orientation);
            } ();

            if (newElementId.IsErr ()) {
                elements (CreateErrorResponse (newElementId.UnwrapErr ().kind, "Failed to place the MEP element."));
                continue;
            }

            elements (CreateElementIdObjectState (MEPUniqueIDToAPIGuid (newElementId.Unwrap ())));
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

ModifyMEPRoutingElementsCommand::ModifyMEPRoutingElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyMEPRoutingElementsCommand::GetName () const
{
    return "ModifyMEPRoutingElements";
}

GS::Optional<GS::UniString> ModifyMEPRoutingElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "routingElementsData": {
                "type": "array",
                "description": "Array of data to modify MEP routing elements. Only provided fields are changed.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "mepSystemId": {
                            "$ref": "#/AttributeId"
                        },
                        "crossSectionWidth": {
                            "type": "number",
                            "description": "New cross section width applied to all segments."
                        },
                        "crossSectionHeight": {
                            "type": "number",
                            "description": "New cross section height applied to all segments."
                        },
                        "crossSectionShape": {
                            "$ref": "#/MEPCrossSectionShape",
                            "description": "The new shape is applied to all segments."
                        },
                        "crossSectionReferenceId": {
                            "type": "integer",
                            "description": "New cross section reference id of the segment preference table applied to all segments (used for circular cross sections). Resolved against preferenceTableId if given, otherwise against the table the segments already use."
                        },
                        "preferenceTableId": {
                            "$ref": "#/Guid",
                            "description": "New segment preference table (guid as returned by GetMEPPreferenceTables) applied to all segments. Not applicable to the CableCarrier domain."
                        },
                        "branchPreferenceTableId": {
                            "$ref": "#/Guid",
                            "description": "New branch preference table (guid as returned by GetMEPPreferenceTables) of the routing element."
                        },
                        "anchor": {
                            "type": "string",
                            "description": "New anchor point of the cross section, on a 3x3 grid, applied to all segments.",
                            "enum": [
                                "TopLeft",
                                "TopCenter",
                                "TopRight",
                                "MiddleLeft",
                                "Center",
                                "MiddleRight",
                                "BottomLeft",
                                "BottomCenter",
                                "BottomRight"
                            ]
                        },
                        "referenceLineOffsetX": {
                            "type": "number",
                            "description": "New horizontal offset of the reference line from the anchor point, applied to all segments."
                        },
                        "referenceLineOffsetY": {
                            "type": "number",
                            "description": "New vertical offset of the reference line from the anchor point, applied to all segments."
                        },
                        "wallThickness": {
                            "type": "number",
                            "description": "New wall thickness applied to all segments."
                        },
                        "flangeSize": {
                            "type": "number",
                            "description": "New flange size applied to all segments."
                        },
                        "connectionStyle": {
                            "$ref": "#/MEPConnectionStyle",
                            "description": "New connection style applied to all segments."
                        },
                        "preferredTransitionPlacement": {
                            "$ref": "#/MEPPreferredTransitionPlacement",
                            "description": "New preferred transition placement applied to all nodes."
                        },
                        "cutPlanePosition": {
                            "$ref": "#/MEPCutPlanePosition",
                            "description": "New cut plane position of the routing element. Available from Archicad 29."
                        },
                        "floorPlanDisplayOption": {
                            "$ref": "#/MEPFloorPlanDisplayOption",
                            "description": "New floor plan display option of the routing element. Available from Archicad 29."
                        },
                        "showProjection": {
                            "$ref": "#/MEPShowProjection",
                            "description": "New show projection setting of the routing element. Available from Archicad 29."
                        },
                        "onStoryVisibility": {
                            "$ref": "#/MEPOnStoryVisibility",
                            "description": "New on-story visibility of the routing element. Available from Archicad 29."
                        },
                        "nodePositions": {
                            "type": "array",
                            "description": "New positions of the routing nodes. The size must match the number of nodes of the route.",
                            "items": {
                                "$ref": "#/Coordinate3D"
                            }
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
            "routingElementsData"
        ]
    })";
}

GS::Optional<GS::UniString> ModifyMEPRoutingElementsCommand::GetRawResponseSchema () const
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

GS::ObjectState ModifyMEPRoutingElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::Array<GS::ObjectState> routingElementsData;
    if (!parameters.Get ("routingElementsData", routingElementsData)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'routingElementsData' parameter.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    for (const GS::ObjectState& routingElementData : routingElementsData) {
        const GS::ObjectState* elementId = routingElementData.Get ("elementId");
        if (elementId == nullptr) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing 'elementId' parameter."));
            continue;
        }

        ACAPI::Result<RoutingElement> routingElement = RoutingElement::Get (Adapter::UniqueID (GetGuidFromObjectState (*elementId)));
        if (routingElement.IsErr ()) {
            executionResults (CreateFailedExecutionResult (routingElement.UnwrapErr ().kind, "Failed to get the routing element."));
            continue;
        }

        // The enum values are checked before anything is changed.
        GS::UniString connectionStyleStr;
        GS::UniString preferredTransitionPlacementStr;
        const bool hasConnectionStyle = routingElementData.Get ("connectionStyle", connectionStyleStr);
        const bool hasPreferredTransitionPlacement = routingElementData.Get ("preferredTransitionPlacement", preferredTransitionPlacementStr);
        const std::optional<ConnectionStyle> connectionStyle = MEPEnumFromString (ConnectionStyleNames, connectionStyleStr);
        const std::optional<PreferredTransitionPlacement> preferredTransitionPlacement = MEPEnumFromString (PreferredTransitionPlacementNames, preferredTransitionPlacementStr);
        if ((hasConnectionStyle && !connectionStyle.has_value ()) || (hasPreferredTransitionPlacement && !preferredTransitionPlacement.has_value ())) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Invalid 'connectionStyle' or 'preferredTransitionPlacement' value."));
            continue;
        }

        GS::UniString cutPlanePositionStr;
        GS::UniString floorPlanDisplayOptionStr;
        GS::UniString showProjectionStr;
        GS::UniString onStoryVisibilityStr;
        const bool hasCutPlanePosition = routingElementData.Get ("cutPlanePosition", cutPlanePositionStr);
        const bool hasFloorPlanDisplayOption = routingElementData.Get ("floorPlanDisplayOption", floorPlanDisplayOptionStr);
        const bool hasShowProjection = routingElementData.Get ("showProjection", showProjectionStr);
        const bool hasOnStoryVisibility = routingElementData.Get ("onStoryVisibility", onStoryVisibilityStr);
        const bool hasFloorPlanDisplay = hasCutPlanePosition || hasFloorPlanDisplayOption || hasShowProjection || hasOnStoryVisibility;
#ifdef ServerMainVers_2900
        const std::optional<CutPlanePosition> cutPlanePosition = MEPEnumFromString (CutPlanePositionNames, cutPlanePositionStr);
        const std::optional<FloorPlanDisplayOption> floorPlanDisplayOption = MEPEnumFromString (FloorPlanDisplayOptionNames, floorPlanDisplayOptionStr);
        const std::optional<ShowProjection> showProjection = MEPEnumFromString (ShowProjectionNames, showProjectionStr);
        const std::optional<OnStoryVisibility> onStoryVisibility = MEPEnumFromString (OnStoryVisibilityNames, onStoryVisibilityStr);
        if ((hasCutPlanePosition && !cutPlanePosition.has_value ()) ||
            (hasFloorPlanDisplayOption && !floorPlanDisplayOption.has_value ()) ||
            (hasShowProjection && !showProjection.has_value ()) ||
            (hasOnStoryVisibility && !onStoryVisibility.has_value ())) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Invalid 'cutPlanePosition', 'floorPlanDisplayOption', 'showProjection' or 'onStoryVisibility' value."));
            continue;
        }
#else
        if (hasFloorPlanDisplay) {
            executionResults (CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "'cutPlanePosition', 'floorPlanDisplayOption', 'showProjection' and 'onStoryVisibility' require Archicad 29 or newer."));
            continue;
        }
#endif

        // The MEP system is resolved before anything is changed, so an unknown one does not clear the route's system.
        const GS::ObjectState* mepSystemId = routingElementData.Get ("mepSystemId");
        const API_AttributeIndex mepSystem = mepSystemId != nullptr ? GetAttributeIndexFromGuid (API_MEPSystemID, GetGuidFromObjectState (*mepSystemId)) : ACAPI_CreateAttributeIndex (-1);
        if (mepSystemId != nullptr && mepSystem == ACAPI_CreateAttributeIndex (-1)) {
            executionResults (CreateFailedExecutionResult (APIERR_BADID, "The 'mepSystemId' is not an MEP system attribute."));
            continue;
        }

        bool success = true;

        if (mepSystemId != nullptr) {
            ACAPI::Result<void> result = routingElement->Modify ([&](RoutingElement::Modifier& modifier) {
                modifier.SetMEPSystem (mepSystem);
            }, "Modify MEP System of Routing Element");
            success &= result.IsOk ();
        }

#ifdef ServerMainVers_2900
        if (hasFloorPlanDisplay) {
            bool settersSucceeded = true;
            ACAPI::Result<void> result = routingElement->Modify ([&](RoutingElement::Modifier& modifier) {
                // The floor plan display option first, the projection setting may depend on it.
                if (floorPlanDisplayOption.has_value ()) {
                    settersSucceeded &= modifier.SetFloorPlanDisplayOption (floorPlanDisplayOption.value ()).IsOk ();
                }
                if (cutPlanePosition.has_value ()) {
                    modifier.SetCutPlanePosition (cutPlanePosition.value ());
                }
                if (showProjection.has_value ()) {
                    settersSucceeded &= modifier.SetShowProjection (showProjection.value ()).IsOk ();
                }
                if (onStoryVisibility.has_value ()) {
                    modifier.SetOnStoryVisibility (onStoryVisibility.value ());
                }
            }, "Modify Floor Plan Display of Routing Element");
            success &= result.IsOk () && settersSucceeded;
        }
#endif

        GS::UniString branchPreferenceTableIdStr;
        if (routingElementData.Get ("branchPreferenceTableId", branchPreferenceTableIdStr)) {
            bool setterSucceeded = true;
            ACAPI::Result<void> result = routingElement->Modify ([&](RoutingElement::Modifier& modifier) {
                setterSucceeded = modifier.SetBranchPreferenceTableId (Adapter::UniqueID (APIGuidFromString (branchPreferenceTableIdStr.ToCStr ()))).IsOk ();
            }, "Modify Branch Preference Table of Routing Element");
            success &= result.IsOk () && setterSucceeded;
        }

        double width = 0.0;
        double height = 0.0;
        GS::UniString shapeStr;
        Int32 referenceId = -1;
        GS::UniString preferenceTableIdStr;
        const bool hasWidth = routingElementData.Get ("crossSectionWidth", width);
        const bool hasHeight = routingElementData.Get ("crossSectionHeight", height);
        const bool hasShape = routingElementData.Get ("crossSectionShape", shapeStr);
        const bool hasReferenceId = routingElementData.Get ("crossSectionReferenceId", referenceId);
        const bool hasPreferenceTableId = routingElementData.Get ("preferenceTableId", preferenceTableIdStr);
        GS::UniString anchorStr;
        double referenceLineOffsetX = 0.0;
        double referenceLineOffsetY = 0.0;
        double wallThickness = 0.0;
        double flangeSize = 0.0;
        const bool hasAnchor = routingElementData.Get ("anchor", anchorStr);
        const bool hasReferenceLineOffsetX = routingElementData.Get ("referenceLineOffsetX", referenceLineOffsetX);
        const bool hasReferenceLineOffsetY = routingElementData.Get ("referenceLineOffsetY", referenceLineOffsetY);
        const bool hasWallThickness = routingElementData.Get ("wallThickness", wallThickness);
        const bool hasFlangeSize = routingElementData.Get ("flangeSize", flangeSize);
        if (hasWidth || hasHeight || hasShape || hasReferenceId || hasPreferenceTableId ||
            hasAnchor || hasReferenceLineOffsetX || hasReferenceLineOffsetY || hasWallThickness || hasFlangeSize || hasConnectionStyle) {
            for (const ACAPI::MEP::UniqueID& segmentId : routingElement->GetRoutingSegmentIds ()) {
                ACAPI::Result<RoutingSegment> segment = RoutingSegment::Get (segmentId);
                if (segment.IsErr ()) {
                    success = false;
                    continue;
                }
                bool settersSucceeded = true;
                ACAPI::Result<void> result = segment->Modify ([&](RoutingSegment::Modifier& modifier) {
                    // Same order as in CreateMEPRoutingElements: table first, then the reference id
                    // that is resolved against it.
                    if (hasPreferenceTableId) {
                        modifier.SetPreferenceTableId (Adapter::UniqueID (APIGuidFromString (preferenceTableIdStr.ToCStr ())));
                    }
                    if (hasShape) {
                        const std::optional<ConnectorShape> shape = ConnectorShapeFromString (shapeStr);
                        if (shape.has_value ()) {
                            modifier.SetCrossSectionShape (shape.value ());
                        }
                    }
                    if (hasWidth) {
                        modifier.SetCrossSectionWidth (width);
                    }
                    if (hasHeight) {
                        modifier.SetCrossSectionHeight (height);
                    }
                    if (hasReferenceId && referenceId >= 0) {
                        modifier.SetCrossSectionReferenceId (static_cast<uint32_t> (referenceId));
                    }
                    if (hasAnchor) {
                        modifier.SetAnchor (AnchorIdFromString (anchorStr));
                    }
                    if (hasReferenceLineOffsetX) {
                        modifier.SetReferenceLineOffsetX (referenceLineOffsetX);
                    }
                    if (hasReferenceLineOffsetY) {
                        modifier.SetReferenceLineOffsetY (referenceLineOffsetY);
                    }
                    if (hasWallThickness) {
                        settersSucceeded &= modifier.SetWallThickness (wallThickness).IsOk ();
                    }
                    if (hasFlangeSize) {
                        settersSucceeded &= modifier.SetFlangeSize (flangeSize).IsOk ();
                    }
                    if (connectionStyle.has_value ()) {
                        settersSucceeded &= modifier.SetConnectionStyle (connectionStyle.value ()).IsOk ();
                    }
                }, "Modify Routing Segment");
                success &= result.IsOk () && settersSucceeded;
            }
        }

        if (preferredTransitionPlacement.has_value ()) {
            for (const ACAPI::MEP::UniqueID& nodeId : routingElement->GetRoutingNodeIds ()) {
                ACAPI::Result<RoutingNode> node = RoutingNode::Get (nodeId);
                if (node.IsErr ()) {
                    success = false;
                    continue;
                }
                ACAPI::Result<void> result = node->Modify ([&](RoutingNode::Modifier& modifier) {
                    modifier.SetPreferredTransitionPlacement (preferredTransitionPlacement.value ());
                }, "Modify Preferred Transition Placement of Routing Node");
                success &= result.IsOk ();
            }
        }

        GS::Array<GS::ObjectState> nodePositions;
        if (routingElementData.Get ("nodePositions", nodePositions)) {
            const std::vector<ACAPI::MEP::UniqueID> nodeIds = routingElement->GetRoutingNodeIds ();
            if (nodePositions.GetSize () != nodeIds.size ()) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "The size of 'nodePositions' must match the number of nodes of the route."));
                continue;
            }
            for (USize i = 0; i < nodePositions.GetSize (); ++i) {
                ACAPI::Result<RoutingNode> node = RoutingNode::Get (nodeIds[i]);
                if (node.IsErr ()) {
                    success = false;
                    continue;
                }
                const API_Coord3D newPosition = Get3DCoordinateFromObjectState (nodePositions[i]);
                ACAPI::Result<void> result = node->Modify ([&](RoutingNode::Modifier& modifier) {
                    modifier.SetPosition (newPosition);
                }, "Modify Position of Routing Node");
                success &= result.IsOk ();
            }
        }

        executionResults (success
            ? CreateSuccessfulExecutionResult ()
            : CreateFailedExecutionResult (APIERR_GENERAL, "Failed to modify the routing element."));
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

ModifyMEPPortsCommand::ModifyMEPPortsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyMEPPortsCommand::GetName () const
{
    return "ModifyMEPPorts";
}

GS::Optional<GS::UniString> ModifyMEPPortsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "portsData": {
                "type": "array",
                "description": "Array of data to modify MEP ports. Only provided fields are changed.",
                "items": {
                    "type": "object",
                    "properties": {
                        "portId": {
                            "$ref": "#/Guid",
                            "description": "The port to modify, as returned by GetMEPPorts."
                        },
                        "shape": {
                            "$ref": "#/MEPCrossSectionShape",
                            "description": "New shape of the port."
                        },
                        "connectionStyle": {
                            "$ref": "#/MEPConnectionStyle",
                            "description": "New connection style of the port. Available from Archicad 30."
                        },
                        "flowDirection": {
                            "$ref": "#/MEPFlowDirection",
                            "description": "New flow direction of a piping or ventilation port."
                        },
                        "outerDiameter": {
                            "type": "number",
                            "description": "New outer diameter of a piping port."
                        },
                        "nominalSize": {
                            "type": "string",
                            "description": "New nominal size of a piping port. Available from Archicad 30."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "portId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "portsData"
        ]
    })";
}

GS::Optional<GS::UniString> ModifyMEPPortsCommand::GetRawResponseSchema () const
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

GS::ObjectState ModifyMEPPortsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::Array<GS::ObjectState> portsData;
    if (!parameters.Get ("portsData", portsData)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'portsData' parameter.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    for (const GS::ObjectState& portData : portsData) {
        GS::UniString portIdStr;
        if (!portData.Get ("portId", portIdStr)) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing 'portId' parameter."));
            continue;
        }

        const ACAPI::MEP::UniqueID portId = Adapter::UniqueID (APIGuidFromString (portIdStr.ToCStr ()));
        ACAPI::Result<Port> port = Port::Get (portId);
        if (port.IsErr ()) {
            executionResults (CreateFailedExecutionResult (port.UnwrapErr ().kind, "Failed to get the MEP port."));
            continue;
        }

        // The values are checked before anything is changed.
        GS::UniString shapeStr;
        GS::UniString connectionStyleStr;
        GS::UniString flowDirectionStr;
        GS::UniString nominalSize;
        double outerDiameter = 0.0;
        const bool hasShape = portData.Get ("shape", shapeStr);
        const bool hasConnectionStyle = portData.Get ("connectionStyle", connectionStyleStr);
        const bool hasFlowDirection = portData.Get ("flowDirection", flowDirectionStr);
        const bool hasNominalSize = portData.Get ("nominalSize", nominalSize);
        const bool hasOuterDiameter = portData.Get ("outerDiameter", outerDiameter);
        const std::optional<ConnectorShape> shape = ConnectorShapeFromString (shapeStr);
        const std::optional<ConnectionStyle> connectionStyle = MEPEnumFromString (ConnectionStyleNames, connectionStyleStr);
        const std::optional<FlowDirection> flowDirection = MEPEnumFromString (FlowDirectionNames, flowDirectionStr);
        if ((hasShape && !shape.has_value ()) ||
            (hasConnectionStyle && !connectionStyle.has_value ()) ||
            (hasFlowDirection && !flowDirection.has_value ())) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Invalid 'shape', 'connectionStyle' or 'flowDirection' value."));
            continue;
        }
#ifndef ServerMainVers_3000
        if (hasConnectionStyle || hasNominalSize) {
            executionResults (CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "'connectionStyle' and 'nominalSize' require Archicad 30 or newer."));
            continue;
        }
#endif

        // Flow direction, outer diameter and nominal size live on the domain specific port classes.
        const Domain domain = port->GetDomain ();
        if ((hasFlowDirection && domain == Domain::CableCarrier) ||
            ((hasOuterDiameter || hasNominalSize) && domain != Domain::Piping)) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "'flowDirection' needs a piping or ventilation port, 'outerDiameter' and 'nominalSize' need a piping port."));
            continue;
        }

        bool success = true;

        if (hasShape || hasConnectionStyle) {
            bool settersSucceeded = true;
            ACAPI::Result<void> result = port->Modify ([&](PortBase::Modifier& modifier) {
                if (shape.has_value ()) {
                    settersSucceeded &= modifier.SetShape (shape.value ()).IsOk ();
                }
#ifdef ServerMainVers_3000
                if (connectionStyle.has_value ()) {
                    settersSucceeded &= modifier.SetConnectionStyle (connectionStyle.value ()).IsOk ();
                }
#endif
            }, "Modify MEP Port");
            success &= result.IsOk () && settersSucceeded;
        }

        if (hasFlowDirection || hasOuterDiameter || hasNominalSize) {
            bool settersSucceeded = true;
            if (domain == Domain::Piping) {
                ACAPI::Result<PipingPort> pipingPort = PipingPort::Get (portId);
                if (pipingPort.IsErr ()) {
                    success = false;
                } else {
                    ACAPI::Result<void> result = pipingPort->Modify ([&](PipingPort::Modifier& modifier) {
                        if (flowDirection.has_value ()) {
#ifdef ServerMainVers_2900
                            settersSucceeded &= modifier.SetFlowDirection (flowDirection.value ()).IsOk ();
#else
                            // Returns void in Archicad 28.
                            modifier.SetFlowDirection (flowDirection.value ());
#endif
                        }
                        if (hasOuterDiameter) {
                            settersSucceeded &= modifier.SetOuterDiameter (outerDiameter).IsOk ();
                        }
#ifdef ServerMainVers_3000
                        if (hasNominalSize) {
                            settersSucceeded &= modifier.SetNominalSize (nominalSize).IsOk ();
                        }
#endif
                    }, "Modify MEP Piping Port");
                    success &= result.IsOk () && settersSucceeded;
                }
            } else {
                ACAPI::Result<VentilationPort> ventilationPort = VentilationPort::Get (portId);
                if (ventilationPort.IsErr ()) {
                    success = false;
                } else {
                    ACAPI::Result<void> result = ventilationPort->Modify ([&](VentilationPort::Modifier& modifier) {
                        if (flowDirection.has_value ()) {
#ifdef ServerMainVers_2900
                            settersSucceeded &= modifier.SetFlowDirection (flowDirection.value ()).IsOk ();
#else
                            // Returns void in Archicad 28.
                            modifier.SetFlowDirection (flowDirection.value ());
#endif
                        }
                    }, "Modify MEP Ventilation Port");
                    success &= result.IsOk () && settersSucceeded;
                }
            }
        }

        executionResults (success
            ? CreateSuccessfulExecutionResult ()
            : CreateFailedExecutionResult (APIERR_GENERAL, "Failed to modify the MEP port."));
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

ConnectMEPElementsCommand::ConnectMEPElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ConnectMEPElementsCommand::GetName () const
{
    return "ConnectMEPElements";
}

GS::Optional<GS::UniString> ConnectMEPElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "connectionsData": {
                "type": "array",
                "description": "Array of connections to create.",
                "items": {
                    "type": "object",
                    "properties": {
                        "routingElementId": {
                            "$ref": "#/ElementId",
                            "description": "The routing element to connect."
                        },
                        "connectToId": {
                            "$ref": "#/ElementId",
                            "description": "The MEP element or routing element to connect to."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "routingElementId",
                        "connectToId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "connectionsData"
        ]
    })";
}

GS::Optional<GS::UniString> ConnectMEPElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "connectionResults": {
                "$ref": "#/MEPConnectionResultsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "connectionResults"
        ]
    })";
}

GS::ObjectState ConnectMEPElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::Array<GS::ObjectState> connectionsData;
    if (!parameters.Get ("connectionsData", connectionsData)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'connectionsData' parameter.");
    }

    GS::ObjectState response;
    const auto& connectionResults = response.AddList<GS::ObjectState> ("connectionResults");

    for (const GS::ObjectState& connectionData : connectionsData) {
        const GS::ObjectState* routingElementId = connectionData.Get ("routingElementId");
        const GS::ObjectState* connectToId = connectionData.Get ("connectToId");
        if (routingElementId == nullptr || connectToId == nullptr) {
            connectionResults (CreateErrorResponse (APIERR_BADPARS, "Missing 'routingElementId' or 'connectToId' parameter."));
            continue;
        }

        ACAPI::Result<RoutingElement> routingElement = RoutingElement::Get (Adapter::UniqueID (GetGuidFromObjectState (*routingElementId)));
        if (routingElement.IsErr ()) {
            connectionResults (CreateErrorResponse (routingElement.UnwrapErr ().kind, "Failed to get the routing element."));
            continue;
        }

        std::optional<RouteConnectionResult> connectionResult;
        std::optional<ACAPI::Error> connectionError;
        ACAPI::Result<void> modifyResult = routingElement->Modify ([&](RoutingElement::Modifier& modifier) {
            ACAPI::Result<RouteConnectionResult> result = modifier.ConnectLogically (Adapter::UniqueID (GetGuidFromObjectState (*connectToId)));
            if (result.IsOk ()) {
                connectionResult = result.Unwrap ();
            } else {
                connectionError = result.UnwrapErr ();
            }
        }, "Connect MEP Elements");

        if (connectionError.has_value ()) {
            connectionResults (CreateErrorResponse (connectionError->kind, "Failed to connect the elements."));
            continue;
        }
        if (modifyResult.IsErr () || !connectionResult.has_value ()) {
            connectionResults (CreateErrorResponse (modifyResult.IsErr () ? modifyResult.UnwrapErr ().kind : APIERR_GENERAL, "Failed to connect the elements."));
            continue;
        }

        GS::ObjectState connectionResultOs;
        if (connectionResult->deletedRoutingElementId.has_value ()) {
            connectionResultOs.Add ("deletedRoutingElementId", CreateGuidObjectState (MEPUniqueIDToAPIGuid (connectionResult->deletedRoutingElementId.value ())));
        }
        if (connectionResult->splittedRoutingElementId.has_value ()) {
            connectionResultOs.Add ("splitRoutingElementId", CreateGuidObjectState (MEPUniqueIDToAPIGuid (connectionResult->splittedRoutingElementId.value ())));
        }
        if (connectionResult->createdBranchId.has_value ()) {
            connectionResultOs.Add ("createdBranchId", CreateGuidObjectState (MEPUniqueIDToAPIGuid (connectionResult->createdBranchId.value ())));
        }
        connectionResults (connectionResultOs);
    }

    return response;
#elif defined (ServerMainVers_2700)
    GS::Array<GS::ObjectState> connectionsData;
    if (!parameters.Get ("connectionsData", connectionsData)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'connectionsData' parameter.");
    }

    GS::ObjectState response;
    const auto& connectionResults = response.AddList<GS::ObjectState> ("connectionResults");

    for (const GS::ObjectState& connectionData : connectionsData) {
        const GS::ObjectState* routingElementId = connectionData.Get ("routingElementId");
        const GS::ObjectState* connectToId = connectionData.Get ("connectToId");
        if (routingElementId == nullptr || connectToId == nullptr) {
            connectionResults (CreateErrorResponse (APIERR_BADPARS, "Missing 'routingElementId' or 'connectToId' parameter."));
            continue;
        }

        ACAPI::Result<RoutingElement> routingElement = RoutingElement::Get (Adapter::UniqueID (GetGuidFromObjectState (*routingElementId)));
        if (routingElement.IsErr ()) {
            connectionResults (CreateErrorResponse (routingElement.UnwrapErr ().kind, "Failed to get the routing element."));
            continue;
        }

        // Archicad 27 connects through RoutingElement::Modifier::Connect, which reports no result.
        std::optional<ACAPI::Error> connectionError;
        ACAPI::Result<void> modifyResult = routingElement->Modify ([&](RoutingElement::Modifier& modifier) {
            ACAPI::Result<void> result = modifier.Connect (Adapter::UniqueID (GetGuidFromObjectState (*connectToId)));
            if (result.IsErr ()) {
                connectionError = result.UnwrapErr ();
            }
        }, "Connect MEP Elements");

        if (connectionError.has_value ()) {
            connectionResults (CreateErrorResponse (connectionError->kind, "Failed to connect the elements."));
            continue;
        }
        if (modifyResult.IsErr ()) {
            connectionResults (CreateErrorResponse (modifyResult.UnwrapErr ().kind, "Failed to connect the elements."));
            continue;
        }

        connectionResults (GS::ObjectState ());
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
}

CheckMEPPortConnectabilityCommand::CheckMEPPortConnectabilityCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CheckMEPPortConnectabilityCommand::GetName () const
{
    return "CheckMEPPortConnectability";
}

GS::Optional<GS::UniString> CheckMEPPortConnectabilityCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "portPairs": {
                "type": "array",
                "description": "The pairs of ports to check.",
                "items": {
                    "type": "object",
                    "properties": {
                        "portId": {
                            "$ref": "#/Guid",
                            "description": "The port to check, as returned by GetMEPPorts."
                        },
                        "otherPortId": {
                            "$ref": "#/Guid",
                            "description": "The port to connect it to, as returned by GetMEPPorts."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "portId",
                        "otherPortId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "portPairs"
        ]
    })";
}

GS::Optional<GS::UniString> CheckMEPPortConnectabilityCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "results": {
                "type": "array",
                "description": "The result for each pair of ports, in the order of the input.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "isConnectable": {
                                    "type": "boolean",
                                    "description": "Whether the port can be connected to the other port."
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "isConnectable"
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
            "results"
        ]
    })";
}

GS::ObjectState CheckMEPPortConnectabilityCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::Array<GS::ObjectState> portPairs;
    if (!parameters.Get ("portPairs", portPairs)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'portPairs' parameter.");
    }

    GS::ObjectState response;
    const auto& results = response.AddList<GS::ObjectState> ("results");

    for (const GS::ObjectState& portPair : portPairs) {
        GS::UniString portIdStr;
        GS::UniString otherPortIdStr;
        if (!portPair.Get ("portId", portIdStr) || !portPair.Get ("otherPortId", otherPortIdStr)) {
            results (CreateErrorResponse (APIERR_BADPARS, "Missing 'portId' or 'otherPortId' parameter."));
            continue;
        }

        const ACAPI::MEP::UniqueID otherPortId = Adapter::UniqueID (APIGuidFromString (otherPortIdStr.ToCStr ()));
        ACAPI::Result<Port> port = Port::Get (Adapter::UniqueID (APIGuidFromString (portIdStr.ToCStr ())));
        ACAPI::Result<Port> otherPort = Port::Get (otherPortId);
        if (port.IsErr () || !port->Exists () || otherPort.IsErr () || !otherPort->Exists ()) {
            results (CreateErrorResponse (APIERR_BADID, "The port given in 'portId' or 'otherPortId' was not found."));
            continue;
        }

#ifdef ServerMainVers_2800
        ACAPI::Result<bool> isConnectable = port->IsPhysicallyConnectable (otherPortId);
#else
        ACAPI::Result<bool> isConnectable = port->IsConnectable (otherPortId);
#endif
        if (isConnectable.IsErr ()) {
            results (CreateErrorResponse (isConnectable.UnwrapErr ().kind, "Failed to check whether the ports can be connected."));
            continue;
        }

        results (GS::ObjectState ("isConnectable", isConnectable.Unwrap ()));
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
}

GetMEPPreferenceTablesCommand::GetMEPPreferenceTablesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetMEPPreferenceTablesCommand::GetName () const
{
    return "GetMEPPreferenceTables";
}

GS::Optional<GS::UniString> GetMEPPreferenceTablesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "domain": {
                "$ref": "#/MEPPreferenceTableDomain"
            },
            "includeFittingTables": {
                "type": "boolean",
                "description": "Also return the elbow, branch and transition preference tables. Defaults to false. Ignored for the CableCarrier domain."
            }
        },
        "additionalProperties": false,
        "required": ["domain"]
    })";
}

GS::Optional<GS::UniString> GetMEPPreferenceTablesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "tables": {
                "type": "array",
                "description": "The circular segment preference tables of the domain. Empty for the CableCarrier domain.",
                "items": {
                    "$ref": "#/MEPPreferenceTable"
                }
            },
            "rectangularTables": {
                "type": "array",
                "description": "The rectangular segment preference tables of the Ventilation domain.",
                "items": {
                    "$ref": "#/MEPValuePreferenceTable"
                }
            },
            "cableCarrierTables": {
                "type": "array",
                "description": "The segment preference tables of the CableCarrier domain.",
                "items": {
                    "$ref": "#/MEPValuePreferenceTable"
                }
            },
            "referenceSet": {
                "$ref": "#/MEPReferenceSet",
                "description": "The reference set of the Piping or Ventilation domain. Not returned for the CableCarrier domain."
            },
            "angleSet": {
                "$ref": "#/MEPAngleSet",
                "description": "Available from Archicad 29."
            },
            "elbowTables": {
                "type": "array",
                "description": "The elbow preference tables of the domain. Only when includeFittingTables is true.",
                "items": {
                    "$ref": "#/MEPElbowPreferenceTable"
                }
            },
            "branchTables": {
                "type": "array",
                "description": "The branch preference tables of the domain. Only when includeFittingTables is true.",
                "items": {
                    "$ref": "#/MEPBranchPreferenceTable"
                }
            },
            "transitionTables": {
                "type": "array",
                "description": "The transition preference tables of the domain. Only when includeFittingTables is true, available from Archicad 29.",
                "items": {
                    "$ref": "#/MEPTransitionPreferenceTable"
                }
            }
        },
        "additionalProperties": false,
        "required": ["tables"]
    })";
}

#ifdef ServerMainVers_2800
// The preference table getters return either a plain value, a std::optional or an ACAPI::Result.
// PreferenceTableValueToOptional turns all of them into a std::optional, so a value that cannot
// be read is left out of the response instead of failing the whole table.
// IsEditable returns an ACAPI::Result<void> that is Ok when the table is editable; PreferenceTableIsEditable
// also handles a Result<bool> or a bool, so a read-only table is never reported as editable.
template <typename T, typename = void>
struct IsPreferenceTableResult : std::false_type {};

template <typename T>
struct IsPreferenceTableResult<T, std::void_t<decltype (std::declval<T&> ().IsOk ())>> : std::true_type {};

template <typename T>
struct IsPreferenceTableOptional : std::false_type {};

template <typename T>
struct IsPreferenceTableOptional<std::optional<T>> : std::true_type {};

template <typename T>
static auto PreferenceTableValueToOptional (T&& value)
{
    using ValueT = std::decay_t<T>;
    if constexpr (IsPreferenceTableResult<ValueT>::value) {
        using ResultValueT = std::decay_t<decltype (*value)>;
        return value.IsOk () ? std::optional<ResultValueT> (*value) : std::optional<ResultValueT> ();
    } else if constexpr (IsPreferenceTableOptional<ValueT>::value) {
        return ValueT (std::forward<T> (value));
    } else {
        return std::optional<ValueT> (std::forward<T> (value));
    }
}

template <typename T, typename = void>
struct HasPreferenceTableResultValue : std::false_type {};

template <typename T>
struct HasPreferenceTableResultValue<T, std::enable_if_t<!std::is_void_v<decltype (*std::declval<T&> ())>>> : std::true_type {};

template <typename T>
static bool PreferenceTableIsEditable (T&& isEditable)
{
    using ValueT = std::decay_t<T>;
    if constexpr (std::is_same_v<ValueT, bool>) {
        return isEditable;
    } else if constexpr (HasPreferenceTableResultValue<ValueT>::value) {
        return isEditable.IsOk () && static_cast<bool> (*isEditable);
    } else {
        return isEditable.IsOk ();
    }
}

template <typename T>
static void AddPreferenceTableValue (GS::ObjectState& os, const char* name, T&& value)
{
    const auto optionalValue = PreferenceTableValueToOptional (std::forward<T> (value));
    if (optionalValue.has_value ()) {
        os.Add (name, *optionalValue);
    }
}

// The segmentation lengths of a segment preference table, available from Archicad 29.
template <typename TableT>
static void AddSegmentationLengths (GS::ObjectState& tableOS, const TableT& table)
{
#ifdef ServerMainVers_2900
    AddPreferenceTableValue (tableOS, "useIndividualSegmentLengths", table.GetUseIndividualSegmentLengths ());
    AddPreferenceTableValue (tableOS, "minimumSegmentationLength", table.GetMinimumSegmentationLength ());
    AddPreferenceTableValue (tableOS, "maximumSegmentationLength", table.GetMaximumSegmentationLength ());
#else
    UNUSED_PARAMETER (tableOS);
    UNUSED_PARAMETER (table);
#endif
}

template <typename TableT>
static GS::ObjectState DumpPreferenceTable (const TableT& table, const GS::Guid& tableGuid)
{
    GS::ObjectState tableOS;
    tableOS.Add ("guid", tableGuid.ToUniString ());
    AddPreferenceTableValue (tableOS, "name", table.GetName ());
    tableOS.Add ("isEditable", PreferenceTableIsEditable (table.IsEditable ()));
    AddSegmentationLengths (tableOS, table);
    const auto& rows = tableOS.AddList<GS::ObjectState> ("rows");
    const uint32_t size = table.GetSize ();
    for (uint32_t i = 0; i < size; ++i) {
        auto rowDiameter = table.GetDiameter (i);
        auto rowReferenceId = table.GetReferenceId (i);
        if (rowDiameter.IsErr () || rowReferenceId.IsErr ()) {
            continue;
        }
        GS::ObjectState rowOS ("index", static_cast<Int32> (i));
        rowOS.Add ("referenceId", static_cast<Int32> (*rowReferenceId));
        rowOS.Add ("diameter", *rowDiameter);
        AddPreferenceTableValue (rowOS, "wallThickness", table.GetWallThickness (i));
        auto rowDescription = table.GetDescription (i);
        if (rowDescription.IsOk ()) {
            rowOS.Add ("description", *rowDescription);
        }
        AddPreferenceTableValue (rowOS, "isValid", table.IsRowValid (i));
        rows (rowOS);
    }
    return tableOS;
}

// Rectangular duct and cable carrier segment preference tables are a list of sizes (values) with a description.
template <typename TableT>
static GS::ObjectState DumpValuePreferenceTable (const TableT& table, const GS::Guid& tableGuid)
{
    GS::ObjectState tableOS;
    tableOS.Add ("guid", tableGuid.ToUniString ());
    AddPreferenceTableValue (tableOS, "name", table.GetName ());
    tableOS.Add ("isEditable", PreferenceTableIsEditable (table.IsEditable ()));
    AddSegmentationLengths (tableOS, table);
    const auto& rows = tableOS.AddList<GS::ObjectState> ("rows");
    const uint32_t size = static_cast<uint32_t> (PreferenceTableValueToOptional (table.GetSize ()).value_or (0));
    for (uint32_t i = 0; i < size; ++i) {
        const auto value = PreferenceTableValueToOptional (table.GetValue (i));
        if (!value.has_value ()) {
            continue;
        }
        GS::ObjectState rowOS ("index", static_cast<Int32> (i));
        rowOS.Add ("value", *value);
        AddPreferenceTableValue (rowOS, "description", table.GetDescription (i));
        rows (rowOS);
    }
    return tableOS;
}

static void AddRowDescription (GS::ObjectState& rowOS, const ACAPI::Result<GS::UniString>& description)
{
    if (description.IsOk ()) {
        rowOS.Add ("description", *description);
    }
}

template <typename TableT>
static GS::ObjectState DumpElbowPreferenceTable (const TableT& table, const GS::Guid& tableGuid)
{
    GS::ObjectState tableOS;
    tableOS.Add ("guid", tableGuid.ToUniString ());
    tableOS.Add ("name", table.GetName ());
    tableOS.Add ("isEditable", PreferenceTableIsEditable (table.IsEditable ()));
    const auto& rows = tableOS.AddList<GS::ObjectState> ("rows");
    for (uint32_t i = 0; i < table.GetSize (); ++i) {
        auto referenceId = table.GetReferenceId (i);
        auto diameter = table.GetDiameter (i);
        auto radius = table.GetRadius (i);
        if (referenceId.IsErr () || diameter.IsErr () || radius.IsErr ()) {
            continue;
        }
        GS::ObjectState rowOS ("index", static_cast<Int32> (i));
        rowOS.Add ("referenceId", static_cast<Int32> (*referenceId));
#ifdef ServerMainVers_2900
        auto angle = table.GetAngle (i);
        if (angle.IsOk ()) {
            rowOS.Add ("angle", *angle);
        }
#endif
        rowOS.Add ("diameter", *diameter);
        rowOS.Add ("radius", *radius);
        AddRowDescription (rowOS, table.GetDescription (i));
        AddPreferenceTableValue (rowOS, "isValid", table.IsRowValid (i));
        rows (rowOS);
    }
    return tableOS;
}

template <typename TableT>
static GS::ObjectState DumpBranchPreferenceTable (const TableT& table, const GS::Guid& tableGuid)
{
    GS::ObjectState tableOS;
    tableOS.Add ("guid", tableGuid.ToUniString ());
    tableOS.Add ("name", table.GetName ());
    tableOS.Add ("isEditable", PreferenceTableIsEditable (table.IsEditable ()));
    const auto& rows = tableOS.AddList<GS::ObjectState> ("rows");
    for (uint32_t i = 0; i < table.GetSize (); ++i) {
        auto mainAxisReferenceId = table.GetMainAxisReferenceId (i);
        auto branchAxisReferenceId = table.GetBranchAxisReferenceId (i);
        auto mainAxisDiameter = table.GetMainAxisDiameter (i);
        auto branchAxisDiameter = table.GetBranchAxisDiameter (i);
        auto length = table.GetLength (i);
        auto branchOffset = table.GetBranchOffset (i);
        auto connectionLength = table.GetConnectionLength (i);
        if (mainAxisReferenceId.IsErr () || branchAxisReferenceId.IsErr () || mainAxisDiameter.IsErr () || branchAxisDiameter.IsErr () ||
            length.IsErr () || branchOffset.IsErr () || connectionLength.IsErr ()) {
            continue;
        }
        GS::ObjectState rowOS ("index", static_cast<Int32> (i));
        rowOS.Add ("mainAxisReferenceId", static_cast<Int32> (*mainAxisReferenceId));
        rowOS.Add ("branchAxisReferenceId", static_cast<Int32> (*branchAxisReferenceId));
#ifdef ServerMainVers_2900
        auto branchAxisAngle = table.GetBranchAxisAngle (i);
        if (branchAxisAngle.IsOk ()) {
            rowOS.Add ("branchAxisAngle", *branchAxisAngle);
        }
#endif
        rowOS.Add ("mainAxisDiameter", *mainAxisDiameter);
        rowOS.Add ("branchAxisDiameter", *branchAxisDiameter);
        rowOS.Add ("length", *length);
        rowOS.Add ("branchOffset", *branchOffset);
        rowOS.Add ("connectionLength", *connectionLength);
        AddRowDescription (rowOS, table.GetDescription (i));
        AddPreferenceTableValue (rowOS, "isValid", table.IsRowValid (i));
        rows (rowOS);
    }
    return tableOS;
}

#ifdef ServerMainVers_2900
template <typename TableT>
static GS::ObjectState DumpTransitionPreferenceTable (const TableT& table, const GS::Guid& tableGuid)
{
    GS::ObjectState tableOS;
    tableOS.Add ("guid", tableGuid.ToUniString ());
    tableOS.Add ("name", table.GetName ());
    tableOS.Add ("isEditable", PreferenceTableIsEditable (table.IsEditable ()));
    const auto& rows = tableOS.AddList<GS::ObjectState> ("rows");
    for (uint32_t i = 0; i < table.GetSize (); ++i) {
        auto beginReferenceId = table.GetBeginReferenceId (i);
        auto endReferenceId = table.GetEndReferenceId (i);
        auto beginDiameter = table.GetBeginDiameter (i);
        auto endDiameter = table.GetEndDiameter (i);
        if (beginReferenceId.IsErr () || endReferenceId.IsErr () || beginDiameter.IsErr () || endDiameter.IsErr ()) {
            continue;
        }
        GS::ObjectState rowOS ("index", static_cast<Int32> (i));
        rowOS.Add ("beginReferenceId", static_cast<Int32> (*beginReferenceId));
        rowOS.Add ("endReferenceId", static_cast<Int32> (*endReferenceId));
        rowOS.Add ("beginDiameter", *beginDiameter);
        rowOS.Add ("endDiameter", *endDiameter);
        // Pipe transitions are defined by their length, duct transitions by their angle.
        if constexpr (std::is_same_v<TableT, PipeTransitionPreferenceTable>) {
            auto length = table.GetLength (i);
            if (length.IsOk ()) {
                rowOS.Add ("length", *length);
            }
        } else {
            auto angle = table.GetAngle (i);
            if (angle.IsOk ()) {
                rowOS.Add ("angle", *angle);
            }
        }
        AddRowDescription (rowOS, table.GetDescription (i));
        AddPreferenceTableValue (rowOS, "isValid", table.IsRowValid (i));
        rows (rowOS);
    }
    return tableOS;
}

static GS::ObjectState DumpAngleSet (const AngleSetBase& angleSet)
{
    GS::ObjectState angleSetOS;
    angleSetOS.Add ("name", angleSet.GetName ());
    angleSetOS.Add ("isEditable", PreferenceTableIsEditable (angleSet.IsEditable ()));
    const auto& angles = angleSetOS.AddList<double> ("angles");
    const auto& angleDetails = angleSetOS.AddList<GS::ObjectState> ("angleDetails");
    for (uint32_t i = 0; i < angleSet.GetSize (); ++i) {
        auto angle = angleSet.GetAngle (i);
        if (angle.IsOk ()) {
            angles (*angle);
            GS::ObjectState angleOS ("angle", *angle);
            AddPreferenceTableValue (angleOS, "isDefined", angleSet.IsAngleDefined (i));
            angleDetails (angleOS);
        }
    }
    return angleSetOS;
}
#endif

static GS::ObjectState DumpReferenceSet (const ReferenceSetBase& referenceSet)
{
    GS::ObjectState referenceSetOS;
    referenceSetOS.Add ("name", referenceSet.GetName ());
    referenceSetOS.Add ("isEditable", PreferenceTableIsEditable (referenceSet.IsEditable ()));
    const auto& referenceIds = referenceSetOS.AddList<Int32> ("referenceIds");
    const auto& referenceIdDetails = referenceSetOS.AddList<GS::ObjectState> ("referenceIdDetails");
    for (uint32_t i = 0; i < referenceSet.GetSize (); ++i) {
        auto referenceId = referenceSet.GetReferenceId (i);
        if (referenceId.IsOk ()) {
            referenceIds (static_cast<Int32> (*referenceId));
            GS::ObjectState referenceIdOS ("referenceId", static_cast<Int32> (*referenceId));
            AddPreferenceTableValue (referenceIdOS, "isDefined", referenceSet.IsReferenceIdDefined (i));
            referenceIdDetails (referenceIdOS);
        }
    }
    return referenceSetOS;
}

// Adds the tables of a preference table container as a list. TableT::Get gives the table of an id.
template <typename TableT, typename ContainerResultT, typename DumpFunc>
static void AddPreferenceTables (GS::ObjectState& response, const char* listName, const ContainerResultT& container, const DumpFunc& dump)
{
    const auto& tables = response.AddList<GS::ObjectState> (listName);
    if (container.IsErr ()) {
        return;
    }
    for (const auto& id : container->GetPreferenceTables ()) {
        auto table = TableT::Get (id);
        if (table.IsOk ()) {
            tables (dump (*table, id.GetGuid ()));
        }
    }
}

GS::ObjectState GetMEPPreferenceTablesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString domainStr;
    parameters.Get ("domain", domainStr);
    const std::optional<Domain> domain = DomainFromString (domainStr);
    if (!domain.has_value ()) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid MEP domain: " + domainStr);
    }
    bool includeFittingTables = false;
    parameters.Get ("includeFittingTables", includeFittingTables);

    GS::ObjectState response;
    const auto& tables = response.AddList<GS::ObjectState> ("tables");
    const auto dumpValueTable = [] (const auto& table, const GS::Guid& guid) { return DumpValuePreferenceTable (table, guid); };

    // Cable carriers have no circular tables, reference set or fitting tables, only their own segment tables.
    if (*domain == Domain::CableCarrier) {
        auto container = GetCableCarrierSegmentPreferenceTableContainer ();
        if (container.IsErr ()) {
            return CreateErrorResponse (APIERR_GENERAL, GS::UniString ("Failed to get the cable carrier segment preference table container: ") + container.UnwrapErr ().text.c_str ());
        }
        AddPreferenceTables<CableCarrierSegmentPreferenceTable> (response, "cableCarrierTables", container, dumpValueTable);
        return response;
    }

    const auto dumpElbow = [] (const auto& table, const GS::Guid& guid) { return DumpElbowPreferenceTable (table, guid); };
    const auto dumpBranch = [] (const auto& table, const GS::Guid& guid) { return DumpBranchPreferenceTable (table, guid); };
#ifdef ServerMainVers_2900
    const auto dumpTransition = [] (const auto& table, const GS::Guid& guid) { return DumpTransitionPreferenceTable (table, guid); };
#endif

    if (*domain == Domain::Piping) {
        auto container = GetPipeSegmentPreferenceTableContainer ();
        if (container.IsErr ()) {
            return CreateErrorResponse (APIERR_GENERAL, GS::UniString ("Failed to get the pipe segment preference table container: ") + container.UnwrapErr ().text.c_str ());
        }
        for (const auto& id : container->GetPreferenceTables ()) {
            auto table = PipeSegmentPreferenceTable::Get (id);
            if (table.IsOk ()) {
                tables (DumpPreferenceTable (*table, id.GetGuid ()));
            }
        }

        auto referenceSet = GetPipeReferenceSet ();
        response.Add ("referenceSet", referenceSet.IsOk () && referenceSet->Exists () ? DumpReferenceSet (*referenceSet) : GS::ObjectState ("name", GS::UniString (), "referenceIds", GS::Array<Int32> ()));
#ifdef ServerMainVers_2900
        auto angleSet = GetPipeAngleSet ();
        if (angleSet.IsOk () && angleSet->Exists ()) {
            response.Add ("angleSet", DumpAngleSet (*angleSet));
        }
#endif
        if (includeFittingTables) {
            AddPreferenceTables<PipeElbowPreferenceTable> (response, "elbowTables", GetPipeElbowPreferenceTableContainer (), dumpElbow);
            AddPreferenceTables<PipeBranchPreferenceTable> (response, "branchTables", GetPipeBranchPreferenceTableContainer (), dumpBranch);
#ifdef ServerMainVers_2900
            AddPreferenceTables<PipeTransitionPreferenceTable> (response, "transitionTables", GetPipeTransitionPreferenceTableContainer (), dumpTransition);
#endif
        }
    } else {
        auto container = GetDuctSegmentPreferenceTableContainer ();
        if (container.IsErr ()) {
            return CreateErrorResponse (APIERR_GENERAL, GS::UniString ("Failed to get the duct segment preference table container: ") + container.UnwrapErr ().text.c_str ());
        }
        for (const auto& id : container->GetPreferenceTables ()) {
            auto table = DuctCircularSegmentPreferenceTable::Get (id);
            if (table.IsOk ()) {
                tables (DumpPreferenceTable (*table, id.GetGuid ()));
            }
        }
        // The rectangular tables are in the same container; DuctRectangularSegmentPreferenceTable::Get picks them.
        AddPreferenceTables<DuctRectangularSegmentPreferenceTable> (response, "rectangularTables", container, dumpValueTable);

        auto referenceSet = GetDuctReferenceSet ();
        response.Add ("referenceSet", referenceSet.IsOk () && referenceSet->Exists () ? DumpReferenceSet (*referenceSet) : GS::ObjectState ("name", GS::UniString (), "referenceIds", GS::Array<Int32> ()));
#ifdef ServerMainVers_2900
        auto angleSet = GetDuctAngleSet ();
        if (angleSet.IsOk () && angleSet->Exists ()) {
            response.Add ("angleSet", DumpAngleSet (*angleSet));
        }
#endif
        if (includeFittingTables) {
            AddPreferenceTables<DuctElbowPreferenceTable> (response, "elbowTables", GetDuctElbowPreferenceTableContainer (), dumpElbow);
            AddPreferenceTables<DuctBranchPreferenceTable> (response, "branchTables", GetDuctBranchPreferenceTableContainer (), dumpBranch);
#ifdef ServerMainVers_2900
            AddPreferenceTables<DuctTransitionPreferenceTable> (response, "transitionTables", GetDuctTransitionPreferenceTableContainer (), dumpTransition);
#endif
        }
    }

    return response;
}
#else
GS::ObjectState GetMEPPreferenceTablesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
}
#endif

ModifyMEPPreferenceTablesCommand::ModifyMEPPreferenceTablesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyMEPPreferenceTablesCommand::GetName () const
{
    return "ModifyMEPPreferenceTables";
}

GS::Optional<GS::UniString> ModifyMEPPreferenceTablesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "domain": {
                "$ref": "#/MEPPreferenceTableDomain"
            },
            "tablesToAdd": {
                "type": "array",
                "description": "New tables to add to the domain, before the other changes.",
                "items": {
                    "type": "object",
                    "properties": {
                        "type": {
                            "$ref": "#/MEPPreferenceTableType"
                        },
                        "name": {
                            "type": "string"
                        }
                    },
                    "additionalProperties": false,
                    "required": ["type", "name"]
                }
            },
            "tables": {
                "type": "array",
                "description": "The tables of the domain to change, in the shape GetMEPPreferenceTables returns them. Each table is changed in one undoable step.",
                "items": {
                    "$ref": "#/MEPPreferenceTableChange"
                }
            },
            "tablesToDelete": {
                "type": "array",
                "description": "The guids of the tables of the domain to delete, after the other table changes.",
                "items": {
                    "type": "string"
                }
            },
            "referenceIdsToDelete": {
                "type": "array",
                "description": "Reference ids to delete from the reference set of the Piping or Ventilation domain.",
                "items": {
                    "type": "integer",
                    "minimum": 0
                }
            },
            "anglesToDelete": {
                "type": "array",
                "description": "Angles to delete from the angle set of the Piping or Ventilation domain, as GetMEPPreferenceTables returns them. Available from Archicad 29.",
                "items": {
                    "type": "number"
                }
            }
        },
        "additionalProperties": false,
        "required": ["domain"]
    })";
}

GS::Optional<GS::UniString> ModifyMEPPreferenceTablesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "addedTables": {
                "type": "array",
                "description": "One item for each item of tablesToAdd: the guid of the new table, or the error.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "guid": {
                                    "type": "string"
                                }
                            },
                            "additionalProperties": false,
                            "required": ["guid"]
                        },
                        {
                            "$ref": "#/FailedExecutionResult"
                        }
                    ]
                }
            },
            "tables": {
                "$ref": "#/ExecutionResults",
                "description": "One result for each item of tables."
            },
            "deletedTables": {
                "$ref": "#/ExecutionResults",
                "description": "One result for each item of tablesToDelete."
            },
            "referenceSet": {
                "$ref": "#/ExecutionResult",
                "description": "Only when referenceIdsToDelete is given."
            },
            "angleSet": {
                "$ref": "#/ExecutionResult",
                "description": "Only when anglesToDelete is given."
            }
        },
        "additionalProperties": false,
        "required": ["addedTables", "tables", "deletedTables"]
    })";
}

#ifdef ServerMainVers_2800
template <typename TableT>
constexpr bool IsCircularSegmentPreferenceTable = std::is_same_v<TableT, PipeSegmentPreferenceTable> || std::is_same_v<TableT, DuctCircularSegmentPreferenceTable>;

template <typename TableT>
constexpr bool IsValuePreferenceTable = std::is_same_v<TableT, DuctRectangularSegmentPreferenceTable> || std::is_same_v<TableT, CableCarrierSegmentPreferenceTable>;

template <typename TableT>
constexpr bool IsElbowPreferenceTable = std::is_same_v<TableT, PipeElbowPreferenceTable> || std::is_same_v<TableT, DuctElbowPreferenceTable>;

template <typename TableT>
constexpr bool IsBranchPreferenceTable = std::is_same_v<TableT, PipeBranchPreferenceTable> || std::is_same_v<TableT, DuctBranchPreferenceTable>;

// The row values ModifyMEPPreferenceTables can set; which of them belong to a table depends on its kind.
static const char* const PreferenceRowValueKeys[] = {
    "diameter", "wallThickness", "value", "radius", "mainAxisDiameter", "branchAxisDiameter", "length",
    "branchOffset", "connectionLength", "beginDiameter", "endDiameter", "angle", "description"
};

template <typename TableT>
static bool IsPreferenceRowValueOfTable (const GS::UniString& key)
{
    if (key == "description") {
        return true;
    }
    if constexpr (IsCircularSegmentPreferenceTable<TableT>) {
        return key == "diameter" || key == "wallThickness";
    } else if constexpr (IsValuePreferenceTable<TableT>) {
        return key == "value";
    } else if constexpr (IsElbowPreferenceTable<TableT>) {
        return key == "diameter" || key == "radius";
    } else if constexpr (IsBranchPreferenceTable<TableT>) {
        return key == "mainAxisDiameter" || key == "branchAxisDiameter" || key == "length" || key == "branchOffset" || key == "connectionLength";
    } else {
#ifdef ServerMainVers_2900
        // Pipe transitions are defined by their length, duct transitions by their angle.
        if constexpr (std::is_same_v<TableT, PipeTransitionPreferenceTable>) {
            return key == "beginDiameter" || key == "endDiameter" || key == "length";
        } else {
            return key == "beginDiameter" || key == "endDiameter" || key == "angle";
        }
#else
        return false;
#endif
    }
}

// Follows where the rows of a preference table move while it is modified, so that every row index given
// refers to the table before the call: deleting a row moves the rows after it up, and setting the value of
// a rectangular duct or cable carrier row can move it, since these tables keep their rows ordered by value.
class PreferenceTableRowIndices
{
public:
    explicit PreferenceTableRowIndices (uint32_t size)
    {
        for (uint32_t i = 0; i < size; ++i) {
            indices.push_back (i);
        }
    }

    uint32_t Current (uint32_t originalIndex) const
    {
        return indices[originalIndex];
    }

    void Moved (uint32_t from, uint32_t to)
    {
        for (uint32_t& index : indices) {
            if (index == from) {
                index = to;
            } else if (from < to && index > from && index <= to) {
                --index;
            } else if (to < from && index >= to && index < from) {
                ++index;
            }
        }
    }

    void Deleted (uint32_t deletedIndex)
    {
        for (uint32_t& index : indices) {
            if (index > deletedIndex) {
                --index;
            }
        }
    }

private:
    std::vector<uint32_t> indices;
};

struct PreferenceTableModifyError {
    GSErrCode code;
    GS::UniString message;
};

static GS::UniString PreferenceRowText (Int32 index)
{
    return " of row " + GS::ValueToUniString (index);
}

// Sets the value of a rectangular duct or cable carrier row unless an earlier change failed. The row can
// move, since these tables keep their rows ordered by value; gives its new index.
template <typename ModifierT>
static std::optional<uint32_t> SetPreferenceRowValue (ModifierT& modifier, uint32_t index, double value, const GS::UniString& what, std::optional<PreferenceTableModifyError>& error)
{
    if (error.has_value ()) {
        return std::nullopt;
    }
    ACAPI::Result<uint32_t> newIndex = modifier.SetValue (index, value);
    if (newIndex.IsErr ()) {
        error = PreferenceTableModifyError { newIndex.UnwrapErr ().kind, "Failed to " + what + "." };
        return std::nullopt;
    }
    return *newIndex;
}

// Changes one preference table: its name, segmentation lengths, the values of its rows, deletes rows and,
// in rectangular duct and cable carrier tables, adds rows. Everything is checked before the table is
// changed, in one undoable step.
template <typename TableT>
static GS::ObjectState ModifyPreferenceTable (TableT& table, const GS::ObjectState& tableData)
{
    if (!PreferenceTableIsEditable (table.IsEditable ())) {
        return CreateFailedExecutionResult (APIERR_NOACCESSRIGHT, "The preference table is not editable.");
    }

    const uint32_t size = static_cast<uint32_t> (PreferenceTableValueToOptional (table.GetSize ()).value_or (0));
    GS::Array<GS::ObjectState> rowsData;
    tableData.Get ("rows", rowsData);
    std::vector<bool> isRowGiven (size, false);
    // The wall thickness of a circular segment row is set by its reference id.
    std::vector<uint32_t> referenceIds (size, 0);
    for (const GS::ObjectState& rowData : rowsData) {
        Int32 index = -1;
        rowData.Get ("index", index);
        if (index < 0 || static_cast<uint32_t> (index) >= size) {
            return CreateFailedExecutionResult (APIERR_BADINDEX, "The table has no row " + GS::ValueToUniString (index) + ".");
        }
        if (isRowGiven[index]) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "Row " + GS::ValueToUniString (index) + " is given more than once.");
        }
        isRowGiven[index] = true;
        bool deleteRow = false;
        rowData.Get ("delete", deleteRow);
        for (const char* key : PreferenceRowValueKeys) {
            if (!rowData.Contains (key)) {
                continue;
            }
            if (!IsPreferenceRowValueOfTable<TableT> (key)) {
                // The elbow tables return the angle of their rows, but it cannot be set.
                if constexpr (IsElbowPreferenceTable<TableT>) {
                    if (GS::UniString (key) == "angle") {
                        continue;
                    }
                }
                return CreateFailedExecutionResult (APIERR_BADPARS, GS::UniString ("The row value '") + key + "' does not belong to this kind of preference table.");
            }
            if (deleteRow) {
                return CreateFailedExecutionResult (APIERR_BADPARS, "Row " + GS::ValueToUniString (index) + " is deleted, so it cannot get new values.");
            }
        }
        if constexpr (IsCircularSegmentPreferenceTable<TableT>) {
            if (rowData.Contains ("wallThickness")) {
                auto referenceId = table.GetReferenceId (static_cast<uint32_t> (index));
                if (referenceId.IsErr ()) {
                    return CreateFailedExecutionResult (referenceId.UnwrapErr ().kind, "Failed to get the reference id" + PreferenceRowText (index) + ".");
                }
                referenceIds[index] = *referenceId;
            }
        }
    }

    double minimumSegmentationLength = 0.0;
    double maximumSegmentationLength = 0.0;
    const bool hasMinimumSegmentationLength = tableData.Get ("minimumSegmentationLength", minimumSegmentationLength);
    const bool hasMaximumSegmentationLength = tableData.Get ("maximumSegmentationLength", maximumSegmentationLength);
    const bool hasSegmentationLengths = hasMinimumSegmentationLength || hasMaximumSegmentationLength || tableData.Contains ("useIndividualSegmentLengths");
    if (hasSegmentationLengths) {
        if constexpr (IsCircularSegmentPreferenceTable<TableT> || IsValuePreferenceTable<TableT>) {
#ifdef ServerMainVers_2900
            // Both lengths are set together, so a missing one keeps its current value.
            if (hasMinimumSegmentationLength != hasMaximumSegmentationLength) {
                const auto currentLength = hasMinimumSegmentationLength
                    ? PreferenceTableValueToOptional (table.GetMaximumSegmentationLength ())
                    : PreferenceTableValueToOptional (table.GetMinimumSegmentationLength ());
                if (!currentLength.has_value ()) {
                    return CreateFailedExecutionResult (APIERR_GENERAL, "Failed to get the current segmentation lengths; give both the minimum and the maximum.");
                }
                (hasMinimumSegmentationLength ? maximumSegmentationLength : minimumSegmentationLength) = *currentLength;
            }
#else
            return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "The segmentation lengths are available from Archicad 29.");
#endif
        } else {
            return CreateFailedExecutionResult (APIERR_BADPARS, "Only segment preference tables have segmentation lengths.");
        }
    }

    GS::Array<GS::ObjectState> newRowsData;
    tableData.Get ("newRows", newRowsData);
    if constexpr (!IsValuePreferenceTable<TableT>) {
        if (!newRowsData.IsEmpty ()) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "Rows can only be added to rectangular duct and cable carrier segment tables.");
        }
    }

    std::optional<PreferenceTableModifyError> error;
    // Calls a setter unless an earlier one failed, and keeps the first error.
    const auto call = [&] (const GS::UniString& what, const auto& setter) {
        if (!error.has_value ()) {
            const std::optional<GSErrCode> callError = GetErrorOfCall (setter);
            if (callError.has_value ()) {
                error = PreferenceTableModifyError { *callError, "Failed to " + what + "." };
            }
        }
    };
    PreferenceTableRowIndices rowIndices (size);
    ACAPI::Result<void> modifyResult = table.Modify ([&] (typename TableT::Modifier& modifier) {
        GS::UniString name;
        if (tableData.Get ("name", name)) {
            call ("set the name of the table", [&] () { return modifier.SetName (name); });
        }
        if constexpr (IsCircularSegmentPreferenceTable<TableT> || IsValuePreferenceTable<TableT>) {
#ifdef ServerMainVers_2900
            bool useIndividualSegmentLengths = false;
            if (tableData.Get ("useIndividualSegmentLengths", useIndividualSegmentLengths)) {
                call ("set whether the table uses individual segment lengths", [&] () { return modifier.SetUseIndividualSegmentLengths (useIndividualSegmentLengths); });
            }
            if (hasMinimumSegmentationLength || hasMaximumSegmentationLength) {
                call ("set the segmentation lengths", [&] () { return modifier.SetSegmentationLengths (minimumSegmentationLength, maximumSegmentationLength); });
            }
#endif
        }

        // New values first, then the deletions, so that the indices of the deleted rows can be followed.
        for (const GS::ObjectState& rowData : rowsData) {
            Int32 originalIndex = 0;
            rowData.Get ("index", originalIndex);
            bool deleteRow = false;
            rowData.Get ("delete", deleteRow);
            if (deleteRow) {
                continue;
            }
            const GS::UniString rowText = PreferenceRowText (originalIndex);
            uint32_t index = rowIndices.Current (static_cast<uint32_t> (originalIndex));
            double number = 0.0;
            if constexpr (IsCircularSegmentPreferenceTable<TableT>) {
                if (rowData.Get ("diameter", number)) {
                    call ("set the diameter" + rowText, [&] () { return modifier.SetDiameter (index, number); });
                }
                if (rowData.Get ("wallThickness", number)) {
                    const uint32_t referenceId = referenceIds[originalIndex];
#ifdef ServerMainVers_2900
                    call ("set the wall thickness" + rowText, [&] () { return modifier.SetWallThicknessByKey (referenceId, number); });
#else
                    call ("set the wall thickness" + rowText, [&] () { return modifier.SetWallThicknessByReferenceId (referenceId, number); });
#endif
                }
            } else if constexpr (IsValuePreferenceTable<TableT>) {
                if (rowData.Get ("value", number)) {
                    const std::optional<uint32_t> newIndex = SetPreferenceRowValue (modifier, index, number, "set the value" + rowText, error);
                    if (newIndex.has_value ()) {
                        rowIndices.Moved (index, *newIndex);
                        index = *newIndex;
                    }
                }
            } else if constexpr (IsElbowPreferenceTable<TableT>) {
                if (rowData.Get ("diameter", number)) {
                    call ("set the diameter" + rowText, [&] () { return modifier.SetDiameter (index, number); });
                }
                if (rowData.Get ("radius", number)) {
                    call ("set the radius" + rowText, [&] () { return modifier.SetRadius (index, number); });
                }
            } else if constexpr (IsBranchPreferenceTable<TableT>) {
                if (rowData.Get ("mainAxisDiameter", number)) {
                    call ("set the main axis diameter" + rowText, [&] () { return modifier.SetMainAxisDiameter (index, number); });
                }
                if (rowData.Get ("branchAxisDiameter", number)) {
                    call ("set the branch axis diameter" + rowText, [&] () { return modifier.SetBranchAxisDiameter (index, number); });
                }
                if (rowData.Get ("length", number)) {
                    call ("set the length" + rowText, [&] () { return modifier.SetLength (index, number); });
                }
                if (rowData.Get ("branchOffset", number)) {
                    call ("set the branch offset" + rowText, [&] () { return modifier.SetBranchOffset (index, number); });
                }
                if (rowData.Get ("connectionLength", number)) {
                    call ("set the connection length" + rowText, [&] () { return modifier.SetConnectionLength (index, number); });
                }
            } else {
#ifdef ServerMainVers_2900
                if (rowData.Get ("beginDiameter", number)) {
                    call ("set the begin diameter" + rowText, [&] () { return modifier.SetBeginDiameter (index, number); });
                }
                if (rowData.Get ("endDiameter", number)) {
                    call ("set the end diameter" + rowText, [&] () { return modifier.SetEndDiameter (index, number); });
                }
                if constexpr (std::is_same_v<TableT, PipeTransitionPreferenceTable>) {
                    if (rowData.Get ("length", number)) {
                        call ("set the length" + rowText, [&] () { return modifier.SetLength (index, number); });
                    }
                } else {
                    if (rowData.Get ("angle", number)) {
                        call ("set the angle" + rowText, [&] () { return modifier.SetAngle (index, number); });
                    }
                }
#endif
            }
            GS::UniString description;
            if (rowData.Get ("description", description)) {
                call ("set the description" + rowText, [&] () { return modifier.SetDescription (index, description); });
            }
        }

        for (const GS::ObjectState& rowData : rowsData) {
            bool deleteRow = false;
            rowData.Get ("delete", deleteRow);
            if (!deleteRow || error.has_value ()) {
                continue;
            }
            Int32 originalIndex = 0;
            rowData.Get ("index", originalIndex);
            const GS::UniString what = "delete row " + GS::ValueToUniString (originalIndex);
            const uint32_t index = rowIndices.Current (static_cast<uint32_t> (originalIndex));
            // The circular segment tables, and the elbow tables of Archicad 28, have a fixed row for each
            // reference id, which can only be emptied; the rows of the other tables are removed.
            bool rowsMoved = true;
            if constexpr (IsCircularSegmentPreferenceTable<TableT>) {
                call (what, [&] () { return modifier.EmptyRow (index); });
                rowsMoved = false;
            } else if constexpr (IsValuePreferenceTable<TableT>) {
                call (what, [&] () { return modifier.Delete (index); });
            } else if constexpr (IsElbowPreferenceTable<TableT>) {
#ifdef ServerMainVers_2900
                call (what, [&] () { return modifier.DeleteRow (index); });
#else
                call (what, [&] () { return modifier.EmptyRow (index); });
                rowsMoved = false;
#endif
            } else {
                call (what, [&] () { return modifier.DeleteRow (index); });
            }
            if (rowsMoved && !error.has_value ()) {
                rowIndices.Deleted (index);
            }
        }

        if constexpr (IsValuePreferenceTable<TableT>) {
            for (UIndex i = 0; i < newRowsData.GetSize () && !error.has_value (); ++i) {
                const GS::UniString rowText = " of new row " + GS::ValueToUniString (i);
                ACAPI::Result<uint32_t> addedIndex = modifier.AddNewPreference ();
                if (addedIndex.IsErr ()) {
                    error = PreferenceTableModifyError { addedIndex.UnwrapErr ().kind, "Failed to add new row " + GS::ValueToUniString (i) + "." };
                    break;
                }
                double value = 0.0;
                newRowsData[i].Get ("value", value);
                const std::optional<uint32_t> index = SetPreferenceRowValue (modifier, *addedIndex, value, "set the value" + rowText, error);
                GS::UniString description;
                if (index.has_value () && newRowsData[i].Get ("description", description)) {
                    call ("set the description" + rowText, [&] () { return modifier.SetDescription (*index, description); });
                }
            }
        }
    }, "Modify MEP Preference Table");

    if (error.has_value ()) {
        return CreateFailedExecutionResult (error->code, error->message + " The changes before it may have been made.");
    }
    if (modifyResult.IsErr ()) {
        return CreateFailedExecutionResult (modifyResult.UnwrapErr ().kind, GS::UniString ("Failed to modify the preference table: ") + modifyResult.UnwrapErr ().text.c_str ());
    }
    return CreateSuccessfulExecutionResult ();
}

// Changes the table if the id is a table of type TableT; gives nothing otherwise.
template <typename TableT>
static std::optional<GS::ObjectState> TryModifyPreferenceTable (const ACAPI::MEP::UniqueID& id, const GS::ObjectState& tableData)
{
    auto table = TableT::Get (id);
    if (table.IsErr ()) {
        return std::nullopt;
    }
    return ModifyPreferenceTable (*table, tableData);
}

static GS::ObjectState ModifyPreferenceTableOfDomain (Domain domain, const GS::ObjectState& tableData)
{
    GS::UniString guidStr;
    tableData.Get ("guid", guidStr);
    const auto id = Adapter::UniqueID (APIGuidFromString (guidStr.ToCStr ()));

    std::optional<GS::ObjectState> result;
    if (domain == Domain::CableCarrier) {
        result = TryModifyPreferenceTable<CableCarrierSegmentPreferenceTable> (id, tableData);
    } else if (domain == Domain::Piping) {
        result = TryModifyPreferenceTable<PipeSegmentPreferenceTable> (id, tableData);
        if (!result.has_value ()) {
            result = TryModifyPreferenceTable<PipeElbowPreferenceTable> (id, tableData);
        }
        if (!result.has_value ()) {
            result = TryModifyPreferenceTable<PipeBranchPreferenceTable> (id, tableData);
        }
#ifdef ServerMainVers_2900
        if (!result.has_value ()) {
            result = TryModifyPreferenceTable<PipeTransitionPreferenceTable> (id, tableData);
        }
#endif
    } else {
        result = TryModifyPreferenceTable<DuctCircularSegmentPreferenceTable> (id, tableData);
        if (!result.has_value ()) {
            result = TryModifyPreferenceTable<DuctRectangularSegmentPreferenceTable> (id, tableData);
        }
        if (!result.has_value ()) {
            result = TryModifyPreferenceTable<DuctElbowPreferenceTable> (id, tableData);
        }
        if (!result.has_value ()) {
            result = TryModifyPreferenceTable<DuctBranchPreferenceTable> (id, tableData);
        }
#ifdef ServerMainVers_2900
        if (!result.has_value ()) {
            result = TryModifyPreferenceTable<DuctTransitionPreferenceTable> (id, tableData);
        }
#endif
    }
    if (!result.has_value ()) {
        return CreateFailedExecutionResult (APIERR_BADID, "'" + guidStr + "' is not a preference table of the domain.");
    }
    return *result;
}

static std::vector<GS::UniString> GetPreferenceTableTypes (Domain domain)
{
    if (domain == Domain::CableCarrier) {
        return { "Segment" };
    }
#ifdef ServerMainVers_2900
    return { "Segment", "Elbow", "Branch", "Transition" };
#else
    return { "Segment", "Elbow", "Branch" };
#endif
}

// Calls func with the preference table container of the domain and type, and gives back its result.
template <typename FuncT>
static GS::ObjectState WithPreferenceTableContainer (Domain domain, const GS::UniString& type, const FuncT& func)
{
    const auto callWith = [&] (auto container) -> GS::ObjectState {
        if (container.IsErr ()) {
            return CreateFailedExecutionResult (container.UnwrapErr ().kind, "Failed to get the " + type + " preference table container.");
        }
        return func (*container);
    };
    const bool isPiping = domain == Domain::Piping;
    if (domain == Domain::CableCarrier) {
        if (type == "Segment") {
            return callWith (GetCableCarrierSegmentPreferenceTableContainer ());
        }
        return CreateFailedExecutionResult (APIERR_BADPARS, "The CableCarrier domain has only segment preference tables.");
    }
    if (type == "Segment") {
        return isPiping ? callWith (GetPipeSegmentPreferenceTableContainer ()) : callWith (GetDuctSegmentPreferenceTableContainer ());
    }
    if (type == "Elbow") {
        return isPiping ? callWith (GetPipeElbowPreferenceTableContainer ()) : callWith (GetDuctElbowPreferenceTableContainer ());
    }
    if (type == "Branch") {
        return isPiping ? callWith (GetPipeBranchPreferenceTableContainer ()) : callWith (GetDuctBranchPreferenceTableContainer ());
    }
    if (type == "Transition") {
#ifdef ServerMainVers_2900
        return isPiping ? callWith (GetPipeTransitionPreferenceTableContainer ()) : callWith (GetDuctTransitionPreferenceTableContainer ());
#else
        return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "The transition preference tables are available from Archicad 29.");
#endif
    }
    return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid preference table type: " + type);
}

static GS::ObjectState AddPreferenceTable (Domain domain, const GS::UniString& type, const GS::UniString& name)
{
    return WithPreferenceTableContainer (domain, type, [&] (auto& container) -> GS::ObjectState {
        std::optional<ACAPI::Result<ACAPI::MEP::UniqueID>> addResult;
        ACAPI::Result<void> modifyResult = container.Modify ([&] (auto& modifier) {
            addResult.emplace (modifier.AddNewTable (name));
        }, "Add MEP Preference Table");
        if (addResult.has_value () && addResult->IsErr ()) {
            return CreateFailedExecutionResult (addResult->UnwrapErr ().kind, "Failed to add the preference table '" + name + "'.");
        }
        if (modifyResult.IsErr () || !addResult.has_value ()) {
            return CreateFailedExecutionResult (modifyResult.IsErr () ? modifyResult.UnwrapErr ().kind : APIERR_GENERAL, "Failed to add the preference table '" + name + "'.");
        }
        return GS::ObjectState ("guid", (*addResult)->GetGuid ().ToUniString ());
    });
}

static GS::ObjectState DeletePreferenceTable (Domain domain, const GS::UniString& guidStr)
{
    const GS::Guid guid = APIGuid2GSGuid (APIGuidFromString (guidStr.ToCStr ()));
    for (const GS::UniString& type : GetPreferenceTableTypes (domain)) {
        bool found = false;
        GS::ObjectState result = WithPreferenceTableContainer (domain, type, [&] (auto& container) -> GS::ObjectState {
            for (const auto& id : container.GetPreferenceTables ()) {
                if (id.GetGuid () != guid) {
                    continue;
                }
                found = true;
                std::optional<GSErrCode> deleteError;
                ACAPI::Result<void> modifyResult = container.Modify ([&] (auto& modifier) {
                    deleteError = GetErrorOfCall ([&] () { return modifier.Delete (id); });
                }, "Delete MEP Preference Table");
                if (deleteError.has_value () || modifyResult.IsErr ()) {
                    return CreateFailedExecutionResult (deleteError.has_value () ? *deleteError : modifyResult.UnwrapErr ().kind, "Failed to delete the preference table '" + guidStr + "'.");
                }
                return CreateSuccessfulExecutionResult ();
            }
            return CreateSuccessfulExecutionResult ();
        });
        if (found) {
            return result;
        }
    }
    return CreateFailedExecutionResult (APIERR_BADID, "'" + guidStr + "' is not a preference table of the domain.");
}

template <typename ReferenceSetResultT>
static GS::ObjectState DeleteReferenceIds (ReferenceSetResultT referenceSet, const GS::Array<Int32>& referenceIdsToDelete)
{
    if (referenceSet.IsErr ()) {
        return CreateFailedExecutionResult (referenceSet.UnwrapErr ().kind, "Failed to get the reference set.");
    }
    GS::Array<Int32> referenceIds;
    for (uint32_t i = 0; i < referenceSet->GetSize (); ++i) {
        auto referenceId = referenceSet->GetReferenceId (i);
        if (referenceId.IsOk ()) {
            referenceIds.Push (static_cast<Int32> (*referenceId));
        }
    }
    for (Int32 referenceId : referenceIdsToDelete) {
        if (!referenceIds.Contains (referenceId)) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "The reference id " + GS::ValueToUniString (referenceId) + " is not in the reference set.");
        }
    }
    std::optional<GSErrCode> deleteError;
    // The modifier type is named, a generic lambda here crashes MSVC 14.44 (C1001).
    ACAPI::Result<void> modifyResult = referenceSet->Modify ([&] (ReferenceSetBase::Modifier& modifier) {
        for (Int32 referenceId : referenceIdsToDelete) {
            if (!deleteError.has_value ()) {
                deleteError = GetErrorOfCall ([&] () { return modifier.Delete (static_cast<uint32_t> (referenceId)); });
            }
        }
    }, "Modify MEP Reference Set");
    if (deleteError.has_value () || modifyResult.IsErr ()) {
        return CreateFailedExecutionResult (deleteError.has_value () ? *deleteError : modifyResult.UnwrapErr ().kind, "Failed to delete the reference ids.");
    }
    return CreateSuccessfulExecutionResult ();
}

#ifdef ServerMainVers_2900
template <typename AngleSetResultT>
static GS::ObjectState DeleteAngles (AngleSetResultT angleSet, const GS::Array<double>& anglesToDelete)
{
    if (angleSet.IsErr ()) {
        return CreateFailedExecutionResult (angleSet.UnwrapErr ().kind, "Failed to get the angle set.");
    }
    // The angles are deleted with the exact value of the set, so a value rounded by JSON still matches.
    std::vector<double> angles;
    for (double angleToDelete : anglesToDelete) {
        std::optional<double> found;
        for (uint32_t i = 0; i < angleSet->GetSize () && !found.has_value (); ++i) {
            auto angle = angleSet->GetAngle (i);
            if (angle.IsOk () && std::abs (*angle - angleToDelete) < 1e-9) {
                found = *angle;
            }
        }
        if (!found.has_value ()) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "The angle " + GS::ValueToUniString (angleToDelete) + " is not in the angle set.");
        }
        angles.push_back (*found);
    }
    std::optional<GSErrCode> deleteError;
    // Like the reference set, the modifier is the base class's: PipeAngleSet::Modifier and DuctAngleSet::Modifier are incomplete types.
    ACAPI::Result<void> modifyResult = angleSet->Modify ([&] (AngleSetBase::Modifier& modifier) {
        for (double angle : angles) {
            if (!deleteError.has_value ()) {
                deleteError = GetErrorOfCall ([&] () { return modifier.Delete (angle); });
            }
        }
    }, "Modify MEP Angle Set");
    if (deleteError.has_value () || modifyResult.IsErr ()) {
        return CreateFailedExecutionResult (deleteError.has_value () ? *deleteError : modifyResult.UnwrapErr ().kind, "Failed to delete the angles.");
    }
    return CreateSuccessfulExecutionResult ();
}
#endif

GS::ObjectState ModifyMEPPreferenceTablesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString domainStr;
    parameters.Get ("domain", domainStr);
    const std::optional<Domain> domain = DomainFromString (domainStr);
    if (!domain.has_value ()) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid MEP domain: " + domainStr);
    }

    GS::Array<GS::ObjectState> tablesToAdd;
    parameters.Get ("tablesToAdd", tablesToAdd);
    GS::Array<GS::ObjectState> tablesData;
    parameters.Get ("tables", tablesData);
    GS::Array<GS::UniString> tablesToDelete;
    parameters.Get ("tablesToDelete", tablesToDelete);
    GS::Array<Int32> referenceIdsToDelete;
    const bool hasReferenceIdsToDelete = parameters.Get ("referenceIdsToDelete", referenceIdsToDelete);
    GS::Array<double> anglesToDelete;
    const bool hasAnglesToDelete = parameters.Get ("anglesToDelete", anglesToDelete);
    if (*domain == Domain::CableCarrier && (hasReferenceIdsToDelete || hasAnglesToDelete)) {
        return CreateErrorResponse (APIERR_BADPARS, "The CableCarrier domain has no reference set or angle set.");
    }
#ifndef ServerMainVers_2900
    if (hasAnglesToDelete) {
        return CreateErrorResponse (APIERR_NOTSUPPORTED, "The angle set is available from Archicad 29.");
    }
#endif

    GS::ObjectState response;
    const auto& addedTables = response.AddList<GS::ObjectState> ("addedTables");
    for (const GS::ObjectState& tableToAdd : tablesToAdd) {
        GS::UniString type;
        GS::UniString name;
        tableToAdd.Get ("type", type);
        tableToAdd.Get ("name", name);
        addedTables (AddPreferenceTable (*domain, type, name));
    }

    const auto& tables = response.AddList<GS::ObjectState> ("tables");
    for (const GS::ObjectState& tableData : tablesData) {
        tables (ModifyPreferenceTableOfDomain (*domain, tableData));
    }

    const auto& deletedTables = response.AddList<GS::ObjectState> ("deletedTables");
    for (const GS::UniString& guidStr : tablesToDelete) {
        deletedTables (DeletePreferenceTable (*domain, guidStr));
    }

    if (hasReferenceIdsToDelete) {
        response.Add ("referenceSet", *domain == Domain::Piping
            ? DeleteReferenceIds (GetPipeReferenceSet (), referenceIdsToDelete)
            : DeleteReferenceIds (GetDuctReferenceSet (), referenceIdsToDelete));
    }
#ifdef ServerMainVers_2900
    if (hasAnglesToDelete) {
        response.Add ("angleSet", *domain == Domain::Piping
            ? DeleteAngles (GetPipeAngleSet (), anglesToDelete)
            : DeleteAngles (GetDuctAngleSet (), anglesToDelete));
    }
#endif

    return response;
}
#else
GS::ObjectState ModifyMEPPreferenceTablesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
}
#endif

#ifdef ServerMainVers_3000

static GS::ObjectState CreateMEPElementIdObjectState (const ACAPI::MEP::UniqueID& id)
{
    return CreateElementIdObjectState (MEPUniqueIDToAPIGuid (id));
}

static GS::ObjectState CreateMEPOrientationObjectState (const Orientation& orientation)
{
    GS::ObjectState orientationOs;
    orientationOs.Add ("direction", Create3DCoordinateObjectState (API_Coord3D { orientation.direction.x, orientation.direction.y, orientation.direction.z }));
    orientationOs.Add ("rotation", Create3DCoordinateObjectState (API_Coord3D { orientation.rotation.x, orientation.rotation.y, orientation.rotation.z }));
    return orientationOs;
}

static void AddElectricalPlacement (GS::ObjectState& os, const ACAPI::MEP::ElementBase& element)
{
    os.Add ("anchorPoint", Create3DCoordinateObjectState (element.GetAnchorPoint ()));
    os.Add ("orientation", CreateMEPOrientationObjectState (element.GetOrientation ()));
    os.Add ("rotation", element.GetBaseRotation ());
    os.Add ("isReflected", element.GetReflected ());
    os.Add ("libraryPartId", CreateGuidObjectState (element.GetObjectId ()));
    os.Add ("xSize", element.GetObjectXSize ());
    os.Add ("ySize", element.GetObjectYSize ());
}

static GS::ObjectState CreateElectricalLoadsObjectState (double total, double phaseA, double phaseB, double phaseC)
{
    GS::ObjectState loadsOs;
    loadsOs.Add ("total", total);
    loadsOs.Add ("phaseA", phaseA);
    loadsOs.Add ("phaseB", phaseB);
    loadsOs.Add ("phaseC", phaseC);
    return loadsOs;
}

// Electrical elements are not listed by GetElemList (API_ExternalElemID), they have to be asked for by their classID.
static GS::Array<API_Guid> CollectElementsOfClass (const API_Guid& classId)
{
    GS::Array<API_Guid> elemList;
    ACAPI_Element_GetElemList (API_ElemType (classId), &elemList);
    return elemList;
}

// The guids of the given elements parameter, or every element accepted by isType when it is not given.
static GS::Array<API_Guid> GetRequestedOrAllElements (const GS::ObjectState& parameters, const char* parameterName, const API_Guid& classId)
{
    GS::Array<GS::ObjectState> elements;
    if (!parameters.Get (parameterName, elements)) {
        return CollectElementsOfClass (classId);
    }
    GS::Array<API_Guid> result;
    for (const GS::ObjectState& element : elements) {
        result.Push (GetGuidFromElementsArrayItem (element));
    }
    return result;
}

static GS::ObjectState GetElectricalCircuitDetails (const ACAPI::MEP::UniqueID& circuitId)
{
    ACAPI::Result<ElectricalCircuit> circuit = ElectricalCircuit::Get (circuitId);
    if (circuit.IsErr ()) {
        return CreateErrorResponse (circuit.UnwrapErr ().kind, "Failed to get the electrical circuit.");
    }

    GS::ObjectState circuitOs;
    circuitOs.Add ("elementId", CreateGuidObjectState (MEPUniqueIDToAPIGuid (circuitId)));
    circuitOs.Add ("name", circuit->GetCircuitNameWithPrefix ());
    circuitOs.Add ("description", circuit->GetCircuitDescription ());
    circuitOs.Add ("ownerPanelId", CreateGuidObjectState (MEPUniqueIDToAPIGuid (circuit->GetOwnerPanelId ())));

    const auto& childDevices = circuitOs.AddList<GS::ObjectState> ("childDevices");
    for (const ACAPI::MEP::UniqueID& deviceId : circuit->GetChildDeviceIds ()) {
        childDevices (CreateMEPElementIdObjectState (deviceId));
    }
    const auto& childPanels = circuitOs.AddList<GS::ObjectState> ("childPanels");
    for (const ACAPI::MEP::UniqueID& panelId : circuit->GetChildPanelIds ()) {
        childPanels (CreateMEPElementIdObjectState (panelId));
    }

    circuitOs.Add ("voltage", circuit->GetVoltage ());
    circuitOs.Add ("phases", circuit->GetPhaseString ());
    circuitOs.Add ("isPhaseAUsed", circuit->IsPhaseAUsed ());
    circuitOs.Add ("isPhaseBUsed", circuit->IsPhaseBUsed ());
    circuitOs.Add ("isPhaseCUsed", circuit->IsPhaseCUsed ());
    circuitOs.Add ("wiringTypeName", circuit->GetWiringTypeName ());
    circuitOs.Add ("connectedLoad", CreateElectricalLoadsObjectState (circuit->GetConnectedLoad (),
        circuit->GetConnectedLoadPhaseA (), circuit->GetConnectedLoadPhaseB (), circuit->GetConnectedLoadPhaseC ()));
    circuitOs.Add ("demandLoad", CreateElectricalLoadsObjectState (circuit->GetDemandLoad (),
        circuit->GetDemandLoadPhaseA (), circuit->GetDemandLoadPhaseB (), circuit->GetDemandLoadPhaseC ()));
    circuitOs.Add ("voltageDrop", circuit->GetVoltageDrop ());
    circuitOs.Add ("estimatedCableLength", circuit->GetEstimatedCableLength ());
    return circuitOs;
}

#else

// Only the pre-Archicad 30 branches use it; on Archicad 30 it would be an unused variable.
static const char* ElectricalRequiresArchicad30 = "This command requires Archicad 30 or newer.";

#endif

// GetElectricalPanels

GetElectricalPanelsCommand::GetElectricalPanelsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetElectricalPanelsCommand::GetName () const
{
    return "GetElectricalPanels";
}

GS::Optional<GS::UniString> GetElectricalPanelsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements",
                "description": "Optional list of electrical panels. Every electrical panel of the project is returned when it is not given."
            }
        },
        "additionalProperties": false,
        "required": []
    })";
}

GS::Optional<GS::UniString> GetElectricalPanelsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "electricalPanels": {
                "type": "array",
                "description": "The electrical panels, in the order of the given elements.",
                "items": {
                    "$ref": "#/ElectricalPanelDetailsOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "electricalPanels"
        ]
    })";
}

GS::ObjectState GetElectricalPanelsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_3000
    GS::ObjectState response;
    const auto& electricalPanels = response.AddList<GS::ObjectState> ("electricalPanels");

    for (const API_Guid& elemGuid : GetRequestedOrAllElements (parameters, "elements", ElectricalPanelID)) {
        ACAPI::Result<ElectricalPanel> panel = ElectricalPanel::Get (Adapter::UniqueID (elemGuid));
        if (panel.IsErr ()) {
            electricalPanels (CreateErrorResponse (panel.UnwrapErr ().kind, "Failed to get the electrical panel."));
            continue;
        }

        GS::ObjectState panelOs;
        panelOs.Add ("elementId", CreateGuidObjectState (elemGuid));
        panelOs.Add ("panelName", panel->GetPanelName ());
        panelOs.Add ("circuitNamePrefix", panel->GetCircuitNamePrefix ());
        panelOs.Add ("circuitIdStyle", static_cast<Int32> (panel->GetCircuitIdStyle ()));

        const auto& circuits = panelOs.AddList<GS::ObjectState> ("circuits");
        for (const ACAPI::MEP::UniqueID& circuitId : panel->GetCircuitIds ()) {
            circuits (CreateMEPElementIdObjectState (circuitId));
        }
        const std::optional<ACAPI::MEP::UniqueID> parentCircuitId = panel->GetParentCircuitId ();
        if (parentCircuitId.has_value ()) {
            panelOs.Add ("parentCircuitId", CreateGuidObjectState (MEPUniqueIDToAPIGuid (*parentCircuitId)));
        }

        AddElectricalPlacement (panelOs, *panel);
        panelOs.Add ("connectedLoad", CreateElectricalLoadsObjectState (panel->GetConnectedLoad (),
            panel->GetConnectedLoadPhaseA (), panel->GetConnectedLoadPhaseB (), panel->GetConnectedLoadPhaseC ()));
        panelOs.Add ("demandLoad", CreateElectricalLoadsObjectState (panel->GetDemandLoad (),
            panel->GetDemandLoadPhaseA (), panel->GetDemandLoadPhaseB (), panel->GetDemandLoadPhaseC ()));
        electricalPanels (panelOs);
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, ElectricalRequiresArchicad30);
#endif
}

// GetElectricalCircuits

GetElectricalCircuitsCommand::GetElectricalCircuitsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetElectricalCircuitsCommand::GetName () const
{
    return "GetElectricalCircuits";
}

GS::Optional<GS::UniString> GetElectricalCircuitsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements",
                "description": "Optional list of electrical circuits."
            },
            "electricalPanels": {
                "$ref": "#/Elements",
                "description": "Optional list of electrical panels whose circuits are returned. When neither elements nor electricalPanels is given, every circuit of every electrical panel of the project is returned."
            }
        },
        "additionalProperties": false,
        "required": []
    })";
}

GS::Optional<GS::UniString> GetElectricalCircuitsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "electricalCircuits": {
                "type": "array",
                "description": "The given circuits first, in their order, then the circuits of the given panels, in the order of the panels.",
                "items": {
                    "$ref": "#/ElectricalCircuitDetailsOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "electricalCircuits"
        ]
    })";
}

GS::ObjectState GetElectricalCircuitsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_3000
    GS::Array<GS::ObjectState> elements;
    const bool hasElements = parameters.Get ("elements", elements);
    GS::Array<GS::ObjectState> panels;
    const bool hasPanels = parameters.Get ("electricalPanels", panels);

    GS::Array<API_Guid> panelGuids;
    if (hasPanels) {
        for (const GS::ObjectState& panel : panels) {
            panelGuids.Push (GetGuidFromElementsArrayItem (panel));
        }
    } else if (!hasElements) {
        panelGuids = CollectElementsOfClass (ElectricalPanelID);
    }

    GS::ObjectState response;
    const auto& electricalCircuits = response.AddList<GS::ObjectState> ("electricalCircuits");

    for (const GS::ObjectState& element : elements) {
        electricalCircuits (GetElectricalCircuitDetails (Adapter::UniqueID (GetGuidFromElementsArrayItem (element))));
    }
    for (const API_Guid& panelGuid : panelGuids) {
        ACAPI::Result<ElectricalPanel> panel = ElectricalPanel::Get (Adapter::UniqueID (panelGuid));
        if (panel.IsErr ()) {
            electricalCircuits (CreateErrorResponse (panel.UnwrapErr ().kind, "Failed to get the electrical panel."));
            continue;
        }
        for (const ACAPI::MEP::UniqueID& circuitId : panel->GetCircuitIds ()) {
            electricalCircuits (GetElectricalCircuitDetails (circuitId));
        }
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, ElectricalRequiresArchicad30);
#endif
}

// GetElectricalDevices

GetElectricalDevicesCommand::GetElectricalDevicesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetElectricalDevicesCommand::GetName () const
{
    return "GetElectricalDevices";
}

GS::Optional<GS::UniString> GetElectricalDevicesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements",
                "description": "Optional list of electrical devices. Every electrical device of the project is returned when it is not given."
            }
        },
        "additionalProperties": false,
        "required": []
    })";
}

GS::Optional<GS::UniString> GetElectricalDevicesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "electricalDevices": {
                "type": "array",
                "description": "The electrical devices, in the order of the given elements.",
                "items": {
                    "$ref": "#/ElectricalDeviceDetailsOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "electricalDevices"
        ]
    })";
}

GS::ObjectState GetElectricalDevicesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_3000
    GS::ObjectState response;
    const auto& electricalDevices = response.AddList<GS::ObjectState> ("electricalDevices");

    for (const API_Guid& elemGuid : GetRequestedOrAllElements (parameters, "elements", ElectricalDeviceID)) {
        ACAPI::Result<ElectricalDevice> device = ElectricalDevice::Get (Adapter::UniqueID (elemGuid));
        if (device.IsErr ()) {
            electricalDevices (CreateErrorResponse (device.UnwrapErr ().kind, "Failed to get the electrical device."));
            continue;
        }

        GS::ObjectState deviceOs;
        deviceOs.Add ("elementId", CreateGuidObjectState (elemGuid));
        const std::optional<ACAPI::MEP::UniqueID> parentCircuitId = device->GetParentCircuitId ();
        if (parentCircuitId.has_value ()) {
            deviceOs.Add ("parentCircuitId", CreateGuidObjectState (MEPUniqueIDToAPIGuid (*parentCircuitId)));
        }
        AddElectricalPlacement (deviceOs, *device);
        deviceOs.Add ("apparentPower", static_cast<double> (device->GetApparentPower ()));
        deviceOs.Add ("isApparentPowerEnabled", device->GetApparentPowerEnabled ());
        deviceOs.Add ("loadClassificationName", device->GetLoadClassificationName ());
        deviceOs.Add ("demandLoad", device->GetDemandLoad ());

        const auto& connectedElements = deviceOs.AddList<GS::ObjectState> ("connectedElements");
        for (const ACAPI::MEP::UniqueID& connectedId : device->GetDirectConnectedMEPElementIds ()) {
            connectedElements (CreateMEPElementIdObjectState (connectedId));
        }
        electricalDevices (deviceOs);
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, ElectricalRequiresArchicad30);
#endif
}

// GetElectricalProjectSettings

GetElectricalProjectSettingsCommand::GetElectricalProjectSettingsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetElectricalProjectSettingsCommand::GetName () const
{
    return "GetElectricalProjectSettings";
}

GS::Optional<GS::UniString> GetElectricalProjectSettingsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "voltage": {
                "type": "number",
                "description": "The default voltage of the project."
            },
            "wiringTypes": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string"
                        },
                        "crossSectionalArea": {
                            "type": "number",
                            "description": "The cross-sectional area of the wire in square millimeters."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "crossSectionalArea"
                    ]
                }
            },
            "loadClassifications": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string"
                        },
                        "demandFactor": {
                            "type": "number",
                            "description": "The demand factor, between 0 and 1."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "demandFactor"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "voltage",
            "wiringTypes",
            "loadClassifications"
        ]
    })";
}

GS::ObjectState GetElectricalProjectSettingsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_3000
    ACAPI::Result<ElectricalProjectSettings> settings = GetElectricalProjectSettings (ACAPI_GetToken ());
    if (settings.IsErr ()) {
        return CreateErrorResponse (settings.UnwrapErr ().kind, "Failed to get the electrical project settings.");
    }

    GS::ObjectState response;
    response.Add ("voltage", settings->GetVoltage ());

    const auto& wiringTypes = response.AddList<GS::ObjectState> ("wiringTypes");
    for (const GS::UniString& name : settings->GetWiringTypeNames ()) {
        wiringTypes (GS::ObjectState ("name", name, "crossSectionalArea", settings->GetWiringTypeCrossSectionalArea (name)));
    }
    const auto& loadClassifications = response.AddList<GS::ObjectState> ("loadClassifications");
    for (const GS::UniString& name : settings->GetLoadClassificationNames ()) {
        loadClassifications (GS::ObjectState ("name", name, "demandFactor", settings->GetLoadClassificationDemandFactor (name)));
    }

    return response;
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, ElectricalRequiresArchicad30);
#endif
}

#ifdef ServerMainVers_3000

static Orientation GetMEPOrientationFromObjectState (const GS::ObjectState& orientationOs)
{
    Orientation orientation = { { 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 } };
    const GS::ObjectState* direction = orientationOs.Get ("direction");
    if (direction != nullptr) {
        const API_Coord3D coord = Get3DCoordinateFromObjectState (*direction);
        orientation.direction = { coord.x, coord.y, coord.z };
    }
    const GS::ObjectState* rotation = orientationOs.Get ("rotation");
    if (rotation != nullptr) {
        const API_Coord3D coord = Get3DCoordinateFromObjectState (*rotation);
        orientation.rotation = { coord.x, coord.y, coord.z };
    }
    return orientation;
}

// Applies the optional library part, size, rotation and mirroring of the given data to an electrical default before placing it.
template <typename DefaultT>
static ACAPI::Result<void> ApplyElectricalDefaultPlacement (DefaultT& elemDefault, const GS::ObjectState& data)
{
    return elemDefault.Modify ([&](ACAPI::MEP::ElementDefault::Modifier& modifier) -> ACAPI::Result<void> {
        // The library part first, the sizes are meant for the new library part.
        const GS::ObjectState* libraryPartId = data.Get ("libraryPartId");
        if (libraryPartId != nullptr) {
            modifier.SetObjectId (GetGuidFromObjectState (*libraryPartId));
        }
        double size = 0.0;
        if (data.Get ("xSize", size)) {
            ACAPI::Result<void> result = modifier.SetObjectXSize (size);
            if (result.IsErr ()) {
                return result;
            }
        }
        if (data.Get ("ySize", size)) {
            ACAPI::Result<void> result = modifier.SetObjectYSize (size);
            if (result.IsErr ()) {
                return result;
            }
        }
        double rotation = 0.0;
        if (data.Get ("rotation", rotation)) {
            modifier.SetBaseRotation (rotation);
        }
        bool isReflected = false;
        if (data.Get ("isReflected", isReflected)) {
            modifier.SetReflected (isReflected);
        }
        return ACAPI::Ok ();
    });
}

// Applies the optional placement fields of the given data to an electrical panel or device. Returns false if any of them failed.
static bool ApplyElectricalPlacement (ModifiableElementBase::Modifier& modifier, const GS::ObjectState& data)
{
    bool success = true;
    const GS::ObjectState* libraryPartId = data.Get ("libraryPartId");
    if (libraryPartId != nullptr) {
        modifier.SetObjectId (GetGuidFromObjectState (*libraryPartId));
    }
    const GS::ObjectState* anchorPoint = data.Get ("anchorPoint");
    if (anchorPoint != nullptr) {
        modifier.SetAnchorPoint (Get3DCoordinateFromObjectState (*anchorPoint));
    }
    const GS::ObjectState* orientation = data.Get ("orientation");
    if (orientation != nullptr) {
        modifier.SetOrientation (GetMEPOrientationFromObjectState (*orientation));
    }
    double rotation = 0.0;
    if (data.Get ("rotation", rotation)) {
        modifier.SetBaseRotation (rotation);
    }
    bool isReflected = false;
    if (data.Get ("isReflected", isReflected)) {
        modifier.SetReflected (isReflected);
    }
    double size = 0.0;
    if (data.Get ("xSize", size)) {
        success &= modifier.SetObjectXSize (size).IsOk ();
    }
    if (data.Get ("ySize", size)) {
        success &= modifier.SetObjectYSize (size).IsOk ();
    }
    return success;
}

// Archicad keeps the old library part when the given one does not exist, so the result has to be checked.
static bool IsRequestedLibraryPartUsed (const API_Guid& usedLibraryPartId, const GS::ObjectState& data)
{
    const GS::ObjectState* libraryPartId = data.Get ("libraryPartId");
    return libraryPartId == nullptr || usedLibraryPartId == GetGuidFromObjectState (*libraryPartId);
}

static const char* LibraryPartNotFound = "The library part given in 'libraryPartId' was not found.";

// Some library parts (for example the electrical outlet) calculate their own size and ignore the given one without an error.
static bool IsRequestedSizeUsed (double xSize, double ySize, const GS::ObjectState& data)
{
    const double tolerance = 1e-4;
    double size = 0.0;
    if (data.Get ("xSize", size) && std::fabs (xSize - size) > tolerance) {
        return false;
    }
    if (data.Get ("ySize", size) && std::fabs (ySize - size) > tolerance) {
        return false;
    }
    return true;
}

static const char* SizeNotApplied = "The library part does not allow the given 'xSize' or 'ySize', it keeps its own size.";

static bool HasAnyField (const GS::ObjectState& data, std::initializer_list<const char*> fieldNames)
{
    for (const char* fieldName : fieldNames) {
        if (data.Contains (fieldName)) {
            return true;
        }
    }
    return false;
}

static bool ContainsName (const std::vector<GS::UniString>& names, const GS::UniString& name)
{
    return std::find (names.begin (), names.end (), name) != names.end ();
}

// An empty load classification name means "Not set".
static bool IsKnownLoadClassification (const GS::UniString& name)
{
    if (name.IsEmpty ()) {
        return true;
    }
    ACAPI::Result<ElectricalProjectSettings> settings = GetElectricalProjectSettings (ACAPI_GetToken ());
    return settings.IsOk () && ContainsName (settings->GetLoadClassificationNames (), name);
}

// An empty wiring type name means "Not set".
static bool IsKnownWiringType (const GS::UniString& name)
{
    if (name.IsEmpty ()) {
        return true;
    }
    ACAPI::Result<ElectricalProjectSettings> settings = GetElectricalProjectSettings (ACAPI_GetToken ());
    return settings.IsOk () && ContainsName (settings->GetWiringTypeNames (), name);
}

#endif

CreateElectricalPanelsCommand::CreateElectricalPanelsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateElectricalPanelsCommand::GetName () const
{
    return "CreateElectricalPanels";
}

GS::Optional<GS::UniString> CreateElectricalPanelsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "electricalPanelsData": {
                "type": "array",
                "description": "Array of data to create electrical panels. The fields that are not given are taken from the default settings of the Electrical Panel tool.",
                "items": {
                    "type": "object",
                    "properties": {
                        "position": {
                            "$ref": "#/Coordinate3D",
                            "description": "The anchor point of the new panel."
                        },
                        "orientation": {
                            "$ref": "#/MEPOrientation",
                            "description": "Defaults to the direction (1, 0, 0) and the rotation (0, 1, 0)."
                        },
                        "rotation": {
                            "type": "number",
                            "description": "The base rotation in radians."
                        },
                        "isReflected": {
                            "type": "boolean"
                        },
                        "libraryPartId": {
                            "$ref": "#/LibPartUnId",
                            "description": "The library part to use, as listed by GetAvailableLibraryParts."
                        },
                        "xSize": {
                            "type": "number",
                            "description": "The size along the local x axis. Some library parts calculate their own size, then the item fails."
                        },
                        "ySize": {
                            "type": "number",
                            "description": "The size along the local y axis. Some library parts calculate their own size, then the item fails."
                        },
                        "panelName": {
                            "type": "string"
                        },
                        "circuitNamePrefix": {
                            "type": "string"
                        },
                        "circuitIdStyle": {
                            "type": "integer"
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
            "electricalPanelsData"
        ]
    })";
}

GS::Optional<GS::UniString> CreateElectricalPanelsCommand::GetRawResponseSchema () const
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

GS::ObjectState CreateElectricalPanelsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_3000
    GS::Array<GS::ObjectState> electricalPanelsData;
    if (!parameters.Get ("electricalPanelsData", electricalPanelsData)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'electricalPanelsData' parameter.");
    }

    GS::ObjectState response;
    const auto& elements = response.AddList<GS::ObjectState> ("elements");

    ACAPI_CallUndoableCommand ("Create Electrical Panels", [&]() -> GSErrCode {
        for (const GS::ObjectState& panelData : electricalPanelsData) {
            const GS::ObjectState* position = panelData.Get ("position");
            if (position == nullptr) {
                elements (CreateErrorResponse (APIERR_BADPARS, "Missing 'position' parameter."));
                continue;
            }

            ACAPI::Result<ElectricalPanelDefault> panelDefault = CreateElectricalPanelDefault ();
            if (panelDefault.IsErr ()) {
                elements (CreateErrorResponse (panelDefault.UnwrapErr ().kind, "Failed to get the default electrical panel."));
                continue;
            }
            ACAPI::Result<void> defaultResult = ApplyElectricalDefaultPlacement (*panelDefault, panelData);
            if (defaultResult.IsErr ()) {
                elements (CreateErrorResponse (defaultResult.UnwrapErr ().kind, "Failed to set the library part, size, rotation or mirroring of the electrical panel."));
                continue;
            }
            if (!IsRequestedLibraryPartUsed (panelDefault->GetObjectId (), panelData)) {
                elements (CreateErrorResponse (APIERR_BADID, LibraryPartNotFound));
                continue;
            }

            const GS::ObjectState* orientationOs = panelData.Get ("orientation");
            const Orientation orientation = orientationOs != nullptr ? GetMEPOrientationFromObjectState (*orientationOs) : GetMEPOrientationFromObjectState (GS::ObjectState ());
            ACAPI::Result<ACAPI::MEP::UniqueID> newPanelId = panelDefault->Place (Get3DCoordinateFromObjectState (*position), orientation);
            if (newPanelId.IsErr ()) {
                elements (CreateErrorResponse (newPanelId.UnwrapErr ().kind, "Failed to place the electrical panel."));
                continue;
            }
            if (HasAnyField (panelData, { "xSize", "ySize" })) {
                ACAPI::Result<ElectricalPanel> placed = ElectricalPanel::Get (newPanelId.Unwrap ());
                if (placed.IsOk () && !IsRequestedSizeUsed (placed->GetObjectXSize (), placed->GetObjectYSize (), panelData)) {
                    ACAPI_Element_Delete ({ MEPUniqueIDToAPIGuid (newPanelId.Unwrap ()) });
                    elements (CreateErrorResponse (APIERR_BADPARS, SizeNotApplied));
                    continue;
                }
            }

            if (HasAnyField (panelData, { "panelName", "circuitNamePrefix", "circuitIdStyle" })) {
                ACAPI::Result<ElectricalPanel> panel = ElectricalPanel::Get (newPanelId.Unwrap ());
                ACAPI::Result<void> modifyResult = panel.IsErr () ? ACAPI::Result<void> (panel.UnwrapErr (), panel.Token ()) : panel->Modify ([&](ElectricalPanel::Modifier& modifier) {
                    GS::UniString text;
                    if (panelData.Get ("panelName", text)) {
                        modifier.SetPanelName (text);
                    }
                    if (panelData.Get ("circuitNamePrefix", text)) {
                        modifier.SetCircuitNamePrefix (text);
                    }
                    Int32 circuitIdStyle = 0;
                    if (panelData.Get ("circuitIdStyle", circuitIdStyle)) {
                        modifier.SetCircuitIdStyle (static_cast<short> (circuitIdStyle));
                    }
                }, "Set Electrical Panel Data");
                if (modifyResult.IsErr ()) {
                    elements (CreateErrorResponse (modifyResult.UnwrapErr ().kind, "The electrical panel was placed, but its name, circuit name prefix or circuit id style could not be set."));
                    continue;
                }
            }

            elements (CreateMEPElementIdObjectState (newPanelId.Unwrap ()));
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, ElectricalRequiresArchicad30);
#endif
}

CreateElectricalDevicesCommand::CreateElectricalDevicesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateElectricalDevicesCommand::GetName () const
{
    return "CreateElectricalDevices";
}

GS::Optional<GS::UniString> CreateElectricalDevicesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "electricalDevicesData": {
                "type": "array",
                "description": "Array of data to create electrical devices. The fields that are not given are taken from the default settings of the Electrical Device tool.",
                "items": {
                    "type": "object",
                    "properties": {
                        "position": {
                            "$ref": "#/Coordinate3D",
                            "description": "The anchor point of the new device."
                        },
                        "orientation": {
                            "$ref": "#/MEPOrientation",
                            "description": "Defaults to the direction (1, 0, 0) and the rotation (0, 1, 0)."
                        },
                        "rotation": {
                            "type": "number",
                            "description": "The base rotation in radians."
                        },
                        "isReflected": {
                            "type": "boolean"
                        },
                        "libraryPartId": {
                            "$ref": "#/LibPartUnId",
                            "description": "The library part to use, as listed by GetAvailableLibraryParts."
                        },
                        "xSize": {
                            "type": "number",
                            "description": "The size along the local x axis. Some library parts calculate their own size, then the item fails."
                        },
                        "ySize": {
                            "type": "number",
                            "description": "The size along the local y axis. Some library parts calculate their own size, then the item fails."
                        },
                        "apparentPower": {
                            "type": "integer",
                            "minimum": 0,
                            "description": "The apparent power in VA."
                        },
                        "isApparentPowerEnabled": {
                            "type": "boolean"
                        },
                        "loadClassificationName": {
                            "type": "string",
                            "description": "One of the load classifications of the electrical project settings, or an empty string for none."
                        },
                        "circuitId": {
                            "$ref": "#/ElementId",
                            "description": "The circuit to connect the new device to."
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
            "electricalDevicesData"
        ]
    })";
}

GS::Optional<GS::UniString> CreateElectricalDevicesCommand::GetRawResponseSchema () const
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

GS::ObjectState CreateElectricalDevicesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_3000
    GS::Array<GS::ObjectState> electricalDevicesData;
    if (!parameters.Get ("electricalDevicesData", electricalDevicesData)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'electricalDevicesData' parameter.");
    }

    GS::ObjectState response;
    const auto& elements = response.AddList<GS::ObjectState> ("elements");

    ACAPI_CallUndoableCommand ("Create Electrical Devices", [&]() -> GSErrCode {
        for (const GS::ObjectState& deviceData : electricalDevicesData) {
            const GS::ObjectState* position = deviceData.Get ("position");
            if (position == nullptr) {
                elements (CreateErrorResponse (APIERR_BADPARS, "Missing 'position' parameter."));
                continue;
            }

            GS::UniString loadClassificationName;
            const bool hasLoadClassification = deviceData.Get ("loadClassificationName", loadClassificationName);
            if (hasLoadClassification && !IsKnownLoadClassification (loadClassificationName)) {
                elements (CreateErrorResponse (APIERR_BADPARS, "The load classification '" + loadClassificationName + "' is not defined in the electrical project settings."));
                continue;
            }

            const GS::ObjectState* circuitId = deviceData.Get ("circuitId");
            if (circuitId != nullptr && ElectricalCircuit::Get (Adapter::UniqueID (GetGuidFromObjectState (*circuitId))).IsErr ()) {
                elements (CreateErrorResponse (APIERR_BADID, "The 'circuitId' is not an electrical circuit."));
                continue;
            }

            ACAPI::Result<ElectricalDeviceDefault> deviceDefault = CreateElectricalDeviceDefault ();
            if (deviceDefault.IsErr ()) {
                elements (CreateErrorResponse (deviceDefault.UnwrapErr ().kind, "Failed to get the default electrical device."));
                continue;
            }
            ACAPI::Result<void> defaultResult = ApplyElectricalDefaultPlacement (*deviceDefault, deviceData);
            if (defaultResult.IsErr ()) {
                elements (CreateErrorResponse (defaultResult.UnwrapErr ().kind, "Failed to set the library part, size, rotation or mirroring of the electrical device."));
                continue;
            }
            if (!IsRequestedLibraryPartUsed (deviceDefault->GetObjectId (), deviceData)) {
                elements (CreateErrorResponse (APIERR_BADID, LibraryPartNotFound));
                continue;
            }

            const GS::ObjectState* orientationOs = deviceData.Get ("orientation");
            const Orientation orientation = orientationOs != nullptr ? GetMEPOrientationFromObjectState (*orientationOs) : GetMEPOrientationFromObjectState (GS::ObjectState ());
            ACAPI::Result<ACAPI::MEP::UniqueID> newDeviceId = deviceDefault->Place (Get3DCoordinateFromObjectState (*position), orientation);
            if (newDeviceId.IsErr ()) {
                elements (CreateErrorResponse (newDeviceId.UnwrapErr ().kind, "Failed to place the electrical device."));
                continue;
            }
            if (HasAnyField (deviceData, { "xSize", "ySize" })) {
                ACAPI::Result<ElectricalDevice> placed = ElectricalDevice::Get (newDeviceId.Unwrap ());
                if (placed.IsOk () && !IsRequestedSizeUsed (placed->GetObjectXSize (), placed->GetObjectYSize (), deviceData)) {
                    ACAPI_Element_Delete ({ MEPUniqueIDToAPIGuid (newDeviceId.Unwrap ()) });
                    elements (CreateErrorResponse (APIERR_BADPARS, SizeNotApplied));
                    continue;
                }
            }

            if (HasAnyField (deviceData, { "apparentPower", "isApparentPowerEnabled", "loadClassificationName", "circuitId" })) {
                ACAPI::Result<ElectricalDevice> device = ElectricalDevice::Get (newDeviceId.Unwrap ());
                ACAPI::Result<void> modifyResult = device.IsErr () ? ACAPI::Result<void> (device.UnwrapErr (), device.Token ()) : device->Modify ([&](ElectricalDevice::Modifier& modifier) {
                    Int32 apparentPower = 0;
                    if (deviceData.Get ("apparentPower", apparentPower)) {
                        modifier.SetApparentPower (static_cast<uint32_t> (apparentPower));
                    }
                    bool isApparentPowerEnabled = false;
                    if (deviceData.Get ("isApparentPowerEnabled", isApparentPowerEnabled)) {
                        modifier.SetApparentPowerEnabled (isApparentPowerEnabled);
                    }
                    if (hasLoadClassification) {
                        modifier.SetLoadClassificationName (loadClassificationName);
                    }
                    if (circuitId != nullptr) {
                        modifier.ConnectToCircuit (Adapter::UniqueID (GetGuidFromObjectState (*circuitId)));
                    }
                }, "Set Electrical Device Data");
                if (modifyResult.IsErr ()) {
                    elements (CreateErrorResponse (modifyResult.UnwrapErr ().kind, "The electrical device was placed, but its electrical data or circuit could not be set."));
                    continue;
                }
            }

            elements (CreateMEPElementIdObjectState (newDeviceId.Unwrap ()));
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, ElectricalRequiresArchicad30);
#endif
}

ModifyElectricalPanelsCommand::ModifyElectricalPanelsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyElectricalPanelsCommand::GetName () const
{
    return "ModifyElectricalPanels";
}

GS::Optional<GS::UniString> ModifyElectricalPanelsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "electricalPanelsData": {
                "type": "array",
                "description": "Array of data to modify electrical panels. Only the given fields are changed.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "panelName": {
                            "type": "string"
                        },
                        "circuitNamePrefix": {
                            "type": "string"
                        },
                        "circuitIdStyle": {
                            "type": "integer"
                        },
                        "anchorPoint": {
                            "$ref": "#/Coordinate3D"
                        },
                        "orientation": {
                            "$ref": "#/MEPOrientation"
                        },
                        "rotation": {
                            "type": "number",
                            "description": "The base rotation in radians."
                        },
                        "isReflected": {
                            "type": "boolean"
                        },
                        "libraryPartId": {
                            "$ref": "#/LibPartUnId",
                            "description": "The library part to use, as listed by GetAvailableLibraryParts."
                        },
                        "xSize": {
                            "type": "number",
                            "description": "The size along the local x axis. Some library parts calculate their own size, then the item fails."
                        },
                        "ySize": {
                            "type": "number",
                            "description": "The size along the local y axis. Some library parts calculate their own size, then the item fails."
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
            "electricalPanelsData"
        ]
    })";
}

GS::Optional<GS::UniString> ModifyElectricalPanelsCommand::GetRawResponseSchema () const
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

GS::ObjectState ModifyElectricalPanelsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_3000
    GS::Array<GS::ObjectState> electricalPanelsData;
    if (!parameters.Get ("electricalPanelsData", electricalPanelsData)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'electricalPanelsData' parameter.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    for (const GS::ObjectState& panelData : electricalPanelsData) {
        const GS::ObjectState* elementId = panelData.Get ("elementId");
        if (elementId == nullptr) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing 'elementId' parameter."));
            continue;
        }

        ACAPI::Result<ElectricalPanel> panel = ElectricalPanel::Get (Adapter::UniqueID (GetGuidFromObjectState (*elementId)));
        if (panel.IsErr ()) {
            executionResults (CreateFailedExecutionResult (panel.UnwrapErr ().kind, "Failed to get the electrical panel."));
            continue;
        }

        bool placementSuccess = true;
        ACAPI::Result<void> modifyResult = panel->Modify ([&](ElectricalPanel::Modifier& modifier) {
            GS::UniString text;
            if (panelData.Get ("panelName", text)) {
                modifier.SetPanelName (text);
            }
            if (panelData.Get ("circuitNamePrefix", text)) {
                modifier.SetCircuitNamePrefix (text);
            }
            Int32 circuitIdStyle = 0;
            if (panelData.Get ("circuitIdStyle", circuitIdStyle)) {
                modifier.SetCircuitIdStyle (static_cast<short> (circuitIdStyle));
            }
            placementSuccess = ApplyElectricalPlacement (modifier, panelData);
        }, "Modify Electrical Panel");

        if (modifyResult.IsErr ()) {
            executionResults (CreateFailedExecutionResult (modifyResult.UnwrapErr ().kind, "Failed to modify the electrical panel."));
        } else if (!placementSuccess) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Failed to set the size of the electrical panel."));
        } else {
            ACAPI::Result<ElectricalPanel> modified = ElectricalPanel::Get (Adapter::UniqueID (GetGuidFromObjectState (*elementId)));
            if (modified.IsOk () && !IsRequestedLibraryPartUsed (modified->GetObjectId (), panelData)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADID, GS::UniString (LibraryPartNotFound) + " The other changes were made."));
            } else if (modified.IsOk () && !IsRequestedSizeUsed (modified->GetObjectXSize (), modified->GetObjectYSize (), panelData)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, GS::UniString (SizeNotApplied) + " The other changes were made."));
            } else {
                executionResults (CreateSuccessfulExecutionResult ());
            }
        }
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, ElectricalRequiresArchicad30);
#endif
}

CreateElectricalCircuitsCommand::CreateElectricalCircuitsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateElectricalCircuitsCommand::GetName () const
{
    return "CreateElectricalCircuits";
}

GS::Optional<GS::UniString> CreateElectricalCircuitsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "electricalCircuitsData": {
                "type": "array",
                "description": "Array of data to create electrical circuits. Each item adds one circuit to the given panel.",
                "items": {
                    "type": "object",
                    "properties": {
                        "electricalPanelId": {
                            "$ref": "#/ElementId",
                            "description": "The panel to add the new circuit to."
                        },
                        "description": {
                            "type": "string"
                        },
                        "voltage": {
                            "type": "number"
                        },
                        "wiringTypeName": {
                            "type": "string",
                            "description": "One of the wiring types of the electrical project settings, or an empty string for none."
                        },
                        "isPhaseAUsed": {
                            "type": "boolean"
                        },
                        "isPhaseBUsed": {
                            "type": "boolean"
                        },
                        "isPhaseCUsed": {
                            "type": "boolean"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "electricalPanelId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "electricalCircuitsData"
        ]
    })";
}

GS::Optional<GS::UniString> CreateElectricalCircuitsCommand::GetRawResponseSchema () const
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

#ifdef ServerMainVers_3000

// Sets the given circuit fields of the data. Returns the error of the modification, if any.
static std::optional<ACAPI::Error> ModifyElectricalCircuit (ElectricalCircuit& circuit, const GS::ObjectState& circuitData, const GS::UniString& undoString)
{
    ACAPI::Result<void> modifyResult = circuit.Modify ([&](ElectricalCircuit::Modifier& modifier) {
        GS::UniString text;
        if (circuitData.Get ("description", text)) {
            modifier.SetCircuitDescription (text);
        }
        if (circuitData.Get ("wiringTypeName", text)) {
            modifier.SetWiringTypeName (text);
        }
        double voltage = 0.0;
        if (circuitData.Get ("voltage", voltage)) {
            modifier.SetVoltage (voltage);
        }
        bool isUsed = false;
        if (circuitData.Get ("isPhaseAUsed", isUsed)) {
            modifier.SetIsPhaseAUsed (isUsed);
        }
        if (circuitData.Get ("isPhaseBUsed", isUsed)) {
            modifier.SetIsPhaseBUsed (isUsed);
        }
        if (circuitData.Get ("isPhaseCUsed", isUsed)) {
            modifier.SetIsPhaseCUsed (isUsed);
        }
    }, undoString);
    if (modifyResult.IsErr ()) {
        return modifyResult.UnwrapErr ();
    }
    return std::nullopt;
}

#endif

GS::ObjectState CreateElectricalCircuitsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_3000
    GS::Array<GS::ObjectState> electricalCircuitsData;
    if (!parameters.Get ("electricalCircuitsData", electricalCircuitsData)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'electricalCircuitsData' parameter.");
    }

    GS::ObjectState response;
    const auto& elements = response.AddList<GS::ObjectState> ("elements");

    for (const GS::ObjectState& circuitData : electricalCircuitsData) {
        const GS::ObjectState* panelId = circuitData.Get ("electricalPanelId");
        if (panelId == nullptr) {
            elements (CreateErrorResponse (APIERR_BADPARS, "Missing 'electricalPanelId' parameter."));
            continue;
        }

        GS::UniString wiringTypeName;
        if (circuitData.Get ("wiringTypeName", wiringTypeName) && !IsKnownWiringType (wiringTypeName)) {
            elements (CreateErrorResponse (APIERR_BADPARS, "The wiring type '" + wiringTypeName + "' is not defined in the electrical project settings."));
            continue;
        }

        ACAPI::Result<ElectricalPanel> panel = ElectricalPanel::Get (Adapter::UniqueID (GetGuidFromObjectState (*panelId)));
        if (panel.IsErr ()) {
            elements (CreateErrorResponse (panel.UnwrapErr ().kind, "Failed to get the electrical panel."));
            continue;
        }

        std::optional<ACAPI::MEP::UniqueID> newCircuitId;
        std::optional<ACAPI::Error> addError;
        ACAPI::Result<void> modifyResult = panel->Modify ([&](ElectricalPanel::Modifier& modifier) {
            ACAPI::Result<ACAPI::MEP::UniqueID> result = modifier.AddCircuit ();
            if (result.IsOk ()) {
                newCircuitId = result.Unwrap ();
            } else {
                addError = result.UnwrapErr ();
            }
        }, "Create Electrical Circuit");

        if (addError.has_value () || modifyResult.IsErr () || !newCircuitId.has_value ()) {
            const GSErrCode errorCode = addError.has_value () ? addError->kind : (modifyResult.IsErr () ? modifyResult.UnwrapErr ().kind : APIERR_GENERAL);
            elements (CreateErrorResponse (errorCode, "Failed to add a circuit to the electrical panel."));
            continue;
        }

        if (HasAnyField (circuitData, { "description", "voltage", "wiringTypeName", "isPhaseAUsed", "isPhaseBUsed", "isPhaseCUsed" })) {
            ACAPI::Result<ElectricalCircuit> circuit = ElectricalCircuit::Get (newCircuitId.value ());
            const std::optional<ACAPI::Error> circuitError = circuit.IsErr () ? std::optional<ACAPI::Error> (circuit.UnwrapErr ()) : ModifyElectricalCircuit (*circuit, circuitData, "Set Electrical Circuit Data");
            if (circuitError.has_value ()) {
                elements (CreateErrorResponse (circuitError->kind, "The circuit was added, but its data could not be set."));
                continue;
            }
        }

        elements (CreateMEPElementIdObjectState (newCircuitId.value ()));
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, ElectricalRequiresArchicad30);
#endif
}

DeleteElectricalCircuitsCommand::DeleteElectricalCircuitsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteElectricalCircuitsCommand::GetName () const
{
    return "DeleteElectricalCircuits";
}

GS::Optional<GS::UniString> DeleteElectricalCircuitsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "electricalCircuits": {
                "$ref": "#/Elements",
                "description": "The circuits to remove from their panels."
            }
        },
        "additionalProperties": false,
        "required": [
            "electricalCircuits"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteElectricalCircuitsCommand::GetRawResponseSchema () const
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

GS::ObjectState DeleteElectricalCircuitsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_3000
    GS::Array<GS::ObjectState> electricalCircuits;
    if (!parameters.Get ("electricalCircuits", electricalCircuits)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'electricalCircuits' parameter.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    for (const GS::ObjectState& circuitItem : electricalCircuits) {
        const ACAPI::MEP::UniqueID circuitId = Adapter::UniqueID (GetGuidFromElementsArrayItem (circuitItem));
        ACAPI::Result<ElectricalCircuit> circuit = ElectricalCircuit::Get (circuitId);
        if (circuit.IsErr ()) {
            executionResults (CreateFailedExecutionResult (circuit.UnwrapErr ().kind, "Failed to get the electrical circuit."));
            continue;
        }

        ACAPI::Result<ElectricalPanel> panel = ElectricalPanel::Get (circuit->GetOwnerPanelId ());
        if (panel.IsErr ()) {
            executionResults (CreateFailedExecutionResult (panel.UnwrapErr ().kind, "Failed to get the panel of the electrical circuit."));
            continue;
        }

        std::optional<ACAPI::Error> removeError;
        ACAPI::Result<void> modifyResult = panel->Modify ([&](ElectricalPanel::Modifier& modifier) {
            ACAPI::Result<void> result = modifier.RemoveCircuit (circuitId);
            if (result.IsErr ()) {
                removeError = result.UnwrapErr ();
            }
        }, "Delete Electrical Circuit");

        if (removeError.has_value ()) {
            executionResults (CreateFailedExecutionResult (removeError->kind, "Failed to remove the circuit from its panel."));
        } else if (modifyResult.IsErr ()) {
            executionResults (CreateFailedExecutionResult (modifyResult.UnwrapErr ().kind, "Failed to remove the circuit from its panel."));
        } else {
            executionResults (CreateSuccessfulExecutionResult ());
        }
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, ElectricalRequiresArchicad30);
#endif
}

ModifyElectricalCircuitsCommand::ModifyElectricalCircuitsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyElectricalCircuitsCommand::GetName () const
{
    return "ModifyElectricalCircuits";
}

GS::Optional<GS::UniString> ModifyElectricalCircuitsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "electricalCircuitsData": {
                "type": "array",
                "description": "Array of data to modify electrical circuits. Only the given fields are changed.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "description": {
                            "type": "string"
                        },
                        "voltage": {
                            "type": "number"
                        },
                        "wiringTypeName": {
                            "type": "string",
                            "description": "One of the wiring types of the electrical project settings, or an empty string for none."
                        },
                        "isPhaseAUsed": {
                            "type": "boolean"
                        },
                        "isPhaseBUsed": {
                            "type": "boolean"
                        },
                        "isPhaseCUsed": {
                            "type": "boolean"
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
            "electricalCircuitsData"
        ]
    })";
}

GS::Optional<GS::UniString> ModifyElectricalCircuitsCommand::GetRawResponseSchema () const
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

GS::ObjectState ModifyElectricalCircuitsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_3000
    GS::Array<GS::ObjectState> electricalCircuitsData;
    if (!parameters.Get ("electricalCircuitsData", electricalCircuitsData)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'electricalCircuitsData' parameter.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    for (const GS::ObjectState& circuitData : electricalCircuitsData) {
        const GS::ObjectState* elementId = circuitData.Get ("elementId");
        if (elementId == nullptr) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing 'elementId' parameter."));
            continue;
        }

        GS::UniString wiringTypeName;
        if (circuitData.Get ("wiringTypeName", wiringTypeName) && !IsKnownWiringType (wiringTypeName)) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "The wiring type '" + wiringTypeName + "' is not defined in the electrical project settings."));
            continue;
        }

        ACAPI::Result<ElectricalCircuit> circuit = ElectricalCircuit::Get (Adapter::UniqueID (GetGuidFromObjectState (*elementId)));
        if (circuit.IsErr ()) {
            executionResults (CreateFailedExecutionResult (circuit.UnwrapErr ().kind, "Failed to get the electrical circuit."));
            continue;
        }

        const std::optional<ACAPI::Error> error = ModifyElectricalCircuit (*circuit, circuitData, "Modify Electrical Circuit");
        executionResults (error.has_value ()
            ? CreateFailedExecutionResult (error->kind, "Failed to modify the electrical circuit.")
            : CreateSuccessfulExecutionResult ());
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, ElectricalRequiresArchicad30);
#endif
}

ModifyElectricalDevicesCommand::ModifyElectricalDevicesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyElectricalDevicesCommand::GetName () const
{
    return "ModifyElectricalDevices";
}

GS::Optional<GS::UniString> ModifyElectricalDevicesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "electricalDevicesData": {
                "type": "array",
                "description": "Array of data to modify electrical devices. Only the given fields are changed.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "apparentPower": {
                            "type": "integer",
                            "minimum": 0,
                            "description": "The apparent power in VA."
                        },
                        "isApparentPowerEnabled": {
                            "type": "boolean"
                        },
                        "loadClassificationName": {
                            "type": "string",
                            "description": "One of the load classifications of the electrical project settings, or an empty string for none."
                        },
                        "anchorPoint": {
                            "$ref": "#/Coordinate3D"
                        },
                        "orientation": {
                            "$ref": "#/MEPOrientation"
                        },
                        "rotation": {
                            "type": "number",
                            "description": "The base rotation in radians."
                        },
                        "isReflected": {
                            "type": "boolean"
                        },
                        "libraryPartId": {
                            "$ref": "#/LibPartUnId",
                            "description": "The library part to use, as listed by GetAvailableLibraryParts."
                        },
                        "xSize": {
                            "type": "number",
                            "description": "The size along the local x axis. Some library parts calculate their own size, then the item fails."
                        },
                        "ySize": {
                            "type": "number",
                            "description": "The size along the local y axis. Some library parts calculate their own size, then the item fails."
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
            "electricalDevicesData"
        ]
    })";
}

GS::Optional<GS::UniString> ModifyElectricalDevicesCommand::GetRawResponseSchema () const
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

GS::ObjectState ModifyElectricalDevicesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_3000
    GS::Array<GS::ObjectState> electricalDevicesData;
    if (!parameters.Get ("electricalDevicesData", electricalDevicesData)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'electricalDevicesData' parameter.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    for (const GS::ObjectState& deviceData : electricalDevicesData) {
        const GS::ObjectState* elementId = deviceData.Get ("elementId");
        if (elementId == nullptr) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing 'elementId' parameter."));
            continue;
        }

        GS::UniString loadClassificationName;
        const bool hasLoadClassification = deviceData.Get ("loadClassificationName", loadClassificationName);
        if (hasLoadClassification && !IsKnownLoadClassification (loadClassificationName)) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "The load classification '" + loadClassificationName + "' is not defined in the electrical project settings."));
            continue;
        }

        ACAPI::Result<ElectricalDevice> device = ElectricalDevice::Get (Adapter::UniqueID (GetGuidFromObjectState (*elementId)));
        if (device.IsErr ()) {
            executionResults (CreateFailedExecutionResult (device.UnwrapErr ().kind, "Failed to get the electrical device."));
            continue;
        }

        bool placementSuccess = true;
        ACAPI::Result<void> modifyResult = device->Modify ([&](ElectricalDevice::Modifier& modifier) {
            Int32 apparentPower = 0;
            if (deviceData.Get ("apparentPower", apparentPower)) {
                modifier.SetApparentPower (static_cast<uint32_t> (apparentPower));
            }
            bool isApparentPowerEnabled = false;
            if (deviceData.Get ("isApparentPowerEnabled", isApparentPowerEnabled)) {
                modifier.SetApparentPowerEnabled (isApparentPowerEnabled);
            }
            if (hasLoadClassification) {
                modifier.SetLoadClassificationName (loadClassificationName);
            }
            placementSuccess = ApplyElectricalPlacement (modifier, deviceData);
        }, "Modify Electrical Device");

        if (modifyResult.IsErr ()) {
            executionResults (CreateFailedExecutionResult (modifyResult.UnwrapErr ().kind, "Failed to modify the electrical device."));
        } else if (!placementSuccess) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Failed to set the size of the electrical device."));
        } else {
            ACAPI::Result<ElectricalDevice> modified = ElectricalDevice::Get (Adapter::UniqueID (GetGuidFromObjectState (*elementId)));
            if (modified.IsOk () && !IsRequestedLibraryPartUsed (modified->GetObjectId (), deviceData)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADID, GS::UniString (LibraryPartNotFound) + " The other changes were made."));
            } else if (modified.IsOk () && !IsRequestedSizeUsed (modified->GetObjectXSize (), modified->GetObjectYSize (), deviceData)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, GS::UniString (SizeNotApplied) + " The other changes were made."));
            } else {
                executionResults (CreateSuccessfulExecutionResult ());
            }
        }
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, ElectricalRequiresArchicad30);
#endif
}

ConnectToElectricalCircuitsCommand::ConnectToElectricalCircuitsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ConnectToElectricalCircuitsCommand::GetName () const
{
    return "ConnectToElectricalCircuits";
}

GS::Optional<GS::UniString> ConnectToElectricalCircuitsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "connectionsData": {
                "type": "array",
                "description": "Array of electrical devices or panels and the circuits to connect them to. A panel connected to a circuit becomes a sub-panel of the panel of that circuit.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId",
                            "description": "An electrical device or an electrical panel."
                        },
                        "circuitId": {
                            "$ref": "#/ElementId",
                            "description": "The electrical circuit to connect to."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "circuitId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "connectionsData"
        ]
    })";
}

GS::Optional<GS::UniString> ConnectToElectricalCircuitsCommand::GetRawResponseSchema () const
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

GS::ObjectState ConnectToElectricalCircuitsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_3000
    GS::Array<GS::ObjectState> connectionsData;
    if (!parameters.Get ("connectionsData", connectionsData)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'connectionsData' parameter.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    for (const GS::ObjectState& connectionData : connectionsData) {
        const GS::ObjectState* elementId = connectionData.Get ("elementId");
        const GS::ObjectState* circuitIdOs = connectionData.Get ("circuitId");
        if (elementId == nullptr || circuitIdOs == nullptr) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing 'elementId' or 'circuitId' parameter."));
            continue;
        }

        const ACAPI::MEP::UniqueID circuitId = Adapter::UniqueID (GetGuidFromObjectState (*circuitIdOs));
        if (ElectricalCircuit::Get (circuitId).IsErr ()) {
            executionResults (CreateFailedExecutionResult (APIERR_BADID, "The 'circuitId' is not an electrical circuit."));
            continue;
        }

        const ACAPI::MEP::UniqueID uniqueId = Adapter::UniqueID (GetGuidFromObjectState (*elementId));
        ACAPI::Result<void> modifyResult = [&]() -> ACAPI::Result<void> {
            ACAPI::Result<ElectricalDevice> device = ElectricalDevice::Get (uniqueId);
            if (device.IsOk ()) {
                return device->Modify ([&](ElectricalDevice::Modifier& modifier) {
                    modifier.ConnectToCircuit (circuitId);
                }, "Connect Electrical Device to Circuit");
            }
            ACAPI::Result<ElectricalPanel> panel = ElectricalPanel::Get (uniqueId);
            if (panel.IsOk ()) {
                return panel->Modify ([&](ElectricalPanel::Modifier& modifier) {
                    modifier.ConnectToCircuit (circuitId);
                }, "Connect Electrical Panel to Circuit");
            }
            return { panel.UnwrapErr (), panel.Token () };
        } ();

        executionResults (modifyResult.IsErr ()
            ? CreateFailedExecutionResult (modifyResult.UnwrapErr ().kind, "Failed to connect the element to the circuit. The element must be an electrical device or panel.")
            : CreateSuccessfulExecutionResult ());
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, ElectricalRequiresArchicad30);
#endif
}

ModifyElectricalProjectSettingsCommand::ModifyElectricalProjectSettingsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyElectricalProjectSettingsCommand::GetName () const
{
    return "ModifyElectricalProjectSettings";
}

GS::Optional<GS::UniString> ModifyElectricalProjectSettingsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "voltage": {
                "type": "number"
            },
            "wiringTypesToAdd": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string"
                        },
                        "crossSectionalArea": {
                            "type": "number"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "crossSectionalArea"
                    ]
                }
            },
            "wiringTypesToRemove": {
                "type": "array",
                "description": "The names of the wiring types to remove.",
                "items": {
                    "type": "string"
                }
            },
            "loadClassificationsToAdd": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string"
                        },
                        "demandFactor": {
                            "type": "number"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "demandFactor"
                    ]
                }
            },
            "loadClassificationsToRemove": {
                "type": "array",
                "description": "The names of the load classifications to remove.",
                "items": {
                    "type": "string"
                }
            }
        },
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> ModifyElectricalProjectSettingsCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState ModifyElectricalProjectSettingsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_3000
    ACAPI::Result<ElectricalProjectSettings> settings = GetElectricalProjectSettings (ACAPI_GetToken ());
    if (settings.IsErr ()) {
        return CreateFailedExecutionResult (settings.UnwrapErr ().kind, "Failed to get the electrical project settings.");
    }

    GS::Array<GS::ObjectState> wiringTypesToAdd;
    parameters.Get ("wiringTypesToAdd", wiringTypesToAdd);
    GS::Array<GS::UniString> wiringTypesToRemove;
    parameters.Get ("wiringTypesToRemove", wiringTypesToRemove);
    GS::Array<GS::ObjectState> loadClassificationsToAdd;
    parameters.Get ("loadClassificationsToAdd", loadClassificationsToAdd);
    GS::Array<GS::UniString> loadClassificationsToRemove;
    parameters.Get ("loadClassificationsToRemove", loadClassificationsToRemove);

    // Removing an unknown name is reported instead of being silently ignored.
    const std::vector<GS::UniString> wiringTypeNames = settings->GetWiringTypeNames ();
    for (const GS::UniString& name : wiringTypesToRemove) {
        if (!ContainsName (wiringTypeNames, name)) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "The wiring type '" + name + "' is not defined in the electrical project settings.");
        }
    }
    const std::vector<GS::UniString> loadClassificationNames = settings->GetLoadClassificationNames ();
    for (const GS::UniString& name : loadClassificationsToRemove) {
        if (!ContainsName (loadClassificationNames, name)) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "The load classification '" + name + "' is not defined in the electrical project settings.");
        }
    }

    // Archicad accepts the same name twice, so adding an existing name is refused unless it is removed in the same call.
    std::vector<GS::UniString> remainingWiringTypeNames;
    for (const GS::UniString& name : wiringTypeNames) {
        if (!wiringTypesToRemove.Contains (name)) {
            remainingWiringTypeNames.push_back (name);
        }
    }
    for (const GS::ObjectState& wiringType : wiringTypesToAdd) {
        GS::UniString name;
        wiringType.Get ("name", name);
        if (ContainsName (remainingWiringTypeNames, name)) {
            return CreateFailedExecutionResult (APIERR_NAMEALREADYUSED, "The wiring type '" + name + "' already exists in the electrical project settings.");
        }
        remainingWiringTypeNames.push_back (name);
    }
    std::vector<GS::UniString> remainingLoadClassificationNames;
    for (const GS::UniString& name : loadClassificationNames) {
        if (!loadClassificationsToRemove.Contains (name)) {
            remainingLoadClassificationNames.push_back (name);
        }
    }
    for (const GS::ObjectState& loadClassification : loadClassificationsToAdd) {
        GS::UniString name;
        loadClassification.Get ("name", name);
        if (ContainsName (remainingLoadClassificationNames, name)) {
            return CreateFailedExecutionResult (APIERR_NAMEALREADYUSED, "The load classification '" + name + "' already exists in the electrical project settings.");
        }
        remainingLoadClassificationNames.push_back (name);
    }

    std::optional<ACAPI::Error> removeError;
    ACAPI::Result<void> modifyResult = settings->Modify ([&](ElectricalProjectSettings::Modifier& modifier) {
        double voltage = 0.0;
        if (parameters.Get ("voltage", voltage)) {
            modifier.SetVoltage (voltage);
        }
        // Removals first, so that a wiring type or load classification can be replaced in one call.
        for (const GS::UniString& name : wiringTypesToRemove) {
            ACAPI::Result<void> result = modifier.RemoveWiringType (name);
            if (result.IsErr () && !removeError.has_value ()) {
                removeError = result.UnwrapErr ();
            }
        }
        for (const GS::UniString& name : loadClassificationsToRemove) {
            ACAPI::Result<void> result = modifier.RemoveLoadClassification (name);
            if (result.IsErr () && !removeError.has_value ()) {
                removeError = result.UnwrapErr ();
            }
        }
        for (const GS::ObjectState& wiringType : wiringTypesToAdd) {
            GS::UniString name;
            double crossSectionalArea = 0.0;
            wiringType.Get ("name", name);
            wiringType.Get ("crossSectionalArea", crossSectionalArea);
            modifier.AddWiringType (name, crossSectionalArea);
        }
        for (const GS::ObjectState& loadClassification : loadClassificationsToAdd) {
            GS::UniString name;
            double demandFactor = 0.0;
            loadClassification.Get ("name", name);
            loadClassification.Get ("demandFactor", demandFactor);
            modifier.AddLoadClassification (name, demandFactor);
        }
    }, "Modify Electrical Project Settings");

    if (removeError.has_value ()) {
        return CreateFailedExecutionResult (removeError->kind, "Failed to remove a wiring type or load classification.");
    }
    if (modifyResult.IsErr ()) {
        return CreateFailedExecutionResult (modifyResult.UnwrapErr ().kind, "Failed to modify the electrical project settings.");
    }
    return CreateSuccessfulExecutionResult ();
#else
    UNUSED_PARAMETER (parameters);
    return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, ElectricalRequiresArchicad30);
#endif
}

#ifdef ServerMainVers_2900

static GS::ObjectState CreateMEPIdObjectState (const char* idFieldName, const ACAPI::MEP::UniqueID& id)
{
    return CreateIdObjectState (idFieldName, MEPUniqueIDToAPIGuid (id));
}

// IsEditable returns an ACAPI::Result<void>: it succeeds when the system or group can be modified and fails otherwise.
static std::optional<bool> ToOptionalBool (ACAPI::Result<void> value)
{
    return value.IsOk ();
}

static void AddIsEditable (GS::ObjectState& os, const std::optional<bool>& isEditable)
{
    if (isEditable.has_value ()) {
        os.Add ("isEditable", isEditable.value ());
    }
}

static GS::ObjectState CreatePhysicalSystemDetails (const ACAPI::MEP::UniqueID& systemId)
{
    ACAPI::Result<PhysicalSystem> system = PhysicalSystem::Get (systemId);
    if (system.IsErr ()) {
        return CreateErrorResponse (system.UnwrapErr ().kind, "Failed to get the physical system.");
    }
    GS::ObjectState systemOs;
    systemOs.Add ("physicalSystemId", CreateGuidObjectState (MEPUniqueIDToAPIGuid (systemId)));
    systemOs.Add ("name", system->GetName ());
    AddIsEditable (systemOs, ToOptionalBool (system->IsEditable ()));
    const std::optional<ACAPI::MEP::UniqueID> rootElementId = system->GetRootElementId ();
    if (rootElementId.has_value ()) {
        systemOs.Add ("rootElementId", CreateGuidObjectState (MEPUniqueIDToAPIGuid (rootElementId.value ())));
    }
    const API_Guid systemCategoryGuid = GetAttributeGuidFromIndex (API_MEPSystemID, system->GetSystemCategory ());
    if (systemCategoryGuid != APINULLGuid) {
        systemOs.Add ("mepSystemId", CreateGuidObjectState (systemCategoryGuid));
    }
    return systemOs;
}

static bool ContainsMEPId (const std::vector<ACAPI::MEP::UniqueID>& ids, const ACAPI::MEP::UniqueID& id)
{
    for (const ACAPI::MEP::UniqueID& other : ids) {
        if (other.GetGuid () == id.GetGuid ()) {
            return true;
        }
    }
    return false;
}

// A physical system does not tell its group, so the group is the one that lists the system.
static std::optional<ACAPI::MEP::UniqueID> FindSystemGroupOfPhysicalSystem (const ACAPI::MEP::UniqueID& systemId)
{
    ACAPI::Result<std::vector<ACAPI::MEP::UniqueID>> groupIds = GetSystemGroupIDs ();
    if (groupIds.IsErr ()) {
        return std::nullopt;
    }
    for (const ACAPI::MEP::UniqueID& groupId : groupIds.Unwrap ()) {
        ACAPI::Result<SystemGroup> group = SystemGroup::Get (groupId);
        if (group.IsErr ()) {
            continue;
        }
        for (const ACAPI::MEP::UniqueID& groupSystemId : group->GetSystemIDs ()) {
            if (groupSystemId.GetGuid () == systemId.GetGuid ()) {
                return groupId;
            }
        }
    }
    return std::nullopt;
}

#else

// Only the pre-Archicad 29 branches use it; on Archicad 29 it would be an unused variable.
static const char* MEPSystemsRequireArchicad29 = "This command requires Archicad 29 or newer.";

#endif

GetMEPSystemGroupsCommand::GetMEPSystemGroupsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetMEPSystemGroupsCommand::GetName () const
{
    return "GetMEPSystemGroups";
}

GS::Optional<GS::UniString> GetMEPSystemGroupsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "systemGroups": {
                "type": "array",
                "description": "The system groups of the project with their physical systems.",
                "items": {
                    "$ref": "#/MEPSystemGroupDetailsOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "systemGroups"
        ]
    })";
}

GS::ObjectState GetMEPSystemGroupsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    ACAPI::Result<std::vector<ACAPI::MEP::UniqueID>> groupIds = GetSystemGroupIDs ();
    if (groupIds.IsErr ()) {
        return CreateErrorResponse (groupIds.UnwrapErr ().kind, "Failed to get the system groups.");
    }

    GS::ObjectState response;
    const auto& systemGroups = response.AddList<GS::ObjectState> ("systemGroups");
    for (const ACAPI::MEP::UniqueID& groupId : groupIds.Unwrap ()) {
        ACAPI::Result<SystemGroup> group = SystemGroup::Get (groupId);
        if (group.IsErr ()) {
            systemGroups (CreateErrorResponse (group.UnwrapErr ().kind, "Failed to get the system group."));
            continue;
        }
        GS::ObjectState groupOs;
        groupOs.Add ("systemGroupId", CreateGuidObjectState (MEPUniqueIDToAPIGuid (groupId)));
        groupOs.Add ("name", group->GetName ());
        AddIsEditable (groupOs, ToOptionalBool (group->IsEditable ()));
        const auto& physicalSystems = groupOs.AddList<GS::ObjectState> ("physicalSystems");
        for (const ACAPI::MEP::UniqueID& systemId : group->GetSystemIDs ()) {
            physicalSystems (CreatePhysicalSystemDetails (systemId));
        }
        systemGroups (groupOs);
    }
    return response;
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, MEPSystemsRequireArchicad29);
#endif
}

CreateMEPSystemGroupsCommand::CreateMEPSystemGroupsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateMEPSystemGroupsCommand::GetName () const
{
    return "CreateMEPSystemGroups";
}

GS::Optional<GS::UniString> CreateMEPSystemGroupsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "systemGroupsData": {
                "type": "array",
                "description": "Array of data to create system groups.",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "systemGroupsData"
        ]
    })";
}

GS::Optional<GS::UniString> CreateMEPSystemGroupsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "systemGroups": {
                "type": "array",
                "items": {
                    "$ref": "#/MEPSystemGroupIdOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "systemGroups"
        ]
    })";
}

GS::ObjectState CreateMEPSystemGroupsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    GS::Array<GS::ObjectState> systemGroupsData;
    if (!parameters.Get ("systemGroupsData", systemGroupsData)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'systemGroupsData' parameter.");
    }

    GS::ObjectState response;
    const auto& systemGroups = response.AddList<GS::ObjectState> ("systemGroups");
    for (const GS::ObjectState& groupData : systemGroupsData) {
        GS::UniString name;
        groupData.Get ("name", name);
        // A system group does not tell its own id, so the new id is the one that was not there before.
        ACAPI::Result<std::vector<ACAPI::MEP::UniqueID>> idsBefore = GetSystemGroupIDs ();
        ACAPI::Result<SystemGroup> group = CreateSystemGroup (name, "Create MEP System Group");
        if (group.IsErr ()) {
            systemGroups (CreateErrorResponse (group.UnwrapErr ().kind, "Failed to create the system group."));
            continue;
        }
        ACAPI::Result<std::vector<ACAPI::MEP::UniqueID>> idsAfter = GetSystemGroupIDs ();
        std::optional<ACAPI::MEP::UniqueID> newGroupId;
        if (idsBefore.IsOk () && idsAfter.IsOk ()) {
            for (const ACAPI::MEP::UniqueID& id : idsAfter.Unwrap ()) {
                if (!ContainsMEPId (idsBefore.Unwrap (), id)) {
                    newGroupId = id;
                }
            }
        }
        if (!newGroupId.has_value ()) {
            systemGroups (CreateErrorResponse (APIERR_GENERAL, "The system group was created, but its id could not be found."));
            continue;
        }
        systemGroups (CreateMEPIdObjectState ("systemGroupId", newGroupId.value ()));
    }
    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, MEPSystemsRequireArchicad29);
#endif
}

CreateMEPPhysicalSystemsCommand::CreateMEPPhysicalSystemsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateMEPPhysicalSystemsCommand::GetName () const
{
    return "CreateMEPPhysicalSystems";
}

GS::Optional<GS::UniString> CreateMEPPhysicalSystemsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "physicalSystemsData": {
                "type": "array",
                "description": "Array of data to create physical systems.",
                "items": {
                    "type": "object",
                    "properties": {
                        "systemGroupId": {
                            "$ref": "#/MEPSystemGroupId",
                            "description": "The system group the new physical system belongs to."
                        },
                        "name": {
                            "type": "string"
                        },
                        "rootElementId": {
                            "$ref": "#/ElementId",
                            "description": "The MEP element the system starts from. Archicad keeps it only while it is part of an MEP network: the root of a stand-alone element is cleared when the MEP networks are recalculated."
                        },
                        "mepSystemId": {
                            "$ref": "#/AttributeId",
                            "description": "The MEP system attribute used as the category of the physical system."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "systemGroupId",
                        "name"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "physicalSystemsData"
        ]
    })";
}

GS::Optional<GS::UniString> CreateMEPPhysicalSystemsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "physicalSystems": {
                "type": "array",
                "items": {
                    "$ref": "#/MEPPhysicalSystemIdOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "physicalSystems"
        ]
    })";
}

GS::ObjectState CreateMEPPhysicalSystemsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    GS::Array<GS::ObjectState> physicalSystemsData;
    if (!parameters.Get ("physicalSystemsData", physicalSystemsData)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'physicalSystemsData' parameter.");
    }

    GS::ObjectState response;
    const auto& physicalSystems = response.AddList<GS::ObjectState> ("physicalSystems");

    {
        for (const GS::ObjectState& systemData : physicalSystemsData) {
            const GS::ObjectState* groupId = systemData.Get ("systemGroupId");
            GS::UniString name;
            systemData.Get ("name", name);
            if (groupId == nullptr) {
                physicalSystems (CreateErrorResponse (APIERR_BADPARS, "Missing 'systemGroupId' parameter."));
                continue;
            }
            ACAPI::Result<SystemGroup> group = SystemGroup::Get (Adapter::UniqueID (GetGuidFromObjectState (*groupId)));
            if (group.IsErr ()) {
                physicalSystems (CreateErrorResponse (APIERR_BADID, "The 'systemGroupId' is not a system group."));
                continue;
            }

            const GS::ObjectState* mepSystemId = systemData.Get ("mepSystemId");
            const API_AttributeIndex systemCategory = mepSystemId != nullptr ? GetAttributeIndexFromGuid (API_MEPSystemID, GetGuidFromObjectState (*mepSystemId)) : ACAPI_CreateAttributeIndex (-1);
            if (mepSystemId != nullptr && systemCategory == ACAPI_CreateAttributeIndex (-1)) {
                physicalSystems (CreateErrorResponse (APIERR_BADID, "The 'mepSystemId' is not an MEP system attribute."));
                continue;
            }

            // Adding through the group gives back the id of the new system.
            std::optional<ACAPI::MEP::UniqueID> newSystemId;
            std::optional<ACAPI::Error> addError;
            ACAPI::Result<void> addResult = group->Modify ([&](SystemGroup::Modifier& modifier) {
                ACAPI::Result<ACAPI::MEP::UniqueID> result = modifier.Add (name);
                if (result.IsOk ()) {
                    newSystemId = result.Unwrap ();
                } else {
                    addError = result.UnwrapErr ();
                }
            }, "Create MEP Physical System");
            if (addError.has_value () || addResult.IsErr () || !newSystemId.has_value ()) {
                const GSErrCode errorCode = addError.has_value () ? addError->kind : (addResult.IsErr () ? addResult.UnwrapErr ().kind : APIERR_GENERAL);
                physicalSystems (CreateErrorResponse (errorCode, "Failed to create the physical system."));
                continue;
            }
            ACAPI::Result<PhysicalSystem> system = PhysicalSystem::Get (newSystemId.value ());
            if (system.IsErr ()) {
                physicalSystems (CreateErrorResponse (system.UnwrapErr ().kind, "The physical system was created, but it could not be read back."));
                continue;
            }

            const GS::ObjectState* rootElementId = systemData.Get ("rootElementId");
            if (rootElementId != nullptr || mepSystemId != nullptr) {
                std::optional<ACAPI::Error> setError;
                ACAPI::Result<void> modifyResult = system->Modify ([&](PhysicalSystem::Modifier& modifier) {
                    if (rootElementId != nullptr) {
                        ACAPI::Result<void> result = modifier.SetRootElement (Adapter::UniqueID (GetGuidFromObjectState (*rootElementId)));
                        if (result.IsErr () && !setError.has_value ()) {
                            setError = result.UnwrapErr ();
                        }
                    }
                    if (mepSystemId != nullptr) {
                        ACAPI::Result<void> result = modifier.SetSystemCategory (systemCategory);
                        if (result.IsErr () && !setError.has_value ()) {
                            setError = result.UnwrapErr ();
                        }
                    }
                }, "Set MEP Physical System Data");
                if (setError.has_value () || modifyResult.IsErr ()) {
                    // Do not leave a half set up system behind.
                    static_cast<void> (group->Modify ([&](SystemGroup::Modifier& modifier) {
                        static_cast<void> (modifier.Remove (newSystemId.value ()));
                    }, "Remove MEP Physical System"));
                    physicalSystems (CreateErrorResponse (setError.has_value () ? setError->kind : modifyResult.UnwrapErr ().kind,
                        "The root element or the MEP system could not be set, so the physical system was not created."));
                    continue;
                }
            }

            physicalSystems (CreateMEPIdObjectState ("physicalSystemId", newSystemId.value ()));
        }
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, MEPSystemsRequireArchicad29);
#endif
}

ModifyMEPSystemGroupsCommand::ModifyMEPSystemGroupsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyMEPSystemGroupsCommand::GetName () const
{
    return "ModifyMEPSystemGroups";
}

GS::Optional<GS::UniString> ModifyMEPSystemGroupsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "systemGroupsData": {
                "type": "array",
                "description": "Array of data to modify system groups.",
                "items": {
                    "type": "object",
                    "properties": {
                        "systemGroupId": {
                            "$ref": "#/MEPSystemGroupId"
                        },
                        "name": {
                            "type": "string",
                            "description": "The new name of the system group."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "systemGroupId",
                        "name"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "systemGroupsData"
        ]
    })";
}

GS::Optional<GS::UniString> ModifyMEPSystemGroupsCommand::GetRawResponseSchema () const
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

GS::ObjectState ModifyMEPSystemGroupsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    GS::Array<GS::ObjectState> systemGroupsData;
    if (!parameters.Get ("systemGroupsData", systemGroupsData)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'systemGroupsData' parameter.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");
    for (const GS::ObjectState& groupData : systemGroupsData) {
        const GS::ObjectState* groupId = groupData.Get ("systemGroupId");
        if (groupId == nullptr) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing 'systemGroupId' parameter."));
            continue;
        }
        GS::UniString name;
        if (!groupData.Get ("name", name) || name.IsEmpty ()) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "The 'name' parameter is missing or empty."));
            continue;
        }
        ACAPI::Result<SystemGroup> group = SystemGroup::Get (Adapter::UniqueID (GetGuidFromObjectState (*groupId)));
        if (group.IsErr ()) {
            executionResults (CreateFailedExecutionResult (APIERR_BADID, "The 'systemGroupId' is not a system group."));
            continue;
        }

        std::optional<GSErrCode> setError;
        ACAPI::Result<void> modifyResult = group->Modify ([&](SystemGroup::Modifier& modifier) {
            setError = GetErrorOfCall ([&] () { return modifier.SetName (name); });
        }, "Modify MEP System Group");
        if (setError.has_value () || modifyResult.IsErr ()) {
            executionResults (CreateFailedExecutionResult (setError.has_value () ? setError.value () : modifyResult.UnwrapErr ().kind,
                "Failed to rename the system group."));
            continue;
        }
        executionResults (CreateSuccessfulExecutionResult ());
    }
    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, MEPSystemsRequireArchicad29);
#endif
}

ModifyMEPPhysicalSystemsCommand::ModifyMEPPhysicalSystemsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyMEPPhysicalSystemsCommand::GetName () const
{
    return "ModifyMEPPhysicalSystems";
}

GS::Optional<GS::UniString> ModifyMEPPhysicalSystemsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "physicalSystemsData": {
                "type": "array",
                "description": "Array of data to modify physical systems. Only the given fields are changed.",
                "items": {
                    "type": "object",
                    "properties": {
                        "physicalSystemId": {
                            "$ref": "#/MEPPhysicalSystemId"
                        },
                        "name": {
                            "type": "string",
                            "description": "The new name of the physical system."
                        },
                        "rootElementId": {
                            "$ref": "#/ElementId",
                            "description": "The MEP element the system starts from. Archicad keeps it only while it is part of an MEP network: the root of a stand-alone element is cleared when the MEP networks are recalculated."
                        },
                        "mepSystemId": {
                            "$ref": "#/AttributeId",
                            "description": "The MEP system attribute used as the category of the physical system."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "physicalSystemId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "physicalSystemsData"
        ]
    })";
}

GS::Optional<GS::UniString> ModifyMEPPhysicalSystemsCommand::GetRawResponseSchema () const
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

GS::ObjectState ModifyMEPPhysicalSystemsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    GS::Array<GS::ObjectState> physicalSystemsData;
    if (!parameters.Get ("physicalSystemsData", physicalSystemsData)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'physicalSystemsData' parameter.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");
    for (const GS::ObjectState& systemData : physicalSystemsData) {
        const GS::ObjectState* systemId = systemData.Get ("physicalSystemId");
        if (systemId == nullptr) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing 'physicalSystemId' parameter."));
            continue;
        }

        GS::UniString name;
        const bool hasName = systemData.Get ("name", name);
        if (hasName && name.IsEmpty ()) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "The 'name' parameter must not be empty."));
            continue;
        }

        const GS::ObjectState* mepSystemId = systemData.Get ("mepSystemId");
        const API_AttributeIndex systemCategory = mepSystemId != nullptr ? GetAttributeIndexFromGuid (API_MEPSystemID, GetGuidFromObjectState (*mepSystemId)) : ACAPI_CreateAttributeIndex (-1);
        if (mepSystemId != nullptr && systemCategory == ACAPI_CreateAttributeIndex (-1)) {
            executionResults (CreateFailedExecutionResult (APIERR_BADID, "The 'mepSystemId' is not an MEP system attribute."));
            continue;
        }

        ACAPI::Result<PhysicalSystem> system = PhysicalSystem::Get (Adapter::UniqueID (GetGuidFromObjectState (*systemId)));
        if (system.IsErr ()) {
            executionResults (CreateFailedExecutionResult (APIERR_BADID, "The 'physicalSystemId' is not a physical system."));
            continue;
        }

        const GS::ObjectState* rootElementId = systemData.Get ("rootElementId");
        std::optional<GSErrCode> setError;
        ACAPI::Result<void> modifyResult = system->Modify ([&](PhysicalSystem::Modifier& modifier) {
            if (hasName) {
                setError = GetErrorOfCall ([&] () { return modifier.SetName (name); });
            }
            if (rootElementId != nullptr && !setError.has_value ()) {
                ACAPI::Result<void> result = modifier.SetRootElement (Adapter::UniqueID (GetGuidFromObjectState (*rootElementId)));
                if (result.IsErr ()) {
                    setError = result.UnwrapErr ().kind;
                }
            }
            if (mepSystemId != nullptr && !setError.has_value ()) {
                ACAPI::Result<void> result = modifier.SetSystemCategory (systemCategory);
                if (result.IsErr ()) {
                    setError = result.UnwrapErr ().kind;
                }
            }
        }, "Modify MEP Physical System");
        if (setError.has_value () || modifyResult.IsErr ()) {
            executionResults (CreateFailedExecutionResult (setError.has_value () ? setError.value () : modifyResult.UnwrapErr ().kind,
                "Failed to modify the physical system."));
            continue;
        }
        executionResults (CreateSuccessfulExecutionResult ());
    }
    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, MEPSystemsRequireArchicad29);
#endif
}

DeleteMEPPhysicalSystemsCommand::DeleteMEPPhysicalSystemsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteMEPPhysicalSystemsCommand::GetName () const
{
    return "DeleteMEPPhysicalSystems";
}

GS::Optional<GS::UniString> DeleteMEPPhysicalSystemsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "physicalSystems": {
                "type": "array",
                "description": "The physical systems to remove from their system groups. Their member elements are not deleted.",
                "items": {
                    "type": "object",
                    "properties": {
                        "physicalSystemId": {
                            "$ref": "#/MEPPhysicalSystemId"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "physicalSystemId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "physicalSystems"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteMEPPhysicalSystemsCommand::GetRawResponseSchema () const
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

GS::ObjectState DeleteMEPPhysicalSystemsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    GS::Array<GS::ObjectState> physicalSystems;
    if (!parameters.Get ("physicalSystems", physicalSystems)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'physicalSystems' parameter.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");
    for (const GS::ObjectState& systemItem : physicalSystems) {
        const GS::ObjectState* systemIdOs = systemItem.Get ("physicalSystemId");
        if (systemIdOs == nullptr) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing 'physicalSystemId' parameter."));
            continue;
        }
        const ACAPI::MEP::UniqueID systemId = Adapter::UniqueID (GetGuidFromObjectState (*systemIdOs));
        const std::optional<ACAPI::MEP::UniqueID> groupId = FindSystemGroupOfPhysicalSystem (systemId);
        if (!groupId.has_value ()) {
            executionResults (CreateFailedExecutionResult (APIERR_BADID, "The 'physicalSystemId' is not a physical system of any system group."));
            continue;
        }
        ACAPI::Result<SystemGroup> group = SystemGroup::Get (groupId.value ());
        if (group.IsErr ()) {
            executionResults (CreateFailedExecutionResult (group.UnwrapErr ().kind, "Failed to get the system group of the physical system."));
            continue;
        }

        std::optional<GSErrCode> removeError;
        ACAPI::Result<void> modifyResult = group->Modify ([&](SystemGroup::Modifier& modifier) {
            removeError = GetErrorOfCall ([&] () { return modifier.Remove (systemId); });
        }, "Delete MEP Physical System");
        if (removeError.has_value () || modifyResult.IsErr ()) {
            executionResults (CreateFailedExecutionResult (removeError.has_value () ? removeError.value () : modifyResult.UnwrapErr ().kind,
                "Failed to remove the physical system from its system group."));
            continue;
        }
        executionResults (CreateSuccessfulExecutionResult ());
    }
    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, MEPSystemsRequireArchicad29);
#endif
}
