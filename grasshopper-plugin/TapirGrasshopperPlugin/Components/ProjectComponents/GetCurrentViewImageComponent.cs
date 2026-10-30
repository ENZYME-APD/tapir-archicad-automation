using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetCurrentViewImageComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetCurrentViewImage";

        public GetCurrentViewImageComponent()
            : base(
                "GetCurrentViewImage",
                "Get what the 3D window, or the PhotoRendering window, shows as an image, " +
                "without writing a file. Available from Archicad 30; the window must be open.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InTextWithDefault(
                "Source",
                "The window whose image is returned: 3DWindow or RenderingWindow.",
                "3DWindow");

            InTextWithDefault(
                "Format",
                "The image format: png or jpg.",
                "png");
        }

        protected override void AddOutputs()
        {
            OutGeneric(
                "Image",
                "The image of the window as a bitmap.");

            OutText(
                "Base64",
                "The base64 encoded image, in the requested format.");

            OutInteger(
                "Width",
                "The width of the image in pixels.");

            OutInteger(
                "Height",
                "The height of the image in pixels.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            da.TryGet(0, out string source);
            da.TryGet(1, out string format);

            var parameters = new JObject
            {
                ["source"] = string.IsNullOrWhiteSpace(source) ? "3DWindow" : source.Trim(),
                ["format"] = string.IsNullOrWhiteSpace(format) ? "png" : format.Trim()
            };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var base64 = response["image"]?.ToString();
            if (string.IsNullOrEmpty(base64))
            {
                this.AddError("The response contains no image.");
                return;
            }

            var bitmap = ImageHelp.FromBase64(base64);
            if (bitmap == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Remark,
                    "Bitmap decoding is not available on this platform; use the Base64 output.");
            }

            da.SetData(
                0,
                bitmap);

            da.SetData(
                1,
                base64);

            da.SetData(
                2,
                response.Value<int?>("width"));

            da.SetData(
                3,
                response.Value<int?>("height"));
        }

        // Reuses the icon of GetElementPreviewImage until this command gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetElementPreviewImage;

        public override Guid ComponentGuid =>
            new Guid("7d3e5c2a-9b41-4f68-a0c5-2e8f6b1d4a93");
    }
}
