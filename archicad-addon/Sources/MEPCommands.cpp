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
#ifdef ServerMainVers_2900
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
            },
            "includeFittingTables": {
                "type": "boolean",
                "description": "Also return the elbow, branch and transition preference tables. Defaults to false."
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
            },
            "referenceSet": {
                "$ref": "#/MEPReferenceSet"
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
        "required": ["tables", "referenceSet"]
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
    const auto& rows = tableOS.AddList<GS::ObjectState> ("rows");
    for (uint32_t i = 0; i < table.GetSize (); ++i) {
        auto referenceId = table.GetReferenceId (i);
        auto diameter = table.GetDiameter (i);
        auto radius = table.GetRadius (i);
        if (referenceId.IsErr () || diameter.IsErr () || radius.IsErr ()) {
            continue;
        }
        GS::ObjectState rowOS;
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
        GS::ObjectState rowOS;
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
    const auto& rows = tableOS.AddList<GS::ObjectState> ("rows");
    for (uint32_t i = 0; i < table.GetSize (); ++i) {
        auto beginReferenceId = table.GetBeginReferenceId (i);
        auto endReferenceId = table.GetEndReferenceId (i);
        auto beginDiameter = table.GetBeginDiameter (i);
        auto endDiameter = table.GetEndDiameter (i);
        if (beginReferenceId.IsErr () || endReferenceId.IsErr () || beginDiameter.IsErr () || endDiameter.IsErr ()) {
            continue;
        }
        GS::ObjectState rowOS;
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
        rows (rowOS);
    }
    return tableOS;
}

static GS::ObjectState DumpAngleSet (const AngleSetBase& angleSet)
{
    GS::ObjectState angleSetOS;
    angleSetOS.Add ("name", angleSet.GetName ());
    const auto& angles = angleSetOS.AddList<double> ("angles");
    for (uint32_t i = 0; i < angleSet.GetSize (); ++i) {
        auto angle = angleSet.GetAngle (i);
        if (angle.IsOk ()) {
            angles (*angle);
        }
    }
    return angleSetOS;
}
#endif

static GS::ObjectState DumpReferenceSet (const ReferenceSetBase& referenceSet)
{
    GS::ObjectState referenceSetOS;
    referenceSetOS.Add ("name", referenceSet.GetName ());
    const auto& referenceIds = referenceSetOS.AddList<Int32> ("referenceIds");
    for (uint32_t i = 0; i < referenceSet.GetSize (); ++i) {
        auto referenceId = referenceSet.GetReferenceId (i);
        if (referenceId.IsOk ()) {
            referenceIds (static_cast<Int32> (*referenceId));
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
    if (!domain.has_value () || *domain == Domain::CableCarrier) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid MEP domain: " + domainStr);
    }
    bool includeFittingTables = false;
    parameters.Get ("includeFittingTables", includeFittingTables);

    GS::ObjectState response;
    const auto& tables = response.AddList<GS::ObjectState> ("tables");

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
        response.Add ("referenceSet", referenceSet.IsOk () ? DumpReferenceSet (*referenceSet) : GS::ObjectState ("name", GS::UniString (), "referenceIds", GS::Array<Int32> ()));
#ifdef ServerMainVers_2900
        auto angleSet = GetPipeAngleSet ();
        if (angleSet.IsOk ()) {
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

        auto referenceSet = GetDuctReferenceSet ();
        response.Add ("referenceSet", referenceSet.IsOk () ? DumpReferenceSet (*referenceSet) : GS::ObjectState ("name", GS::UniString (), "referenceIds", GS::Array<Int32> ()));
#ifdef ServerMainVers_2900
        auto angleSet = GetDuctAngleSet ();
        if (angleSet.IsOk ()) {
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

#ifdef ServerMainVers_2900

static GS::ObjectState CreateMEPIdObjectState (const char* idFieldName, const ACAPI::MEP::UniqueID& id)
{
    return CreateIdObjectState (idFieldName, MEPUniqueIDToAPIGuid (id));
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
