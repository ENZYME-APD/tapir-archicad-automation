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
                "marquee (curved segments are approximated with a polyline). Without an outline the " +
                "marquee is removed.",
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

            SetOptionality(new[] { 0, 1 });
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

            var parameters = new SetMarqueeParameters();

            if (outline != null)
            {
                if (!TryConvertOutline(
                        outline,
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
        // the closing repeat of the first vertex is dropped.
        private static bool TryConvertOutline(
            Curve outline,
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

            if (IsAxisAlignedRectangle(vertices))
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
