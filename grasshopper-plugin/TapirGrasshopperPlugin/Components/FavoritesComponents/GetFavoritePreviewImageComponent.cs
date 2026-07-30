using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.FavoritesComponents
{
    public class GetFavoritePreviewImageComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetFavoritePreviewImage";

        public GetFavoritePreviewImageComponent()
            : base(
                "GetFavoritePreviewImage",
                "Get the preview image of the given Favorite.",
                GroupNames.Favorites)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "FavoriteName",
                "The name of the Favorite.");

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
                    out string favorite) ||
                string.IsNullOrEmpty(favorite))
            {
                this.AddError("FavoriteName is required.");
                return;
            }

            da.TryGet(1, out string imageType);
            da.TryGet(2, out string format);
            da.TryGet(3, out int width);
            da.TryGet(4, out int height);

            var parameters = new JObject
            {
                ["favorite"] = favorite,
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
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetFavoritePreviewImage;

        public override Guid ComponentGuid =>
            new Guid("bdab0bc1-dae1-4d24-a13a-7c12bfadefce");
    }
}
