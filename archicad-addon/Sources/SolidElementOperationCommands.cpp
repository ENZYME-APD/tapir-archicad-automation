#include "SolidElementOperationCommands.hpp"

#include "HashSet.hpp"
#include "MigrationHelper.hpp"

static API_SolidOperationID OperationFromString (const GS::UniString& str)
{
    if (str == "SubtractionUpwards")   return APISolid_SubstUp;
    if (str == "SubtractionDownwards") return APISolid_SubstDown;
    if (str == "Intersection")         return APISolid_Intersect;
    if (str == "Addition")             return APISolid_Add;
    return APISolid_Substract;
}

static GS::UniString OperationToString (API_SolidOperationID op)
{
    switch (op) {
        case APISolid_SubstUp:   return "SubtractionUpwards";
        case APISolid_SubstDown: return "SubtractionDownwards";
        case APISolid_Intersect: return "Intersection";
        case APISolid_Add:       return "Addition";
        default:                 return "Subtraction";
    }
}

static GSFlags FlagsFromObjectState (const GS::ObjectState& os)
{
    GSFlags flags = 0;
    bool inheritAttr = false, skipHoles = false;
    os.Get ("inheritOperatorAttributes", inheritAttr);
    os.Get ("skipPolygonHoles", skipHoles);
    if (inheritAttr) flags |= APISolidFlag_OperatorAttrib;
    if (skipHoles)   flags |= APISolidFlag_SkipPolygonHoles;
    return flags;
}

static GS::ObjectState FlagsToObjectState (GSFlags flags)
{
    GS::ObjectState os;
    os.Add ("inheritOperatorAttributes", (flags & APISolidFlag_OperatorAttrib) != 0);
    os.Add ("skipPolygonHoles",          (flags & APISolidFlag_SkipPolygonHoles) != 0);
    return os;
}

// ---------------------------------------------------------------------------
// CreateSolidElementLinks
// ---------------------------------------------------------------------------

CreateSolidElementLinksCommand::CreateSolidElementLinksCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String CreateSolidElementLinksCommand::GetName () const
{
    return "CreateSolidElementLinks";
}

GS::Optional<GS::UniString> CreateSolidElementLinksCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "solidLinks": {
                "type": "array",
                "description": "List of solid element operation links to create.",
                "items": {
                    "type": "object",
                    "properties": {
                        "targetId": {
                            "$ref": "#/ElementId",
                            "description": "The element to be cut or modified."
                        },
                        "operatorId": {
                            "$ref": "#/ElementId",
                            "description": "The element performing the operation."
                        },
                        "operation": {
                            "$ref": "#/SolidOperationType"
                        },
                        "linkFlags": {
                            "$ref": "#/SolidLinkFlags"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "targetId",
                        "operatorId",
                        "operation"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "solidLinks"
        ]
    })";
}

GS::Optional<GS::UniString> CreateSolidElementLinksCommand::GetRawResponseSchema () const
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

GS::ObjectState CreateSolidElementLinksCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> solidLinks;
    parameters.Get ("solidLinks", solidLinks);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("CreateSolidElementLinks", [&] () -> GSErrCode {
        for (const GS::ObjectState& link : solidLinks) {
            GS::ObjectState targetIdOs, operatorIdOs;
            if (!link.Get ("targetId", targetIdOs) || !link.Get ("operatorId", operatorIdOs)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "targetId or operatorId is missing."));
                continue;
            }

            const API_Guid targetGuid   = GetGuidFromObjectState (targetIdOs);
            const API_Guid operatorGuid = GetGuidFromObjectState (operatorIdOs);

            GS::UniString operationStr = "Subtraction";
            link.Get ("operation", operationStr);
            const API_SolidOperationID operation = OperationFromString (operationStr);

            GSFlags flags = 0;
            GS::ObjectState flagsOs;
            if (link.Get ("linkFlags", flagsOs)) {
                flags = FlagsFromObjectState (flagsOs);
            }

            const GSErrCode err = ACAPI_Element_SolidLink_Create (targetGuid, operatorGuid, operation, flags);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to create solid element link."));
            } else {
                executionResults (CreateSuccessfulExecutionResult ());
            }
        }
        return NoError;
    });

    return response;
}

// ---------------------------------------------------------------------------
// RemoveSolidElementLinks
// ---------------------------------------------------------------------------

RemoveSolidElementLinksCommand::RemoveSolidElementLinksCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String RemoveSolidElementLinksCommand::GetName () const
{
    return "RemoveSolidElementLinks";
}

GS::Optional<GS::UniString> RemoveSolidElementLinksCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "solidLinks": {
                "type": "array",
                "description": "List of solid element operation links to remove.",
                "items": {
                    "type": "object",
                    "properties": {
                        "targetId": {
                            "$ref": "#/ElementId"
                        },
                        "operatorId": {
                            "$ref": "#/ElementId"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "targetId",
                        "operatorId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "solidLinks"
        ]
    })";
}

GS::Optional<GS::UniString> RemoveSolidElementLinksCommand::GetRawResponseSchema () const
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

GS::ObjectState RemoveSolidElementLinksCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> solidLinks;
    parameters.Get ("solidLinks", solidLinks);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("RemoveSolidElementLinks", [&] () -> GSErrCode {
        for (const GS::ObjectState& link : solidLinks) {
            GS::ObjectState targetIdOs, operatorIdOs;
            if (!link.Get ("targetId", targetIdOs) || !link.Get ("operatorId", operatorIdOs)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "targetId or operatorId is missing."));
                continue;
            }

            const API_Guid targetGuid   = GetGuidFromObjectState (targetIdOs);
            const API_Guid operatorGuid = GetGuidFromObjectState (operatorIdOs);

            const GSErrCode err = ACAPI_Element_SolidLink_Remove (targetGuid, operatorGuid);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to remove solid element link."));
            } else {
                executionResults (CreateSuccessfulExecutionResult ());
            }
        }
        return NoError;
    });

    return response;
}

// ---------------------------------------------------------------------------
// GetSolidElementLinks
// ---------------------------------------------------------------------------

GetSolidElementLinksCommand::GetSolidElementLinksCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String GetSolidElementLinksCommand::GetName () const
{
    return "GetSolidElementLinks";
}

GS::Optional<GS::UniString> GetSolidElementLinksCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements",
                "description": "Elements to query. Returns all solid links where each element is a target or an operator."
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> GetSolidElementLinksCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "solidLinks": {
                "type": "array",
                "description": "For each input element, the solid links where it acts as target and where it acts as operator.",
                "items": {
                    "type": "object",
                    "properties": {
                        "solidLinksWithTheGivenTarget": {
                            "type": "array",
                            "description": "Links where the given element is the target (being cut or modified).",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "operatorId": { "$ref": "#/ElementId" },
                                    "operation":  { "$ref": "#/SolidOperationType" },
                                    "linkFlags":  { "$ref": "#/SolidLinkFlags" }
                                },
                                "additionalProperties": false,
                                "required": [ "operatorId", "operation", "linkFlags" ]
                            }
                        },
                        "solidLinksWithTheGivenOperator": {
                            "type": "array",
                            "description": "Links where the given element is the operator (performing the cut).",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "targetId":  { "$ref": "#/ElementId" },
                                    "operation": { "$ref": "#/SolidOperationType" },
                                    "linkFlags": { "$ref": "#/SolidLinkFlags" }
                                },
                                "additionalProperties": false,
                                "required": [ "targetId", "operation", "linkFlags" ]
                            }
                        }
                    },
                    "additionalProperties": false,
                    "required": [ "solidLinksWithTheGivenTarget", "solidLinksWithTheGivenOperator" ]
                }
            }
        },
        "additionalProperties": false,
        "required": [ "solidLinks" ]
    })";
}

GS::ObjectState GetSolidElementLinksCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& solidLinksAdder = response.AddList<GS::ObjectState> ("solidLinks");

    for (const GS::ObjectState& elemOs : elements) {
        const API_Guid elemGuid = GetGuidFromElementsArrayItem (elemOs);

        GS::ObjectState entry;
        const auto& asTargetAdder   = entry.AddList<GS::ObjectState> ("solidLinksWithTheGivenTarget");
        const auto& asOperatorAdder = entry.AddList<GS::ObjectState> ("solidLinksWithTheGivenOperator");

        if (elemGuid != APINULLGuid) {
            GS::Array<API_Guid> operators;
            if (ACAPI_Element_SolidLink_GetOperators (elemGuid, &operators) == NoError) {
                for (const API_Guid& opGuid : operators) {
                    API_SolidOperationID operation = APISolid_Substract;
                    ACAPI_Element_SolidLink_GetOperation (elemGuid, opGuid, &operation);
                    GSFlags flags = 0;
                    ACAPI_Element_SolidLink_GetFlags (elemGuid, opGuid, &flags);

                    GS::ObjectState link;
                    link.Add ("operatorId", CreateGuidObjectState (opGuid));
                    link.Add ("operation",  OperationToString (operation));
                    link.Add ("linkFlags",  FlagsToObjectState (flags));
                    asTargetAdder (link);
                }
            }

            GS::Array<API_Guid> targets;
            if (ACAPI_Element_SolidLink_GetTargets (elemGuid, &targets) == NoError) {
                for (const API_Guid& tGuid : targets) {
                    API_SolidOperationID operation = APISolid_Substract;
                    ACAPI_Element_SolidLink_GetOperation (tGuid, elemGuid, &operation);
                    GSFlags flags = 0;
                    ACAPI_Element_SolidLink_GetFlags (tGuid, elemGuid, &flags);

                    GS::ObjectState link;
                    link.Add ("targetId",  CreateGuidObjectState (tGuid));
                    link.Add ("operation", OperationToString (operation));
                    link.Add ("linkFlags", FlagsToObjectState (flags));
                    asOperatorAdder (link);
                }
            }
        }

        solidLinksAdder (entry);
    }

    return response;
}

// ---------------------------------------------------------------------------
// Trims: roofs and shells cutting construction elements
// ---------------------------------------------------------------------------

static API_TrimTypeID TrimTypeFromString (const GS::UniString& str)
{
    if (str == "KeepOutside") return APITrim_KeepOutside;
    if (str == "KeepAll")     return APITrim_KeepAll;
    if (str == "No")          return APITrim_No;
    return APITrim_KeepInside;
}

static GS::UniString TrimTypeToString (API_TrimTypeID trimType)
{
    switch (trimType) {
        case APITrim_KeepOutside: return "KeepOutside";
        case APITrim_KeepAll:     return "KeepAll";
        case APITrim_No:          return "No";
        default:                  return "KeepInside";
    }
}

static GS::Array<API_Guid> GuidsFromElements (const GS::ObjectState& parameters)
{
    // One guid per input item, in order, so GetElementTrims' promise of one
    // result per queried element holds: a missing or unknown elementId comes
    // back as that item's error there, and fails the whole call in
    // TrimElements as the API does. Same helper the other commands use.
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);
    return elements.Transform<API_Guid> (GetGuidFromElementsArrayItem);
}

TrimElementsCommand::TrimElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String TrimElementsCommand::GetName () const
{
    return "TrimElements";
}

GS::Optional<GS::UniString> TrimElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements",
                "description": "The construction elements to trim. Without trimmingElement the roofs and shells among them do the trimming."
            },
            "trimmingElement": {
                "$ref": "#/ElementId",
                "description": "Optional. The roof or shell that trims every element in the list."
            },
            "trimType": {
                "type": "string",
                "enum": [ "KeepInside", "KeepOutside", "KeepAll", "No" ],
                "description": "Which side of the trimming element the elements keep. Used with trimmingElement; defaults to KeepInside."
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> TrimElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState TrimElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    const GS::Array<API_Guid> guids = GuidsFromElements (parameters);
    if (guids.IsEmpty ()) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "elements is empty.");
    }
    GS::ObjectState trimmingOs;
    const bool withElement = parameters.Get ("trimmingElement", trimmingOs);
    GS::UniString trimTypeStr = "KeepInside";
    const bool withTrimType = parameters.Get ("trimType", trimTypeStr);
    if (withTrimType && !withElement) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "trimType applies only with trimmingElement; without one the roofs and shells in the list trim with their own settings.");
    }
    const API_TrimTypeID trimType = TrimTypeFromString (trimTypeStr);

    GSErrCode err = NoError;
    const GSErrCode undoErr = ACAPI_CallUndoableCommand ("TrimElements", [&] () -> GSErrCode {
        err = withElement
            ? ACAPI_Element_Trim_ElementsWith (guids, GetGuidFromObjectState (trimmingOs), trimType)
            : ACAPI_Element_Trim_Elements (guids);
        return err;
    });
    if (err == NoError && undoErr != NoError) {
        // The undo scope did not open (nested undoable command, teamwork state): the lambda never ran.
        err = undoErr;
    }
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to trim the elements: every element must be a construction element and at least one roof or shell must take part.");
    }
    return CreateSuccessfulExecutionResult ();
}

RemoveElementTrimsCommand::RemoveElementTrimsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String RemoveElementTrimsCommand::GetName () const
{
    return "RemoveElementTrims";
}

GS::Optional<GS::UniString> RemoveElementTrimsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementPairs": {
                "type": "array",
                "description": "The trimmed element and the roof or shell trimming it, per pair.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "trimmingElementId": {
                            "$ref": "#/ElementId"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "trimmingElementId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementPairs"
        ]
    })";
}

GS::Optional<GS::UniString> RemoveElementTrimsCommand::GetRawResponseSchema () const
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

GS::ObjectState RemoveElementTrimsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> pairs;
    parameters.Get ("elementPairs", pairs);
    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");
    const GSErrCode undoErr = ACAPI_CallUndoableCommand ("RemoveElementTrims", [&] () -> GSErrCode {
        for (const GS::ObjectState& pair : pairs) {
            GS::ObjectState elementOs, trimmingOs;
            if (!pair.Get ("elementId", elementOs) || !pair.Get ("trimmingElementId", trimmingOs)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "elementId or trimmingElementId is missing."));
                continue;
            }
            const GSErrCode err = ACAPI_Element_Trim_Remove (GetGuidFromObjectState (elementOs), GetGuidFromObjectState (trimmingOs));
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to remove the trim."));
            } else {
                executionResults (CreateSuccessfulExecutionResult ());
            }
        }
        return NoError;
    });
    if (undoErr != NoError) {
        // The undo scope did not open, so no pair was tried: say so once per pair,
        // keeping one result per input as the schema promises.
        for (USize i = 0; i < pairs.GetSize (); ++i) {
            executionResults (CreateFailedExecutionResult (undoErr, "Failed to open an undoable command; no trim was removed."));
        }
    }
    return response;
}

GetElementTrimsCommand::GetElementTrimsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetElementTrimsCommand::GetName () const
{
    return "GetElementTrims";
}

GS::Optional<GS::UniString> GetElementTrimsCommand::GetInputParametersSchema () const
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

GS::Optional<GS::UniString> GetElementTrimsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementTrims": {
                "type": "array",
                "description": "One item per queried element, in order. An unknown or deleted element is an error item, so it cannot be mistaken for an element that is simply not trimmed.",
                "items": {
                    "$ref": "#/ElementTrimsOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementTrims"
        ]
    })";
}

GS::ObjectState GetElementTrimsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    const GS::Array<API_Guid> guids = GuidsFromElements (parameters);
    GS::ObjectState response;
    const auto& items = response.AddList<GS::ObjectState> ("elementTrims");
    for (const API_Guid& guid : guids) {
        // A read that fails is reported as an error item, never as "not
        // trimmed": an unknown or deleted guid must stay distinguishable from
        // an element with no trims.
        GS::Array<API_Guid> trimming;
        GSErrCode err = ACAPI_Element_Trim_GetTrimmingElements (guid, &trimming);
        if (err != NoError) {
            items (CreateErrorResponse (err, "Failed to read the trimming elements of the element."));
            continue;
        }
        GS::ObjectState item;
        const auto& trimmedBy = item.AddList<GS::ObjectState> ("trimmedBy");
        bool failed = false;
        for (const API_Guid& t : trimming) {
            API_TrimTypeID trimType = APITrim_No;
            err = ACAPI_Element_Trim_GetTrimType (guid, t, &trimType);
            if (err != NoError) {
                failed = true;
                break;
            }
            GS::ObjectState entry;
            entry.Add ("elementId", CreateGuidObjectState (t));
            entry.Add ("trimType", TrimTypeToString (trimType));
            trimmedBy (entry);
        }
        if (failed) {
            items (CreateErrorResponse (err, "Failed to read the trim type of the element."));
            continue;
        }
        const auto& trims = item.AddList<GS::ObjectState> ("trims");
        GS::Array<API_Guid> trimmed;
        err = ACAPI_Element_Trim_GetTrimmedElements (guid, &trimmed);
        if (err != NoError) {
            items (CreateErrorResponse (err, "Failed to read the elements this element trims."));
            continue;
        }
        for (const API_Guid& t : trimmed) {
            trims (CreateElementIdObjectState (t));
        }
        items (item);
    }
    return response;
}

