using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Drawing;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetRoomImageComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetRoomImage";

        public GetRoomImageComponent()
            : base(
                "GetRoomImage",
                "Get the room image of the given zone.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "ZoneGuid",
                "Identifier of the zone.");

            InTextWithDefault(
                "Format",
                "The image format: png or jpg.",
                "png");

            InInteger(
                "Width",
                "The width of the image in pixels.",
                256);

            InInteger(
                "Height",
                "The height of the image in pixels.",
                256);

            InNumber(
                "Offset",
                "Offset of the clip polygon from the edge of the zone.",
                0.001);

            InNumber(
                "Scale",
                "Scale of the view (e.g. 0.005 for 1:200).",
                0.005);

            InColor(
                "BackgroundColor",
                "Background color of the generated image.",
                Color.White);
        }

        protected override void AddOutputs()
        {
            OutGeneric(
                "Image",
                "The room image as a bitmap.");

            OutText(
                "Base64",
                "The base64 encoded room image.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out GH_ObjectWrapper zoneWrapper))
            {
                return;
            }

            var zoneId = GuidObject<ElementGuid>.CreateFromWrapper(zoneWrapper);
            if (zoneId == null)
            {
                this.AddError("Invalid ZoneGuid.");
                return;
            }

            da.TryGet(1, out string format);
            da.TryGet(2, out int width);
            da.TryGet(3, out int height);
            da.TryGet(4, out double offset);
            da.TryGet(5, out double scale);
            da.TryGet(6, out Color backgroundColor);

            var parameters = new JObject
            {
                ["zoneId"] = new JObject { ["guid"] = zoneId.Guid },
                ["format"] = string.IsNullOrEmpty(format) ? "png" : format,
                ["width"] = width,
                ["height"] = height,
                ["offset"] = offset,
                ["scale"] = scale,
                ["backgroundColor"] = new JObject
                {
                    ["red"] = backgroundColor.R / 255.0,
                    ["green"] = backgroundColor.G / 255.0,
                    ["blue"] = backgroundColor.B / 255.0
                }
            };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var base64 = response["roomImage"]?.ToString();
            if (string.IsNullOrEmpty(base64))
            {
                this.AddError("The response contains no room image.");
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
            Properties.Resources.GetRoomImage;

        public override Guid ComponentGuid =>
            new Guid("3291c2d7-dd42-4661-8452-392e4012885f");
    }
}
