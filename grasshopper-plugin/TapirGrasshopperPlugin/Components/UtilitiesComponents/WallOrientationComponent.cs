using Grasshopper.Kernel;
using Rhino.Geometry;
using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.UtilitiesComponents
{
    public class WallOrientationComponent : Component
    {
        public static string Doc =>
            "Calculates the orientation of walls based on start and end points.";

        public WallOrientationComponent()
            : base(
                "Wall Orientation",
                "WallOrient",
                Doc,
                GroupNames.Utilities)
        {
        }

        protected override void AddInputs()
        {
            InPoints(
                "Start Points",
                "Start point(s) of wall segment(s)");

            InPoints(
                "End Points",
                "End point(s) of wall segment(s)");

            InNumber(
                "North Rotation",
                "Rotation angle for north direction",
                0.0);

            Params.Input[2].Optional = true;
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "Normal Vectors",
                "Normal vectors as 2D lists");

            OutNumbers(
                "Orientation Angles",
                "Orientation angles in degrees");

            OutTexts(
                "Orientation Codes",
                "Orientation codes (N, NE, E, etc.)");

            OutVectors(
                "Rhino Vectors",
                "Rhino Vector3d objects");

            OutPoints(
                "Midpoints",
                "Midpoints of wall segments");

            OutCurves(
                "Vector Lines",
                "Vector lines at wall midpoints");
        }

        protected override void SolveInstance(
            IGH_DataAccess da)
        {
            // Get inputs
            var startPoints = new List<Point3d>();
            if (!da.GetDataList(
                    0,
                    startPoints))
            {
                return;
            }

            var endPoints = new List<Point3d>();
            if (!da.GetDataList(
                    1,
                    endPoints))
            {
                return;
            }

            var northRotation = 0.0;
            da.GetData(
                2,
                ref northRotation);

            // Ensure inputs are lists of equal length
            if (startPoints.Count != endPoints.Count)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Start points and end points lists must have the same length");
                return;
            }

            // Initialize outputs
            var normalVectors = new List<List<double>>();
            var orientationAngles = new List<double>();
            var orientationCodes = new List<string>();
            var rhinoVectors = new List<Vector3d>();
            var midpoints = new List<Point3d>();
            var vectorLines = new List<Curve>();

            // Process each wall
            for (var i = 0; i < startPoints.Count; i++)
            {
                var start = startPoints[i];
                var end = endPoints[i];

                // Calculate normal vector and midpoint
                var (normalVector, midpoint) = CalculateNormalVector(
                    start,
                    end);

                // Skip invalid results
                if (normalVector == null)
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
                var orientationAngle = CalculateOrientation(
                    normalVector,
                    northRotation);

                // Get orientation code
                var orientationCode = GetOrientationCode(orientationAngle);

                // Create Rhino vector
                var rhinoVector = new Vector3d(
                    normalVector[0],
                    normalVector[1],
                    0);

                // Create vector line with hardcoded length of 2
                var vecEndpoint = new Point3d(
                    midpoint.X +
                    normalVector[0] * 2, // Length of vector is hardcoded as 2
                    midpoint.Y + normalVector[1] * 2,
                    0);
                var vectorLine = new LineCurve(
                    midpoint,
                    vecEndpoint);

                // Append results
                normalVectors.Add(normalVector);
                orientationAngles.Add(orientationAngle);
                orientationCodes.Add(orientationCode);
                rhinoVectors.Add(rhinoVector);
                midpoints.Add(midpoint);
                vectorLines.Add(vectorLine);
            }

            // Set outputs
            da.SetDataList(
                0,
                normalVectors);
            da.SetDataList(
                1,
                orientationAngles);
            da.SetDataList(
                2,
                orientationCodes);
            da.SetDataList(
                3,
                rhinoVectors);
            da.SetDataList(
                4,
                midpoints);
            da.SetDataList(
                5,
                vectorLines);

            // Update component message
            Message = "Wall Orientation\nV1.0";
        }

        private (List<double>, Point3d) CalculateNormalVector(
            Point3d start,
            Point3d end)
        {
            // Extract coordinates
            var startX = start.X;
            var startY = start.Y;
            var endX = end.X;
            var endY = end.Y;

            // Calculate midpoint
            var midpoint = new Point3d(
                (startX + endX) / 2,
                (startY + endY) / 2,
                0);

            var dx = endX - startX;
            var dy = endY - startY;

            // Handle zero-length line
            if (dx == 0 && dy == 0)
            {
                return (null, new Point3d());
            }

            // Calculate normal vector
            var normalVector = new List<double> { -dy, dx };
            var length = Math.Sqrt(
                normalVector[0] * normalVector[0] +
                normalVector[1] * normalVector[1]);
            normalVector[0] /= length;
            normalVector[1] /= length;

            return (normalVector, midpoint);
        }

        private double CalculateOrientation(
            List<double> normalVector,
            double northRotation)
        {
            if (normalVector == null)
            {
                return double.NaN;
            }

            var angle = Math.Atan2(
                normalVector[0],
                normalVector[1]);
            var angleDegrees = angle * 180.0 / Math.PI;

            // Apply north rotation
            var adjustedAngle = (angleDegrees - northRotation) % 360;
            if (adjustedAngle < 0)
            {
                adjustedAngle += 360;
            }

            return adjustedAngle;
        }

        private string GetOrientationCode(
            double angle)
        {
            if (double.IsNaN(angle))
            {
                return null;
            }

            // Define direction boundaries
            if ((angle >= 337.5 && angle < 360) || (angle >= 0 && angle < 22.5))
            {
                return "N";
            }
            else if (angle >= 22.5 && angle < 67.5)
            {
                return "NE";
            }
            else if (angle >= 67.5 && angle < 112.5)
            {
                return "E";
            }
            else if (angle >= 112.5 && angle < 157.5)
            {
                return "SE";
            }
            else if (angle >= 157.5 && angle < 202.5)
            {
                return "S";
            }
            else if (angle >= 202.5 && angle < 247.5)
            {
                return "SW";
            }
            else if (angle >= 247.5 && angle < 292.5)
            {
                return "W";
            }
            else if (angle >= 292.5 && angle < 337.5)
            {
                return "NW";
            }

            return null;
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.WallOrientation;

        public override Guid ComponentGuid =>
            new Guid("A4B11162-627B-455B-B9C0-963E76B36DC4");
    }
}