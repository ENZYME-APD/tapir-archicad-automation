using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateZonesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateZones";

        public CreateZonesComponent()
            : base(
                "CreateZones",
                "Create Zone elements based on the given parameters. Give either the zone polygons " +
                "(one branch per zone) for manually drawn zones, or the reference positions for " +
                "automatic zones detected from the surrounding walls. Category, stamp position and " +
                "further settings can be given through the AdditionalSettings input.",
                GroupNames.ElementCreation)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Name of each zone.");

            InTexts(
                "NumberStrs",
                "Number string of each zone. Input only 1 to use the same value for all zones.");

            inManager.AddPointParameter(
                "Vertices",
                "Vertices",
                "Outline points of each zone (one branch per zone, at least 3 points; only X and Y are used). Give either this or the ReferencePositions input.",
                GH_ParamAccess.tree);

            InPoints(
                "ReferencePositions",
                "Reference position of each automatic zone (only X and Y are used). Give either this or the Vertices input.");

            InTexts(
                "AdditionalSettings",
                "One JSON object per zone with further optional settings matching the " +
                "command's documented item schema. Input only 1 to use the same settings for all. Optional.");

            InTexts(
                "FavoriteNames",
                "Name of a favorite to base the new zone on. Its settings are applied first, " +
                "then the other inputs override them. Input only 1 to use the same favorite for all. Optional.");

            SetOptionality(new[] { 2, 3, 4, 5 });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifiers of the created zones (null for failed items).");

            OutTexts(
                "ErrorMessages",
                "Error message for each item (empty when the zone was created successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> names))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<string> numberStrs))
            {
                return;
            }

            var zoneCount = names.Count;
            if (zoneCount == 0)
            {
                this.AddError("The Names input must contain at least one item.");
                return;
            }

            if (numberStrs.Count != 1 && numberStrs.Count != zoneCount)
            {
                this.AddError(
                    "The size of the input NumberStrs must be 1 or equal to the size of the input Names.");
                return;
            }

            da.TryGetTree(
                2,
                out GH_Structure<GH_Point> polygons);
            var polygonBranchCount = polygons?.Branches.Count ?? 0;

            var referencePositions = new List<Rhino.Geometry.Point3d>();
            da.GetDataList(3, referencePositions);

            if ((polygonBranchCount == 0) == (referencePositions.Count == 0))
            {
                this.AddError(
                    "Give exactly one of the Vertices and ReferencePositions inputs.");
                return;
            }

            if (polygonBranchCount > 0 &&
                polygonBranchCount != 1 && polygonBranchCount != zoneCount)
            {
                this.AddError(
                    "The number of branches in the Vertices input must be 1 or equal to the size of the input Names.");
                return;
            }

            if (referencePositions.Count > 0 &&
                referencePositions.Count != 1 && referencePositions.Count != zoneCount)
            {
                this.AddError(
                    "The size of the input ReferencePositions must be 1 or equal to the size of the input Names.");
                return;
            }

            da.TryGetList(
                4,
                out List<string> additionalSettings);
            additionalSettings = additionalSettings ?? new List<string>();

            da.TryGetList(
                5,
                out List<string> favoriteNames);
            favoriteNames = favoriteNames ?? new List<string>();
            if (favoriteNames.Count > 1 &&
                favoriteNames.Count != zoneCount)
            {
                this.AddError(
                    "The size of the input FavoriteNames must be 0, 1 or equal to the size of the input Names.");
                return;
            }
            if (additionalSettings.Count > 1 &&
                additionalSettings.Count != zoneCount)
            {
                this.AddError(
                    "The size of the input AdditionalSettings must be 0, 1 or equal to the size of the input Names.");
                return;
            }

            var items = new JArray();
            for (var i = 0; i < zoneCount; i++)
            {
                JObject geometry;
                if (polygonBranchCount > 0)
                {
                    var branch = polygons.Branches[polygonBranchCount == 1 ? 0 : i];
                    if (branch.Count < 3)
                    {
                        this.AddError(
                            "Each zone polygon (branch) must contain at least 3 points.");
                        return;
                    }

                    var polygonCoordinates = new JArray();
                    foreach (var ghPoint in branch)
                    {
                        polygonCoordinates.Add(
                            new JObject
                            {
                                ["x"] = ghPoint.Value.X,
                                ["y"] = ghPoint.Value.Y
                            });
                    }

                    geometry = new JObject
                    {
                        ["polygonCoordinates"] = polygonCoordinates
                    };
                }
                else
                {
                    var position = referencePositions[referencePositions.Count == 1 ? 0 : i];
                    geometry = new JObject
                    {
                        ["referencePosition"] = new JObject
                        {
                            ["x"] = position.X,
                            ["y"] = position.Y
                        }
                    };
                }

                var item = new JObject
                {
                    ["name"] = names[i],
                    ["numberStr"] = numberStrs[numberStrs.Count == 1 ? 0 : i],
                    ["geometry"] = geometry
                };

                if (favoriteNames.Count > 0)
                {
                    var favoriteName = favoriteNames[favoriteNames.Count == 1 ? 0 : i];
                    if (!string.IsNullOrEmpty(favoriteName))
                    {
                        item["favoriteName"] = favoriteName;
                    }
                }

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

            var parameters = new JObject { ["zonesData"] = items };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            CreateElementsComponentBase.SetCreatedElementsOutputs(da, response, 0, 1);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateZones;

        public override Guid ComponentGuid =>
            new Guid("efb3d7fd-67db-487d-8419-b05946060943");
    }
}
