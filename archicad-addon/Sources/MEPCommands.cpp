#include "MEPCommands.hpp"

#ifdef ServerMainVers_2800
#include "ACAPI/Result.hpp"
#include "ACAPI/MEPAdapter.hpp"
#include "ACAPI/MEPTypes.hpp"
#include "ACAPI/MEPEnums.hpp"
#include "ACAPI/MEPElement.hpp"
#include "ACAPI/MEPPort.hpp"
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
            segmentOs.Add ("crossSectionWidth", segment->GetCrossSectionWidth ());
            segmentOs.Add ("crossSectionHeight", segment->GetCrossSectionHeight ());
            segmentOs.Add ("crossSectionShape", ConnectorShapeToString (segment->GetCrossSectionShape ()));
            ACAPI::Result<uint32_t> referenceId = segment->GetCrossSectionReferenceId ();
            if (referenceId.IsOk ()) {
                segmentOs.Add ("crossSectionReferenceId", static_cast<Int32> (referenceId.Unwrap ()));
            }
            segmentOs.Add ("preferenceTableId", segment->GetPreferenceTableId ().GetGuid ().ToUniString ());
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
            nodeOs.Add ("position", Create3DCoordinateObjectState (node->GetPosition ()));
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

        GS::ObjectState elementPortsOs;
        const auto& ports = elementPortsOs.AddList<GS::ObjectState> ("ports");

        for (const ACAPI::MEP::UniqueID& portId : mepElement->GetPortIDs ()) {
            ACAPI::Result<Port> port = Port::Get (portId);
            if (port.IsErr ()) {
                continue;
            }

            GS::ObjectState portOs;
            portOs.Add ("portId", APIGuidToString (MEPUniqueIDToAPIGuid (portId)));
            portOs.Add ("name", port->GetName ());
            portOs.Add ("position", Create3DCoordinateObjectState (port->GetPosition ()));
            const Orientation orientation = port->GetOrientation ();
            portOs.Add ("direction", Create3DCoordinateObjectState (API_Coord3D { orientation.direction.x, orientation.direction.y, orientation.direction.z }));
            portOs.Add ("shape", ConnectorShapeToString (port->GetShape ()));
            portOs.Add ("width", port->GetWidth ());
            portOs.Add ("height", port->GetHeight ());
            portOs.Add ("domain", DomainToString (port->GetDomain ()));
            portOs.Add ("mepSystemId", CreateMEPSystemIdObjectState (port->GetMEPSystem ()));
            portOs.Add ("isPhysicallyConnected", port->IsPhysicallyConnected ());

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

            const GS::ObjectState* mepSystemId = routingElementData.Get ("mepSystemId");
            routingElementDefault->Modify ([&](RoutingElementDefault::Modifier& modifier) {
                modifier.SetRoutingSegmentDefault (segmentDefault);
                if (mepSystemId != nullptr) {
                    modifier.SetMEPSystem (GetAttributeIndexFromGuid (API_MEPSystemID, GetGuidFromObjectState (*mepSystemId)));
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

        bool success = true;

        const GS::ObjectState* mepSystemId = routingElementData.Get ("mepSystemId");
        if (mepSystemId != nullptr) {
            ACAPI::Result<void> result = routingElement->Modify ([&](RoutingElement::Modifier& modifier) {
                modifier.SetMEPSystem (GetAttributeIndexFromGuid (API_MEPSystemID, GetGuidFromObjectState (*mepSystemId)));
            }, "Modify MEP System of Routing Element");
            success &= result.IsOk ();
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
        if (hasWidth || hasHeight || hasShape || hasReferenceId || hasPreferenceTableId) {
            for (const ACAPI::MEP::UniqueID& segmentId : routingElement->GetRoutingSegmentIds ()) {
                ACAPI::Result<RoutingSegment> segment = RoutingSegment::Get (segmentId);
                if (segment.IsErr ()) {
                    success = false;
                    continue;
                }
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
                }, "Modify Cross Section of Routing Segment");
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
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
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
                "description": "The circular segment preference tables of the domain.",
                "items": {
                    "$ref": "#/MEPPreferenceTable"
                }
            }
        },
        "additionalProperties": false,
        "required": ["tables"]
    })";
}

#ifdef ServerMainVers_2800
template <typename TableT>
static GS::ObjectState DumpPreferenceTable (const TableT& table, const GS::Guid& tableGuid)
{
    GS::ObjectState tableOS;
    tableOS.Add ("guid", tableGuid.ToUniString ());
    const auto& rows = tableOS.AddList<GS::ObjectState> ("rows");
    const uint32_t size = table.GetSize ();
    for (uint32_t i = 0; i < size; ++i) {
        auto rowDiameter = table.GetDiameter (i);
        auto rowReferenceId = table.GetReferenceId (i);
        if (rowDiameter.IsErr () || rowReferenceId.IsErr ()) {
            continue;
        }
        GS::ObjectState rowOS;
        rowOS.Add ("referenceId", static_cast<Int32> (*rowReferenceId));
        rowOS.Add ("diameter", *rowDiameter);
        auto rowDescription = table.GetDescription (i);
        if (rowDescription.IsOk ()) {
            rowOS.Add ("description", *rowDescription);
        }
        rows (rowOS);
    }
    return tableOS;
}

GS::ObjectState GetMEPPreferenceTablesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString domainStr;
    parameters.Get ("domain", domainStr);
    const std::optional<Domain> domain = DomainFromString (domainStr);
    if (!domain.has_value () || *domain == Domain::CableCarrier) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid MEP domain: " + domainStr);
    }

    GS::ObjectState response;
    const auto& tables = response.AddList<GS::ObjectState> ("tables");

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
                            "type": "number"
                        },
                        "ySize": {
                            "type": "number"
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
                            "type": "number"
                        },
                        "ySize": {
                            "type": "number"
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
                            "type": "number"
                        },
                        "ySize": {
                            "type": "number"
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
        } else if (!IsRequestedLibraryPartUsed (ElectricalPanel::Get (Adapter::UniqueID (GetGuidFromObjectState (*elementId)))->GetObjectId (), panelData)) {
            executionResults (CreateFailedExecutionResult (APIERR_BADID, GS::UniString (LibraryPartNotFound) + " The other changes were made."));
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
                            "type": "number"
                        },
                        "ySize": {
                            "type": "number"
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
        } else if (!IsRequestedLibraryPartUsed (ElectricalDevice::Get (Adapter::UniqueID (GetGuidFromObjectState (*elementId)))->GetObjectId (), deviceData)) {
            executionResults (CreateFailedExecutionResult (APIERR_BADID, GS::UniString (LibraryPartNotFound) + " The other changes were made."));
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
