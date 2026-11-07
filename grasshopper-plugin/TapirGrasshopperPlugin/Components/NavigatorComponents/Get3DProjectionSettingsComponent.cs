using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class Get3DProjectionSettingsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "Get3DProjectionSettings";

        public Get3DProjectionSettingsComponent()
            : base(
                "Get3DProjectionSettings",
                "Get the 3D projection settings of the 3D window: whether it is perspective or " +
                "axonometric, the camera and target position, view cone, roll angle, distance and " +
                "azimuth of a perspective, or the transformation matrix of an axonometry.",
                GroupNames.Navigator)
        {
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "IsPerspective",
                "True for a perspective projection, false for an axonometric one.");

            OutGeneric(
                "CameraElementGuid",
                "The Camera element the projection was set from. Empty when the projection does not come from a camera.");

            OutGeneric(
                "ActiveCameraSetElementGuid",
                "The active Camera Set element. Empty when there is none.");

            OutPointItem(
                "CameraPosition",
                "Position of the camera. Perspective only.");

            OutPointItem(
                "TargetPosition",
                "Position of the target point the camera looks at. Perspective only.");

            OutNumberItem(
                "Distance",
                "Distance between the camera and the target. Perspective only.");

            OutNumberItem(
                "Azimuth",
                "Azimuth angle of the camera. Perspective only.");

            OutNumberItem(
                "ViewCone",
                "View cone angle of the camera. Perspective only.");

            OutNumberItem(
                "RollAngle",
                "Roll angle of the camera. Perspective only.");

            OutBoolean(
                "IsTwoPointPerspective",
                "True for a two-point perspective (vertical edges stay vertical). Perspective only.");

            OutNumberList(
                "TransformationMatrix",
                "The 3x4 transformation matrix of the axonometric projection, row by row (12 numbers). Axonometry only.");
        }

        private void OutPointItem(
            string name,
            string description)
        {
            outManager.AddPointParameter(
                name,
                name,
                description.WithTypeName("pointItem"),
                GH_ParamAccess.item);
        }

        private void OutNumberItem(
            string name,
            string description)
        {
            outManager.AddNumberParameter(
                name,
                name,
                description.WithTypeName("numberItem"),
                GH_ParamAccess.item);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(
                0,
                response.Value<bool?>("isPersp") ?? false);

            var cameraGuid = JsonOutputHelp.GuidOf(response["cameraElementId"]);
            if (!string.IsNullOrEmpty(cameraGuid))
            {
                da.SetData(
                    1,
                    new ElementGuidWrapper { ElementId = ElementGuid.FromString(cameraGuid) });
            }

            var cameraSetGuid = JsonOutputHelp.GuidOf(response["activeCameraSetElementId"]);
            if (!string.IsNullOrEmpty(cameraSetGuid))
            {
                da.SetData(
                    2,
                    new ElementGuidWrapper { ElementId = ElementGuid.FromString(cameraSetGuid) });
            }

            if (response["perspective"] is JObject perspective)
            {
                da.SetData(3, JsonOutputHelp.Point(perspective["cameraPosition"]));
                da.SetData(4, JsonOutputHelp.Point(perspective["targetPosition"]));
                da.SetData(5, perspective.Value<double?>("distance"));
                da.SetData(6, perspective.Value<double?>("azimuth"));
                da.SetData(7, perspective.Value<double?>("viewCone"));
                da.SetData(8, perspective.Value<double?>("rollAngle"));
                da.SetData(9, perspective.Value<bool?>("isTwoPointPersp"));
            }

            if (response["axonometric"]?["transformationMatrix"] is JArray matrix)
            {
                da.SetDataList(
                    10,
                    matrix.Select(v => v.Value<double>()).ToList());
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.Get3DProjectionSettings;

        public override Guid ComponentGuid =>
            new Guid("6e333573-e115-49c1-936f-34374ada6c92");
    }
}
