using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetElementPreviewImageComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetElementPreviewImage";

        public GetElementPreviewImageComponent()
            : base(
                "GetElementPreviewImage",
                "Get the preview image of the given element.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "ElementGuid",
                "Identifier of the element.");

            InTextWithDefault(
                "ImageType",
                "The type of the preview image: 2D, Section or 3D.",
                "3D");

            InTextWithDefault(
                "Format",
                "The image format: png or jpg.",
                "png");

            InInteger(
                "Width",
                "The width of the preview image in pixels.",
                128);

            InInteger(
                "Height",
                "The height of the preview image in pixels.",
                128);
        }

        protected override void AddOutputs()
        {
            OutGeneric(
                "Image",
                "The preview image as a bitmap.");

            OutText(
                "Base64",
                "The base64 encoded preview image.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out GH_ObjectWrapper elementWrapper))
            {
                return;
            }

            var elementId = GuidObject<ElementGuid>.CreateFromWrapper(elementWrapper);
            if (elementId == null)
            {
                this.AddError("Invalid ElementGuid.");
                return;
            }

            da.TryGet(1, out string imageType);
            da.TryGet(2, out string format);
            da.TryGet(3, out int width);
            da.TryGet(4, out int height);

            var parameters = new JObject
            {
                ["elementId"] = new JObject { ["guid"] = elementId.Guid },
                ["imageType"] = string.IsNullOrEmpty(imageType) ? "3D" : imageType,
                ["format"] = string.IsNullOrEmpty(format) ? "png" : format,
                ["width"] = width,
                ["height"] = height
            };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var base64 = response["previewImage"]?.ToString();
            if (string.IsNullOrEmpty(base64))
            {
                this.AddError("The response contains no preview image.");
                return;
            }

            da.SetData(
                0,
                ImageHelp.FromBase64(base64));

            da.SetData(
                1,
                base64);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetElementPreviewImage;

        public override Guid ComponentGuid =>
            new Guid("f62c15c8-ab86-422b-bc86-03adf0a483f5");
    }
}
