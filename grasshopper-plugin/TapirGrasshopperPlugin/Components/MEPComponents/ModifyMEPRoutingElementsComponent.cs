using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class ModifyMEPRoutingElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ModifyMEPRoutingElements";

        public ModifyMEPRoutingElementsComponent()
            : base(
                "ModifyMEPRoutingElements",
                "Modify the given MEP routing elements: MEP system, cross section data of all segments and node positions. " +
                "Only provided fields are changed. Available from Archicad 28.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the MEP routing elements to modify.");

            InGenerics(
                "MEPSystemGuids",
                "New MEP system attribute of each route (input only 1 to use the same system for all). Optional.");

            InNumbers(
                "CrossSectionWidths",
                "New cross section width applied to all segments of each route (input only 1 to use the same width for all). Optional.");

            InNumbers(
                "CrossSectionHeights",
                "New cross section height applied to all segments of each route (input only 1 to use the same height for all). Optional.");

            InTexts(
                "CrossSectionShapes",
                "New cross section shape applied to all segments of each route: Rectangular, Circular, Oval or UShape " +
                "(input only 1 to use the same shape for all). Optional.");

            inManager.AddPointParameter(
                "NodePositions",
                "NodePositions",
                "New positions of the routing nodes of each route (one branch per route; " +
                "the branch size must match the node count of the route; empty branch = no change). Optional.",
                GH_ParamAccess.tree);

            SetOptionality(new[] { 1, 2, 3, 4, 5 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each routing element (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> elementWrappers))
            {
                return;
            }

            da.TryGetList(1, out List<GH_ObjectWrapper> mepSystemWrappers);
            mepSystemWrappers = mepSystemWrappers ?? new List<GH_ObjectWrapper>();
            da.TryGetList(2, out List<double> widths);
            widths = widths ?? new List<double>();
            da.TryGetList(3, out List<double> heights);
            heights = heights ?? new List<double>();
            da.TryGetList(4, out List<string> shapes);
            shapes = shapes ?? new List<string>();
            da.TryGetTree(5, out GH_Structure<GH_Point> nodePositions);

            foreach (var pair in new (string Name, int Count)[]
                     {
                         ("MEPSystemGuids", mepSystemWrappers.Count),
                         ("CrossSectionWidths", widths.Count),
                         ("CrossSectionHeights", heights.Count),
                         ("CrossSectionShapes", shapes.Count)
                     })
            {
                if (pair.Count > 1 &&
                    pair.Count != elementWrappers.Count)
                {
                    this.AddError(
                        $"The size of the input {pair.Name} must be 0, 1 or equal to the size of the input ElementGuids.");
                    return;
                }
            }

            var mepSystemIds = new List<AttributeGuidObject>();
            foreach (var wrapper in mepSystemWrappers)
            {
                var id = GuidObject<AttributeGuidObject>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError("Invalid attribute identifier in the MEPSystemGuids input.");
                    return;
                }
                mepSystemIds.Add(id);
            }

            var items = new JArray();
            for (var i = 0; i < elementWrappers.Count; i++)
            {
                var id = GuidObject<ElementGuid>.CreateFromWrapper(elementWrappers[i]);
                if (id == null)
                {
                    this.AddError("Invalid element identifier in the ElementGuids input.");
                    return;
                }

                var item = new JObject
                {
                    ["elementId"] = new JObject { ["guid"] = id.Guid }
                };
                if (mepSystemIds.Count > 0)
                {
                    item["mepSystemId"] = new JObject
                    {
                        ["guid"] = mepSystemIds[mepSystemIds.Count == 1 ? 0 : i].Guid
                    };
                }
                if (widths.Count > 0)
                {
                    item["crossSectionWidth"] = widths[widths.Count == 1 ? 0 : i];
                }
                if (heights.Count > 0)
                {
                    item["crossSectionHeight"] = heights[heights.Count == 1 ? 0 : i];
                }
                if (shapes.Count > 0)
                {
                    item["crossSectionShape"] = shapes[shapes.Count == 1 ? 0 : i];
                }

                if (nodePositions != null &&
                    i < nodePositions.Branches.Count &&
                    nodePositions.Branches[i].Count > 0)
                {
                    var positions = new JArray();
                    foreach (var ghPoint in nodePositions.Branches[i])
                    {
                        positions.Add(
                            new JObject
                            {
                                ["x"] = ghPoint.Value.X,
                                ["y"] = ghPoint.Value.Y,
                                ["z"] = ghPoint.Value.Z
                            });
                    }
                    item["nodePositions"] = positions;
                }

                items.Add(item);
            }

            var parameters = new JObject { ["routingElementsData"] = items };

            SetCadValuesWithErrorMessages(
                CommandName,
                parameters,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyMEPRoutingElements;

        public override Guid ComponentGuid =>
            new Guid("355d61d1-7ad2-46d0-a07c-d1b9dbeda140");
    }
}
