using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateBeamsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateBeams";

        public CreateBeamsComponent()
            : base(
                "CreateBeams",
                "Create Beam elements based on the given parameters. " +
                "The Z coordinate of each line's start point is used as the beam's Z coordinate.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            inManager.AddLineParameter(
                "Lines",
                "Lines",
                "Reference lines of the beams (start and end point; the start point's Z is used as the beam's Z coordinate).",
                GH_ParamAccess.list);

            InTexts(
                "AdditionalSettings",
                "One JSON object per beam with further optional settings " +
                "(e.g. {\"width\":0.3,\"height\":0.5,\"floorIndex\":0,\"offset\":0," +
                "\"slantAngle\":0,\"arcAngle\":0,\"anchorPoint\":\"Center\"}). " +
                "Input only 1 to use the same settings for all. Optional.");

            SetOptionality(1);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<Line> lines))
            {
                return;
            }

            da.TryGetList(
                1,
                out List<string> additionalSettings);
            additionalSettings = additionalSettings ?? new List<string>();
            if (additionalSettings.Count > 1 &&
                additionalSettings.Count != lines.Count)
            {
                this.AddError(
                    "The size of the input AdditionalSettings must be 0, 1 or equal to the size of the input Lines.");
                return;
            }

            var items = new JArray();
            for (var i = 0; i < lines.Count; i++)
            {
                var line = lines[i];
                var item = new JObject
                {
                    ["begCoordinate"] = new JObject
                    {
                        ["x"] = line.From.X,
                        ["y"] = line.From.Y
                    },
                    ["endCoordinate"] = new JObject
                    {
                        ["x"] = line.To.X,
                        ["y"] = line.To.Y
                    },
                    ["zCoordinate"] = line.From.Z
                };

                if (additionalSettings.Count > 0)
                {
                    var json = additionalSettings[additionalSettings.Count == 1 ? 0 : i];
                    try
                    {
                        item.Merge(
                            JObject.Parse(json),
                            new JsonMergeSettings
                            {
                                MergeArrayHandling = MergeArrayHandling.Replace
                            });
                    }
                    catch (Exception ex)
                    {
                        this.AddError(
                            $"Invalid JSON in the AdditionalSettings input: {ex.Message}");
                        return;
                    }
                }

                items.Add(item);
            }

            var parameters = new JObject { ["beamsData"] = items };

            TryGetCadResponse(
                CommandName,
                parameters,
                ToAddOn,
                out _);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateBeams;

        public override Guid ComponentGuid =>
            new Guid("257a44ff-33c2-43c0-9fca-e587cfddec78");
    }
}
