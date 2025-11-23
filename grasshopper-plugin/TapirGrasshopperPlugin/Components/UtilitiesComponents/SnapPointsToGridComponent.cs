using Grasshopper.Kernel;
using Rhino;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.UtilitiesComponents
{
    public class SnapPointsToGridComponent : Component
    {
        public SnapPointsToGridComponent()
            : base(
                "SnapPoints",
                "Snap points to an axis-aligned grid.",
                GroupNames.Utilities)
        {
        }

        protected override void AddInputs()
        {
            InPoints(
                "Points",
                "List of points to snap.");

            InNumber(
                "Grid size",
                "Size of the grid.",
                1.0);

            Params.Input[1].Optional = true;
        }

        protected override void AddOutputs()
        {
            OutPoints(
                "Points",
                "List of points snapped to the grid.");
        }

        protected override void SolveInstance(
            IGH_DataAccess da)
        {
            if (!da.TryGetItems(
                    0,
                    out List<Point3d> points))
            {
                return;
            }

            var gridSize = da.GetOptionalItem(
                1,
                1.0);

            if (gridSize < 0.0 || RhinoMath.EpsilonEquals(
                    gridSize,
                    0.0,
                    RhinoMath.Epsilon))
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Grid size must be positive.");
                return;
            }

            var snappedPoints = new List<Point3d>();
            foreach (var point in points)
            {
                snappedPoints.Add(
                    new Point3d()
                    {
                        X = Math.Floor(point.X / gridSize) * gridSize,
                        Y = Math.Floor(point.Y / gridSize) * gridSize,
                        Z = Math.Floor(point.Z / gridSize) * gridSize
                    });
            }

            da.SetDataList(
                0,
                snappedPoints);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SnapPt;

        public override Guid ComponentGuid =>
            new Guid("b8b3282c-bdd5-497e-b7d3-7b1af0c71eaa");
    }
}