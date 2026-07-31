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
#include "ACAPI/MEPUniqueID.hpp"
#include "ACAPI/MEPPipeSegmentPreferenceTable.hpp"
#include "ACAPI/MEPPipeSegmentPreferenceTableContainer.hpp"
#include "ACAPI/MEPDuctCircularSegmentPreferenceTable.hpp"
#include "ACAPI/MEPDuctSegmentPreferenceTableContainer.hpp"

#include <optional>

using namespace ACAPI::MEP;
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
                    "type": "string",
                    "enum": ["RoutingElement", "RigidSegment", "Elbow", "Transition", "Branch", "Terminal", "Accessory", "Equipment", "Fitting", "FlexibleSegment", "TakeOff"]
                }
            },
            "domains": {
                "type": "array",
                "description": "Optional filter for the MEP domains.",
                "items": {
                    "type": "string",
                    "enum": ["Ventilation", "Piping", "CableCarrier"]
                }
            }
        },
        "additionalProperties": false,
        "required": []
    })";
}

GS::Optional<GS::UniString> GetMEPElementsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "type": "array",
                "description": "The MEP elements.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "type": {
                            "type": "string",
                            "description": "The type of the MEP element."
                        },
                        "domain": {
                            "type": "string",
                            "description": "The MEP domain of the element. Empty for domain-independent elements (e.g. Equipment)."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "type",
                        "domain"
                    ]
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

GS::Optional<GS::UniString> GetMEPRoutingElementsCommand::GetResponseSchema () const
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

GS::Optional<GS::UniString> GetMEPPortsCommand::GetResponseSchema () const
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

GS::Optional<GS::UniString> GetMEPDistributionSystemsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "distributionSystems": {
                "type": "array",
                "description": "The distribution systems of the project.",
                "items": {
                    "type": "object",
                    "properties": {
                        "domain": {
                            "type": "string"
                        },
                        "mepSystemId": {
                            "$ref": "#/AttributeId"
                        },
                        "elements": {
                            "$ref": "#/Elements"
                        }
                    },
                    "additionalProperties": false,
                    "required": ["domain", "elements"]
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
                            "type": "string",
                            "enum": ["Ventilation", "Piping", "CableCarrier"]
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
                            "type": "string",
                            "description": "Optional cross section shape applied to all segments.",
                            "enum": ["Rectangular", "Circular", "Oval", "UShape"]
                        },
                        "crossSectionReferenceId": {
                            "type": "integer",
                            "description": "Optional cross section reference id of the segment preference table (used for circular cross sections)."
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

GS::Optional<GS::UniString> CreateMEPRoutingElementsCommand::GetResponseSchema () const
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
            const bool hasWidth = routingElementData.Get ("crossSectionWidth", width);
            const bool hasHeight = routingElementData.Get ("crossSectionHeight", height);
            const bool hasShape = routingElementData.Get ("crossSectionShape", shapeStr);
            const bool hasReferenceId = routingElementData.Get ("crossSectionReferenceId", referenceId);
            if (hasWidth || hasHeight || hasShape || hasReferenceId) {
                segmentDefault.Modify ([&](RoutingSegmentDefault::Modifier& modifier) {
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
                            "type": "string",
                            "enum": ["Terminal", "Accessory", "Equipment", "Fitting"]
                        },
                        "domain": {
                            "type": "string",
                            "description": "The MEP domain of the element. Required for all types except Equipment.",
                            "enum": ["Ventilation", "Piping", "CableCarrier"]
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

GS::Optional<GS::UniString> CreateMEPElementsCommand::GetResponseSchema () const
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
                            "type": "string",
                            "description": "New cross section shape applied to all segments.",
                            "enum": ["Rectangular", "Circular", "Oval", "UShape"]
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

GS::Optional<GS::UniString> ModifyMEPRoutingElementsCommand::GetResponseSchema () const
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
        const bool hasWidth = routingElementData.Get ("crossSectionWidth", width);
        const bool hasHeight = routingElementData.Get ("crossSectionHeight", height);
        const bool hasShape = routingElementData.Get ("crossSectionShape", shapeStr);
        if (hasWidth || hasHeight || hasShape) {
            for (const ACAPI::MEP::UniqueID& segmentId : routingElement->GetRoutingSegmentIds ()) {
                ACAPI::Result<RoutingSegment> segment = RoutingSegment::Get (segmentId);
                if (segment.IsErr ()) {
                    success = false;
                    continue;
                }
                ACAPI::Result<void> result = segment->Modify ([&](RoutingSegment::Modifier& modifier) {
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

GS::Optional<GS::UniString> ConnectMEPElementsCommand::GetResponseSchema () const
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
    CommandBase (CommonSchema::NotUsed)
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
                "type": "string",
                "description": "The MEP domain of the segment preference tables.",
                "enum": ["Piping", "Ventilation"]
            }
        },
        "additionalProperties": false,
        "required": ["domain"]
    })";
}

GS::Optional<GS::UniString> GetMEPPreferenceTablesCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "tables": {
                "type": "array",
                "description": "The circular segment preference tables of the domain.",
                "items": {
                    "type": "object",
                    "properties": {
                        "guid": { "type": "string" },
                        "rows": {
                            "type": "array",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "referenceId": { "type": "integer" },
                                    "diameter": { "type": "number" },
                                    "description": { "type": "string" }
                                },
                                "required": ["referenceId", "diameter"],
                                "additionalProperties": false
                            }
                        }
                    },
                    "required": ["guid", "rows"],
                    "additionalProperties": false
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
        for (const UniqueID& id : container->GetPreferenceTables ()) {
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
        for (const UniqueID& id : container->GetPreferenceTables ()) {
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
