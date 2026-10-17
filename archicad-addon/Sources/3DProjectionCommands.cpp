#include "3DProjectionCommands.hpp"
#include "MigrationHelper.hpp"

// Both commands mirror API_3DProjectionInfo (the 3D Projection Settings of the 3D window):
// the common part (isPersp, camGuid, actCamSet) plus either the perspective parameters
// (API_PerspPars) or the axonometric transformation (API_AxonoPars), whichever the
// projection currently is.

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
            "cameraPosition", Create3DCoordinateObjectState (API_Coord3D { persp.pos.x, persp.pos.y, persp.cameraZ }),
            "targetPosition", Create3DCoordinateObjectState (API_Coord3D { persp.target.x, persp.target.y, persp.targetZ }),
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
    return R"({
    "type": "object",
    "properties": {
        "perspective": {
            "$ref": "#/PerspectiveProjectionParameters"
        }
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

Get3DStylesCommand::Get3DStylesCommand () :
    CommandBase (CommonSchema::NotUsed)
{}

GS::String Get3DStylesCommand::GetName () const
{
    return "Get3DStyles";
}

GS::Optional<GS::UniString> Get3DStylesCommand::GetRawResponseSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "styleNames": {
            "type": "array",
            "description": "The names of the 3D styles of the project.",
            "items": {
                "type": "string"
            }
        },
        "currentStyleName": {
            "type": "string",
            "description": "The name of the current 3D style, the one the 3D window uses."
        }
    },
    "additionalProperties": false,
    "required": [
        "styleNames",
        "currentStyleName"
    ]
})";
}

GS::ObjectState Get3DStylesCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::UniString> styles;
    GS::UniString current;
    const GSErrCode err = ACAPI_View_Get3DStyleList (&styles, &current);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the 3D styles.");
    }

    GS::ObjectState response;
    const auto& styleNames = response.AddList<GS::UniString> ("styleNames");
    for (const GS::UniString& style : styles) {
        styleNames (style);
    }
    response.Add ("currentStyleName", current);

    return response;
}

SetCurrent3DStyleCommand::SetCurrent3DStyleCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String SetCurrent3DStyleCommand::GetName () const
{
    return "SetCurrent3DStyle";
}

GS::Optional<GS::UniString> SetCurrent3DStyleCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "name": {
            "type": "string",
            "description": "The name of the 3D style to make current, one of the names Get3DStyles returns."
        }
    },
    "additionalProperties": false,
    "required": [
        "name"
    ]
})";
}

GS::Optional<GS::UniString> SetCurrent3DStyleCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState SetCurrent3DStyleCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString name;
    if (!parameters.Get ("name", name) || name.IsEmpty ()) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid parameter: name.");
    }

    // The 3D style is a setting of the 3D window, like the projection: it is changed
    // in an undoable command the same way Set3DProjectionSettings does.
    const GSErrCode err = ACAPI_CallUndoableCommand ("SetCurrent3DStyleCommand", [&]() -> GSErrCode {
        return ACAPI_View_SetCurrent3DStyle (&name);
    });

    return err == NoError
        ? CreateSuccessfulExecutionResult ()
        : CreateFailedExecutionResult (err, "Failed to set the current 3D style. Is the name one of the styles Get3DStyles returns?");
}

ShowElementsIn3DCommand::ShowElementsIn3DCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String ShowElementsIn3DCommand::GetName () const
{
    return "ShowElementsIn3D";
}

GS::Optional<GS::UniString> ShowElementsIn3DCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "elements": {
            "$ref": "#/Elements",
            "description": "The elements to show in 3D. They become the selection, replacing the current one. Missing or empty: every element is shown, as Show All in 3D does."
        }
    },
    "additionalProperties": false,
    "required": []
})";
}

GS::Optional<GS::UniString> ShowElementsIn3DCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState ShowElementsIn3DCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    if (!parameters.Get ("elements", elements) || elements.IsEmpty ()) {
        const GSErrCode err = ACAPI_View_ShowAllIn3D ();
        return err == NoError
            ? CreateSuccessfulExecutionResult ()
            : CreateFailedExecutionResult (err, "Failed to show all elements in 3D.");
    }

    // Show Selection in 3D shows exactly the selected elements, so the given ones
    // replace the current selection instead of being added to it.
    GS::Array<API_Neig> neigs;
    for (const GS::ObjectState& element : elements) {
        const API_Guid guid = GetGuidFromElementsArrayItem (element);
        if (guid == APINULLGuid) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid parameter: elementId is missing or invalid.");
        }
        neigs.Push (API_Neig (guid));
    }

    GSErrCode err = ACAPI_Selection_DeselectAll ();
    if (err != NoError && err != APIERR_NOSEL) {
        return CreateFailedExecutionResult (err, "Failed to clear the selection.");
    }

    err = ACAPI_Selection_Select (neigs, true);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to select the elements.");
    }

    err = ACAPI_View_ShowSelectionIn3D ();
    return err == NoError
        ? CreateSuccessfulExecutionResult ()
        : CreateFailedExecutionResult (err, "Failed to show the selected elements in 3D.");
}
