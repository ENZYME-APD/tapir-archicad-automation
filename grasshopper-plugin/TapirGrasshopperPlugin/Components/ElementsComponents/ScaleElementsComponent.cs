using Grasshopper.Kernel;
using Rhino.Geometry;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ScaleElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ScaleElements";

        public ScaleElementsComponent()
            : base(
                "ScaleElements",
                "Scale elements by a factor around a center point with Archicad's Resize. " +
                "Supported: Wall, Column, Beam, Slab, Roof, Mesh, Zone, Line, PolyLine, Arc, Circle, " +
                "Spline, Hatch and Hotspot; other types are refused.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "ElementGuids of the elements to scale.");

            InPoint(
                "Center",
                "The fixed point of the scaling on the floor plan (its Z is ignored).");

            InNumber(
                "Factor",
                "The scale factor, greater than 0: 2 doubles the distances from the center, 0.5 halves them.",
                1.0);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when it was scaled).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return;
            }

            if (!da.TryGet(
                    1,
                    out Point3d center))
            {
                return;
            }

            if (!da.TryGet(
                    2,
                    out double factor))
            {
                return;
            }

            if (factor <= 0.0)
            {
                this.AddError("Factor must be greater than 0.");
                return;
            }

            var input = new ScaleElementsParameters
            {
                Elements = elements.Elements,
                Center = new Point2D { X = center.X, Y = center.Y },
                Factor = factor
            };

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        // Reuses the MoveElements icon until ScaleElements gets its own
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MoveElements;

        public override Guid ComponentGuid =>
            new Guid("c7d2e4a9-1b6f-4e38-9a05-6f3b8d2c1e74");
    }
}
