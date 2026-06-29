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

GS::Optional<GS::UniString> CreateSolidElementLinksCommand::GetResponseSchema () const
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

GS::Optional<GS::UniString> RemoveSolidElementLinksCommand::GetResponseSchema () const
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

GS::Optional<GS::UniString> GetSolidElementLinksCommand::GetResponseSchema () const
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
