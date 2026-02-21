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
    CommandBase (CommonSchema::NotUsed)
{}


static GSErrCode _Set3DCutPlanes (const GS::Array<GS::ObjectState>& cutPlanes)
{
    API_3DCutPlanesInfo cutInfo {};

    GSErrCode err = ACAPI_View_Get3DCuttingPlanes (&cutInfo);
    if (err == NoError) {
        if (cutInfo.shapes != nullptr)
            BMKillHandle ((GSHandle*) &(cutInfo.shapes));

        cutInfo.isCutPlanes = true;
        cutInfo.useCustom = false;
        cutInfo.nShapes = (short) cutPlanes.GetSize ();
        cutInfo.shapes = reinterpret_cast<API_3DCutShapeType**> (BMAllocateHandle (cutInfo.nShapes * sizeof (API_3DCutShapeType), ALLOCATE_CLEAR, 0));
        if (cutInfo.shapes != nullptr) {
            // it is possible to define further attributes like cutStatus and cutPen
            // you can look it up here:
            // https://graphisoft.github.io/archicad-api-devkit/struct_a_p_i__3_d_cut_shape_type.html
            // for now, we only pass coordinates
            if (!cutPlanes.IsEmpty ()) {
                for (int i = 0; i < cutInfo.nShapes; ++i) {
                    cutPlanes[i].Get ("pa", (*cutInfo.shapes)[i].pa);
                    cutPlanes[i].Get ("pb", (*cutInfo.shapes)[i].pb);
                    cutPlanes[i].Get ("pc", (*cutInfo.shapes)[i].pc);
                    cutPlanes[i].Get ("pd", (*cutInfo.shapes)[i].pd);
                }
            } else {
                return APIERR_BADPARS;;
            }
        } else {
            return APIERR_MEMFULL;
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
            "minItems": 1,
            "items": {
                "type": "object",
                "description": "Defines a 3D cut plane using the plane equation: pa*x + pb*y + pc*z + pd = 0",
                "properties": {
                    "pa": {
                        "type": "number",
                        "description": "Coefficient of x in the plane equation. The x coordinate of the normal vector of the plane."
                    },
                    "pb": {
                        "type": "number",
                        "description": "Coefficient of y in the plane equation. The y coordinate of the normal vector of the plane."
                    },
                    "pc": {
                        "type": "number",
                        "description": "Coefficient of z in the plane equation. The z coordinate of the normal vector of the plane."
                    },
                    "pd": {
                        "type": "number",
                        "description": "Constant term in the plane equation. The distance of the plane from the origin along the normal vector."
                    }
                },
                "required": [
                    "pa",
                    "pb",
                    "pc",
                    "pd"
                ],
                "additionalProperties": false
            }
        }
    },
    "additionalProperties": false
})";
}


GS::Optional<GS::UniString> Set3DCutPlanesCommand::GetResponseSchema () const
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