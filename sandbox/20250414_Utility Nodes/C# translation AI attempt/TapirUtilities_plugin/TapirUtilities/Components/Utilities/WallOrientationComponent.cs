using Grasshopper.Kernel;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Linq;

namespace Tapir.Components.Utilities
{
    public class WallOrientationComponent : Component
    {
        /// <summary>
        /// Initializes a new instance of the WallOrientationComponent class.
        /// </summary>
        public WallOrientationComponent()
            : base("Wall Orientation", "WallOrient", 
                  "Calculates the orientation of walls based on start and end points", "Utilities")
        {
        }

        /// <summary>
        /// Registers all the input parameters for this component.
        /// </summary>
        protected override void RegisterInputParams(GH_Component.GH_InputParamManager pManager)
        {
            pManager.AddPointParameter("Start Points", "S", "Start point(s) of wall segment(s)", GH_ParamAccess.list);
            pManager.AddPointParameter("End Points", "E", "End point(s) of wall segment(s)", GH_ParamAccess.list);
            pManager.AddNumberParameter("North Rotation", "N", "Rotation angle for north direction", GH_ParamAccess.item, 0.0);
            pManager[2].Optional = true;
        }

        /// <summary>
        /// Registers all the output parameters for this component.
        /// </summary>
        protected override void RegisterOutputParams(GH_Component.GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter("Normal Vectors", "N", "Normal vectors as 2D lists", GH_ParamAccess.list);
            pManager.AddNumberParameter("Orientation Angles", "A", "Orientation angles in degrees", GH_ParamAccess.list);
            pManager.AddTextParameter("Orientation Codes", "C", "Orientation codes (N, NE, E, etc.)", GH_ParamAccess.list);
            pManager.AddVectorParameter("Rhino Vectors", "V", "Rhino Vector3d objects", GH_ParamAccess.list);
            pManager.AddPointParameter("Midpoints", "M", "Midpoints of wall segments", GH_ParamAccess.list);
            pManager.AddCurveParameter("Vector Lines", "L", "Vector lines at wall midpoints", GH_ParamAccess.list);
        }

        /// <summary>
        /// This is the method that actually does the work.
        /// </summary>
        /// <param name="DA">The DA object is used to retrieve from inputs and store in outputs.</param>
        protected override void SolveInstance(IGH_DataAccess DA)
        {
            // Get inputs
            List<Point3d> startPoints = new List<Point3d>();
            if (!DA.GetDataList(0, startPoints))
                return;

            List<Point3d> endPoints = new List<Point3d>();
            if (!DA.GetDataList(1, endPoints))
                return;

            double northRotation = 0.0;
            DA.GetData(2, ref northRotation);

            // Ensure inputs are lists of equal length
            if (startPoints.Count != endPoints.Count)
            {
                this.AddRuntimeMessage(GH_RuntimeMessageLevel.Error, "Start points and end points lists must have the same length");
                return;
            }

            // Initialize outputs
            List<List<double>> normalVectors = new List<List<double>>();
            List<double> orientationAngles = new List<double>();
            List<string> orientationCodes = new List<string>();
            List<Vector3d> rhinoVectors = new List<Vector3d>();
            List<Point3d> midpoints = new List<Point3d>();
            List<Curve> vectorLines = new List<Curve>();

            // Process each wall
            for (int i = 0; i < startPoints.Count; i++)
            {
                Point3d start = startPoints[i];
                Point3d end = endPoints[i];

                // Calculate normal vector and midpoint
                var (normalVector, midpoint) = CalculateNormalVector(start, end);

                // Skip invalid results
                if (normalVector == null || midpoint == null)
                {
                    normalVectors.Add(null);
                    orientationAngles.Add(double.NaN);
                    orientationCodes.Add(null);
                    rhinoVectors.Add(Vector3d.Unset);
                    midpoints.Add(Point3d.Unset);
                    vectorLines.Add(null);
                    continue;
                }

                // Determine orientation angle
                double orientationAngle = CalculateOrientation(normalVector, northRotation);

                // Get orientation code
                string orientationCode = GetOrientationCode(orientationAngle);

                // Create Rhino vector
                Vector3d rhinoVector = new Vector3d(normalVector[0], normalVector[1], 0);

                // Create vector line with hardcoded length of 2
                Point3d vecEndpoint = new Point3d(
                    midpoint.X + normalVector[0] * 2,  // Length of vector is hardcoded as 2
                    midpoint.Y + normalVector[1] * 2,
                    0
                );
                LineCurve vectorLine = new LineCurve(midpoint, vecEndpoint);

                // Append results
                normalVectors.Add(normalVector);
                orientationAngles.Add(orientationAngle);
                orientationCodes.Add(orientationCode);
                rhinoVectors.Add(rhinoVector);
                midpoints.Add(midpoint);
                vectorLines.Add(vectorLine);
            }

            // Set outputs
            DA.SetDataList(0, normalVectors);
            DA.SetDataList(1, orientationAngles);
            DA.SetDataList(2, orientationCodes);
            DA.SetDataList(3, rhinoVectors);
            DA.SetDataList(4, midpoints);
            DA.SetDataList(5, vectorLines);

            // Update component message
            this.Message = "Wall Orientation\nV1.0";
        }

        /// <summary>
        /// Calculates the normal vector and midpoint for a wall segment.
        /// </summary>
        private (List<double>, Point3d) CalculateNormalVector(Point3d start, Point3d end)
        {
            if (start == null || end == null)
                return (null, null);

            // Extract coordinates
            double startX = start.X;
            double startY = start.Y;
            double endX = end.X;
            double endY = end.Y;

            // Calculate midpoint
            Point3d midpoint = new Point3d(
                (startX + endX) / 2,
                (startY + endY) / 2,
                0
            );

            double dx = endX - startX;
            double dy = endY - startY;

            // Handle zero-length line
            if (dx == 0 && dy == 0)
                return (null, null);

            // Calculate normal vector
            List<double> normalVector = new List<double> { -dy, dx };
            double length = Math.Sqrt(normalVector[0] * normalVector[0] + normalVector[1] * normalVector[1]);
            normalVector[0] /= length;
            normalVector[1] /= length;

            return (normalVector, midpoint);
        }

        /// <summary>
        /// Calculates the orientation angle based on the normal vector and north rotation.
        /// </summary>
        private double CalculateOrientation(List<double> normalVector, double northRotation)
        {
            if (normalVector == null)
                return double.NaN;

            double angle = Math.Atan2(normalVector[0], normalVector[1]);
            double angleDegrees = angle * 180.0 / Math.PI;

            // Apply north rotation
            double adjustedAngle = (angleDegrees - northRotation) % 360;
            if (adjustedAngle < 0)
                adjustedAngle += 360;

            return adjustedAngle;
        }

        /// <summary>
        /// Maps an angle to a cardinal direction code.
        /// </summary>
        private string GetOrientationCode(double angle)
        {
            if (double.IsNaN(angle))
                return null;

            // Define direction boundaries
            if ((angle >= 337.5 && angle < 360) || (angle >= 0 && angle < 22.5))
                return "N";
            else if (angle >= 22.5 && angle < 67.5)
                return "NE";
            else if (angle >= 67.5 && angle < 112.5)
                return "E";
            else if (angle >= 112.5 && angle < 157.5)
                return "SE";
            else if (angle >= 157.5 && angle < 202.5)
                return "S";
            else if (angle >= 202.5 && angle < 247.5)
                return "SW";
            else if (angle >= 247.5 && angle < 292.5)
                return "W";
            else if (angle >= 292.5 && angle < 337.5)
                return "NW";

            return null;
        }

        /// <summary>
        /// Provides an Icon for the component.
        /// </summary>
        protected override System.Drawing.Bitmap Icon
        {
            get
            {
                return null;
            }
        }

        /// <summary>
        /// Gets the unique ID for this component. Do not change this ID after release.
        /// </summary>
        public override Guid ComponentGuid
        {
            get { return new Guid("A4B11162-627B-455B-B9C0-963E76B36DC4"); }
        }
    }
}
