using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateColumnsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateColumns";

        public CreateColumnsComponent()
            : base(
                "CreateColumns",
                "Create Column elements based on the given parameters.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InPoints(
                "Points",
                "Insertion points of the columns.");

            InTexts(
                "AdditionalSettings",
                "One JSON object per column with further optional settings " +
                "(e.g. {\"height\":3.0,\"width\":0.4,\"depth\":0.4," +
                "\"axisRotationAngle\":0,\"coreAnchor\":\"Center\",\"floorIndex\":0}). " +
                "Input only 1 to use the same settings for all. Optional.");

            SetOptionality(1);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<Point3d> points))
            {
                return;
            }

            da.TryGetList(
                1,
                out List<string> additionalSettings);
            additionalSettings = additionalSettings ?? new List<string>();
            if (additionalSettings.Count > 1 &&
                additionalSettings.Count != points.Count)
            {
                this.AddError(
                    "The size of the input AdditionalSettings must be 0, 1 or equal to the size of the input Points.");
                return;
            }

            var items = new JArray();
            for (var i = 0; i < points.Count; i++)
            {
                var item = new JObject
                {
                    ["coordinates"] = new JObject
                    {
                        ["x"] = points[i].X,
                        ["y"] = points[i].Y,
                        ["z"] = points[i].Z
                    }
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

            var parameters = new JObject { ["columnsData"] = items };

            TryGetCadResponse(
                CommandName,
                parameters,
                ToAddOn,
                out _);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateColumns;

        public override Guid ComponentGuid =>
            new Guid("6d34d401-1119-497a-9014-ac05345973ff");
    }
}
