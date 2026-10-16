using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetMarkerParametersComponent : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetMarkerParametersOfElements";

        public GetMarkerParametersComponent()
            : base(
                "MarkerParameters",
                "Gets the marker library part and the marker's GDL parameters of the given Window and Door " +
                "elements. The marker is the stamp drawn next to the opening on the floor plan; its parameters " +
                "are separate from the opening's own GDL parameters, which ElementGDLParameters returns.",
                GroupNames.ElementDetails)
        {
        }

        protected override string ResponseArrayKey => "markerParametersOfElements";

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the Window and Door elements to get the marker parameters of.");

            InText(
                "ParameterName",
                "Name of the marker parameter to get. Optional, all parameters are returned when it is not given.");
            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "MarkerNames",
                "Name of the marker library part of each element.");

            OutTextTree(
                "ParameterNames",
                "Name of each marker parameter (one branch per element).");

            OutTextTree(
                "ParameterDisplayNames",
                "Display name of each marker parameter (one branch per element).");

            OutTextTree(
                "ParameterTypes",
                "Type of each marker parameter (one branch per element).");

            OutGenericTree(
                "ParameterValues",
                "Value of each marker parameter (one branch per element); array values as JSON.");

            OutErrorMessages(
                "Error message of each element (empty when successful), e.g. when it is not a window or a door.");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var parameterName = da.GetOptional<string>(
                1,
                null);

            var markerNames = new List<object>();
            var names = new DataTree<object>();
            var displayNames = new DataTree<object>();
            var types = new DataTree<object>();
            var values = new DataTree<object>();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                names.EnsurePath(path);
                displayNames.EnsurePath(path);
                types.EnsurePath(path);
                values.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    markerNames.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                markerNames.Add(item["libPart"]?["name"]?.ToString());
                errors.Add("");

                if (!(item["parameters"] is JArray parameters))
                {
                    continue;
                }

                foreach (var parameter in parameters)
                {
                    var name = parameter["name"]?.ToString();
                    if (!string.IsNullOrEmpty(parameterName) &&
                        name != parameterName)
                    {
                        continue;
                    }

                    names.Add(name, path);
                    displayNames.Add(parameter["displayName"]?.ToString(), path);
                    types.Add(parameter["type"]?.ToString(), path);
                    values.Add(ValueOf(parameter["value"]), path);
                }
            }

            da.SetDataList(0, markerNames);
            da.SetDataTree(1, names);
            da.SetDataTree(2, displayNames);
            da.SetDataTree(3, types);
            da.SetDataTree(4, values);
            da.SetDataList(5, errors);
        }

        private static object ValueOf(
            JToken value)
        {
            if (value == null || value.Type == JTokenType.Null)
            {
                return null;
            }

            return value is JValue scalar
                ? scalar.Value
                : value.ToString(Formatting.None);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetMarkerParametersOfElements;

        public override Guid ComponentGuid =>
            new Guid("9795d8bf-6b9a-4898-8ea2-a6b4d328ec25");
    }
}
