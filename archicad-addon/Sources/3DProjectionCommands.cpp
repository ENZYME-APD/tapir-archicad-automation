#include "3DProjectionCommands.hpp"
#include "MigrationHelper.hpp"

// Both commands mirror API_3DProjectionInfo (the 3D Projection Settings of the 3D window):
// the common part (isPersp, camGuid, actCamSet) plus either the perspective parameters
// (API_PerspPars) or the axonometric transformation (API_AxonoPars), whichever the
// projection currently is.

static const char* PerspectiveParametersSchema = R"({
    "type": "object",
    "description": "The parameters of a perspective projection.",
    "properties": {
        "cameraPosition": {
            "$ref": "#/Coordinate3D",
            "description": "The position of the camera."
        },
        "targetPosition": {
            "$ref": "#/Coordinate3D",
            "description": "The position of the target point the camera looks at."
        },
        "distance": {
            "type": "number",
            "description": "The distance between the camera and the target."
        },
        "azimuth": {
            "type": "number",
            "description": "The azimuth angle of the camera."
        },
        "viewCone": {
            "type": "number",
            "description": "The view cone angle of the camera."
        },
        "rollAngle": {
            "type": "number",
            "description": "The roll angle of the camera."
        },
        "isTwoPointPersp": {
            "type": "boolean",
            "description": "True for a two-point perspective (vertical edges stay vertical)."
        }
    },
    "additionalProperties": false,
    "required": [
        "cameraPosition",
        "targetPosition",
        "distance",
        "azimuth",
        "viewCone",
        "rollAngle",
        "isTwoPointPersp"
    ]
})";

Get3DProjectionSettingsCommand::Get3DProjectionSettingsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String Get3DProjectionSettingsCommand::GetName () const
{
    return "Get3DProjectionSettings";
}

GS::Optional<GS::UniString> Get3DProjectionSettingsCommand::GetRawResponseSchema () const
{
    return GS::UniString (R"({
    "type": "object",
    "properties": {
        "isPersp": {
            "type": "boolean",
            "description": "True for a perspective projection, false for an axonometric one."
        },
        "cameraElementId": {
            "$ref": "#/ElementId",
            "description": "The Camera element the projection was set from. Missing when the projection does not come from a camera."
        },
        "activeCameraSetElementId": {
            "$ref": "#/ElementId",
            "description": "The active Camera Set element. Missing when there is none."
        },
        "perspective": )") + PerspectiveParametersSchema + R"(,
        "axonometric": {
            "type": "object",
            "description": "The parameters of an axonometric projection.",
            "properties": {
                "transformationMatrix": {
                    "type": "array",
                    "description": "The 3x4 transformation matrix of the axonometric projection, row by row (12 numbers).",
                    "items": {
                        "type": "number"
                    },
                    "minItems": 12,
                    "maxItems": 12
                }
            },
            "additionalProperties": false,
            "required": [
                "transformationMatrix"
            ]
        }
    },
    "additionalProperties": false,
    "required": [
        "isPersp"
    ]
})";
}

GS::ObjectState Get3DProjectionSettingsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    API_3DProjectionInfo projectionInfo {};
    GSErrCode err = ACAPI_View_Get3DProjectionSets (&projectionInfo);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the 3D projection settings.");
    }

    GS::ObjectState response ("isPersp", projectionInfo.isPersp);
    if (projectionInfo.camGuid != APINULLGuid) {
        response.Add ("cameraElementId", CreateElementIdObjectState (projectionInfo.camGuid));
    }
    if (projectionInfo.actCamSet != APINULLGuid) {
        response.Add ("activeCameraSetElementId", CreateElementIdObjectState (projectionInfo.actCamSet));
    }

    if (projectionInfo.isPersp) {
        const API_PerspPars& persp = projectionInfo.u.persp;
        response.Add ("perspective", GS::ObjectState (
            "cameraPosition", Create3DCoordinateObjectState (persp.pos),
            "targetPosition", Create3DCoordinateObjectState (persp.target),
            "distance", persp.distance,
            "azimuth", persp.azimuth,
            "viewCone", persp.viewCone,
            "rollAngle", persp.rollAngle,
            "isTwoPointPersp", persp.isTwoPointPersp));
    } else {
        GS::ObjectState axonometric;
        const auto& transformationMatrix = axonometric.AddList<double> ("transformationMatrix");
        for (double value : projectionInfo.u.axono.tranmat.tmx) {
            transformationMatrix (value);
        }
        response.Add ("axonometric", axonometric);
    }

    return response;
}

Set3DProjectionSettingsCommand::Set3DProjectionSettingsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String Set3DProjectionSettingsCommand::GetName () const
{
    return "Set3DProjectionSettings";
}

GS::Optional<GS::UniString> Set3DProjectionSettingsCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
    "type": "object",
    "properties": {
        "perspective": )") + PerspectiveParametersSchema + R"(
    },
    "additionalProperties": false,
    "required": [
        "perspective"
    ]
})";
}

GS::Optional<GS::UniString> Set3DProjectionSettingsCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState Set3DProjectionSettingsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::ObjectState perspective;
    if (!parameters.Get ("perspective", perspective)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid parameter: perspective.");
    }

    GS::ObjectState cameraPosition;
    GS::ObjectState targetPosition;
    double distance = 0.0;
    double azimuth = 0.0;
    double viewCone = 0.0;
    double rollAngle = 0.0;
    bool isTwoPointPersp = false;
    if (!perspective.Get ("cameraPosition", cameraPosition) ||
        !perspective.Get ("targetPosition", targetPosition) ||
        !perspective.Get ("distance", distance) ||
        !perspective.Get ("azimuth", azimuth) ||
        !perspective.Get ("viewCone", viewCone) ||
        !perspective.Get ("rollAngle", rollAngle) ||
        !perspective.Get ("isTwoPointPersp", isTwoPointPersp)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid parameter: every field of perspective is required.");
    }

    // Start from the current settings so the active camera set is kept; the camera
    // reference is cleared because the new values no longer describe any Camera element.
    API_3DProjectionInfo projectionInfo {};
    GSErrCode err = ACAPI_View_Get3DProjectionSets (&projectionInfo);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to get the current 3D projection settings.");
    }

    projectionInfo.isPersp = true;
    projectionInfo.camGuid = APINULLGuid;
    API_PerspPars& persp = projectionInfo.u.persp;
    persp = API_PerspPars {};
    persp.pos = Get3DCoordinateFromObjectState (cameraPosition);
    persp.target = Get3DCoordinateFromObjectState (targetPosition);
    persp.distance = distance;
    persp.azimuth = azimuth;
    persp.viewCone = viewCone;
    persp.rollAngle = rollAngle;
    persp.isTwoPointPersp = isTwoPointPersp;

    err = ACAPI_CallUndoableCommand ("Set3DProjectionSettingsCommand", [&]() -> GSErrCode {
        return ACAPI_View_Change3DProjectionSets (&projectionInfo, nullptr);
    });

    return err == NoError
        ? CreateSuccessfulExecutionResult ()
        : CreateFailedExecutionResult (err, "Failed to set the 3D projection settings.");
}
