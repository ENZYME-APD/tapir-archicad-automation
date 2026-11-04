using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class Get2DBoundingBoxesOfElementsComponent
        : ElementsStructuredGetterComponent
    {
        public override string CommandName => "Get2DBoundingBoxes";

        public Get2DBoundingBoxesOfElementsComponent()
            : base(
                "Element2DBoundingBoxes",
                "Gets the 2D bounding box of elements as Archicad draws them, in the coordinates of the " +
                "database the element is in: the floor plan, a section, a worksheet, a layout or a master " +
                "layout. Works for 2D elements (Line, PolyLine, Arc, Hatch, Text, Label, Dimension, " +
                "Drawing ...) as well as for model elements; a Text's box covers its drawn lines, a " +
                "Drawing's box its frame with the title. The current window does not need to show the " +
                "element. The outputs are aligned with the input list of elements.",
                GroupNames.ElementDetails)
        {
        }

        protected override string ResponseArrayKey => "boundingBoxes2D";

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to get the 2D bounding box of.");
        }

        protected override void AddOutputs()
        {
            OutCurves(
                "BoundingBoxes",
                "The 2D bounding box of each element as a closed rectangle on the XY plane (null for an error).");

            OutPoints(
                "MinPoints",
                "The corner of each bounding box with the smallest X and Y (null for an error).");

            OutPoints(
                "MaxPoints",
                "The corner of each bounding box with the largest X and Y (null for an error).");

            OutErrorMessages(
                "Error message of each element (empty when successful).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var boxes = new List<object>();
            var minPoints = new List<object>();
            var maxPoints = new List<object>();
            var errors = new List<string>();

            foreach (var item in items)
            {
                var box = item?["boundingBox2D"];
                if (JsonOutputHelp.IsError(item) || box == null)
                {
                    boxes.Add(null);
                    minPoints.Add(null);
                    maxPoints.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                var min = new Point3d(
                    box.Value<double?>("xMin") ?? 0.0,
                    box.Value<double?>("yMin") ?? 0.0,
                    0.0);
                var max = new Point3d(
                    box.Value<double?>("xMax") ?? 0.0,
                    box.Value<double?>("yMax") ?? 0.0,
                    0.0);

                boxes.Add(
                    new PolylineCurve(
                        new Rectangle3d(
                            Plane.WorldXY,
                            min,
                            max).ToPolyline()));
                minPoints.Add(min);
                maxPoints.Add(max);
                errors.Add("");
            }

            da.SetDataList(
                0,
                boxes);
            da.SetDataList(
                1,
                minPoints);
            da.SetDataList(
                2,
                maxPoints);
            da.SetDataList(
                3,
                errors);
        }

        // Shares the icon of Element3DBoundingBoxes until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.Elems3DBoxes;

        public override Guid ComponentGuid =>
            new Guid("5126fbdb-1d28-40ee-99f5-17e8e4a9a47b");
    }
}
