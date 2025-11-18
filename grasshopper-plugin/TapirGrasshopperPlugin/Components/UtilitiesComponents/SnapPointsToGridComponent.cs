using Grasshopper.Kernel;
using Rhino;
using Rhino.Geometry;
using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.UtilitiesComponents
{
    public class SnapPointsToGridComponent : Component
    {
        public static string Doc => "Snap points to an axis-aligned grid.";

        public SnapPointsToGridComponent()
            : base(
                "Snap Points",
                "SnapPt",
                Doc,
                GroupNames.Utilities)
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
            pManager.AddPointParameter(
                "Points",
                "P",
                "List of points to snap.",
                GH_ParamAccess.list);
            pManager.AddNumberParameter(
                "Grid size",
                "G",
                "Size of the grid.",
                GH_ParamAccess.item,
                1.0);
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddPointParameter(
                "Points",
                "P",
                "List of points snapped to the grid.",
                GH_ParamAccess.list);
        }

        protected override void SolveInstance(
            IGH_DataAccess DA)
        {
            var points = new List<Point3d>();
            if (!DA.GetDataList(
                    0,
                    points))
            {
                return;
            }

            var gridSize = 1.0;
            if (!DA.GetData(
                    1,
                    ref gridSize))
            {
                return;
            }

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

            DA.SetDataList(
                0,
                snappedPoints);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SnapPt;

        public override Guid ComponentGuid =>
            new Guid("b8b3282c-bdd5-497e-b7d3-7b1af0c71eaa");
    }
}