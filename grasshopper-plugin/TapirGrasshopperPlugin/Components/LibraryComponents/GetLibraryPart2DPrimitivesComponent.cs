using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Components.NavigatorComponents;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class GetLibraryPart2DPrimitivesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetLibraryPart2DPrimitives";

        public GetLibraryPart2DPrimitivesComponent()
            : base(
                "GetLibraryPart2DPrimitives",
                "Gets the lines, arcs and polygons (with their holes, and whether they are fills) of the 2D symbol " +
                "of a loaded library part, drawn with its default parameters or with the parameters of a placed " +
                "instance, in the coordinate system of the library part. Texts, hotspots and other primitives are " +
                "not returned.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Name",
                "Name of the loaded library part, without the file extension (a trailing '.gsm' is accepted too).");

            InGeneric(
                "ElementGuid",
                "Identifier of a placed instance of the library part whose parameters are used to draw the symbol. " +
                "Without it the default parameters of the library part are used. Optional.");

            NavigatorComponentHelp.AddOptionalInteger(
                this,
                "GdlContext",
                "The GDL context the 2D symbol is drawn in, the value of the GLOB_CONTEXT GDL global (2 is the " +
                "floor plan). 2 by default. Optional.");

            SetOptionality(new[] { 1 });
        }

        protected override void AddOutputs()
        {
            OutCurves(
                "Lines",
                "The lines of the 2D symbol, in the order Archicad draws them.");

            OutCurves(
                "Arcs",
                "The arcs and full circles of the 2D symbol.");

            OutCurves(
                "Polygons",
                "The outline of each polygon of the 2D symbol.");

            OutBooleans(
                "IsFills",
                "Whether each polygon is the outline of a fill.");

            OutCurveTree(
                "Holes",
                "The holes of each polygon (one branch per polygon).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            string name = null;
            if (!da.GetData(0, ref name) ||
                string.IsNullOrEmpty(name))
            {
                return;
            }

            var parameters = new JObject { ["name"] = name };

            if (da.TryGet(
                    1,
                    out GH_ObjectWrapper elementWrapper) &&
                elementWrapper != null)
            {
                var elementId = GuidObject<TapirGrasshopperPlugin.Types.Element.ElementGuid>.CreateFromWrapper(elementWrapper);
                if (elementId == null)
                {
                    this.AddError("Invalid ElementGuid.");
                    return;
                }
                parameters["elementId"] = new JObject { ["guid"] = elementId.Guid };
            }

            NavigatorComponentHelp.CopyOptional<int>(da, 2, parameters, "gdlContext");

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var lines = new List<Curve>();
            var arcs = new List<Curve>();
            var polygons = new List<Curve>();
            var isFills = new List<bool>();
            var holes = new DataTree<Curve>();

            foreach (var primitive in JsonOutputHelp.Items(response, "primitives"))
            {
                switch (primitive["type"]?.ToString())
                {
                    case "Line":
                        lines.Add(
                            new LineCurve(
                                ElementGeometryJsonHelp.Point2D(primitive["begCoordinate"]),
                                ElementGeometryJsonHelp.Point2D(primitive["endCoordinate"])));
                        break;
                    case "Arc":
                        var arc = ArcOf(primitive);
                        if (arc != null)
                        {
                            arcs.Add(arc);
                        }
                        break;
                    case "Polygon":
                        var path = new GH_Path(polygons.Count);
                        holes.EnsurePath(path);
                        polygons.Add(
                            ElementGeometryJsonHelp.Polyline2D(
                                primitive["coordinates"],
                                true));
                        isFills.Add((bool?)primitive["isFill"] ?? false);
                        if (primitive["holes"] is JArray polygonHoles)
                        {
                            foreach (var hole in polygonHoles)
                            {
                                holes.Add(
                                    ElementGeometryJsonHelp.Polyline2D(
                                        hole["coordinates"],
                                        true),
                                    path);
                            }
                        }
                        break;
                }
            }

            da.SetDataList(0, lines);
            da.SetDataList(1, arcs);
            da.SetDataList(2, polygons);
            da.SetDataList(3, isFills);
            da.SetDataTree(4, holes);
        }

        // An arc counterclockwise from begAngle to endAngle around origin, or
        // a full circle; null without a positive radius.
        private static Curve ArcOf(
            JToken primitive)
        {
            var radius = (double?)primitive["radius"] ?? 0.0;
            if (radius <= 0.0)
            {
                return null;
            }

            var plane = Plane.WorldXY;
            plane.Origin = ElementGeometryJsonHelp.Point2D(primitive["origin"]);
            var circle = new Circle(
                plane,
                radius);

            if ((bool?)primitive["isWhole"] == true)
            {
                return new ArcCurve(circle);
            }

            var begAngle = (double?)primitive["begAngle"] ?? 0.0;
            var endAngle = (double?)primitive["endAngle"] ?? 0.0;
            while (endAngle <= begAngle)
            {
                endAngle += 2.0 * Math.PI;
            }

            return new ArcCurve(
                new Arc(
                    circle,
                    new Interval(
                        begAngle,
                        endAngle)));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetDrawingPrimitivesOfElements;

        public override Guid ComponentGuid =>
            new Guid("d5a05456-386e-42a0-8ffb-3be41466273a");
    }
}
