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


GSErrCode _Set3DCutPlanes (GS::Array<GS::ObjectState>& shapeCoordinates)
{
    API_3DCutPlanesInfo cutInfo {};

    GSErrCode err = ACAPI_View_Get3DCuttingPlanes (&cutInfo);
    if (err == NoError) {
        if (cutInfo.shapes != nullptr)
            BMKillHandle ((GSHandle*) &(cutInfo.shapes));

        cutInfo.isCutPlanes = true;
        cutInfo.useCustom = false;
        cutInfo.nShapes = (short) shapeCoordinates.GetSize ();
        cutInfo.shapes = reinterpret_cast<API_3DCutShapeType**> (BMAllocateHandle (cutInfo.nShapes * sizeof (API_3DCutShapeType), ALLOCATE_CLEAR, 0));
        if (cutInfo.shapes != nullptr) {

            // it is possible to define further attributes like cutStatus and cutPen
            // you can look it up here:
            // https://graphisoft.github.io/archicad-api-devkit/struct_a_p_i__3_d_cut_shape_type.html
            // for now, we only pass coordinates
            if (!shapeCoordinates.IsEmpty ()) {
                for (int i = 0; i < cutInfo.nShapes; ++i) {
                    double pa;
                    double pb;
                    double pc;
                    double pd;

                    shapeCoordinates[i].Get ("pa", pa);
                    shapeCoordinates[i].Get ("pb", pb);
                    shapeCoordinates[i].Get ("pc", pc);
                    shapeCoordinates[i].Get ("pd", pd);

                    (*cutInfo.shapes)[i].pa = pa;
                    (*cutInfo.shapes)[i].pb = pb;
                    (*cutInfo.shapes)[i].pc = pc;
                    (*cutInfo.shapes)[i].pd = pd;
                }
            } else {
                return err;
            }
        } else {
            return err;
        }

        err = ACAPI_View_Change3DCuttingPlanes (&cutInfo);
        if (err == NoError) {
            API_WindowInfo windowInfo {};
            windowInfo.typeID = APIWind_3DModelID;
            err = ACAPI_Window_ChangeWindow (&windowInfo);
        }

        BMKillHandle ((GSHandle*) &(cutInfo.shapes));
    }
    return err;
}


GS::Optional<GS::UniString> Set3DCutPlanesCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "shapeCoordinates": {
            "type": "array",
            "items": {
                "type": "object",
                "properties": {
                    "pa": {
                        "type": "number"
                    },
                    "pb": {
                        "type": "number"
                    },
                    "pc": {
                        "type": "number"
                    },
                    "pd": {
                        "type": "number"
                    }
                },
                "required": [
                    "pa",
                    "pb",
                    "pc",
                    "pd"
                ],
                "minItems": 4,
                "maxItems": 4,
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
    GSErrCode err = NoError;

    GS::Array<GS::ObjectState> shapeCoordinates;
    if (!parameters.Get ("shapeCoordinates", shapeCoordinates)) {
        return CreateFailedExecutionResult (err, "Invalid parameter: shapeCoordinates.");
    }


    ACAPI_CallUndoableCommand ("Set3DCutPlanesCommand", [&]() -> GSErrCode {
        err = _Set3DCutPlanes (shapeCoordinates);
        return err;
    });

    return err == NoError
        ? CreateSuccessfulExecutionResult ()
        : CreateFailedExecutionResult (err, "Failed to set 3D cut planes.");
}