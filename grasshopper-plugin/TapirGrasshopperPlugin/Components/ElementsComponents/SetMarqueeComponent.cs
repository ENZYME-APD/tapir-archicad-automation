using Grasshopper.Kernel;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class SetMarqueeComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetMarquee";

        public SetMarqueeComponent()
            : base(
                "SetMarquee",
                "Draws a marquee in the current window of Archicad from a closed curve in the XY plane: " +
                "an axis-aligned rectangle becomes a rectangle marquee, any other closed curve a polygon " +
                "marquee (curved segments are approximated with a polyline). With RotationAngle the outline " +
                "has to be a rectangle rotated by that angle, and it becomes a rotated rectangle marquee. " +
                "Without an outline the marquee is removed.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            inManager.AddCurveParameter(
                "Outline",
                "Outline",
                "Closed curve of the marquee in the XY plane (a Rectangle is accepted). Leave empty to remove the marquee."
                    .WithTypeName("curveItem"),
                GH_ParamAccess.item);

            InBoolean(
                "MultiStory",
                "True for a multi-story (thick) marquee, false (default) for a single-story (thin) one.");

            inManager.AddNumberParameter(
                "RotationAngle",
                "RotationAngle",
                ("Optional. Rotation angle (radians) of a rectangle marquee. The Outline has to be a rectangle " +
                 "whose sides are rotated by this angle from the X and Y axes; it becomes a rotated rectangle " +
                 "marquee. When not given (or 0), an axis-aligned rectangle becomes a rectangle marquee and " +
                 "any other outline a polygon marquee.")
                    .WithTypeName("numberItem"),
                GH_ParamAccess.item);

            SetOptionality(new[] { 0, 1, 2 });
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult.Message),
                ExecutionResult.Doc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var outline = da.GetOptional<Curve>(
                0,
                null);

            var multiStory = da.GetOptional(
                1,
                false);

            var rotationAngle = 0.0;
            da.GetData(
                2,
                ref rotationAngle);

            var parameters = new SetMarqueeParameters();

            if (outline != null)
            {
                if (!TryConvertOutline(
                        outline,
                        rotationAngle,
                        parameters,
                        out string error))
                {
                    this.AddError(error);
                    return;
                }

                parameters.MultiStory = multiStory;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    parameters,
                    ToAddOn,
                    ExecutionResultResponse.Deserialize,
                    out ExecutionResultResponse response))
            {
                return;
            }

            da.SetData(
                0,
                response.Message());
        }

        // Fills either the box (axis-aligned rectangle) or the polygon of the
        // parameters from the curve. The add-on closes the polygon itself, so
        // the closing repeat of the first vertex is dropped. With a rotation
        // angle the curve has to be a rectangle rotated by it: Archicad stores
        // such a box in the coordinate system rotated by the angle around the
        // project origin, so the corners are rotated back by the angle.
        private static bool TryConvertOutline(
            Curve outline,
            double rotationAngle,
            SetMarqueeParameters parameters,
            out string error)
        {
            error = null;

            if (!outline.IsClosed)
            {
                error = "The outline has to be a closed curve.";
                return false;
            }

            if (!outline.TryGetPolyline(out Polyline polyline))
            {
                var polylineCurve = outline.ToPolyline(
                    0,
                    0,
                    0.05,
                    0.0,
                    0.0,
                    0.01,
                    0.0,
                    0.0,
                    true);
                polyline = polylineCurve?.ToPolyline();
                if (polyline == null)
                {
                    error = "The outline could not be approximated with a polyline.";
                    return false;
                }
            }

            var vertices = polyline.ToList();
            if (vertices.Count > 1 &&
                vertices[0].DistanceTo(vertices[vertices.Count - 1]) < Helps.Convert.Epsilon)
            {
                vertices.RemoveAt(vertices.Count - 1);
            }

            if (vertices.Count < 3)
            {
                error = "The outline needs at least three vertices.";
                return false;
            }

            if (Math.Abs(rotationAngle) > 1e-9)
            {
                var cos = Math.Cos(-rotationAngle);
                var sin = Math.Sin(-rotationAngle);
                var rotated = vertices
                    .Select(v => new Point3d(
                        v.X * cos - v.Y * sin,
                        v.X * sin + v.Y * cos,
                        0.0))
                    .ToList();
                if (!IsAxisAlignedRectangle(rotated))
                {
                    error = "With RotationAngle the outline has to be a rectangle whose sides are rotated by that angle.";
                    return false;
                }

                parameters.Box = new MarqueeBox
                {
                    XMin = rotated.Min(v => v.X),
                    YMin = rotated.Min(v => v.Y),
                    XMax = rotated.Max(v => v.X),
                    YMax = rotated.Max(v => v.Y)
                };
                parameters.RotationAngle = rotationAngle;
            }
            else if (IsAxisAlignedRectangle(vertices))
            {
                parameters.Box = new MarqueeBox
                {
                    XMin = vertices.Min(v => v.X),
                    YMin = vertices.Min(v => v.Y),
                    XMax = vertices.Max(v => v.X),
                    YMax = vertices.Max(v => v.Y)
                };
            }
            else
            {
                parameters.Polygon = vertices
                    .Select(v => new Point2D
                    {
                        X = v.X,
                        Y = v.Y
                    })
                    .ToList();
            }

            return true;
        }

        // Four corners whose sides are all horizontal or vertical.
        private static bool IsAxisAlignedRectangle(
            List<Point3d> vertices)
        {
            if (vertices.Count != 4)
            {
                return false;
            }

            for (var i = 0; i < 4; ++i)
            {
                var from = vertices[i];
                var to = vertices[(i + 1) % 4];
                var horizontal = Math.Abs(to.Y - from.Y) < Helps.Convert.Epsilon;
                var vertical = Math.Abs(to.X - from.X) < Helps.Convert.Epsilon;
                if (horizontal == vertical)
                {
                    return false;
                }
            }

            return true;
        }

        // Shares the icon of ZoomToElements until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ZoomToElements;

        public override Guid ComponentGuid =>
            new Guid("e4a9c6b1-3d7f-4b28-9c5e-6a1f8d2b0e97");
    }
}
