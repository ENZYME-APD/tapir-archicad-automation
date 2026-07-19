using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateWallsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateWalls";

        public CreateWallsComponent()
            : base(
                "CreateWalls",
                "Create Wall elements based on the given parameters. " +
                "The Z coordinate of each line's start point is used as the wall's Z coordinate.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            inManager.AddLineParameter(
                "Lines",
                "Lines",
                "Reference lines of the walls (start and end point; the start point's Z is used as the wall's Z coordinate).",
                GH_ParamAccess.list);

            InNumbers(
                "Heights",
                "Height of each wall (input only 1 to use the same height for all).");

            InNumbers(
                "Thicknesses",
                "Thickness of each wall (input only 1 to use the same thickness for all).");

            InTexts(
                "AdditionalSettings",
                "One JSON object per wall with further optional settings " +
                "(e.g. {\"floorIndex\":0,\"offset\":0,\"arcAngle\":0,\"referenceLineLocation\":\"Center\"," +
                "\"structureType\":\"Basic\",\"buildingMaterialId\":{\"guid\":\"...\"}}). " +
                "Input only 1 to use the same settings for all. Optional.");

            SetOptionality(3);
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

            if (!da.TryGetList(
                    1,
                    out List<double> heights))
            {
                return;
            }

            if (!da.TryGetList(
                    2,
                    out List<double> thicknesses))
            {
                return;
            }

            if (heights.Count != 1 &&
                heights.Count != lines.Count)
            {
                this.AddError(
                    "The size of the input Heights must be 1 or equal to the size of the input Lines.");
                return;
            }

            if (thicknesses.Count != 1 &&
                thicknesses.Count != lines.Count)
            {
                this.AddError(
                    "The size of the input Thicknesses must be 1 or equal to the size of the input Lines.");
                return;
            }

            da.TryGetList(
                3,
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
                    ["zCoordinate"] = line.From.Z,
                    ["height"] = heights[heights.Count == 1 ? 0 : i],
                    ["thickness"] = thicknesses[thicknesses.Count == 1 ? 0 : i]
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

            var parameters = new JObject { ["wallsData"] = items };

            TryGetCadResponse(
                CommandName,
                parameters,
                ToAddOn,
                out _);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateWalls;

        public override Guid ComponentGuid =>
            new Guid("2dd6d5d2-a759-40fb-b3b2-6469ea675b67");
    }
}
