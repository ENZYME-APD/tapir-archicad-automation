using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class CreateDrawingsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateDrawings";

        public CreateDrawingsComponent()
            : base(
                "CreateDrawings",
                "Create Drawing elements on the specified or active layout from navigator items.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "NavigatorItemGuids",
                "Identifiers of the navigator items (views) to place as drawings.");

            InTexts(
                "Names",
                "Names of the drawings (input only 1 to use the same name for all).");

            InPoints(
                "Positions",
                "Position of each drawing on the layout (only X and Y are used).");

            InNumbers(
                "Scales",
                "Scale of each drawing (input only 1 to use the same scale for all). Optional.");

            InGeneric(
                "LayoutDatabaseGuid",
                "Identifier of the target layout database. Optional; defaults to the active layout.");

            InGenericTree(
                "ClipPolygons",
                "Clip polygon points for each drawing (one branch per drawing, at least 3 points; empty branch = no clipping). Optional.");

            SetOptionality(new[] { 3, 4, 5 });
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> navWrappers))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<string> names))
            {
                return;
            }

            if (!da.TryGetList(
                    2,
                    out List<Point3d> positions))
            {
                return;
            }

            if (names.Count != 1 &&
                names.Count != navWrappers.Count)
            {
                this.AddError(
                    "The size of the input Names must be 1 or equal to the size of the input NavigatorItemGuids.");
                return;
            }

            if (positions.Count != 1 &&
                positions.Count != navWrappers.Count)
            {
                this.AddError(
                    "The size of the input Positions must be 1 or equal to the size of the input NavigatorItemGuids.");
                return;
            }

            da.TryGetList(
                3,
                out List<double> scales);
            scales = scales ?? new List<double>();
            if (scales.Count > 1 &&
                scales.Count != navWrappers.Count)
            {
                this.AddError(
                    "The size of the input Scales must be 0, 1 or equal to the size of the input NavigatorItemGuids.");
                return;
            }

            DatabaseGuidObject layoutId = null;
            if (da.TryGet(
                    4,
                    out GH_ObjectWrapper layoutWrapper) &&
                layoutWrapper?.Value != null)
            {
                layoutId = GuidObject<DatabaseGuidObject>.CreateFromWrapper(layoutWrapper);
                if (layoutId == null)
                {
                    this.AddError("Invalid LayoutDatabaseGuid.");
                    return;
                }
            }

            da.TryGetTree(
                5,
                out GH_Structure<IGH_Goo> clipTree);

            var items = new JArray();
            for (var i = 0; i < navWrappers.Count; i++)
            {
                var navId = GuidObject<NavigatorGuid>.CreateFromWrapper(navWrappers[i]);
                if (navId == null)
                {
                    this.AddError(
                        "Invalid navigator item identifier in the NavigatorItemGuids input.");
                    return;
                }

                var position = positions[positions.Count == 1 ? 0 : i];
                var item = new JObject
                {
                    ["navigatorItemId"] = new JObject { ["guid"] = navId.Guid },
                    ["name"] = names[names.Count == 1 ? 0 : i],
                    ["position"] = new JObject
                    {
                        ["x"] = position.X,
                        ["y"] = position.Y
                    }
                };

                if (scales.Count > 0)
                {
                    item["scale"] = scales[scales.Count == 1 ? 0 : i];
                }

                if (layoutId != null)
                {
                    item["layoutDatabaseId"] = new JObject { ["guid"] = layoutId.Guid };
                }

                if (clipTree != null &&
                    i < clipTree.Branches.Count &&
                    clipTree.Branches[i].Count > 0)
                {
                    var polygon = new JArray();
                    foreach (var goo in clipTree.Branches[i])
                    {
                        var ghPoint = goo as GH_Point;
                        if (ghPoint == null)
                        {
                            this.AddError(
                                "The ClipPolygons input must contain points.");
                            return;
                        }
                        polygon.Add(
                            new JObject
                            {
                                ["x"] = ghPoint.Value.X,
                                ["y"] = ghPoint.Value.Y
                            });
                    }

                    if (polygon.Count < 3)
                    {
                        this.AddError(
                            "Each clip polygon must contain at least 3 points.");
                        return;
                    }

                    item["clipPolygon"] = polygon;
                }

                items.Add(item);
            }

            var parameters = new JObject { ["drawingsData"] = items };

            TryGetCadResponse(
                CommandName,
                parameters,
                ToAddOn,
                out _);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateDrawings;

        public override Guid ComponentGuid =>
            new Guid("3611ccff-185d-4f6c-b6c4-679022f8f256");
    }
}
