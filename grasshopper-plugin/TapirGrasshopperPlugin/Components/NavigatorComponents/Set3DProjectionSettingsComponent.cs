using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class Set3DProjectionSettingsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "Set3DProjectionSettings";

        public Set3DProjectionSettingsComponent()
            : base(
                "Set3DProjectionSettings",
                "Set a perspective projection for the 3D window. Every perspective parameter is " +
                "required: read a known good set with Get3DProjectionSettings and pass it back, " +
                "with the values to change replaced.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InPointItem(
                "CameraPosition",
                "Position of the camera.");

            InPointItem(
                "TargetPosition",
                "Position of the target point the camera looks at.");

            InNumberItem(
                "Distance",
                "Distance between the camera and the target.");

            InNumberItem(
                "Azimuth",
                "Azimuth angle of the camera.");

            InNumberItem(
                "ViewCone",
                "View cone angle of the camera.");

            InNumberItem(
                "RollAngle",
                "Roll angle of the camera.");

            InBoolean(
                "IsTwoPointPerspective",
                "True for a two-point perspective (vertical edges stay vertical).");
        }

        private void InPointItem(
            string name,
            string description)
        {
            inManager.AddPointParameter(
                name,
                name,
                description.WithTypeName("pointItem"),
                GH_ParamAccess.item);
        }

        private void InNumberItem(
            string name,
            string description)
        {
            inManager.AddNumberParameter(
                name,
                name,
                description.WithTypeName("numberItem"),
                GH_ParamAccess.item);
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult.Message),
                ExecutionResult.Doc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(0, out Point3d cameraPosition) ||
                !da.TryGet(1, out Point3d targetPosition) ||
                !da.TryGet(2, out double distance) ||
                !da.TryGet(3, out double azimuth) ||
                !da.TryGet(4, out double viewCone) ||
                !da.TryGet(5, out double rollAngle) ||
                !da.TryGet(6, out bool isTwoPointPersp))
            {
                return;
            }

            var parameters = new JObject
            {
                ["perspective"] = new JObject
                {
                    ["cameraPosition"] = ToCoordinate3D(cameraPosition),
                    ["targetPosition"] = ToCoordinate3D(targetPosition),
                    ["distance"] = distance,
                    ["azimuth"] = azimuth,
                    ["viewCone"] = viewCone,
                    ["rollAngle"] = rollAngle,
                    ["isTwoPointPersp"] = isTwoPointPersp
                }
            };

            if (!TryGetConvertedCadValues(
                    CommandName,
                    parameters,
                    ToAddOn,
                    ExecutionResult.Deserialize,
                    out ExecutionResult response))
            {
                return;
            }

            if (!response.Success)
            {
                this.AddError(response.Message());
            }

            da.SetData(0, response.Message());
        }

        private static JObject ToCoordinate3D(
            Point3d point)
        {
            return new JObject
            {
                ["x"] = point.X,
                ["y"] = point.Y,
                ["z"] = point.Z
            };
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.Set3DProjectionSettings;

        public override Guid ComponentGuid =>
            new Guid("b8991fbb-6654-4cc1-a5e7-9e1c7a37b4ca");
    }
}
