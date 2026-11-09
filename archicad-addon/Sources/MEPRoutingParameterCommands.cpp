#include "MEPRoutingParameterCommands.hpp"
#include "NotificationCommands.hpp"

#ifdef ServerMainVers_2800
#include "ACAPI/Result.hpp"
#include "ACAPI/MEPAdapter.hpp"
#include "ACAPI/MEPTypes.hpp"
#include "ACAPI/MEPEnums.hpp"
#include "ACAPI/MEPRoutingElement.hpp"
#include "ACAPI/MEPRoutingElementDefault.hpp"
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

// Reads the optional preferenceTableId of 'parameters'. Returns an error when it is given but is not a
// guid string (Archicad does not check the type of a $ref field, see GetOptionalGuidField).
static std::optional<GS::UniString> ReadRoutingParameterTableId (const GS::ObjectState& parameters, std::optional<ACAPI::MEP::UniqueID>& tableId)
{
    API_Guid guid = APINULLGuid;
    const OptionalGuidField field = GetOptionalGuidField (parameters, "preferenceTableId", guid);
    if (field == OptionalGuidField::Invalid) {
        return GS::UniString ("preferenceTableId must be a guid string.");
    }
    if (field == OptionalGuidField::Valid) {
        tableId = Adapter::UniqueID (guid);
    }
    return std::nullopt;
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

static GS::UniString RoutingParameterErrorText (const ACAPI::Error& error)
{
    return GS::UniString (error.text.c_str ());
}

// Calls a setter and keeps the first error it gives; setters without a result cannot fail.
static void CheckRoutingParameterSetter (const ACAPI::Result<void>& result, const char* what, std::optional<GS::UniString>& error)
{
    if (result.IsErr () && !error.has_value ()) {
        error = GS::UniString ("Failed to set ") + what + ": " + RoutingParameterErrorText (result.UnwrapErr ());
    }
}

// The keys of routingSegmentParameters, read once so that the default of a route and each of its
// segments get the same changes.
struct RoutingSegmentChanges {
    std::optional<ConnectorShape>       shape;
    std::optional<double>               width;
    std::optional<double>               height;
    std::optional<uint32_t>             referenceId;
    std::optional<ACAPI::MEP::UniqueID> preferenceTableId;

    bool IsEmpty () const
    {
        return !shape.has_value () && !width.has_value () && !height.has_value () && !referenceId.has_value () && !preferenceTableId.has_value ();
    }
};

// Returns an error when a value is not valid.
static std::optional<GS::UniString> ReadRoutingSegmentChanges (const GS::ObjectState& parameters, RoutingSegmentChanges& changes)
{
    double number = 0.0;
    if (parameters.Get ("crossSectionWidth", number)) {
        changes.width = number;
    }
    if (parameters.Get ("crossSectionHeight", number)) {
        changes.height = number;
    }
    GS::UniString shapeStr;
    if (parameters.Get ("crossSectionShape", shapeStr)) {
        changes.shape = RoutingParameterEnumFromName (RoutingParameterShapeNames, shapeStr);
        if (!changes.shape.has_value ()) {
            return GS::UniString ("Unknown crossSectionShape '") + shapeStr + "'.";
        }
    }
    Int32 referenceId = -1;
    if (parameters.Get ("crossSectionReferenceId", referenceId)) {
        if (referenceId < 0) {
            return GS::UniString ("crossSectionReferenceId cannot be negative.");
        }
        changes.referenceId = static_cast<uint32_t> (referenceId);
    }
    return ReadRoutingParameterTableId (parameters, changes.preferenceTableId);
}

// Applies the changes with the setters of a RoutingSegmentDefault::Modifier or a RoutingSegment::Modifier,
// which have the same names. The preference table is set before the reference id, because the reference
// id is resolved against the table the segment uses.
template <typename ModifierT>
static void ApplyRoutingSegmentChanges (ModifierT& modifier, const RoutingSegmentChanges& changes, std::optional<GS::UniString>& error)
{
    if (changes.preferenceTableId.has_value ()) {
        CheckRoutingParameterSetter (modifier.SetPreferenceTableId (changes.preferenceTableId.value ()), "the segment preference table", error);
    }
    if (changes.shape.has_value ()) {
        CheckRoutingParameterSetter (modifier.SetCrossSectionShape (changes.shape.value ()), "the cross section shape", error);
    }
    if (changes.width.has_value ()) {
        CheckRoutingParameterSetter (modifier.SetCrossSectionWidth (changes.width.value ()), "the cross section width", error);
    }
    if (changes.height.has_value ()) {
        CheckRoutingParameterSetter (modifier.SetCrossSectionHeight (changes.height.value ()), "the cross section height", error);
    }
    if (changes.referenceId.has_value ()) {
        CheckRoutingParameterSetter (modifier.SetCrossSectionReferenceId (changes.referenceId.value ()), "the cross section reference id", error);
    }
}

static std::optional<GS::UniString> ApplyRoutingSegmentChanges (RoutingSegmentDefault& segmentDefault, const RoutingSegmentChanges& changes)
{
    std::optional<GS::UniString> error;
    ACAPI::Result<void> result = segmentDefault.Modify ([&](RoutingSegmentDefault::Modifier& modifier) {
        ApplyRoutingSegmentChanges (modifier, changes, error);
    });
    CheckRoutingParameterSetter (result, "the routing segment default", error);
    return error;
}

#ifdef ServerMainVers_2900
// The keys of an elbow parameters object.
struct ElbowChanges {
    std::optional<double>               factorRadius;
    std::optional<ACAPI::MEP::UniqueID> preferenceTableId;
};

// Returns an error when a value is not valid.
static std::optional<GS::UniString> ReadElbowChanges (const GS::ObjectState& parameters, ElbowChanges& changes)
{
    double factorRadius = 0.0;
    if (parameters.Get ("factorRadius", factorRadius)) {
        changes.factorRadius = factorRadius;
    }
    return ReadRoutingParameterTableId (parameters, changes.preferenceTableId);
}

static std::optional<GS::UniString> ApplyElbowChanges (ElbowDefault& elbowDefault, const ElbowChanges& changes)
{
    std::optional<GS::UniString> error;
    ACAPI::Result<void> result = elbowDefault.Modify ([&](ElbowDefault::Modifier& modifier) {
        if (changes.preferenceTableId.has_value ()) {
            CheckRoutingParameterSetter (modifier.SetPreferenceTable (changes.preferenceTableId.value ()), "the elbow preference table", error);
        }
        if (changes.factorRadius.has_value ()) {
            CheckRoutingParameterSetter (modifier.SetFactorRadius (changes.factorRadius.value ()), "the elbow radius factor", error);
        }
    });
    CheckRoutingParameterSetter (result, "the elbow default", error);
    return error;
}

static std::optional<GS::UniString> ApplyTransitionTableChange (TransitionDefault& transitionDefault, const std::optional<ACAPI::MEP::UniqueID>& tableId)
{
    if (!tableId.has_value ()) {
        return std::nullopt;
    }
    std::optional<GS::UniString> error;
    ACAPI::Result<void> result = transitionDefault.Modify ([&](TransitionDefault::Modifier& modifier) {
        CheckRoutingParameterSetter (modifier.SetPreferenceTable (tableId.value ()), "the transition preference table", error);
    });
    CheckRoutingParameterSetter (result, "the transition default", error);
    return error;
}
#endif

// The preference tables a change asks for. Archicad 29 answers OK to the table setters also when it keeps
// the old table (fork issue #10), so they are read back after the change.
struct RoutingTableChanges {
    std::optional<ACAPI::MEP::UniqueID> segment;
    std::optional<ACAPI::MEP::UniqueID> elbow;
    std::optional<ACAPI::MEP::UniqueID> transition;

    bool IsEmpty () const
    {
        return !segment.has_value () && !elbow.has_value () && !transition.has_value ();
    }
};

template <typename IdT>
static bool IsPreferenceTable (const IdT& tableId, const ACAPI::MEP::UniqueID& wanted)
{
    const auto value = RoutingParameterValueOf (tableId);
    return value.has_value () && value.value ().GetGuid () == wanted.GetGuid ();
}

static bool SegmentUsesTable (const ACAPI::MEP::UniqueID& segmentId, const RoutingTableChanges& tables)
{
    if (!tables.segment.has_value ()) {
        return true;
    }
    ACAPI::Result<RoutingSegment> segment = RoutingSegment::Get (segmentId);
    return segment.IsOk () && IsPreferenceTable (segment->GetPreferenceTableId (), tables.segment.value ());
}

static bool NodeUsesTables (const ACAPI::MEP::UniqueID& nodeId, const RoutingTableChanges& tables)
{
#ifdef ServerMainVers_2900
    if (!tables.elbow.has_value () && !tables.transition.has_value ()) {
        return true;
    }
    ACAPI::Result<RoutingNode> node = RoutingNode::Get (nodeId);
    if (node.IsErr ()) {
        return false;
    }
    if (tables.elbow.has_value () && !IsPreferenceTable (node->GetElbowDefaultParameters ().GetPreferenceTable (), tables.elbow.value ())) {
        return false;
    }
    if (tables.transition.has_value () && !IsPreferenceTable (node->GetTransitionDefaultParameters ().GetPreferenceTable (), tables.transition.value ())) {
        return false;
    }
#else
    (void) nodeId;
    (void) tables;
#endif
    return true;
}

static bool RouteUsesTables (const RoutingElement& routingElement, const RoutingTableChanges& tables)
{
    for (const ACAPI::MEP::UniqueID& segmentId : routingElement.GetRoutingSegmentIds ()) {
        if (!SegmentUsesTable (segmentId, tables)) {
            return false;
        }
    }
    for (const ACAPI::MEP::UniqueID& nodeId : routingElement.GetRoutingNodeIds ()) {
        if (!NodeUsesTables (nodeId, tables)) {
            return false;
        }
    }
    return true;
}

// Sets the tables on each segment and node of the route itself, the way ModifyMEPRoutingElements sets
// its segments: the route's defaults only reach the segments and nodes that were not customized.
static void SetTablesOnSegmentsAndNodes (const RoutingElement& routingElement, const RoutingSegmentChanges& segmentChanges, const RoutingTableChanges& tables, GS::Array<GS::UniString>& notes)
{
    if (!segmentChanges.IsEmpty ()) {
        for (const ACAPI::MEP::UniqueID& segmentId : routingElement.GetRoutingSegmentIds ()) {
            ACAPI::Result<RoutingSegment> segment = RoutingSegment::Get (segmentId);
            if (segment.IsErr ()) {
                continue;
            }
            std::optional<GS::UniString> error;
            ACAPI::Result<void> result = segment->Modify ([&](RoutingSegment::Modifier& modifier) {
                ApplyRoutingSegmentChanges (modifier, segmentChanges, error);
            }, "Set Routing Parameters of Routing Segment");
            CheckRoutingParameterSetter (result, "the routing segment", error);
            if (error.has_value ()) {
                notes.Push ("segment: " + error.value ());
            }
        }
    }
#ifdef ServerMainVers_2900
    if (!tables.elbow.has_value () && !tables.transition.has_value ()) {
        return;
    }
    for (const ACAPI::MEP::UniqueID& nodeId : routingElement.GetRoutingNodeIds ()) {
        ACAPI::Result<RoutingNode> node = RoutingNode::Get (nodeId);
        if (node.IsErr ()) {
            continue;
        }
        ElbowDefault elbowDefault = node->GetElbowDefaultParameters ();
        TransitionDefault transitionDefault = node->GetTransitionDefaultParameters ();
        std::optional<GS::UniString> error;
        if (tables.elbow.has_value ()) {
            error = ApplyElbowChanges (elbowDefault, ElbowChanges { std::nullopt, tables.elbow });
        }
        if (!error.has_value ()) {
            error = ApplyTransitionTableChange (transitionDefault, tables.transition);
        }
        ACAPI::Result<void> result = node->Modify ([&](RoutingNode::Modifier& modifier) {
            if (tables.elbow.has_value ()) {
                CheckRoutingParameterSetter (modifier.SetElbowDefaultParameters (elbowDefault), "the elbow parameters of the node", error);
            }
            if (tables.transition.has_value ()) {
                CheckRoutingParameterSetter (modifier.SetTransitionDefaultParameters (transitionDefault), "the transition parameters of the node", error);
            }
        }, "Set Routing Parameters of Routing Node");
        CheckRoutingParameterSetter (result, "the routing node", error);
        if (error.has_value ()) {
            notes.Push ("node: " + error.value ());
        }
    }
#else
    (void) tables;
#endif
}

#ifdef ServerMainVers_2900
// Picks up the settings of a node, changes the tables in them and injects them back, as Archicad's pick
// up and inject parameters does.
static void InjectTablesIntoNode (const ACAPI::MEP::UniqueID& nodeId, const RoutingTableChanges& tables, GS::Array<GS::UniString>& notes)
{
    if (!tables.elbow.has_value () && !tables.transition.has_value ()) {
        return;
    }
    ACAPI::Result<RoutingNode> node = RoutingNode::Get (nodeId);
    if (node.IsErr ()) {
        return;
    }
    RoutingNodeDefault nodeDefault = node->PickUpDefault ();
    ElbowDefault elbowDefault = nodeDefault.GetElbowDefault ();
    TransitionDefault transitionDefault = nodeDefault.GetTransitionDefault ();
    std::optional<GS::UniString> error;
    if (tables.elbow.has_value ()) {
        error = ApplyElbowChanges (elbowDefault, ElbowChanges { std::nullopt, tables.elbow });
    }
    if (!error.has_value ()) {
        error = ApplyTransitionTableChange (transitionDefault, tables.transition);
    }
    CheckRoutingParameterSetter (nodeDefault.Modify ([&](RoutingNodeDefault::Modifier& modifier) {
        modifier.SetElbowDefault (elbowDefault);
        modifier.SetTransitionDefault (transitionDefault);
    }), "the picked up node settings", error);
    ACAPI::Result<void> result = node->Modify ([&](RoutingNode::Modifier& modifier) {
        CheckRoutingParameterSetter (modifier.Inject (nodeDefault), "the picked up node settings", error);
    }, "Inject Routing Parameters into Routing Node");
    CheckRoutingParameterSetter (result, "the routing node", error);
    if (error.has_value ()) {
        notes.Push ("node inject: " + error.value ());
    }
}
#endif

// Picks up the settings of each segment and node, changes the tables in them and injects them back.
static void InjectTablesIntoSegmentsAndNodes (const RoutingElement& routingElement, const RoutingSegmentChanges& segmentChanges, const RoutingTableChanges& tables, GS::Array<GS::UniString>& notes)
{
    if (!segmentChanges.IsEmpty ()) {
        for (const ACAPI::MEP::UniqueID& segmentId : routingElement.GetRoutingSegmentIds ()) {
            ACAPI::Result<RoutingSegment> segment = RoutingSegment::Get (segmentId);
            if (segment.IsErr ()) {
                continue;
            }
            RoutingSegmentDefault segmentDefault = segment->PickUpDefault ();
            std::optional<GS::UniString> error = ApplyRoutingSegmentChanges (segmentDefault, segmentChanges);
            ACAPI::Result<void> result = segment->Modify ([&](RoutingSegment::Modifier& modifier) {
                CheckRoutingParameterSetter (modifier.Inject (segmentDefault), "the picked up segment settings", error);
            }, "Inject Routing Parameters into Routing Segment");
            CheckRoutingParameterSetter (result, "the routing segment", error);
            if (error.has_value ()) {
                notes.Push ("segment inject: " + error.value ());
            }
        }
    }
#ifdef ServerMainVers_2900
    for (const ACAPI::MEP::UniqueID& nodeId : routingElement.GetRoutingNodeIds ()) {
        InjectTablesIntoNode (nodeId, tables, notes);
    }
#else
    (void) tables;
#endif
}

// Picks up the settings of the whole route, puts the changed defaults in them and injects them back.
static void InjectTablesIntoRoute (RoutingElement& routingElement, const RoutingSegmentDefault& segmentDefault, const RoutingNodeDefault& nodeDefault, GS::Array<GS::UniString>& notes)
{
    RoutingElementDefault routeDefault = routingElement.PickUpDefault ();
    std::optional<GS::UniString> error;
    CheckRoutingParameterSetter (routeDefault.Modify ([&](RoutingElementDefault::Modifier& modifier) {
        modifier.SetRoutingSegmentDefault (segmentDefault);
        modifier.SetRoutingNodeDefault (nodeDefault);
    }), "the picked up route settings", error);
    if (!error.has_value ()) {
        ACAPI::Result<void> result = routingElement.Modify ([&](RoutingElement::Modifier& modifier) {
            CheckRoutingParameterSetter (modifier.Inject (routeDefault), "the picked up route settings", error);
        }, "Inject Routing Parameters into Routing Element");
        CheckRoutingParameterSetter (result, "the routing element", error);
    }
    if (error.has_value ()) {
        notes.Push ("route inject: " + error.value ());
    }
}

static GS::UniString JoinRoutingParameterNotes (const GS::Array<GS::UniString>& notes)
{
    GS::UniString text;
    for (const GS::UniString& note : notes) {
        text += " " + note + ".";
    }
    return text;
}

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

    RoutingSegmentChanges segmentChanges;
    if (segmentParameters != nullptr) {
        std::optional<GS::UniString> invalid = ReadRoutingSegmentChanges (*segmentParameters, segmentChanges);
        if (invalid.has_value ()) {
            return CreateFailedExecutionResult (APIERR_BADPARS, invalid.value ());
        }
    }
    RoutingTableChanges tables;
    tables.segment = segmentChanges.preferenceTableId;

    RoutingSegmentDefault segmentDefault = routingElement->GetRoutingSegmentDefaultParameters ();
    if (!segmentChanges.IsEmpty ()) {
        std::optional<GS::UniString> error = ApplyRoutingSegmentChanges (segmentDefault, segmentChanges);
        if (error.has_value ()) {
            return CreateFailedExecutionResult (APIERR_BADPARS, error.value () + " Nothing was changed.");
        }
    }
    RoutingNodeDefault nodeDefault = routingElement->GetRoutingNodeDefaultParameters ();
#ifdef ServerMainVers_2900
    const GS::ObjectState* elbowParameters = nodeParameters != nullptr ? nodeParameters->Get ("elbow") : nullptr;
    const GS::ObjectState* transitionParameters = nodeParameters != nullptr ? nodeParameters->Get ("transition") : nullptr;
    if (nodeParameters != nullptr) {
        ElbowDefault elbowDefault = nodeDefault.GetElbowDefault ();
        TransitionDefault transitionDefault = nodeDefault.GetTransitionDefault ();
        std::optional<GS::UniString> error;
        if (elbowParameters != nullptr) {
            ElbowChanges elbowChanges;
            error = ReadElbowChanges (*elbowParameters, elbowChanges);
            if (!error.has_value ()) {
                tables.elbow = elbowChanges.preferenceTableId;
                error = ApplyElbowChanges (elbowDefault, elbowChanges);
            }
        }
        if (transitionParameters != nullptr && !error.has_value ()) {
            error = ReadRoutingParameterTableId (*transitionParameters, tables.transition);
            if (!error.has_value ()) {
                error = ApplyTransitionTableChange (transitionDefault, tables.transition);
            }
        }
        if (error.has_value ()) {
            return CreateFailedExecutionResult (APIERR_BADPARS, error.value () + " Nothing was changed.");
        }
        CheckRoutingParameterSetter (nodeDefault.Modify ([&](RoutingNodeDefault::Modifier& modifier) {
            if (elbowParameters != nullptr) {
                modifier.SetElbowDefault (elbowDefault);
            }
            if (transitionParameters != nullptr) {
                modifier.SetTransitionDefault (transitionDefault);
            }
        }), "the routing node default", error);
        if (error.has_value ()) {
            return CreateFailedExecutionResult (APIERR_BADPARS, error.value () + " Nothing was changed.");
        }
    }
#endif

    std::optional<GS::UniString> error;
    ACAPI::Result<void> result = routingElement->Modify ([&](RoutingElement::Modifier& modifier) {
        if (segmentParameters != nullptr) {
            CheckRoutingParameterSetter (modifier.SetRoutingSegmentDefaultParameters (segmentDefault), "the routing segment parameters of the route", error);
        }
        if (nodeParameters != nullptr) {
            CheckRoutingParameterSetter (modifier.SetRoutingNodeDefaultParameters (nodeDefault), "the routing node parameters of the route", error);
        }
    }, "Set Routing Parameters of Routing Element");
    if (result.IsErr ()) {
        return CreateFailedExecutionResult (result.UnwrapErr ().kind, "Failed to modify the routing element: " + RoutingParameterErrorText (result.UnwrapErr ()));
    }
    if (error.has_value ()) {
        return CreateFailedExecutionResult (APIERR_GENERAL, error.value ());
    }
    if (tables.IsEmpty ()) {
        return CreateSuccessfulExecutionResult ();
    }

    // Archicad reads the new tables back inside the command but keeps the old ones once it ends (fork
    // issue #10), so the read-back cannot tell which way works: the tables are also set on each segment
    // and node, and injected into them and into the route.
    // Only the table goes to each segment: the other keys change the route's default, as without a table.
    RoutingSegmentChanges segmentTableChange;
    segmentTableChange.preferenceTableId = tables.segment;
    GS::Array<GS::UniString> notes;
    SetTablesOnSegmentsAndNodes (*routingElement, segmentTableChange, tables, notes);
    InjectTablesIntoSegmentsAndNodes (*routingElement, segmentTableChange, tables, notes);
    InjectTablesIntoRoute (*routingElement, segmentDefault, nodeDefault, notes);
    if (RouteUsesTables (*routingElement, tables)) {
        return CreateSuccessfulExecutionResult ();
    }
    return CreateFailedExecutionResult (APIERR_GENERAL,
        "Archicad accepted the preference tables but the route's segments and nodes kept their old tables, also when set on each segment and node and when injected." + JoinRoutingParameterNotes (notes));
}

#ifdef ServerMainVers_2900
static GS::ObjectState SetRoutingParametersOfRoutingNode (const ACAPI::MEP::UniqueID& elementId, const GS::ObjectState* elbowParameters, const GS::ObjectState* transitionParameters)
{
    ACAPI::Result<RoutingNode> node = RoutingNode::Get (elementId);
    if (node.IsErr ()) {
        return CreateFailedExecutionResult (APIERR_BADID, "elbowParameters and transitionParameters need an existing MEP routing node.");
    }

    RoutingTableChanges tables;
    ElbowDefault elbowDefault = node->GetElbowDefaultParameters ();
    TransitionDefault transitionDefault = node->GetTransitionDefaultParameters ();
    std::optional<GS::UniString> error;
    if (elbowParameters != nullptr) {
        ElbowChanges elbowChanges;
        error = ReadElbowChanges (*elbowParameters, elbowChanges);
        if (!error.has_value ()) {
            tables.elbow = elbowChanges.preferenceTableId;
            error = ApplyElbowChanges (elbowDefault, elbowChanges);
        }
    }
    if (transitionParameters != nullptr && !error.has_value ()) {
        error = ReadRoutingParameterTableId (*transitionParameters, tables.transition);
        if (!error.has_value ()) {
            error = ApplyTransitionTableChange (transitionDefault, tables.transition);
        }
    }
    if (error.has_value ()) {
        return CreateFailedExecutionResult (APIERR_BADPARS, error.value () + " Nothing was changed.");
    }

    ACAPI::Result<void> result = node->Modify ([&](RoutingNode::Modifier& modifier) {
        if (elbowParameters != nullptr) {
            CheckRoutingParameterSetter (modifier.SetElbowDefaultParameters (elbowDefault), "the elbow parameters of the node", error);
        }
        if (transitionParameters != nullptr) {
            CheckRoutingParameterSetter (modifier.SetTransitionDefaultParameters (transitionDefault), "the transition parameters of the node", error);
        }
    }, "Set Routing Parameters of Routing Node");
    if (result.IsErr ()) {
        return CreateFailedExecutionResult (result.UnwrapErr ().kind, "Failed to modify the routing node: " + RoutingParameterErrorText (result.UnwrapErr ()));
    }
    if (error.has_value ()) {
        return CreateFailedExecutionResult (APIERR_GENERAL, error.value ());
    }
    if (tables.IsEmpty ()) {
        return CreateSuccessfulExecutionResult ();
    }
    // As for a route (fork issue #10), the tables are also injected.
    GS::Array<GS::UniString> notes;
    InjectTablesIntoNode (elementId, tables, notes);
    if (NodeUsesTables (elementId, tables)) {
        return CreateSuccessfulExecutionResult ();
    }
    return CreateFailedExecutionResult (APIERR_GENERAL, "Archicad accepted the preference tables but the node kept its old tables, also when injected." + JoinRoutingParameterNotes (notes));
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
    return R"({
        "type": "object",
        "properties": {
            "routingParameters": {
                "type": "array",
                "description": "The routing parameters of the given elements, in their order.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "$ref": "#/MEPRoutingParameters"
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
                    "$ref": "#/ElementMEPRoutingParameters"
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

    // The changed routes are reported to the notification clients as changed (#1825); a routing
    // node is reported only when Archicad has an element header for it.
    ElementChangeReporter changeReporter;
    changeReporter.AddElementIdsOf (elements);

    ACAPI_CallUndoableCommand ("Set MEP Routing Parameters", [&]() -> GSErrCode {
        for (const GS::ObjectState& elementData : elements) {
            const GS::ObjectState result = SetRoutingParameters (elementData);
            changeReporter.MarkChangedIfSucceeded (elementData, result);
            executionResults (result);
        }
        return NoError;
    });

    changeReporter.Report ();

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}
