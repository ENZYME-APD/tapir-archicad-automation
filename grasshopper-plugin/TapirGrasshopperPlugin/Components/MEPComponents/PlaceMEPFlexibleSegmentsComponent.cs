using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class PlaceMEPFlexibleSegmentsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "PlaceMEPFlexibleSegments";

        public PlaceMEPFlexibleSegmentsComponent()
            : base(
                "PlaceMEPFlexibleSegments",
                "Places MEP flexible segments through the given control points with the current settings of the " +
                "Flexible Segment tool of their domain, optionally with the orientation at both ends. " +
                "Available from Archicad 28.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            inManager.AddPointParameter(
                "ControlPoints",
                "ControlPoints",
                "Control points of each flexible segment from its beginning to its end " +
                "(one branch per segment, at least 2 points).",
                GH_ParamAccess.tree);

            InTexts(
                "Domains",
                "MEP domain of each flexible segment: Ventilation, Piping or CableCarrier " +
                "(input only 1 to use the same domain for all).");

            InVectors(
                "StartDirections",
                "Direction vector of the orientation at the beginning of each segment; give StartRotations too " +
                "(input only 1 to use the same vector for all). Optional; defaults to the direction from the first " +
                "control point to the second one.");

            InVectors(
                "StartRotations",
                "Rotation vector of the orientation at the beginning of each segment; give StartDirections too " +
                "(input only 1 to use the same vector for all). Optional; defaults to a horizontal vector " +
                "perpendicular to the direction.");

            InVectors(
                "EndDirections",
                "Direction vector of the orientation at the end of each segment; give EndRotations too " +
                "(input only 1 to use the same vector for all). Optional; defaults to the direction from the last " +
                "but one control point to the last one.");

            InVectors(
                "EndRotations",
                "Rotation vector of the orientation at the end of each segment; give EndDirections too " +
                "(input only 1 to use the same vector for all). Optional; defaults to a horizontal vector " +
                "perpendicular to the direction.");

            SetOptionality(new[] { 2, 3, 4, 5 });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifier of each placed flexible segment.");

            OutErrorMessages(
                "Error message of each flexible segment (empty when it was placed).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetTree(
                    0,
                    out GH_Structure<GH_Point> controlPoints))
            {
                return;
            }

            var segmentCount = controlPoints.Branches.Count;
            if (segmentCount == 0)
            {
                this.AddError("The ControlPoints input must contain at least one branch.");
                return;
            }

            if (!da.TryGetList(1, out List<string> domains))
            {
                return;
            }
            if (domains.Count != 1 &&
                domains.Count != segmentCount)
            {
                this.AddError(
                    "The size of the input Domains must be 1 or equal to the number of branches in the ControlPoints input.");
                return;
            }

            var fields = new ElectricalItemFields(this, 0, segmentCount);
            if (!fields.AddOrientations(da, 2, 3, "startOrientation") ||
                !fields.AddOrientations(da, 4, 5, "endOrientation"))
            {
                return;
            }

            var items = new JArray();
            for (var i = 0; i < segmentCount; i++)
            {
                var branch = controlPoints.Branches[i];
                if (branch.Count < 2)
                {
                    this.AddError("Each flexible segment (branch) must contain at least 2 control points.");
                    return;
                }

                var points = new JArray();
                foreach (var ghPoint in branch)
                {
                    points.Add(ElectricalComponentHelp.Coordinate(ghPoint.Value));
                }

                var item = new JObject
                {
                    ["domain"] = domains[domains.Count == 1 ? 0 : i],
                    ["controlPoints"] = points
                };
                fields.ApplyTo(item, i);
                items.Add(item);
            }

            SetCadValuesWithCreatedIds<ElementGuid>(
                CommandName,
                new JObject { ["flexibleSegmentsData"] = items },
                ToAddOn,
                da,
                "elements",
                "elementId");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateMEPRoutingElements;

        public override Guid ComponentGuid =>
            new Guid("06756219-7e17-4c4f-b13e-7a5ff5e6d29c");
    }
}
