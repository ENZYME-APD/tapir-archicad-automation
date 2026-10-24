using Grasshopper.Kernel;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetMarqueeComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetMarquee";

        public GetMarqueeComponent()
            : base(
                "GetMarquee",
                "Gets the marquee of the current window of Archicad: its shape, whether it is " +
                "multi-story, and its outline as a closed polyline in the XY plane.",
                GroupNames.Elements)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                "Type",
                "The shape of the marquee: rectangle, rotatedRectangle or polygon, " +
                "or none when there is no marquee.");

            OutBoolean(
                "MultiStory",
                "True for a multi-story (thick) marquee, false for a single-story (thin) one. " +
                "Empty when there is no marquee.");

            outManager.AddCurveParameter(
                "Outline",
                "Outline",
                "The outline of the marquee as a closed polyline in the XY plane. Empty when there is no marquee."
                    .WithTypeName("curveItem"),
                GH_ParamAccess.item);

            outManager.AddNumberParameter(
                "RotationAngle",
                "RotationAngle",
                "The rotation angle (radians) of a rotatedRectangle marquee, 0 for the other shapes. Empty when there is no marquee."
                    .WithTypeName("numberItem"),
                GH_ParamAccess.item);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetConvertedCadValues(
                    CommandName,
                    null,
                    ToAddOn,
                    JHelp.Deserialize<MarqueeResponse>,
                    out MarqueeResponse response))
            {
                return;
            }

            da.SetData(
                0,
                response.Type ?? MarqueeResponse.NoneType);

            if (!response.HasMarquee)
            {
                return;
            }

            da.SetData(
                1,
                response.MultiStory ?? false);

            var outline = ToClosedPolyline(response.Polygon);
            if (outline != null)
            {
                da.SetData(
                    2,
                    outline.ToPolylineCurve());
            }

            da.SetData(
                3,
                response.RotationAngle ?? 0.0);
        }

        // The command lists each vertex once; the polyline repeats the first
        // one at the end to close.
        private static Polyline ToClosedPolyline(
            List<Point2D> polygon)
        {
            if (polygon == null || polygon.Count < 2)
            {
                return null;
            }

            var points = polygon
                .Select(p => new Point3d(
                    p.X,
                    p.Y,
                    0.0))
                .ToList();
            points.Add(points[0]);

            return new Polyline(points);
        }

        // Shares the icon of SelectedElements until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SelectedElems;

        public override Guid ComponentGuid =>
            new Guid("7c1e5d2a-9b4f-4e3d-8a6c-2f0b9d7e1c43");
    }
}
