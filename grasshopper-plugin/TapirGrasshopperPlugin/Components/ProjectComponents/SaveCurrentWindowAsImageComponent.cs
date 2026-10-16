using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class SaveCurrentWindowAsImageComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SaveCurrentWindowAsImage";

        public SaveCurrentWindowAsImageComponent()
            : base(
                "SaveCurrentWindowAsImage",
                "Save what the current window shows as an image file (png, jpg, bmp, tiff or gif), " +
                "as File > Save as does with a picture format. Only model, layout and 3D windows " +
                "can be saved.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "ImageFilePath",
                "Absolute path of the image file to write. An existing file is overwritten.");

            InText(
                "Format",
                "Optional. The image file format: png, jpg, bmp, tiff or gif. When it is not " +
                "given, it comes from the extension of ImageFilePath (.png, .jpg or .jpeg, " +
                ".bmp, .tif or .tiff, .gif).");

            InText(
                "ColorDepth",
                "Optional, trueColor by default. The color depth of the image, as in the Save as " +
                "picture dialog: blackAndWhite, 16Colors, 256Colors, 16Grays, 256Grays, " +
                "trueColor or trueColorWithAlpha.");

            InBoolean(
                "Dithered",
                "Whether the image is dithered when its color depth drops colors.",
                false);

            InBoolean(
                "EntireDrawing",
                "Whether the image covers the entire drawing of a 2D window instead of its " +
                "current zoom, as the Entire Drawing option of the Save as picture dialog does.",
                false);

            SetOptionality(new[] { 1, 2 });
        }

        protected override void AddOutputs()
        {
            OutText(
                "ImageFilePath",
                "Path of the written image file.");

            OutPointItem(
                "ZoomMin",
                "Minimum corner of the zoom box of the window the image shows, as " +
                "GetView2DTransformations reports it. Empty for the 3D window.");

            OutPointItem(
                "ZoomMax",
                "Maximum corner of the zoom box of the window the image shows, as " +
                "GetView2DTransformations reports it. Empty for the 3D window.");

            OutNumberItem(
                "Rotation",
                "Orientation of the window in radians, as GetView2DTransformations reports it. " +
                "Empty for the 3D window.");
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
            if (!da.TryGet(
                    0,
                    out string imageFilePath) ||
                string.IsNullOrWhiteSpace(imageFilePath))
            {
                return;
            }

            var parameters = new JObject
            {
                ["imageFilePath"] = imageFilePath,
                ["dithered"] = da.GetOptional(3, false),
                ["entireDrawing"] = da.GetOptional(4, false)
            };

            var format = da.GetOptional<string>(1, null);
            if (!string.IsNullOrWhiteSpace(format))
            {
                parameters["format"] = format.Trim();
            }

            var colorDepth = da.GetOptional<string>(2, null);
            if (!string.IsNullOrWhiteSpace(colorDepth))
            {
                parameters["colorDepth"] = colorDepth.Trim();
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(
                0,
                response["imageFilePath"]?.ToString());

            if (response["zoom"] is JObject zoom)
            {
                da.SetData(
                    1,
                    new Point3d(
                        zoom.Value<double?>("xMin") ?? 0.0,
                        zoom.Value<double?>("yMin") ?? 0.0,
                        0.0));
                da.SetData(
                    2,
                    new Point3d(
                        zoom.Value<double?>("xMax") ?? 0.0,
                        zoom.Value<double?>("yMax") ?? 0.0,
                        0.0));
            }

            da.SetData(
                3,
                response.Value<double?>("rotation"));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SaveCurrentWindowAsImage;

        public override Guid ComponentGuid =>
            new Guid("b51a2af5-64fe-41cb-b393-c16ff7db490c");
    }
}
