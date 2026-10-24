#include "AnalyticalCommands.hpp"
#include "MigrationHelper.hpp"

// The Structural Analytical Model functions are called with their ACAPI_Analytical_* names and the
// add-on's own token, which is how the Archicad 27 and newer DevKits declare them. Archicad 25 and 26
// name them ACAPI_AnalyticalModel_*, so the commands report "not supported" there.
#ifdef ServerMainVers_2700
#include "ACAPI_Analytical.hpp"
#endif

GetCurrentAnalyticalModelCommand::GetCurrentAnalyticalModelCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetCurrentAnalyticalModelCommand::GetName () const
{
    return "GetCurrentAnalyticalModel";
}

GS::Optional<GS::UniString> GetCurrentAnalyticalModelCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "analyticalModelId": {
                "$ref": "#/GuidId",
                "description": "The identifier of the Structural Analytical Model of the current view."
            }
        },
        "additionalProperties": false,
        "required": [
            "analyticalModelId"
        ]
    })";
}

GS::ObjectState GetCurrentAnalyticalModelCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    API_Guid analyticalModelGuid = APINULLGuid;
    const GSErrCode err = ACAPI_Analytical_GetCurrentAnalyticalModel (ACAPI_GetToken (), analyticalModelGuid);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the Structural Analytical Model of the current view.");
    }

    return GS::ObjectState ("analyticalModelId", CreateGuidObjectState (analyticalModelGuid));
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
}

UpdateAnalyticalModelCommand::UpdateAnalyticalModelCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String UpdateAnalyticalModelCommand::GetName () const
{
    return "UpdateAnalyticalModel";
}

GS::Optional<GS::UniString> UpdateAnalyticalModelCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "analyticalModelId": {
                "$ref": "#/GuidId",
                "description": "The identifier of the Structural Analytical Model to update. The model of the current view is updated when it is omitted."
            }
        },
        "additionalProperties": false,
        "required": []
    })";
}

GS::Optional<GS::UniString> UpdateAnalyticalModelCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState UpdateAnalyticalModelCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    API_Guid analyticalModelGuid = APINULLGuid;
    const GS::ObjectState* analyticalModelId = parameters.Get ("analyticalModelId");
    if (analyticalModelId != nullptr) {
        analyticalModelGuid = GetGuidFromObjectState (*analyticalModelId);
        if (analyticalModelGuid == APINULLGuid) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid analyticalModelId.");
        }
    } else {
        const GSErrCode err = ACAPI_Analytical_GetCurrentAnalyticalModel (ACAPI_GetToken (), analyticalModelGuid);
        if (err != NoError) {
            return CreateFailedExecutionResult (err, "Failed to get the Structural Analytical Model of the current view.");
        }
    }

    const GSErrCode err = ACAPI_CallUndoableCommand ("Update Analytical Model", [&] () -> GSErrCode {
        return ACAPI_Analytical_UpdateAnalyticalModel (ACAPI_GetToken (), analyticalModelGuid);
    });
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to update the Structural Analytical Model.");
    }

    return CreateSuccessfulExecutionResult ();
#else
    UNUSED_VARIABLE (parameters);
    return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
}

SetAnalyticalGenerationSettingsToNoRuleCommand::SetAnalyticalGenerationSettingsToNoRuleCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetAnalyticalGenerationSettingsToNoRuleCommand::GetName () const
{
    return "SetAnalyticalGenerationSettingsToNoRule";
}

GS::Optional<GS::UniString> SetAnalyticalGenerationSettingsToNoRuleCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState SetAnalyticalGenerationSettingsToNoRuleCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    const GSErrCode err = ACAPI_CallUndoableCommand ("Set Analytical Generation Settings To No Rule", [&] () -> GSErrCode {
        return ACAPI_Analytical_SetGenerationSettingsToNoRule (ACAPI_GetToken ());
    });
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to set the Structural Analytical Model generation settings to no rule.");
    }

    return CreateSuccessfulExecutionResult ();
#else
    return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
}

GetAnalyticalModelCheckTypeIdCommand::GetAnalyticalModelCheckTypeIdCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAnalyticalModelCheckTypeIdCommand::GetName () const
{
    return "GetAnalyticalModelCheckTypeId";
}

GS::Optional<GS::UniString> GetAnalyticalModelCheckTypeIdCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "modelCheckTypeId": {
                "$ref": "#/GuidId",
                "description": "The type identifier of the Structural Analytical Model check."
            }
        },
        "additionalProperties": false,
        "required": [
            "modelCheckTypeId"
        ]
    })";
}

GS::ObjectState GetAnalyticalModelCheckTypeIdCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    API_Guid typeId = APINULLGuid;
    const GSErrCode err = ACAPI_Analytical_GetModelCheckTypeId (ACAPI_GetToken (), typeId);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the Structural Analytical Model check type identifier.");
    }

    return GS::ObjectState ("modelCheckTypeId", CreateGuidObjectState (typeId));
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}
