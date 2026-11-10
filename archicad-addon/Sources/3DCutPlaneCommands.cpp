#include "GSUnID.hpp"
#include "3DCutPlaneCommands.hpp"
#include "MigrationHelper.hpp"
#include "ApplicationCommands.hpp"

GS::String Set3DCutPlanesCommand::GetName () const
{
    return "Set3DCutPlanes";
}


Set3DCutPlanesCommand::Set3DCutPlanesCommand () :
    CommandBase (CommonSchema::Used)
{}


static bool IsPenIndex (Int32 pen)
{
    return pen >= 1 && pen <= 255;
}


// Reads an optional pen and an optional surface (#/AttributeId) of the input; an error text when one is invalid.
static GS::Optional<GS::UniString> GetPenAndSurface (const GS::ObjectState& os, const char* penField, const char* surfaceField,
                                                    GS::Optional<short>& pen, GS::Optional<API_AttributeIndex>& surface)
{
    Int32 penIndex = 0;
    if (os.Get (penField, penIndex)) {
        if (!IsPenIndex (penIndex)) {
            return GS::UniString::Printf ("%s must be a pen index from 1 to 255.", penField);
        }
        pen = (short) penIndex;
    }
    const GS::ObjectState* surfaceId = os.Get (surfaceField);
    if (surfaceId != nullptr) {
        const API_AttributeIndex index = GetAttributeIndexFromGuid (API_MaterialID, GetGuidFromObjectState (*surfaceId));
        if (index == ACAPI_CreateAttributeIndex (-1)) {
            return GS::UniString::Printf ("%s is not a surface attribute.", surfaceField);
        }
        surface = index;
    }
    return GS::NoValue;
}


GS::Optional<GS::UniString> Set3DCutPlanesCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "cutPlanes": {
            "type": "array",
            "description": "The cut planes, replacing the current ones. Each cuts away the side of its plane that its normal vector (pa, pb, pc) points to; several planes cut away the union of those sides. Setting planes also turns 3D Cutaway on (off and on again when it is on), which puts them in force in the 3D window. An empty array switches the 3D cut and 3D Cutaway off. A plane without cutPen or cutSurfaceId keeps those of the current plane in its place, or of the last current plane, or else takes customPen and customSurfaceId.",
            "items": {
                "$ref": "#/CutPlane"
            }
        },
        "useCustom": {
            "type": "boolean",
            "description": "The Custom setting of the cut faces, kept for the 3D Cutting Planes settings; in the Archicad 29 3D window the cut faces take each plane's cutSurfaceId either way. Left unchanged when omitted."
        },
        "customPen": {
            "type": "integer",
            "minimum": 1,
            "maximum": 255,
            "description": "The pen of the cut edges when useCustom is true. Left unchanged when omitted."
        },
        "customSurfaceId": {
            "$ref": "#/AttributeId",
            "description": "The surface of the cut faces when useCustom is true. Left unchanged when omitted."
        }
    },
    "additionalProperties": false,
    "required": ["cutPlanes"]
})";
}


GS::Optional<GS::UniString> Set3DCutPlanesCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState Set3DCutPlanesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> cutPlanes;
    if (!parameters.Get ("cutPlanes", cutPlanes)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid parameter: cutPlanes.");
    }

    GS::Optional<short> customPen;
    GS::Optional<API_AttributeIndex> customSurface;
    GS::Optional<GS::UniString> inputError = GetPenAndSurface (parameters, "customPen", "customSurfaceId", customPen, customSurface);
    GS::Array<GS::Optional<short>> cutPens;
    GS::Array<GS::Optional<API_AttributeIndex>> cutSurfaces;
    for (UIndex i = 0; i < cutPlanes.GetSize () && !inputError.HasValue (); ++i) {
        cutPens.Push (GS::NoValue);
        cutSurfaces.Push (GS::NoValue);
        inputError = GetPenAndSurface (cutPlanes[i], "cutPen", "cutSurfaceId", cutPens[i], cutSurfaces[i]);
    }
    if (inputError.HasValue ()) {
        return CreateFailedExecutionResult (APIERR_BADPARS, *inputError);
    }

    // Planes changed while 3D Cutaway is off are dropped when it is turned on, so it is turned on first.
    if (!cutPlanes.IsEmpty ()) {
        const GSErrCode cutawayErr = Set3DCutawayEnabled (true);
        if (cutawayErr != NoError && cutawayErr != APIERR_NOTSUPPORTED) {
            return CreateFailedExecutionResult (cutawayErr, "Failed to turn 3D Cutaway on. Is the 3D window open?");
        }
    }

    GSErrCode err = ACAPI_CallUndoableCommand ("Set3DCutPlanesCommand", [&]() -> GSErrCode {
        API_3DCutPlanesInfo cutInfo {};
        GSErrCode err = ACAPI_View_Get3DCuttingPlanes (&cutInfo);
        if (err != NoError) {
            return err;
        }

        // The current planes, whose pen and surface the new planes keep, so their cut faces are not drawn with pen and surface 0.
        GS::Array<API_3DCutShapeType> currentShapes;
        if (cutInfo.shapes != nullptr) {
            for (short i = 0; i < cutInfo.nShapes; ++i) {
                currentShapes.Push ((*cutInfo.shapes)[i]);
            }
            BMKillHandle ((GSHandle*) &(cutInfo.shapes));
        }

        bool useCustom = false;
        if (parameters.Get ("useCustom", useCustom)) {
            cutInfo.useCustom = useCustom;
        }
        if (customPen.HasValue ()) {
            cutInfo.customPen = *customPen;
        }
        if (customSurface.HasValue ()) {
            cutInfo.customMater = *customSurface;
        }

        cutInfo.nShapes = (short) cutPlanes.GetSize ();
        if (cutPlanes.IsEmpty ()) {
            // no cut planes: switch the cut off
            cutInfo.isCutPlanes = false;
            cutInfo.shapes = nullptr;
            return ACAPI_View_Change3DCuttingPlanes (&cutInfo);
        }

        cutInfo.isCutPlanes = true;
        cutInfo.shapes = reinterpret_cast<API_3DCutShapeType**> (BMAllocateHandle (cutInfo.nShapes * sizeof (API_3DCutShapeType), ALLOCATE_CLEAR, 0));
        if (cutInfo.shapes == nullptr) {
            return APIERR_MEMFULL;
        }

        for (UIndex i = 0; i < cutPlanes.GetSize (); ++i) {
            API_3DCutShapeType& shape = (*cutInfo.shapes)[i];
            if (!currentShapes.IsEmpty ()) {
                shape = currentShapes[i < currentShapes.GetSize () ? i : currentShapes.GetSize () - 1];
            } else {
                shape.cutPen = cutInfo.customPen;
                shape.cutMater = cutInfo.customMater;
            }
            cutPlanes[i].Get ("pa", shape.pa);
            cutPlanes[i].Get ("pb", shape.pb);
            cutPlanes[i].Get ("pc", shape.pc);
            cutPlanes[i].Get ("pd", shape.pd);
            if (cutPens[i].HasValue ()) {
                shape.cutPen = *cutPens[i];
            }
            if (cutSurfaces[i].HasValue ()) {
                shape.cutMater = *cutSurfaces[i];
            }
        }

        err = ACAPI_View_Change3DCuttingPlanes (&cutInfo);

        BMKillHandle ((GSHandle*) &(cutInfo.shapes));
        return err;
    });

    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to set 3D cut planes.");
    }

    // The 3D window keeps cutting with the planes in force when 3D Cutaway was last turned on, so it is
    // turned off and on again to put the new planes in force; with no planes it stays off.
    GSErrCode cutawayErr = Set3DCutawayEnabled (false);
    if (cutawayErr == NoError && !cutPlanes.IsEmpty ()) {
        cutawayErr = Set3DCutawayEnabled (true);
    }
    if (cutawayErr != NoError && cutawayErr != APIERR_NOTSUPPORTED) {
        return CreateFailedExecutionResult (cutawayErr, "The cut planes were set, but 3D Cutaway could not be turned off and on to put them in force.");
    }
    return CreateSuccessfulExecutionResult ();
}


GS::String Get3DCutPlanesCommand::GetName () const
{
    return "Get3DCutPlanes";
}


Get3DCutPlanesCommand::Get3DCutPlanesCommand () :
    CommandBase (CommonSchema::Used)
{}


GS::Optional<GS::UniString> Get3DCutPlanesCommand::GetRawResponseSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "isCutPlanes": {
            "type": "boolean",
            "description": "Whether the 3D cut planes are switched on."
        },
        "useCustom": {
            "type": "boolean",
            "description": "Whether the cut faces of the elements take customPen and customSurfaceId."
        },
        "customPen": {
            "type": "integer",
            "description": "The pen of the cut edges when useCustom is true."
        },
        "customSurfaceId": {
            "$ref": "#/AttributeId",
            "description": "The surface of the cut faces when useCustom is true. Missing when it is no surface attribute."
        },
        "cutPlanes": {
            "type": "array",
            "description": "The cut planes, in the form accepted by Set3DCutPlanes: pa*x + pb*y + pc*z = pd, pd in meters.",
            "items": {
                "$ref": "#/CutPlane"
            }
        }
    },
    "additionalProperties": false,
    "required": ["isCutPlanes", "useCustom", "customPen", "cutPlanes"]
})";
}


// Adds the surface as an #/AttributeId, or nothing when the index is no surface (e.g. 0).
static void AddSurfaceId (GS::ObjectState& os, const char* field, API_AttributeIndex surface)
{
    const API_Guid guid = GetAttributeGuidFromIndex (API_MaterialID, surface);
    if (guid != APINULLGuid) {
        os.Add (field, CreateGuidObjectState (guid));
    }
}


GS::ObjectState Get3DCutPlanesCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    API_3DCutPlanesInfo cutInfo {};

    GSErrCode err = ACAPI_View_Get3DCuttingPlanes (&cutInfo);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get 3D cut planes.");
    }

    GS::ObjectState response;
    response.Add ("isCutPlanes", (bool) cutInfo.isCutPlanes);
    response.Add ("useCustom", (bool) cutInfo.useCustom);
    response.Add ("customPen", (Int32) cutInfo.customPen);
    AddSurfaceId (response, "customSurfaceId", cutInfo.customMater);
    const auto& cutPlanes = response.AddList<GS::ObjectState> ("cutPlanes");
    if (cutInfo.shapes != nullptr) {
        for (int i = 0; i < cutInfo.nShapes; ++i) {
            GS::ObjectState plane;
            plane.Add ("pa", (*cutInfo.shapes)[i].pa);
            plane.Add ("pb", (*cutInfo.shapes)[i].pb);
            plane.Add ("pc", (*cutInfo.shapes)[i].pc);
            plane.Add ("pd", (*cutInfo.shapes)[i].pd);
            plane.Add ("cutPen", (Int32) (*cutInfo.shapes)[i].cutPen);
            AddSurfaceId (plane, "cutSurfaceId", (*cutInfo.shapes)[i].cutMater);
            cutPlanes (plane);
        }
        BMKillHandle ((GSHandle*) &(cutInfo.shapes));
    }

    return response;
}