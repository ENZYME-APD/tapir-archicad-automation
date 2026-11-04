using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.UtilitiesComponents
{
    public class ConvertImagesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "ConvertImages";

        public ConvertImagesComponent()
            : base(
                "ConvertImages",
                "Convert base64 encoded images from one image format to another with Archicad's own image " +
                "converters, for example a PNG preview image to JPEG. The formats are Bitmap, GIF, TIFF, JPEG and " +
                "PNG, or the MIME type of another format Archicad can read or write.",
                GroupNames.Utilities)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Images",
                "The base64 encoded images, for example the output of GetElementPreviewImage.");

            InTexts(
                "InputFormats",
                "The format of each image: Bitmap, GIF, TIFF, JPEG, PNG or a MIME type (input only 1 to use the " +
                "same format for all).");

            InTexts(
                "OutputFormats",
                "The format to convert each image to: Bitmap, GIF, TIFF, JPEG, PNG or a MIME type (input only 1 " +
                "to use the same format for all).");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Images",
                "The base64 encoded converted images, aligned with the input (null where the conversion failed).");

            OutErrorMessages(
                "Error message of each image (empty when it was converted).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> images) ||
                !da.TryGetList(
                    1,
                    out List<string> inputFormats) ||
                !da.TryGetList(
                    2,
                    out List<string> outputFormats) ||
                !this.CheckSizes(
                    images.Count,
                    "Images",
                    ("InputFormats", inputFormats.Count),
                    ("OutputFormats", outputFormats.Count)))
            {
                return;
            }

            var items = new JArray();
            for (var i = 0; i < images.Count; i++)
            {
                items.Add(
                    new JObject
                    {
                        ["image"] = images[i],
                        ["inputFormat"] = inputFormats.At(i),
                        ["outputFormat"] = outputFormats.At(i)
                    });
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["images"] = items },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var converted = new List<string>();
            var errors = new List<string>();
            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "images"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    converted.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                converted.Add(item["image"]?.ToString());
                errors.Add("");
            }

            da.SetDataList(0, converted);
            da.SetDataList(1, errors);
        }

        // Shares the icon of GetElementPreviewImage, whose images it converts.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetElementPreviewImage;

        public override Guid ComponentGuid =>
            new Guid("134c3298-4457-4e2d-8c12-4cd5a74a7bc8");
    }
}
