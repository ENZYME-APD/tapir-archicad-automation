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
    public class CreateMEPRoutingElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateMEPRoutingElements";

        public CreateMEPRoutingElementsComponent()
            : base(
                "CreateMEPRoutingElements",
                "Create MEP routing elements (duct, pipe or cable carrier routes) along the given polylines. " +
                "The polyline points are given as a tree with one branch per route. " +
                "Available from Archicad 28.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            inManager.AddPointParameter(
                "Polylines",
                "Polylines",
                "Corner points of each route polyline (one branch per route, at least 2 points).",
                GH_ParamAccess.tree);

            InTexts(
                "Domains",
                "MEP domain of each route: Ventilation, Piping or CableCarrier (input only 1 to use the same domain for all).");

            InNumbers(
                "CrossSectionWidths",
                "Cross section width of each route applied to all segments (input only 1 to use the same width for all). Optional.");

            InNumbers(
                "CrossSectionHeights",
                "Cross section height of each route applied to all segments (input only 1 to use the same height for all). Optional.");

            InTexts(
                "CrossSectionShapes",
                "Cross section shape of each route: Rectangular, Circular, Oval or UShape (input only 1 to use the same shape for all). Optional.");

            InGenerics(
                "MEPSystemGuids",
                "MEP system attribute of each route (input only 1 to use the same system for all). Optional.");

            SetOptionality(new[] { 2, 3, 4, 5 });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifier of each created routing element.");

            OutErrorMessages(
                "Error message of each routing element (empty when it was created successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetTree(
                    0,
                    out GH_Structure<GH_Point> polylines))
            {
                return;
            }

            var routeCount = polylines.Branches.Count;
            if (routeCount == 0)
            {
                this.AddError("The Polylines input must contain at least one branch.");
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<string> domains))
            {
                return;
            }
            if (domains.Count != 1 &&
                domains.Count != routeCount)
            {
                this.AddError(
                    "The size of the input Domains must be 1 or equal to the number of branches in the Polylines input.");
                return;
            }

            da.TryGetList(2, out List<double> widths);
            widths = widths ?? new List<double>();
            da.TryGetList(3, out List<double> heights);
            heights = heights ?? new List<double>();
            da.TryGetList(4, out List<string> shapes);
            shapes = shapes ?? new List<string>();
            da.TryGetList(5, out List<GH_ObjectWrapper> mepSystemWrappers);
            mepSystemWrappers = mepSystemWrappers ?? new List<GH_ObjectWrapper>();

            foreach (var pair in new (string Name, int Count)[]
                     {
                         ("CrossSectionWidths", widths.Count),
                         ("CrossSectionHeights", heights.Count),
                         ("CrossSectionShapes", shapes.Count),
                         ("MEPSystemGuids", mepSystemWrappers.Count)
                     })
            {
                if (pair.Count > 1 &&
                    pair.Count != routeCount)
                {
                    this.AddError(
                        $"The size of the input {pair.Name} must be 0, 1 or equal to the number of branches in the Polylines input.");
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
            for (var i = 0; i < routeCount; i++)
            {
                var branch = polylines.Branches[i];
                if (branch.Count < 2)
                {
                    this.AddError("Each route polyline (branch) must contain at least 2 points.");
                    return;
                }

                var nodeCoordinates = new JArray();
                foreach (var ghPoint in branch)
                {
                    nodeCoordinates.Add(
                        new JObject
                        {
                            ["x"] = ghPoint.Value.X,
                            ["y"] = ghPoint.Value.Y,
                            ["z"] = ghPoint.Value.Z
                        });
                }

                var item = new JObject
                {
                    ["domain"] = domains[domains.Count == 1 ? 0 : i],
                    ["nodeCoordinates"] = nodeCoordinates
                };
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
                if (mepSystemIds.Count > 0)
                {
                    item["mepSystemId"] = new JObject
                    {
                        ["guid"] = mepSystemIds[mepSystemIds.Count == 1 ? 0 : i].Guid
                    };
                }
                items.Add(item);
            }

            var parameters = new JObject { ["routingElementsData"] = items };

            SetCadValuesWithCreatedIds<ElementGuid>(
                CommandName,
                parameters,
                ToAddOn,
                da,
                "elements",
                "elementId");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateMEPRoutingElements;

        public override Guid ComponentGuid =>
            new Guid("0d1f66cd-cadc-42e0-98f3-e8354fd8c4b2");
    }
}
