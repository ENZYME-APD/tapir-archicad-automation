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
                "Set a perspective or an axonometric projection for the 3D window. For a perspective " +
                "every perspective parameter is required: read a known good set with " +
                "Get3DProjectionSettings and pass it back, with the values to change replaced. For an " +
                "axonometry give AxonometryProjectionType, AxonometryAzimuth and/or TransformationMatrix " +
                "instead (and no perspective parameter); only the given values change. The 3D window " +
                "shows the change after a rebuild (RebuildView).",
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

            NavigatorComponentHelp.AddOptionalText(
                this,
                "AxonometryProjectionType",
                "Makes the projection axonometric with this projection type: YZ, XZ, XY, Frontal, Monometric, " +
                "Isometric, Dimetric, Bottom, FrontalBottom, MonometricBottom, IsometricBottom, DimetricBottom " +
                "or Custom (Custom keeps TransformationMatrix as given). Optional.");

            NavigatorComponentHelp.AddOptionalNumber(
                this,
                "AxonometryAzimuth",
                "Makes the projection axonometric with this camera azimuth in degrees, counterclockwise from " +
                "the X axis (270 looks from the south). Optional.");

            inManager.AddNumberParameter(
                "TransformationMatrix",
                "TransformationMatrix",
                ("Makes the projection axonometric with this 3x4 transformation matrix, row by row (12 numbers), " +
                 "as Get3DProjectionSettings returns it. Optional.").WithTypeName("numberList"),
                GH_ParamAccess.list);

            // The perspective inputs are only required for a perspective.
            SetOptionality(new[] { 0, 1, 2, 3, 4, 5, 6, 9 });
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
            var axonometry = new JObject();
            NavigatorComponentHelp.CopyOptional<string>(da, 7, axonometry, "projectionType");
            NavigatorComponentHelp.CopyOptional<double>(da, 8, axonometry, "azimuth");

            var matrix = da.OptionalList<double>(9);
            if (matrix.Count > 0)
            {
                if (matrix.Count != 12)
                {
                    this.AddError(
                        "The input TransformationMatrix must hold 12 numbers (a 3x4 matrix, row by row).");
                    return;
                }

                axonometry["transformationMatrix"] = new JArray(matrix);
            }

            if (axonometry.Count > 0)
            {
                var point = default(Point3d);
                var number = 0.0;
                if (da.GetData(0, ref point) ||
                    da.GetData(1, ref point) ||
                    da.GetData(2, ref number) ||
                    da.GetData(3, ref number) ||
                    da.GetData(4, ref number) ||
                    da.GetData(5, ref number))
                {
                    this.AddError(
                        "Give either the perspective inputs or the axonometry inputs (AxonometryProjectionType, " +
                        "AxonometryAzimuth, TransformationMatrix), not both.");
                    return;
                }

                SendProjection(
                    da,
                    new JObject { ["axonometry"] = axonometry });
                return;
            }

            if (!da.TryGet(0, out Point3d cameraPosition) ||
                !da.TryGet(1, out Point3d targetPosition) ||
                !da.TryGet(2, out double distance) ||
                !da.TryGet(3, out double azimuth) ||
                !da.TryGet(4, out double viewCone) ||
                !da.TryGet(5, out double rollAngle) ||
                !da.TryGet(6, out bool isTwoPointPersp))
            {
                this.AddError(
                    "Give every perspective input (CameraPosition, TargetPosition, Distance, Azimuth, ViewCone, " +
                    "RollAngle, IsTwoPointPerspective), or the axonometry inputs instead.");
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

            SendProjection(
                da,
                parameters);
        }

        private void SendProjection(
            IGH_DataAccess da,
            JObject parameters)
        {
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
