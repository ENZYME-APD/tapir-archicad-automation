using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateSlabsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateSlabs";

        public CreateSlabsComponent()
            : base(
                "CreateSlabs",
                "Create Slab elements based on the given parameters. " +
                "The polygon points are given as a tree with one branch per slab; " +
                "if no Levels are given, the Z coordinate of each branch's first point is used as the slab level.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            inManager.AddPointParameter(
                "Polygons",
                "Polygons",
                "Outline points of each slab (one branch per slab, at least 3 points; only X and Y are used).",
                GH_ParamAccess.tree);

            InNumbers(
                "Levels",
                "The Z coordinate of the reference line of each slab (input only 1 to use the same level for all). " +
                "Optional; defaults to the Z coordinate of each branch's first point.");

            InTexts(
                "AdditionalSettings",
                "One JSON object per slab with further optional settings " +
                "(e.g. {\"thickness\":0.2,\"referencePlaneLocation\":\"Top\",\"floorIndex\":0," +
                "\"holes\":[...],\"polygonArcs\":[...]}). " +
                "Input only 1 to use the same settings for all. Optional.");

            SetOptionality(new[] { 1, 2 });
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetTree(
                    0,
                    out GH_Structure<GH_Point> polygons))
            {
                return;
            }

            var slabCount = polygons.Branches.Count;
            if (slabCount == 0)
            {
                this.AddError("The Polygons input must contain at least one branch.");
                return;
            }

            da.TryGetList(
                1,
                out List<double> levels);
            levels = levels ?? new List<double>();
            if (levels.Count > 1 &&
                levels.Count != slabCount)
            {
                this.AddError(
                    "The size of the input Levels must be 0, 1 or equal to the number of branches in the Polygons input.");
                return;
            }

            da.TryGetList(
                2,
                out List<string> additionalSettings);
            additionalSettings = additionalSettings ?? new List<string>();
            if (additionalSettings.Count > 1 &&
                additionalSettings.Count != slabCount)
            {
                this.AddError(
                    "The size of the input AdditionalSettings must be 0, 1 or equal to the number of branches in the Polygons input.");
                return;
            }

            var items = new JArray();
            for (var i = 0; i < slabCount; i++)
            {
                var branch = polygons.Branches[i];
                if (branch.Count < 3)
                {
                    this.AddError(
                        "Each slab polygon (branch) must contain at least 3 points.");
                    return;
                }

                var polygon = new JArray();
                foreach (var ghPoint in branch)
                {
                    polygon.Add(
                        new JObject
                        {
                            ["x"] = ghPoint.Value.X,
                            ["y"] = ghPoint.Value.Y
                        });
                }

                double level;
                if (levels.Count > 0)
                {
                    level = levels[levels.Count == 1 ? 0 : i];
                }
                else
                {
                    level = branch[0].Value.Z;
                }

                var item = new JObject
                {
                    ["level"] = level,
                    ["polygonCoordinates"] = polygon
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

            var parameters = new JObject { ["slabsData"] = items };

            TryGetCadResponse(
                CommandName,
                parameters,
                ToAddOn,
                out _);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateSlabs;

        public override Guid ComponentGuid =>
            new Guid("6c7d2368-5d7f-4ef5-9313-03cb01d07ad5");
    }
}
