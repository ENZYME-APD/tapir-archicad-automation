using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetGridElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetGridElements";

        public GetGridElementsComponent()
            : base(
                "GetGridElements",
                "Get the grid elements of the project, placed with the Grid Tool or created by CreateGridElements, " +
                "with their line, story, layer, library part and optionally their GDL parameters. " +
                "Available from Archicad 26.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InBoolean(
                "IncludeParameters",
                "Also get the GDL parameters of each grid element.",
                false);
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifier of each grid element.");

            OutLines(
                "Lines",
                "Grid line of each element: from its insertion point in its direction, as long as the grid line (at Z = 0).");

            OutNumberList(
                "Angles",
                "Direction of each grid line in radians.");

            OutNumberList(
                "Lengths",
                "Length of each grid line.");

            OutIntegers(
                "FloorIndices",
                "Home story index of each grid element.");

            OutIntegers(
                "LayerIndices",
                "Layer attribute index of each grid element.");

            OutTexts(
                "LibraryPartNames",
                "Name of the library part of each grid element.");

            OutTexts(
                "Parameters",
                "GDL parameters of each grid element as a JSON array (name, displayName, type and value). " +
                "Only with IncludeParameters.");

            OutTexts(
                "ParameterSettings",
                "The settable GDL parameters of each grid element as AdditionalSettings for CreateGridElements: " +
                "numbers, booleans and strings, without arrays, titles and AC_Length, so a new grid looks like this " +
                "one and keeps its own length. Only with IncludeParameters.");
        }

        private void OutLines(
            string name,
            string description)
        {
            outManager.AddLineParameter(
                name,
                name,
                description.WithTypeName("lineList"),
                GH_ParamAccess.list);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var includeParameters = false;
            da.GetData(
                0,
                ref includeParameters);

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["includeParameters"] = includeParameters },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var elementGuids = new List<object>();
            var lines = new List<Line>();
            var angles = new List<double>();
            var lengths = new List<double>();
            var floorIndices = new List<int>();
            var layerIndices = new List<int>();
            var libraryPartNames = new List<string>();
            var parameters = new List<string>();
            var parameterSettings = new List<string>();

            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "gridElements"))
            {
                elementGuids.Add(ElementsStructuredGetterComponent.ElementIdOf(item));
                var angle = item.Value<double?>("angle") ?? 0.0;
                var length = item.Value<double?>("length") ?? 0.0;
                lines.Add(LineOf(item, angle, length));
                angles.Add(angle);
                lengths.Add(length);
                floorIndices.Add(item.Value<int?>("floorIndex") ?? 0);
                layerIndices.Add(item.Value<int?>("layerIndex") ?? 0);
                libraryPartNames.Add(item.Value<string>("libraryPartName"));
                // Without IncludeParameters the two parameter outputs stay empty
                // instead of holding one null per grid element.
                if (includeParameters)
                {
                    parameters.Add(ParametersOf(item));
                    parameterSettings.Add(ParameterSettingsOf(item));
                }
            }

            da.SetDataList(0, elementGuids);
            da.SetDataList(1, lines);
            da.SetDataList(2, angles);
            da.SetDataList(3, lengths);
            da.SetDataList(4, floorIndices);
            da.SetDataList(5, layerIndices);
            da.SetDataList(6, libraryPartNames);
            da.SetDataList(7, parameters);
            da.SetDataList(8, parameterSettings);
        }

        private static Line LineOf(
            JToken item,
            double angle,
            double length)
        {
            var start = new Point3d(
                item["pos"]?.Value<double?>("x") ?? 0.0,
                item["pos"]?.Value<double?>("y") ?? 0.0,
                0.0);
            var end = new Point3d(
                start.X + Math.Cos(angle) * length,
                start.Y + Math.Sin(angle) * length,
                0.0);
            return new Line(start, end);
        }

        // The parameters array of a grid element as JSON text; null when the
        // response has no parameters (IncludeParameters is false).
        internal static string ParametersOf(
            JToken item)
        {
            return item?["parameters"] is JArray array
                ? array.ToString(Formatting.None)
                : null;
        }

        // The parameters CreateGridElements can set, as an AdditionalSettings
        // object ({"parameters": [{"name", "value"}]}). Array and Dictionary
        // parameters carry no value and are left out, as are titles and
        // AC_Length: AC_Length holds the line length, which CreateGridElements
        // sets from the start and end points before it applies the parameters.
        // Null when the response has no parameters.
        internal static string ParameterSettingsOf(
            JToken item)
        {
            if (!(item?["parameters"] is JArray array))
            {
                return null;
            }

            var settings = new JArray();
            foreach (var parameter in array)
            {
                var name = parameter.Value<string>("name");
                var value = parameter["value"];
                if (string.IsNullOrEmpty(name) ||
                    value == null ||
                    value.Type == JTokenType.Null ||
                    name == "AC_Length" ||
                    parameter.Value<string>("type") == "Title")
                {
                    continue;
                }
                settings.Add(
                    new JObject
                    {
                        ["name"] = name,
                        ["value"] = value
                    });
            }

            return new JObject { ["parameters"] = settings }.ToString(Formatting.None);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetGridElements;

        public override Guid ComponentGuid =>
            new Guid("9fecdc96-3f49-47e3-be94-73caf06570b8");
    }
}
