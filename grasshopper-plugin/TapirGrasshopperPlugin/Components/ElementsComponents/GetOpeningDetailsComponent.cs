using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetOpeningDetailsComponent : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetOpeningDetails";

        public GetOpeningDetailsComponent()
            : base(
                "GetOpeningDetails",
                "Returns the details of Opening elements: whether they exist, the parent element, the cut and " +
                "connected elements, the geometry (anchor point, extrusion direction, axes, polygon) and the " +
                "extrusion parameters (constraint, limit type, linked status, shape type, size, anchor, offsets, " +
                "surface overrides, as a JSON object). Available from Archicad 29.",
                GroupNames.ElementDetails)
        {
        }

        protected override string ResponseArrayKey => "openings";

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the Opening elements (made with the Opening tool) to read.");
        }

        protected override void AddOutputs()
        {
            OutBooleans(
                "Exists",
                "Whether each opening exists.");

            OutGenerics(
                "ParentElementGuids",
                "The element each opening was placed in (null when it has none).");

            OutGenericTree(
                "CutElementGuids",
                "The elements each opening cuts (one branch per opening).");

            OutGenericTree(
                "ConnectedOpeningGuids",
                "The openings Archicad reports as connected to each opening (one branch per opening).");

            OutPoints(
                "AnchorPoints",
                "The anchor point of each opening in world coordinates.");

            OutVectors(
                "ExtrusionDirections",
                "The direction each opening is extruded in.");

            OutVectors(
                "XAxes",
                "The X axis of the plane of each opening's polygon.");

            OutVectors(
                "YAxes",
                "The Y axis of the plane of each opening's polygon.");

            OutPointTree(
                "Polygons",
                "The vertices of each opening's polygon, in the plane of its X and Y axes (one branch per opening; " +
                "Z is 0). SetOpeningDetails takes them back.");

            OutTexts(
                "ExtrusionParameters",
                "The size and extrusion settings of each opening, as a JSON object (SetOpeningDetails takes it back).");

            OutErrorMessages(
                "Error message of each element (empty when it was read).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var exists = new List<bool?>();
            var parents = new List<object>();
            var cutElements = new DataTree<object>();
            var connectedOpenings = new DataTree<object>();
            var anchorPoints = new List<object>();
            var extrusionDirections = new List<Vector3d?>();
            var xAxes = new List<Vector3d?>();
            var yAxes = new List<Vector3d?>();
            var polygons = new DataTree<Point3d>();
            var extrusionParameters = new List<string>();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                cutElements.EnsurePath(path);
                connectedOpenings.EnsurePath(path);
                polygons.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    exists.Add(null);
                    parents.Add(null);
                    anchorPoints.Add(null);
                    extrusionDirections.Add(null);
                    xAxes.Add(null);
                    yAxes.Add(null);
                    extrusionParameters.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                exists.Add(item.Value<bool?>("exists"));
                parents.Add(ItemListHelp.ElementGuidOf(item["parentElementId"]));
                AddElementIds(cutElements, item["cutElements"], path);
                AddElementIds(connectedOpenings, item["connectedOpenings"], path);

                var geometry = item["geometry"];
                anchorPoints.Add(JsonOutputHelp.Point(geometry?["anchorPoint"]));
                extrusionDirections.Add(VectorOf(geometry?["extrusionDirection"]));
                xAxes.Add(VectorOf(geometry?["xAxis"]));
                yAxes.Add(VectorOf(geometry?["yAxis"]));
                polygons.AddRange(PolygonHelp.ToPoints(geometry?["polygon"]), path);

                extrusionParameters.Add(OpeningSettingsHelp.ToText(item["extrusionParameters"]));
                errors.Add("");
            }

            da.SetDataList(0, exists);
            da.SetDataList(1, parents);
            da.SetDataTree(2, cutElements);
            da.SetDataTree(3, connectedOpenings);
            da.SetDataList(4, anchorPoints);
            da.SetDataList(5, extrusionDirections);
            da.SetDataList(6, xAxes);
            da.SetDataList(7, yAxes);
            da.SetDataTree(8, polygons);
            da.SetDataList(9, extrusionParameters);
            da.SetDataList(10, errors);
        }

        private static void AddElementIds(
            DataTree<object> tree,
            JToken elements,
            GH_Path path)
        {
            if (!(elements is JArray array))
            {
                return;
            }

            foreach (var element in array)
            {
                tree.Add(ElementIdOf(element), path);
            }
        }

        internal static Vector3d? VectorOf(
            JToken coordinate)
        {
            if (coordinate == null || coordinate.Type == JTokenType.Null)
            {
                return null;
            }

            return new Vector3d(
                coordinate.Value<double?>("x") ?? 0.0,
                coordinate.Value<double?>("y") ?? 0.0,
                coordinate.Value<double?>("z") ?? 0.0);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetOpeningDetails;

        public override Guid ComponentGuid =>
            new Guid("7da580c3-8628-4bba-81b2-cfe6806c0d3c");
    }
}
