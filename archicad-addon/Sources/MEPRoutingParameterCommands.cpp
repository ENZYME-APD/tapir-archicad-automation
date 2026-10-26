#include "MEPRoutingParameterCommands.hpp"

#ifdef ServerMainVers_2800
#include "ACAPI/Result.hpp"
#include "ACAPI/MEPAdapter.hpp"
#include "ACAPI/MEPTypes.hpp"
#include "ACAPI/MEPEnums.hpp"
#include "ACAPI/MEPRoutingElement.hpp"
#include "ACAPI/MEPRoutingSegment.hpp"
#include "ACAPI/MEPRoutingNode.hpp"
#include "ACAPI/MEPRoutingSegmentDefault.hpp"
#include "ACAPI/MEPRoutingNodeDefault.hpp"
#include "ACAPI/MEPTransitionDefault.hpp"
#ifdef ServerMainVers_2900
#include "ACAPI/MEPElbowDefault.hpp"
#else
#include "ACAPI/MEPRigidSegmentDefault.hpp"
#endif

#include <optional>
#include <utility>

using namespace ACAPI::MEP;
#endif

#ifdef ServerMainVers_2800

// The helpers below are the ones of GetMEPToolDefaults (#1461), copied with their own names so this
// file does not depend on that pull request.

// RoutingParameterValueOf gives the value when there is one: some getters return the value, others an
// optional or a Result.
template <typename T>
static std::optional<T> RoutingParameterValueOf (const T& value)
{
    return value;
}

template <typename T>
static std::optional<T> RoutingParameterValueOf (const std::optional<T>& value)
{
    return value;
}

template <typename T>
static std::optional<T> RoutingParameterValueOf (ACAPI::Result<T> value)
{
    if (value.IsErr ()) {
        return std::nullopt;
    }
    return value.Unwrap ();
}

template <typename ValueT>
static void AddRoutingParameterNumber (GS::ObjectState& os, const char* fieldName, ValueT&& value)
{
    const auto number = RoutingParameterValueOf (std::forward<ValueT> (value));
    if (number.has_value ()) {
        os.Add (fieldName, static_cast<double> (number.value ()));
    }
}

template <typename ValueT>
static void AddRoutingParameterTableId (GS::ObjectState& os, const char* fieldName, ValueT&& value)
{
    const auto tableId = RoutingParameterValueOf (std::forward<ValueT> (value));
    if (tableId.has_value ()) {
        os.Add (fieldName, tableId.value ().GetGuid ().ToUniString ());
    }
}

// The MEP enums go over JSON as the names of their values.
template <typename EnumT>
struct RoutingParameterEnumName {
    EnumT       value;
    const char* name;
};

// Adds the name of the value; a value without a name is left out of the response.
template <typename EnumT, size_t N, typename ValueT>
static void AddRoutingParameterEnumName (GS::ObjectState& os, const char* fieldName, const RoutingParameterEnumName<EnumT> (&names)[N], ValueT&& value)
{
    const auto enumValue = RoutingParameterValueOf (std::forward<ValueT> (value));
    if (!enumValue.has_value ()) {
        return;
    }
    for (const RoutingParameterEnumName<EnumT>& entry : names) {
        if (entry.value == enumValue.value ()) {
            os.Add (fieldName, GS::UniString (entry.name));
            return;
        }
    }
}

template <typename EnumT, size_t N>
static std::optional<EnumT> RoutingParameterEnumFromName (const RoutingParameterEnumName<EnumT> (&names)[N], const GS::UniString& name)
{
    for (const RoutingParameterEnumName<EnumT>& entry : names) {
        if (name == entry.name) {
            return entry.value;
        }
    }
    return std::nullopt;
}

static const RoutingParameterEnumName<ConnectorShape> RoutingParameterShapeNames[] = {
    { ConnectorShape::Rectangular, "Rectangular" },
    { ConnectorShape::Circular,    "Circular" },
    { ConnectorShape::Oval,        "Oval" },
    { ConnectorShape::UShape,      "UShape" }
};

static const RoutingParameterEnumName<ConnectionStyle> RoutingParameterConnectionStyleNames[] = {
    { ConnectionStyle::SimpleBody, "SimpleBody" },
    { ConnectionStyle::Flanged,    "Flanged" },
    { ConnectionStyle::Welded,     "Welded" }
};

static const RoutingParameterEnumName<PreferredTransitionPlacement> RoutingParameterTransitionPlacementNames[] = {
    { PreferredTransitionPlacement::SmallToLarge, "SmallToLarge" },
    { PreferredTransitionPlacement::LargeToSmall, "LargeToSmall" }
};

static ACAPI::MEP::UniqueID RoutingParameterTableIdFromString (const GS::UniString& tableIdStr)
{
    return Adapter::UniqueID (APIGuidFromString (tableIdStr.ToCStr ()));
}

static GS::ObjectState CreateRoutingSegmentParametersObjectState (const RoutingSegmentDefault& segmentDefault)
{
    GS::ObjectState segmentOs;
    AddRoutingParameterEnumName (segmentOs, "crossSectionShape", RoutingParameterShapeNames, segmentDefault.GetCrossSectionShape ());
    AddRoutingParameterNumber (segmentOs, "crossSectionWidth", segmentDefault.GetCrossSectionWidth ());
    AddRoutingParameterNumber (segmentOs, "crossSectionHeight", segmentDefault.GetCrossSectionHeight ());
    const auto referenceId = RoutingParameterValueOf (segmentDefault.GetCrossSectionReferenceId ());
    if (referenceId.has_value ()) {
        segmentOs.Add ("crossSectionReferenceId", static_cast<Int32> (referenceId.value ()));
    }
    AddRoutingParameterTableId (segmentOs, "preferenceTableId", segmentDefault.GetPreferenceTableId ());
    const auto anchor = RoutingParameterValueOf (segmentDefault.GetAnchor ());
    if (anchor.has_value ()) {
        segmentOs.Add ("anchor", AnchorIdToString (anchor.value ()));
    }
    AddRoutingParameterNumber (segmentOs, "referenceLineOffsetX", segmentDefault.GetReferenceLineOffsetX ());
    AddRoutingParameterNumber (segmentOs, "referenceLineOffsetY", segmentDefault.GetReferenceLineOffsetY ());
    AddRoutingParameterNumber (segmentOs, "wallThickness", segmentDefault.GetWallThickness ());
    AddRoutingParameterNumber (segmentOs, "flangeSize", segmentDefault.GetFlangeSize ());
    AddRoutingParameterEnumName (segmentOs, "connectionStyle", RoutingParameterConnectionStyleNames, segmentDefault.GetConnectionStyle ());
    return segmentOs;
}

static GS::ObjectState CreateTransitionParametersObjectState (const TransitionDefault& transitionDefault)
{
    GS::ObjectState transitionOs;
    AddRoutingParameterNumber (transitionOs, "angle", transitionDefault.GetAngle ());
    transitionOs.Add ("offsetY", transitionDefault.GetOffsetY ());
    transitionOs.Add ("offsetZ", transitionDefault.GetOffsetZ ());
#ifdef ServerMainVers_2900
    AddRoutingParameterTableId (transitionOs, "preferenceTableId", transitionDefault.GetPreferenceTable ());
#endif
    return transitionOs;
}

#ifdef ServerMainVers_2900
static GS::ObjectState CreateElbowParametersObjectState (const ElbowDefault& elbowDefault)
{
    GS::ObjectState elbowOs;
    AddRoutingParameterNumber (elbowOs, "factorRadius", elbowDefault.GetFactorRadius ());
    AddRoutingParameterTableId (elbowOs, "preferenceTableId", elbowDefault.GetPreferenceTable ());
    return elbowOs;
}
#endif

static GS::ObjectState CreateRoutingNodeParametersObjectState (const RoutingNodeDefault& nodeDefault)
{
    GS::ObjectState nodeOs;
    AddRoutingParameterEnumName (nodeOs, "preferredTransitionPlacement", RoutingParameterTransitionPlacementNames, nodeDefault.GetPreferredTransitionPlacement ());
#ifdef ServerMainVers_2900
    // Archicad 28 calls the elbow default a bend default (RoutingNodeDefault::GetBendDefault); it is
    // not returned there.
    nodeOs.Add ("elbow", CreateElbowParametersObjectState (nodeDefault.GetElbowDefault ()));
#endif
    nodeOs.Add ("transition", CreateTransitionParametersObjectState (nodeDefault.GetTransitionDefault ()));
    return nodeOs;
}

// Changes the given keys of a routing segment default; the same setters as CreateMEPRoutingElements.
static void ApplyRoutingSegmentParameters (RoutingSegmentDefault& segmentDefault, const GS::ObjectState& parameters)
{
    double width = 0.0;
    double height = 0.0;
    GS::UniString shapeStr;
    Int32 referenceId = -1;
    GS::UniString preferenceTableIdStr;
    const bool hasWidth = parameters.Get ("crossSectionWidth", width);
    const bool hasHeight = parameters.Get ("crossSectionHeight", height);
    const bool hasShape = parameters.Get ("crossSectionShape", shapeStr);
    const bool hasReferenceId = parameters.Get ("crossSectionReferenceId", referenceId);
    const bool hasPreferenceTableId = parameters.Get ("preferenceTableId", preferenceTableIdStr);
    if (!hasWidth && !hasHeight && !hasShape && !hasReferenceId && !hasPreferenceTableId) {
        return;
    }
    segmentDefault.Modify ([&](RoutingSegmentDefault::Modifier& modifier) {
        // The preference table is set before the reference id, because the reference id is resolved
        // against the table the segment uses.
        if (hasPreferenceTableId) {
            modifier.SetPreferenceTableId (RoutingParameterTableIdFromString (preferenceTableIdStr));
        }
        if (hasShape) {
            const std::optional<ConnectorShape> shape = RoutingParameterEnumFromName (RoutingParameterShapeNames, shapeStr);
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

#ifdef ServerMainVers_2900
// Changes the given keys of an elbow default. Returns false when a setter failed.
static bool ApplyElbowParameters (ElbowDefault& elbowDefault, const GS::ObjectState& parameters)
{
    double factorRadius = 0.0;
    GS::UniString preferenceTableIdStr;
    const bool hasFactorRadius = parameters.Get ("factorRadius", factorRadius);
    const bool hasPreferenceTableId = parameters.Get ("preferenceTableId", preferenceTableIdStr);
    if (!hasFactorRadius && !hasPreferenceTableId) {
        return true;
    }
    bool settersSucceeded = true;
    elbowDefault.Modify ([&](ElbowDefault::Modifier& modifier) {
        if (hasPreferenceTableId) {
            settersSucceeded &= modifier.SetPreferenceTable (RoutingParameterTableIdFromString (preferenceTableIdStr)).IsOk ();
        }
        if (hasFactorRadius) {
            settersSucceeded &= modifier.SetFactorRadius (factorRadius).IsOk ();
        }
    });
    return settersSucceeded;
}

// Changes the given keys of a transition default. Returns false when a setter failed.
static bool ApplyTransitionParameters (TransitionDefault& transitionDefault, const GS::ObjectState& parameters)
{
    GS::UniString preferenceTableIdStr;
    if (!parameters.Get ("preferenceTableId", preferenceTableIdStr)) {
        return true;
    }
    bool settersSucceeded = true;
    transitionDefault.Modify ([&](TransitionDefault::Modifier& modifier) {
        settersSucceeded &= modifier.SetPreferenceTable (RoutingParameterTableIdFromString (preferenceTableIdStr)).IsOk ();
    });
    return settersSucceeded;
}
#endif

static GS::ObjectState SetRoutingParametersOfRoutingElement (const ACAPI::MEP::UniqueID& elementId, const GS::ObjectState* nodeParameters, const GS::ObjectState* segmentParameters)
{
    ACAPI::Result<RoutingElement> routingElement = RoutingElement::Get (elementId);
    if (routingElement.IsErr ()) {
        return CreateFailedExecutionResult (APIERR_BADID, "routingNodeParameters and routingSegmentParameters need an existing MEP routing element.");
    }
#ifndef ServerMainVers_2900
    if (nodeParameters != nullptr) {
        return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "routingNodeParameters can be set from Archicad 29.");
    }
#endif

    bool settersSucceeded = true;
    RoutingSegmentDefault segmentDefault = routingElement->GetRoutingSegmentDefaultParameters ();
    if (segmentParameters != nullptr) {
        ApplyRoutingSegmentParameters (segmentDefault, *segmentParameters);
    }
    RoutingNodeDefault nodeDefault = routingElement->GetRoutingNodeDefaultParameters ();
#ifdef ServerMainVers_2900
    if (nodeParameters != nullptr) {
        const GS::ObjectState* elbowParameters = nodeParameters->Get ("elbow");
        const GS::ObjectState* transitionParameters = nodeParameters->Get ("transition");
        ElbowDefault elbowDefault = nodeDefault.GetElbowDefault ();
        TransitionDefault transitionDefault = nodeDefault.GetTransitionDefault ();
        if (elbowParameters != nullptr) {
            settersSucceeded &= ApplyElbowParameters (elbowDefault, *elbowParameters);
        }
        if (transitionParameters != nullptr) {
            settersSucceeded &= ApplyTransitionParameters (transitionDefault, *transitionParameters);
        }
        if (!settersSucceeded) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "A value of routingNodeParameters could not be set; nothing was changed.");
        }
        nodeDefault.Modify ([&](RoutingNodeDefault::Modifier& modifier) {
            if (elbowParameters != nullptr) {
                modifier.SetElbowDefault (elbowDefault);
            }
            if (transitionParameters != nullptr) {
                modifier.SetTransitionDefault (transitionDefault);
            }
        });
    }
#endif

    ACAPI::Result<void> result = routingElement->Modify ([&](RoutingElement::Modifier& modifier) {
        if (segmentParameters != nullptr) {
            settersSucceeded &= modifier.SetRoutingSegmentDefaultParameters (segmentDefault).IsOk ();
        }
        if (nodeParameters != nullptr) {
            settersSucceeded &= modifier.SetRoutingNodeDefaultParameters (nodeDefault).IsOk ();
        }
    }, "Set Routing Parameters of Routing Element");
    if (result.IsErr ()) {
        return CreateFailedExecutionResult (result.UnwrapErr ().kind, "Failed to modify the routing element.");
    }
    if (!settersSucceeded) {
        return CreateFailedExecutionResult (APIERR_GENERAL, "Some routing parameters of the routing element could not be set.");
    }
    return CreateSuccessfulExecutionResult ();
}

#ifdef ServerMainVers_2900
static GS::ObjectState SetRoutingParametersOfRoutingNode (const ACAPI::MEP::UniqueID& elementId, const GS::ObjectState* elbowParameters, const GS::ObjectState* transitionParameters)
{
    ACAPI::Result<RoutingNode> node = RoutingNode::Get (elementId);
    if (node.IsErr ()) {
        return CreateFailedExecutionResult (APIERR_BADID, "elbowParameters and transitionParameters need an existing MEP routing node.");
    }

    bool settersSucceeded = true;
    ElbowDefault elbowDefault = node->GetElbowDefaultParameters ();
    TransitionDefault transitionDefault = node->GetTransitionDefaultParameters ();
    if (elbowParameters != nullptr) {
        settersSucceeded &= ApplyElbowParameters (elbowDefault, *elbowParameters);
    }
    if (transitionParameters != nullptr) {
        settersSucceeded &= ApplyTransitionParameters (transitionDefault, *transitionParameters);
    }
    if (!settersSucceeded) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "A value of elbowParameters or transitionParameters could not be set; nothing was changed.");
    }

    ACAPI::Result<void> result = node->Modify ([&](RoutingNode::Modifier& modifier) {
        if (elbowParameters != nullptr) {
            settersSucceeded &= modifier.SetElbowDefaultParameters (elbowDefault).IsOk ();
        }
        if (transitionParameters != nullptr) {
            settersSucceeded &= modifier.SetTransitionDefaultParameters (transitionDefault).IsOk ();
        }
    }, "Set Routing Parameters of Routing Node");
    if (result.IsErr ()) {
        return CreateFailedExecutionResult (result.UnwrapErr ().kind, "Failed to modify the routing node.");
    }
    if (!settersSucceeded) {
        return CreateFailedExecutionResult (APIERR_GENERAL, "Some routing parameters of the routing node could not be set.");
    }
    return CreateSuccessfulExecutionResult ();
}
#endif

static GS::ObjectState SetRoutingParameters (const GS::ObjectState& elementData)
{
    const GS::ObjectState* elementId = elementData.Get ("elementId");
    if (elementId == nullptr) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Missing 'elementId' parameter.");
    }
    const GS::ObjectState* nodeParameters = elementData.Get ("routingNodeParameters");
    const GS::ObjectState* segmentParameters = elementData.Get ("routingSegmentParameters");
    const GS::ObjectState* elbowParameters = elementData.Get ("elbowParameters");
    const GS::ObjectState* transitionParameters = elementData.Get ("transitionParameters");

    const bool forRoutingElement = nodeParameters != nullptr || segmentParameters != nullptr;
    const bool forRoutingNode = elbowParameters != nullptr || transitionParameters != nullptr;
    if (forRoutingElement && forRoutingNode) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "routingNodeParameters and routingSegmentParameters are for routing elements, elbowParameters and transitionParameters for routing nodes; an item cannot have both.");
    }
    if (!forRoutingElement && !forRoutingNode) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "No routing parameters are given.");
    }

    const ACAPI::MEP::UniqueID id = Adapter::UniqueID (GetGuidFromObjectState (*elementId));
    if (forRoutingElement) {
        return SetRoutingParametersOfRoutingElement (id, nodeParameters, segmentParameters);
    }
#ifdef ServerMainVers_2900
    return SetRoutingParametersOfRoutingNode (id, elbowParameters, transitionParameters);
#else
    return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "elbowParameters and transitionParameters can be set from Archicad 29.");
#endif
}

#endif

// The schema of the routing parameters, shared by the response of GetMEPRoutingParameters.
static const char* RoutingSegmentParametersReadSchema = R"({
    "type": "object",
    "description": "The routing segment default parameters of the routing element (RoutingElement::GetRoutingSegmentDefaultParameters).",
    "properties": {
        "crossSectionShape": { "$ref": "#/MEPCrossSectionShape" },
        "crossSectionWidth": { "type": "number" },
        "crossSectionHeight": { "type": "number" },
        "crossSectionReferenceId": { "type": "integer" },
        "preferenceTableId": { "$ref": "#/Guid" },
        "anchor": { "type": "string" },
        "referenceLineOffsetX": { "type": "number" },
        "referenceLineOffsetY": { "type": "number" },
        "wallThickness": { "type": "number" },
        "flangeSize": { "type": "number" },
        "connectionStyle": { "type": "string", "enum": [ "SimpleBody", "Flanged", "Welded" ] }
    },
    "additionalProperties": false,
    "required": []
})";

static const char* ElbowParametersReadSchema = R"({
    "type": "object",
    "description": "Elbow parameters. Archicad 29 or newer.",
    "properties": {
        "factorRadius": { "type": "number" },
        "preferenceTableId": { "$ref": "#/Guid" }
    },
    "additionalProperties": false,
    "required": []
})";

static const char* TransitionParametersReadSchema = R"({
    "type": "object",
    "description": "Transition parameters. preferenceTableId is given from Archicad 29.",
    "properties": {
        "angle": { "type": "number" },
        "offsetY": { "type": "number" },
        "offsetZ": { "type": "number" },
        "preferenceTableId": { "$ref": "#/Guid" }
    },
    "additionalProperties": false,
    "required": []
})";

GetMEPRoutingParametersCommand::GetMEPRoutingParametersCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetMEPRoutingParametersCommand::GetName () const
{
    return "GetMEPRoutingParameters";
}

GS::Optional<GS::UniString> GetMEPRoutingParametersCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements",
                "description": "MEP routing elements, routing nodes or routing segments."
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> GetMEPRoutingParametersCommand::GetRawResponseSchema () const
{
    return GS::UniString (R"({
        "type": "object",
        "properties": {
            "routingParameters": {
                "type": "array",
                "description": "The routing parameters of the given elements, in their order.",
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
                                    "type": "string",
                                    "enum": [
                                        "RoutingElement",
                                        "RoutingNode",
                                        "RoutingSegment"
                                    ]
                                },
                                "routingNodeParameters": {
                                    "type": "object",
                                    "description": "The routing node default parameters of a routing element (RoutingElement::GetRoutingNodeDefaultParameters). The elbow is given from Archicad 29.",
                                    "properties": {
                                        "preferredTransitionPlacement": {
                                            "type": "string",
                                            "enum": [
                                                "SmallToLarge",
                                                "LargeToSmall"
                                            ]
                                        },
                                        "elbow": )") + ElbowParametersReadSchema + R"(,
                                        "transition": )" + TransitionParametersReadSchema + R"(
                                    },
                                    "additionalProperties": false,
                                    "required": []
                                },
                                "routingSegmentParameters": )" + RoutingSegmentParametersReadSchema + R"(,
                                "elbowParameters": )" + ElbowParametersReadSchema + R"(,
                                "transitionParameters": )" + TransitionParametersReadSchema + R"(,
                                "rigidSegmentParameters": {
                                    "type": "object",
                                    "description": "The rigid segment default parameters of a routing segment (RoutingSegment::GetRigidSegmentDefaultParameters). Archicad 28 only.",
                                    "properties": {
                                        "width": { "type": "number" },
                                        "height": { "type": "number" },
                                        "shape": { "$ref": "#/MEPCrossSectionShape" }
                                    },
                                    "additionalProperties": false,
                                    "required": []
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "elementId",
                                "type"
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
            "routingParameters"
        ]
    })";
}

GS::ObjectState GetMEPRoutingParametersCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::Array<GS::ObjectState> elements;
    if (!parameters.Get ("elements", elements)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'elements' parameter.");
    }

    GS::ObjectState response;
    const auto& routingParameters = response.AddList<GS::ObjectState> ("routingParameters");

    for (const GS::ObjectState& element : elements) {
        const API_Guid elemGuid = GetGuidFromElementsArrayItem (element);
        const ACAPI::MEP::UniqueID id = Adapter::UniqueID (elemGuid);

        GS::ObjectState parametersOs;
        parametersOs.Add ("elementId", CreateGuidObjectState (elemGuid));

        ACAPI::Result<RoutingElement> routingElement = RoutingElement::Get (id);
        if (routingElement.IsOk ()) {
            parametersOs.Add ("type", "RoutingElement");
            parametersOs.Add ("routingNodeParameters", CreateRoutingNodeParametersObjectState (routingElement->GetRoutingNodeDefaultParameters ()));
            parametersOs.Add ("routingSegmentParameters", CreateRoutingSegmentParametersObjectState (routingElement->GetRoutingSegmentDefaultParameters ()));
            routingParameters (parametersOs);
            continue;
        }

        ACAPI::Result<RoutingNode> node = RoutingNode::Get (id);
        if (node.IsOk ()) {
            parametersOs.Add ("type", "RoutingNode");
#ifdef ServerMainVers_2900
            // Archicad 28 calls the elbow parameters bend parameters (RoutingNode::GetBendDefaultParameters);
            // they are not returned there.
            parametersOs.Add ("elbowParameters", CreateElbowParametersObjectState (node->GetElbowDefaultParameters ()));
#endif
            parametersOs.Add ("transitionParameters", CreateTransitionParametersObjectState (node->GetTransitionDefaultParameters ()));
            routingParameters (parametersOs);
            continue;
        }

        ACAPI::Result<RoutingSegment> segment = RoutingSegment::Get (id);
        if (segment.IsOk ()) {
            parametersOs.Add ("type", "RoutingSegment");
#ifndef ServerMainVers_2900
            const RigidSegmentDefault rigidSegmentDefault = segment->GetRigidSegmentDefaultParameters ();
            GS::ObjectState rigidSegmentOs;
            rigidSegmentOs.Add ("width", rigidSegmentDefault.GetWidth ());
            rigidSegmentOs.Add ("height", rigidSegmentDefault.GetHeight ());
            AddRoutingParameterEnumName (rigidSegmentOs, "shape", RoutingParameterShapeNames, rigidSegmentDefault.GetShape ());
            parametersOs.Add ("rigidSegmentParameters", rigidSegmentOs);
#endif
            routingParameters (parametersOs);
            continue;
        }

        routingParameters (CreateErrorResponse (APIERR_BADID, "The element is not an MEP routing element, routing node or routing segment, or it does not exist."));
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

SetMEPRoutingParametersCommand::SetMEPRoutingParametersCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetMEPRoutingParametersCommand::GetName () const
{
    return "SetMEPRoutingParameters";
}

GS::Optional<GS::UniString> SetMEPRoutingParametersCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "type": "array",
                "description": "The routing parameters to change. Only the given keys are changed. An item gives either routingNodeParameters and/or routingSegmentParameters for a routing element, or elbowParameters and/or transitionParameters for a routing node.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "routingNodeParameters": {
                            "type": "object",
                            "description": "The routing node default parameters of a routing element (RoutingElement::Modifier::SetRoutingNodeDefaultParameters). Archicad 29 or newer.",
                            "properties": {
                                "elbow": {
                                    "type": "object",
                                    "properties": {
                                        "factorRadius": { "type": "number" },
                                        "preferenceTableId": {
                                            "$ref": "#/Guid",
                                            "description": "An elbow preference table, as returned by GetMEPPreferenceTables."
                                        }
                                    },
                                    "additionalProperties": false,
                                    "required": []
                                },
                                "transition": {
                                    "type": "object",
                                    "properties": {
                                        "preferenceTableId": {
                                            "$ref": "#/Guid",
                                            "description": "A transition preference table, as returned by GetMEPPreferenceTables."
                                        }
                                    },
                                    "additionalProperties": false,
                                    "required": []
                                }
                            },
                            "additionalProperties": false,
                            "required": []
                        },
                        "routingSegmentParameters": {
                            "type": "object",
                            "description": "The routing segment default parameters of a routing element (RoutingElement::Modifier::SetRoutingSegmentDefaultParameters).",
                            "properties": {
                                "crossSectionShape": { "$ref": "#/MEPCrossSectionShape" },
                                "crossSectionWidth": { "type": "number" },
                                "crossSectionHeight": { "type": "number" },
                                "crossSectionReferenceId": {
                                    "type": "integer",
                                    "description": "Cross section reference id of the segment preference table. Resolved against preferenceTableId if given, otherwise against the table already set."
                                },
                                "preferenceTableId": {
                                    "$ref": "#/Guid",
                                    "description": "A segment preference table, as returned by GetMEPPreferenceTables."
                                }
                            },
                            "additionalProperties": false,
                            "required": []
                        },
                        "elbowParameters": {
                            "type": "object",
                            "description": "The elbow parameters of a routing node (RoutingNode::Modifier::SetElbowDefaultParameters). Archicad 29 or newer.",
                            "properties": {
                                "factorRadius": { "type": "number" },
                                "preferenceTableId": {
                                    "$ref": "#/Guid",
                                    "description": "An elbow preference table, as returned by GetMEPPreferenceTables."
                                }
                            },
                            "additionalProperties": false,
                            "required": []
                        },
                        "transitionParameters": {
                            "type": "object",
                            "description": "The transition parameters of a routing node (RoutingNode::Modifier::SetTransitionDefaultParameters). Archicad 29 or newer.",
                            "properties": {
                                "preferenceTableId": {
                                    "$ref": "#/Guid",
                                    "description": "A transition preference table, as returned by GetMEPPreferenceTables."
                                }
                            },
                            "additionalProperties": false,
                            "required": []
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
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> SetMEPRoutingParametersCommand::GetRawResponseSchema () const
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

GS::ObjectState SetMEPRoutingParametersCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::Array<GS::ObjectState> elements;
    if (!parameters.Get ("elements", elements)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'elements' parameter.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Set MEP Routing Parameters", [&]() -> GSErrCode {
        for (const GS::ObjectState& elementData : elements) {
            executionResults (SetRoutingParameters (elementData));
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}
