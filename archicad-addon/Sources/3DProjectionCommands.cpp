#include "3DProjectionCommands.hpp"
#include "MigrationHelper.hpp"

// Both commands mirror API_3DProjectionInfo (the 3D Projection Settings of the 3D window):
// the common part (isPersp, camGuid, actCamSet) plus either the perspective parameters
// (API_PerspPars) or the axonometric ones (API_AxonoPars: projection type, azimuth and
// transformation matrix), whichever the projection currently is.

// The projMod values of API_AxonoPars (API_Projection_* in APIdefs_Elements.h), by the
// names the commands use. API_Projection_Perspective is left out: it is never the type
// of an axonometry.
static const std::pair<short, const char*> AxonometryProjectionTypes[] = {
    { API_Projection_YZ,          "YZ" },
    { API_Projection_XZ,          "XZ" },
    { API_Projection_XY,          "XY" },
    { API_Projection_Frontal,     "Frontal" },
    { API_Projection_Monometric,  "Monometric" },
    { API_Projection_Isometric,   "Isometric" },
    { API_Projection_Dimetric,    "Dimetric" },
    { API_Projection_Bottom,      "Bottom" },
    { API_Projection_FrontalB,    "FrontalBottom" },
    { API_Projection_MonometricB, "MonometricBottom" },
    { API_Projection_IzometricB,  "IsometricBottom" },
    { API_Projection_DimetricB,   "DimetricBottom" },
    { API_Projection_Parallel,    "Parallel" },
    { API_Projection_FreeAx,      "Custom" },
};

static GS::UniString GetAxonometryProjectionTypeName (short projMod)
{
    for (const auto& type : AxonometryProjectionTypes) {
        if (type.first == projMod) {
            return type.second;
        }
    }
    return "Unknown";
}

static bool GetAxonometryProjectionType (const GS::UniString& name, short& projMod)
{
    for (const auto& type : AxonometryProjectionTypes) {
        if (name == type.second) {
            projMod = type.first;
            return true;
        }
    }
    return false;
}

Get3DProjectionSettingsCommand::Get3DProjectionSettingsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String Get3DProjectionSettingsCommand::GetName () const
{
    return "Get3DProjectionSettings";
}

GS::Optional<GS::UniString> Get3DProjectionSettingsCommand::GetRawResponseSchema () const
{
    return R"({
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
        "perspective": {
            "$ref": "#/PerspectiveProjectionParameters",
            "description": "The perspective parameters. Present only when isPersp is true."
        },
        "axonometric": {
            "type": "object",
            "description": "The parameters of an axonometric projection. Present only when isPersp is false.",
            "properties": {
                "projectionType": {
                    "$ref": "#/AxonometryProjectionType"
                },
                "azimuth": {
                    "type": "number",
                    "description": "The camera azimuth in degrees, the Azimuth of the 3D Projection Settings dialog."
                },
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
                "projectionType",
                "azimuth",
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
            "cameraPosition", Create3DCoordinateObjectState (API_Coord3D { persp.pos.x, persp.pos.y, persp.cameraZ }),
            "targetPosition", Create3DCoordinateObjectState (API_Coord3D { persp.target.x, persp.target.y, persp.targetZ }),
            "distance", persp.distance,
            "azimuth", persp.azimuth,
            "viewCone", persp.viewCone,
            "rollAngle", persp.rollAngle,
            "isTwoPointPersp", persp.isTwoPointPersp));
    } else {
        const API_AxonoPars& axono = projectionInfo.u.axono;
        GS::ObjectState axonometric (
            "projectionType", GetAxonometryProjectionTypeName (axono.projMod),
            "azimuth", axono.azimuth);
        const auto& transformationMatrix = axonometric.AddList<double> ("transformationMatrix");
        for (double value : axono.tranmat.tmx) {
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
    return R"({
    "type": "object",
    "properties": {
        "perspective": {
            "$ref": "#/PerspectiveProjectionParameters"
        },
        "axonometry": {
            "type": "object",
            "description": "Makes the projection axonometric. Only the given values change; the others are kept from the current axonometry, or from the last one when the projection is a perspective.",
            "properties": {
                "projectionType": {
                    "$ref": "#/AxonometryProjectionType"
                },
                "azimuth": {
                    "type": "number",
                    "description": "The camera azimuth in degrees, the Azimuth of the 3D Projection Settings dialog."
                },
                "transformationMatrix": {
                    "type": "array",
                    "description": "The 3x4 transformation matrix of the axonometric projection, row by row (12 numbers), as Get3DProjectionSettings returns it. Its 3x3 part must be invertible.",
                    "items": {
                        "type": "number"
                    },
                    "minItems": 12,
                    "maxItems": 12
                }
            },
            "additionalProperties": false,
            "minProperties": 1
        }
    },
    "additionalProperties": false,
    "oneOf": [
        {
            "required": [
                "perspective"
            ]
        },
        {
            "required": [
                "axonometry"
            ]
        }
    ]
})";
}

GS::Optional<GS::UniString> Set3DProjectionSettingsCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

static GS::ObjectState SetPerspective (const GS::ObjectState& perspective)
{
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

    // The sun position is part of both the perspective and the axonometric settings;
    // keep the current one instead of zeroing it with the rest of the parameters.
    const API_SunAngleSettings sunAngSets = projectionInfo.isPersp ? projectionInfo.u.persp.sunAngSets : projectionInfo.u.axono.sunAngSets;

    projectionInfo.isPersp = true;
    projectionInfo.camGuid = APINULLGuid;
    API_PerspPars& persp = projectionInfo.u.persp;
    persp = API_PerspPars {};
    persp.sunAngSets = sunAngSets;
    // API_PerspPars keeps the X/Y of the camera and the target as 2D coordinates and
    // their heights in separate fields.
    const API_Coord3D camera = Get3DCoordinateFromObjectState (cameraPosition);
    const API_Coord3D target = Get3DCoordinateFromObjectState (targetPosition);
    persp.pos = API_Coord { camera.x, camera.y };
    persp.cameraZ = camera.z;
    persp.target = API_Coord { target.x, target.y };
    persp.targetZ = target.z;
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

static GS::ObjectState SetAxonometry (const GS::ObjectState& axonometry)
{
    short projMod = 0;
    GS::UniString projectionType;
    const bool hasProjectionType = axonometry.Get ("projectionType", projectionType);
    if (hasProjectionType && !GetAxonometryProjectionType (projectionType, projMod)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid parameter: projectionType.");
    }

    double azimuth = 0.0;
    const bool hasAzimuth = axonometry.Get ("azimuth", azimuth);

    API_Tranmat tranmat {};
    API_Tranmat invtranmat {};
    GS::Array<double> transformationMatrix;
    const bool hasTransformationMatrix = axonometry.Get ("transformationMatrix", transformationMatrix);
    if (hasTransformationMatrix) {
        if (transformationMatrix.GetSize () != 12) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid parameter: transformationMatrix must have 12 numbers.");
        }
        for (UIndex i = 0; i < 12; ++i) {
            tranmat.tmx[i] = transformationMatrix[i];
        }
        // Archicad keeps the inverse next to the matrix, to get model coordinates back from projected ones.
        if (!InvertTranmat (tranmat, invtranmat)) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid parameter: the 3x3 part of transformationMatrix is not invertible.");
        }
    }

    if (!hasProjectionType && !hasAzimuth && !hasTransformationMatrix) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid parameter: give at least one of projectionType, azimuth and transformationMatrix in axonometry.");
    }

    const GSErrCode err = ACAPI_CallUndoableCommand ("Set3DProjectionSettingsCommand", [&]() -> GSErrCode {
        API_3DProjectionInfo projectionInfo {};
        GSErrCode err = ACAPI_View_Get3DProjectionSets (&projectionInfo);
        if (err != NoError) {
            return err;
        }

        // A perspective shares its memory with the axonometric parameters, so they are
        // read after switching back to the last axonometry; the values not given come from it.
        if (projectionInfo.isPersp) {
            projectionInfo.isPersp = false;
            bool switchOnlyAxonoOrPersp = true;
            err = ACAPI_View_Change3DProjectionSets (&projectionInfo, &switchOnlyAxonoOrPersp);
            if (err == NoError) {
                err = ACAPI_View_Get3DProjectionSets (&projectionInfo);
            }
            if (err != NoError) {
                return err;
            }
        }

        API_AxonoPars& axono = projectionInfo.u.axono;
        if (hasProjectionType) {
            axono.projMod = projMod;
        }
        if (hasAzimuth) {
            axono.azimuth = azimuth;
        }
        if (hasTransformationMatrix) {
            axono.tranmat = tranmat;
            axono.invtranmat = invtranmat;
        }
        return ACAPI_View_Change3DProjectionSets (&projectionInfo, nullptr);
    });

    return err == NoError
        ? CreateSuccessfulExecutionResult ()
        : CreateFailedExecutionResult (err, "Failed to set the 3D projection settings.");
}

GS::ObjectState Set3DProjectionSettingsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::ObjectState perspective;
    GS::ObjectState axonometry;
    const bool hasPerspective = parameters.Get ("perspective", perspective);
    const bool hasAxonometry = parameters.Get ("axonometry", axonometry);
    if (hasPerspective == hasAxonometry) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid parameters: give exactly one of perspective and axonometry.");
    }

    return hasPerspective ? SetPerspective (perspective) : SetAxonometry (axonometry);
}
// Both commands mirror API_3DWindowInfo: the pixel size of the 3D window (hSize, vSize) and
// the zoom of its projection (zoomScaleX/Y, zoomDispX/Y). Archicad applies the size only
// when setWindowSize is true and the zoom only when setZoom is true.

static const char* Window3DSettingsSchema = R"({
    "type": "object",
    "properties": {
        "hSize": {
            "type": "integer",
            "description": "The horizontal size of the 3D window in pixels."
        },
        "vSize": {
            "type": "integer",
            "description": "The vertical size of the 3D window in pixels."
        },
        "zoomScaleX": {
            "type": "number",
            "description": "The horizontal zoom scale of the 3D projection."
        },
        "zoomScaleY": {
            "type": "number",
            "description": "The vertical zoom scale of the 3D projection."
        },
        "zoomDispX": {
            "type": "number",
            "description": "The horizontal displacement (pan) of the 3D projection."
        },
        "zoomDispY": {
            "type": "number",
            "description": "The vertical displacement (pan) of the 3D projection."
        }
    },
    "additionalProperties": false,
    "required": [
        "hSize",
        "vSize",
        "zoomScaleX",
        "zoomScaleY",
        "zoomDispX",
        "zoomDispY"
    ]
})";

static GS::ObjectState Create3DWindowSettingsObjectState (const API_3DWindowInfo& windowInfo)
{
    return GS::ObjectState (
        "hSize", static_cast<Int32> (windowInfo.hSize),
        "vSize", static_cast<Int32> (windowInfo.vSize),
        "zoomScaleX", windowInfo.zoomScaleX,
        "zoomScaleY", windowInfo.zoomScaleY,
        "zoomDispX", windowInfo.zoomDispX,
        "zoomDispY", windowInfo.zoomDispY);
}

Get3DWindowSettingsCommand::Get3DWindowSettingsCommand () :
    CommandBase (CommonSchema::NotUsed)
{}

GS::String Get3DWindowSettingsCommand::GetName () const
{
    return "Get3DWindowSettings";
}

GS::Optional<GS::UniString> Get3DWindowSettingsCommand::GetRawResponseSchema () const
{
    return GS::UniString (Window3DSettingsSchema);
}

GS::ObjectState Get3DWindowSettingsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    API_3DWindowInfo windowInfo {};
    GSErrCode err = ACAPI_View_Get3DWindowSets (&windowInfo);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the 3D window settings.");
    }

    return Create3DWindowSettingsObjectState (windowInfo);
}

Set3DWindowSettingsCommand::Set3DWindowSettingsCommand () :
    CommandBase (CommonSchema::NotUsed)
{}

GS::String Set3DWindowSettingsCommand::GetName () const
{
    return "Set3DWindowSettings";
}

GS::Optional<GS::UniString> Set3DWindowSettingsCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "hSize": {
            "type": "integer",
            "description": "The horizontal size of the 3D window in pixels.",
            "minimum": 1,
            "maximum": 32767
        },
        "vSize": {
            "type": "integer",
            "description": "The vertical size of the 3D window in pixels.",
            "minimum": 1,
            "maximum": 32767
        },
        "zoomScaleX": {
            "type": "number",
            "description": "The horizontal zoom scale of the 3D projection.",
            "exclusiveMinimum": 0
        },
        "zoomScaleY": {
            "type": "number",
            "description": "The vertical zoom scale of the 3D projection.",
            "exclusiveMinimum": 0
        },
        "zoomDispX": {
            "type": "number",
            "description": "The horizontal displacement (pan) of the 3D projection."
        },
        "zoomDispY": {
            "type": "number",
            "description": "The vertical displacement (pan) of the 3D projection."
        }
    },
    "additionalProperties": false,
    "minProperties": 1,
    "required": []
})";
}

GS::Optional<GS::UniString> Set3DWindowSettingsCommand::GetRawResponseSchema () const
{
    return GS::UniString (Window3DSettingsSchema);
}

GS::ObjectState Set3DWindowSettingsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    // Start from the current settings so every value that is not given is kept.
    API_3DWindowInfo windowInfo {};
    GSErrCode err = ACAPI_View_Get3DWindowSets (&windowInfo);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the current 3D window settings.");
    }
    windowInfo.setWindowSize = false;
    windowInfo.setZoom = false;

    // API_3DWindowInfo keeps the window size in shorts.
    const auto getSize = [&] (const char* name, short& size) -> bool {
        Int32 value = 0;
        if (!parameters.Get (name, value) || value < 1 || value > 32767) {
            return false;
        }
        size = static_cast<short> (value);
        windowInfo.setWindowSize = true;
        return true;
    };
    const auto getZoom = [&] (const char* name, bool mustBePositive, double& zoom) -> bool {
        double value = 0.0;
        if (!parameters.Get (name, value) || (mustBePositive && !(value > 0.0))) {
            return false;
        }
        zoom = value;
        windowInfo.setZoom = true;
        return true;
    };

    if (parameters.Contains ("hSize") && !getSize ("hSize", windowInfo.hSize)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid parameter: hSize must be an integer from 1 to 32767.");
    }
    if (parameters.Contains ("vSize") && !getSize ("vSize", windowInfo.vSize)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid parameter: vSize must be an integer from 1 to 32767.");
    }
    if (parameters.Contains ("zoomScaleX") && !getZoom ("zoomScaleX", true, windowInfo.zoomScaleX)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid parameter: zoomScaleX must be greater than 0.");
    }
    if (parameters.Contains ("zoomScaleY") && !getZoom ("zoomScaleY", true, windowInfo.zoomScaleY)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid parameter: zoomScaleY must be greater than 0.");
    }
    if (parameters.Contains ("zoomDispX") && !getZoom ("zoomDispX", false, windowInfo.zoomDispX)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid parameter: zoomDispX must be a number.");
    }
    if (parameters.Contains ("zoomDispY") && !getZoom ("zoomDispY", false, windowInfo.zoomDispY)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid parameter: zoomDispY must be a number.");
    }

    if (!windowInfo.setWindowSize && !windowInfo.setZoom) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid parameters: give at least one of hSize, vSize, zoomScaleX, zoomScaleY, zoomDispX and zoomDispY.");
    }

    err = ACAPI_View_Change3DWindowSets (&windowInfo);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to set the 3D window settings.");
    }

    API_3DWindowInfo newWindowInfo {};
    err = ACAPI_View_Get3DWindowSets (&newWindowInfo);
    if (err != NoError) {
        return CreateErrorResponse (err, "The 3D window settings were changed, but reading them back failed.");
    }

    return Create3DWindowSettingsObjectState (newWindowInfo);
}
