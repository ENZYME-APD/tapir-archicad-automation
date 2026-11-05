#include "GSUnID.hpp"
#include "3DCutPlaneCommands.hpp"
#include "MigrationHelper.hpp"

GS::String Set3DCutPlanesCommand::GetName () const
{
    return "Set3DCutPlanes";
}


// there is no 3DCutPlane scheme definition.
// I am not sure if it is needed, because it will not occur in other commands
Set3DCutPlanesCommand::Set3DCutPlanesCommand () :
    CommandBase (CommonSchema::Used)
{}


static GSErrCode _Set3DCutPlanes (const GS::Array<GS::ObjectState>& cutPlanes)
{
    API_3DCutPlanesInfo cutInfo {};

    GSErrCode err = ACAPI_View_Get3DCuttingPlanes (&cutInfo);
    if (err == NoError) {
        if (cutInfo.shapes != nullptr)
            BMKillHandle ((GSHandle*) &(cutInfo.shapes));

        cutInfo.useCustom = false;
        cutInfo.nShapes = (short) cutPlanes.GetSize ();
        if (cutPlanes.IsEmpty ()) {
            // no cut planes: switch the cut off
            cutInfo.isCutPlanes = false;
            cutInfo.shapes = nullptr;
            return ACAPI_View_Change3DCuttingPlanes (&cutInfo);
        }

        cutInfo.isCutPlanes = true;
        cutInfo.shapes = reinterpret_cast<API_3DCutShapeType**> (BMAllocateHandle (cutInfo.nShapes * sizeof (API_3DCutShapeType), ALLOCATE_CLEAR, 0));
        if (cutInfo.shapes == nullptr)
            return APIERR_MEMFULL;

        // it is possible to define further attributes like cutStatus and cutPen
        // you can look it up here:
        // https://graphisoft.github.io/archicad-api-devkit/struct_a_p_i__3_d_cut_shape_type.html
        // for now, we only pass coordinates
        for (int i = 0; i < cutInfo.nShapes; ++i) {
            cutPlanes[i].Get ("pa", (*cutInfo.shapes)[i].pa);
            cutPlanes[i].Get ("pb", (*cutInfo.shapes)[i].pb);
            cutPlanes[i].Get ("pc", (*cutInfo.shapes)[i].pc);
            cutPlanes[i].Get ("pd", (*cutInfo.shapes)[i].pd);
        }

        err = ACAPI_View_Change3DCuttingPlanes (&cutInfo);

        BMKillHandle ((GSHandle*) &(cutInfo.shapes));
    }
    return err;
}


GS::Optional<GS::UniString> Set3DCutPlanesCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "cutPlanes": {
            "type": "array",
            "description": "The cut planes. An empty array switches the 3D cut off. The side of a plane its normal vector points to is cut away and the other side is kept (to be confirmed in Archicad). pd is in meters.",
            "items": {
                "$ref": "#/CutPlane"
            }
        }
    },
    "additionalProperties": false
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

    GSErrCode err = ACAPI_CallUndoableCommand ("Set3DCutPlanesCommand", [&]() -> GSErrCode {
        return _Set3DCutPlanes (cutPlanes);
    });

    return err == NoError
        ? CreateSuccessfulExecutionResult ()
        : CreateFailedExecutionResult (err, "Failed to set 3D cut planes.");
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
            "description": "Whether the custom cut settings are used."
        },
        "cutPlanes": {
            "type": "array",
            "description": "The cut planes, in the form accepted by Set3DCutPlanes: pa*x + pb*y + pc*z + pd = 0, pd in meters.",
            "items": {
                "$ref": "#/CutPlane"
            }
        }
    },
    "additionalProperties": false,
    "required": ["isCutPlanes", "useCustom", "cutPlanes"]
})";
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
    const auto& cutPlanes = response.AddList<GS::ObjectState> ("cutPlanes");
    if (cutInfo.shapes != nullptr) {
        for (int i = 0; i < cutInfo.nShapes; ++i) {
            GS::ObjectState plane;
            plane.Add ("pa", (*cutInfo.shapes)[i].pa);
            plane.Add ("pb", (*cutInfo.shapes)[i].pb);
            plane.Add ("pc", (*cutInfo.shapes)[i].pc);
            plane.Add ("pd", (*cutInfo.shapes)[i].pd);
            cutPlanes (plane);
        }
        BMKillHandle ((GSHandle*) &(cutInfo.shapes));
    }

    return response;
}