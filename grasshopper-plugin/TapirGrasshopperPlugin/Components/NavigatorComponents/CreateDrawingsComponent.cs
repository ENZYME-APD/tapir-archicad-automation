using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
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
                "Names of the drawings (input only 1 to use the same name for all). Optional; " +
                "a name implies the CustomName name type unless NameTypes is given.");

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

            InTexts(
                "NameTypes",
                "How the title name of each drawing is assembled: ViewOrSourceFileName, ViewIdAndName or CustomName " +
                "(input only 1 to use the same value for all). Optional.");

            InNumbers(
                "Angles",
                "Rotation angle of each drawing in radians (input only 1 to use the same value for all). Optional.");

            InNumbers(
                "DrawingScales",
                "The nominal scale of each drawing (input only 1 to use the same value for all). Optional.");

            InPoints(
                "ModelOffsets",
                "Offset of the model origin within each drawing (only X and Y are used; input only 1 to use the same value for all). Optional.");

            SetOptionality(new[] { 1, 3, 4, 5, 6, 7, 8, 9 });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifier of each created drawing.");

            OutErrorMessages(
                "Error message of each drawing (empty when it was created successfully).");
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

            da.TryGetList(
                1,
                out List<string> names);
            names = names ?? new List<string>();

            if (!da.TryGetList(
                    2,
                    out List<Point3d> positions))
            {
                return;
            }

            if (names.Count > 1 &&
                names.Count != navWrappers.Count)
            {
                this.AddError(
                    "The size of the input Names must be 0, 1 or equal to the size of the input NavigatorItemGuids.");
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

            da.TryGetList(
                6,
                out List<string> nameTypes);
            nameTypes = nameTypes ?? new List<string>();
            da.TryGetList(
                7,
                out List<double> angles);
            angles = angles ?? new List<double>();
            da.TryGetList(
                8,
                out List<double> drawingScales);
            drawingScales = drawingScales ?? new List<double>();
            da.TryGetList(
                9,
                out List<Point3d> modelOffsets);
            modelOffsets = modelOffsets ?? new List<Point3d>();

            foreach (var pair in new (string Name, int Count)[]
                     {
                         ("NameTypes", nameTypes.Count),
                         ("Angles", angles.Count),
                         ("DrawingScales", drawingScales.Count),
                         ("ModelOffsets", modelOffsets.Count)
                     })
            {
                if (pair.Count > 1 &&
                    pair.Count != navWrappers.Count)
                {
                    this.AddError(
                        $"The size of the input {pair.Name} must be 0, 1 or equal to the size of the input NavigatorItemGuids.");
                    return;
                }
            }

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
                    ["position"] = new JObject
                    {
                        ["x"] = position.X,
                        ["y"] = position.Y
                    }
                };

                if (names.Count > 0)
                {
                    item["name"] = names[names.Count == 1 ? 0 : i];
                }

                if (nameTypes.Count > 0)
                {
                    item["nameType"] = nameTypes[nameTypes.Count == 1 ? 0 : i];
                }

                if (angles.Count > 0)
                {
                    item["angle"] = angles[angles.Count == 1 ? 0 : i];
                }

                if (drawingScales.Count > 0)
                {
                    item["drawingScale"] = drawingScales[drawingScales.Count == 1 ? 0 : i];
                }

                if (modelOffsets.Count > 0)
                {
                    var modelOffset = modelOffsets[modelOffsets.Count == 1 ? 0 : i];
                    item["modelOffset"] = new JObject
                    {
                        ["x"] = modelOffset.X,
                        ["y"] = modelOffset.Y
                    };
                }

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

            SetCadValuesWithCreatedIds<ElementGuid>(
                CommandName,
                parameters,
                ToAddOn,
                da,
                "elements",
                "elementId");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateDrawings;

        public override Guid ComponentGuid =>
            new Guid("3611ccff-185d-4f6c-b6c4-679022f8f256");
    }
}
