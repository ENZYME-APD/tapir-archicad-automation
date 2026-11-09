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

// The sun position of a projection (API_SunAngleSettings), by the names the commands use.
static GS::ObjectState CreateSunObjectState (const API_SunAngleSettings& sunAngSets)
{
    return GS::ObjectState (
        "sunPositionOption", GS::UniString (sunAngSets.sunPosOpt == API_SunPosition_GivenByDate ? "DateAndTime" : "Angles"),
        "sunAzimuth", sunAngSets.sunAzimuth,
        "sunAltitude", sunAngSets.sunAltitude,
        "year", static_cast<Int32> (sunAngSets.year),
        "month", static_cast<Int32> (sunAngSets.month),
        "day", static_cast<Int32> (sunAngSets.day),
        "hour", static_cast<Int32> (sunAngSets.hour),
        "minute", static_cast<Int32> (sunAngSets.minute),
        "second", static_cast<Int32> (sunAngSets.second),
        "summerTime", sunAngSets.summerTime);
}

// Overwrites the fields of sunAngSets that sun carries and keeps the others. Returns
// false, with the reason in error, when a value is invalid; sunAngSets may then be
// partly changed, so check on a copy first when that matters.
static bool ApplySunFromObjectState (const GS::ObjectState& sun, API_SunAngleSettings& sunAngSets, GS::UniString& error)
{
    if (sun.Contains ("sunPositionOption")) {
        GS::UniString sunPositionOption;
        sun.Get ("sunPositionOption", sunPositionOption);
        if (sunPositionOption == "Angles") {
            sunAngSets.sunPosOpt = API_SunPosition_GivenByAngles;
        } else if (sunPositionOption == "DateAndTime") {
            sunAngSets.sunPosOpt = API_SunPosition_GivenByDate;
        } else {
            error = "Invalid parameter: sun.sunPositionOption must be Angles or DateAndTime.";
            return false;
        }
    }

    const auto getAngle = [&] (const char* name, double& target) -> bool {
        if (!sun.Contains (name)) {
            return true;
        }
        if (!sun.Get (name, target)) {
            error = GS::UniString::Printf ("Invalid parameter: sun.%s must be a number.", name);
            return false;
        }
        return true;
    };
    // The date and time fields of API_SunAngleSettings are unsigned short. Year, month
    // and day accept 0: a sun given only by angles may carry no date, and what
    // Get3DProjectionSettings returns must be accepted back.
    const auto getDateField = [&] (const char* name, Int32 minimum, Int32 maximum, unsigned short& target) -> bool {
        if (!sun.Contains (name)) {
            return true;
        }
        Int32 value = 0;
        if (!sun.Get (name, value) || value < minimum || value > maximum) {
            error = GS::UniString::Printf ("Invalid parameter: sun.%s must be an integer from %d to %d.", name, (int) minimum, (int) maximum);
            return false;
        }
        target = static_cast<unsigned short> (value);
        return true;
    };

    if (!getAngle ("sunAzimuth", sunAngSets.sunAzimuth) ||
        !getAngle ("sunAltitude", sunAngSets.sunAltitude) ||
        !getDateField ("year", 0, 9999, sunAngSets.year) ||
        !getDateField ("month", 0, 12, sunAngSets.month) ||
        !getDateField ("day", 0, 31, sunAngSets.day) ||
        !getDateField ("hour", 0, 23, sunAngSets.hour) ||
        !getDateField ("minute", 0, 59, sunAngSets.minute) ||
        !getDateField ("second", 0, 59, sunAngSets.second)) {
        return false;
    }

    if (sun.Contains ("summerTime") && !sun.Get ("summerTime", sunAngSets.summerTime)) {
        error = "Invalid parameter: sun.summerTime must be a boolean.";
        return false;
    }
    return true;
}

// A perspective (API_PerspPars) as the PerspectiveProjectionParameters schema describes it.
// API_PerspPars keeps the X/Y of the camera and the target as 2D coordinates and their
// heights in separate fields.
static GS::ObjectState CreatePerspectiveObjectState (const API_PerspPars& persp)
{
    return GS::ObjectState (
        "cameraPosition", Create3DCoordinateObjectState (API_Coord3D { persp.pos.x, persp.pos.y, persp.cameraZ }),
        "targetPosition", Create3DCoordinateObjectState (API_Coord3D { persp.target.x, persp.target.y, persp.targetZ }),
        "distance", persp.distance,
        "azimuth", persp.azimuth,
        "viewCone", persp.viewCone,
        "rollAngle", persp.rollAngle,
        "isTwoPointPersp", persp.isTwoPointPersp,
        "sun", CreateSunObjectState (persp.sunAngSets));
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
                    "description": "The camera azimuth in degrees, the Azimuth of the 3D Projection Settings dialog: for a side view the direction from the model to the camera, counterclockwise from the X axis (270 looks from the south). A predefined axonometry looks from 45 degrees further on."
                },
                "transformationMatrix": {
                    "type": "array",
                    "description": "The 3x4 transformation matrix of the axonometric projection, row by row (12 numbers).",
                    "items": {
                        "type": "number"
                    },
                    "minItems": 12,
                    "maxItems": 12
                },
                "sun": {
                    "$ref": "#/ProjectionSunSettings",
                    "description": "The sun position of the axonometry."
                }
            },
            "additionalProperties": false,
            "required": [
                "projectionType",
                "azimuth",
                "transformationMatrix",
                "sun"
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
        response.Add ("perspective", CreatePerspectiveObjectState (projectionInfo.u.persp));
    } else {
        const API_AxonoPars& axono = projectionInfo.u.axono;
        GS::ObjectState axonometric (
            "projectionType", GetAxonometryProjectionTypeName (axono.projMod),
            "azimuth", axono.azimuth,
            "sun", CreateSunObjectState (axono.sunAngSets));
        const auto& transformationMatrix = axonometric.AddList<double> ("transformationMatrix");
        for (double value : axono.tranmat.tmx) {
            transformationMatrix (value);
        }
        response.Add ("axonometric", axonometric);
    }

    return response;
}

GetCamerasCommand::GetCamerasCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String GetCamerasCommand::GetName () const
{
    return "GetCameras";
}

GS::Optional<GS::UniString> GetCamerasCommand::GetRawResponseSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "cameraSets": {
            "type": "array",
            "description": "The Camera Sets (camera paths) of the project, each with its cameras.",
            "items": {
                "type": "object",
                "properties": {
                    "cameraSetElementId": {
                        "$ref": "#/ElementId"
                    },
                    "name": {
                        "type": "string",
                        "description": "The name of the Camera Set, as the navigator shows it."
                    },
                    "isActive": {
                        "type": "boolean",
                        "description": "True for the active Camera Set."
                    },
                    "activeCameraElementId": {
                        "$ref": "#/ElementId",
                        "description": "The current camera of the set. Missing when the set has none."
                    },
                    "cameras": {
                        "type": "array",
                        "description": "The cameras of the set, in the order of the path.",
                        "items": {
                            "type": "object",
                            "properties": {
                                "cameraElementId": {
                                    "$ref": "#/ElementId"
                                },
                                "isActiveInPath": {
                                    "type": "boolean",
                                    "description": "True when the camera is active in the path of its Camera Set."
                                },
                                "perspective": {
                                    "$ref": "#/PerspectiveProjectionParameters",
                                    "description": "The perspective of the camera, in the shape Set3DProjectionSettings accepts as its perspective, so passing it there shows the camera's view in the 3D window."
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "cameraElementId",
                                "isActiveInPath",
                                "perspective"
                            ]
                        }
                    }
                },
                "additionalProperties": false,
                "required": [
                    "cameraSetElementId",
                    "name",
                    "isActive",
                    "cameras"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "cameraSets"
    ]
})";
}

static GS::ObjectState CreateCameraObjectState (const API_CameraType& camera)
{
    return GS::ObjectState (
        "cameraElementId", CreateElementIdObjectState (camera.head.guid),
        "isActiveInPath", camera.perspCam.active,
        "perspective", CreatePerspectiveObjectState (camera.perspCam.persp));
}

GS::ObjectState GetCamerasCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<API_Guid> cameraGuids;
    GSErrCode err = ACAPI_Element_GetElemList (API_CameraID, &cameraGuids);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to list the cameras.");
    }
    GS::Array<API_Guid> cameraSetGuids;
    err = ACAPI_Element_GetElemList (API_CamSetID, &cameraSetGuids);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to list the camera sets.");
    }

    // Every camera, read once; the sets then take theirs from here in path order.
    GS::HashTable<API_Guid, API_CameraType> cameras;
    GS::Array<API_Guid> cameraOrder;
    for (const API_Guid& guid : cameraGuids) {
        API_Element element {};
        element.header.guid = guid;
        if (ACAPI_Element_Get (&element) != NoError || GetElemTypeId (element.header) != API_CameraID) {
            continue;
        }
        cameras.Add (guid, element.camera);
        cameraOrder.Push (guid);
    }

    GS::HashSet<API_Guid> listedCameras;
    GS::ObjectState response;
    const auto& cameraSetsList = response.AddList<GS::ObjectState> ("cameraSets");
    for (const API_Guid& setGuid : cameraSetGuids) {
        API_Element element {};
        element.header.guid = setGuid;
        if (ACAPI_Element_Get (&element) != NoError || GetElemTypeId (element.header) != API_CamSetID) {
            continue;
        }
        const API_CamSetType& camSet = element.camset;

        GS::ObjectState cameraSet (
            "cameraSetElementId", CreateElementIdObjectState (setGuid),
            "name", GS::UniString (camSet.name),
            "isActive", camSet.active);
        if (camSet.actCam != APINULLGuid) {
            cameraSet.Add ("activeCameraElementId", CreateElementIdObjectState (camSet.actCam));
        }

        const auto& setCamerasList = cameraSet.AddList<GS::ObjectState> ("cameras");
        // The path: from the first camera along nextCam. A closed path leads back to the
        // first camera, so a camera already listed ends the walk.
        API_Guid cameraGuid = camSet.firstCam;
        while (cameraGuid != APINULLGuid && !listedCameras.Contains (cameraGuid)) {
            const API_CameraType* camera = cameras.GetPtr (cameraGuid);
            if (camera == nullptr || camera->camSetGuid != setGuid) {
                break;
            }
            setCamerasList (CreateCameraObjectState (*camera));
            listedCameras.Add (cameraGuid);
            cameraGuid = camera->perspCam.nextCam;
        }
        // Cameras of the set the walk did not reach, in the order Archicad lists them.
        for (const API_Guid& guid : cameraOrder) {
            const API_CameraType* camera = cameras.GetPtr (guid);
            if (camera != nullptr && camera->camSetGuid == setGuid && !listedCameras.Contains (guid)) {
                setCamerasList (CreateCameraObjectState (*camera));
                listedCameras.Add (guid);
            }
        }

        cameraSetsList (cameraSet);
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
            "description": "Makes the projection axonometric. Only the given values change, in sun too; the others are kept from the current axonometry, or from the last one when the projection is a perspective.",
            "properties": {
                "projectionType": {
                    "$ref": "#/AxonometryProjectionType"
                },
                "azimuth": {
                    "type": "number",
                    "description": "The camera azimuth in degrees, the Azimuth of the 3D Projection Settings dialog: for a side view the direction from the model to the camera, counterclockwise from the X axis (270 looks from the south). A predefined axonometry looks from 45 degrees further on."
                },
                "transformationMatrix": {
                    "type": "array",
                    "description": "The 3x4 transformation matrix of the axonometric projection, row by row (12 numbers), as Get3DProjectionSettings returns it. Its 3x3 part must be invertible.",
                    "items": {
                        "type": "number"
                    },
                    "minItems": 12,
                    "maxItems": 12
                },
                "sun": {
                    "$ref": "#/ProjectionSunSettings",
                    "description": "The sun position of the axonometry. Only the values given change."
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
    GS::ObjectState sun;
    if (perspective.Get ("sun", sun)) {
        GS::UniString error;
        if (!ApplySunFromObjectState (sun, persp.sunAngSets, error)) {
            return CreateFailedExecutionResult (APIERR_BADPARS, error);
        }
    }
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
        // Checked before the selection is cleared, so a wrong guid keeps the user's selection.
        API_Elem_Head head = {};
        head.guid = guid;
        if (ACAPI_Element_GetHeader (&head) != NoError) {
            return CreateFailedExecutionResult (APIERR_BADID, "Element not found.");
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

    GS::ObjectState sun;
    const bool hasSun = axonometry.Get ("sun", sun);
    if (hasSun) {
        // Checked here, on a copy, so an invalid value changes nothing.
        API_SunAngleSettings checkedSunAngSets {};
        GS::UniString error;
        if (!ApplySunFromObjectState (sun, checkedSunAngSets, error)) {
            return CreateFailedExecutionResult (APIERR_BADPARS, error);
        }
    }

    if (!hasProjectionType && !hasAzimuth && !hasTransformationMatrix && !hasSun) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid parameter: give at least one of projectionType, azimuth, transformationMatrix and sun in axonometry.");
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
        if (hasSun) {
            // Checked above, so this cannot fail.
            GS::UniString error;
            ApplySunFromObjectState (sun, axono.sunAngSets, error);
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
        "styleName": {
            "type": "string",
            "description": "The name of an existing 3D style, as Get3DStyles returns it.",
            "minLength": 1
        }
    },
    "additionalProperties": false,
    "required": [
        "styleName"
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
    GS::UniString styleName;
    if (!parameters.Get ("styleName", styleName) || styleName.IsEmpty ()) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "styleName is missing or empty.");
    }

    GS::Array<GS::UniString> styles;
    GS::UniString current;
    GSErrCode err = ACAPI_View_Get3DStyleList (&styles, &current);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to get the 3D styles.");
    }
    if (!styles.Contains (styleName)) {
        return CreateFailedExecutionResult (APIERR_BADNAME, "There is no 3D style with the name '" + styleName + "'.");
    }

    // ACAPI_View_SetCurrent3DStyle is not called: in Archicad 28 and 29 it stops Archicad with the assertion "can't open
    // modification scope inside transaction" whatever it is called from - from this command, inside
    // ACAPI_CallUndoableCommand, and from the main event loop (ACAPI_AddOnAddOnCommunication_CallFromEventLoop),
    // because it runs one of Archicad's commands inside another. It has not been seen working in any Archicad, so the
    // command refuses rather than crash Archicad.
    return CreateFailedExecutionResult (APIERR_NOTSUPPORTED,
        "Switching the current 3D style is not available: Archicad's API function for it crashes Archicad.");
}

StoreViewSettingsCommand::StoreViewSettingsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String StoreViewSettingsCommand::GetName () const
{
    return "StoreViewSettings";
}

GS::Optional<GS::UniString> StoreViewSettingsCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "action": {
            "type": "string",
            "description": "Store saves the current view settings, Restore puts the stored ones back. Call them in pairs: a second Store before a Restore is refused.",
            "enum": [
                "Store",
                "Restore"
            ]
        }
    },
    "additionalProperties": false,
    "required": [
        "action"
    ]
})";
}

GS::Optional<GS::UniString> StoreViewSettingsCommand::GetRawResponseSchema () const
{
    return R"({
    "$ref": "#/ExecutionResult"
})";
}

GS::ObjectState StoreViewSettingsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString action;
    parameters.Get ("action", action);
    if (action != "Store" && action != "Restore") {
        return CreateFailedExecutionResult (APIERR_BADPARS, "action must be Store or Restore.");
    }
    const bool store = (action == "Store");

    const GSErrCode err = ACAPI_View_StoreViewSettings (store);
    if (err == APIERR_BADPARS) {
        return CreateFailedExecutionResult (err, "Store and Restore must be called in pairs, one after the other.");
    }

    return err == NoError
        ? CreateSuccessfulExecutionResult ()
        : CreateFailedExecutionResult (err, store ? "Failed to store the view settings." : "Failed to restore the view settings.");
}
