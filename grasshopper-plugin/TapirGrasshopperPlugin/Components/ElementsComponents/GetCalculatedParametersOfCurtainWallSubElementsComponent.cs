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
    public class GetCalculatedParametersOfCurtainWallSubElementsComponent
        : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetCalculatedParametersOfCurtainWallSubElements";

        public GetCalculatedParametersOfCurtainWallSubElementsComponent()
            : base(
                "CurtainWallCalculatedParameters",
                "Gets the GDL parameters (name, type, value) of the given Curtain Wall Frame, Panel, Junction " +
                "and Accessory elements as Archicad calculated them for their place in the curtain wall. " +
                "ElementGDLParameters returns the parameters the sub-element was set up with instead.",
                GroupNames.ElementDetails)
        {
        }

        protected override string ResponseArrayKey => "calculatedParametersOfElements";

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the Curtain Wall Frame, Panel, Junction and Accessory elements.");

            InText(
                "ParameterName",
                "Name of the parameter to get. Optional, all parameters are returned when it is not given.");
            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutTextTree(
                "ParameterNames",
                "Name of each parameter (one branch per element).");

            OutTextTree(
                "ParameterDisplayNames",
                "Display name of each parameter (one branch per element).");

            OutTextTree(
                "ParameterTypes",
                "Type of each parameter (one branch per element).");

            OutGenericTree(
                "ParameterValues",
                "Value of each parameter (one branch per element); array values as JSON.");

            OutErrorMessages(
                "Error message of each element (empty when successful), e.g. when it is not a curtain wall " +
                "frame, panel, junction or accessory.");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var parameterName = da.GetOptional<string>(
                1,
                null);

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
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

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

            da.SetDataTree(0, names);
            da.SetDataTree(1, displayNames);
            da.SetDataTree(2, types);
            da.SetDataTree(3, values);
            da.SetDataList(4, errors);
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

        // Shares the icon of ElementGDLParameters until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ElemGDLParameters;

        public override Guid ComponentGuid =>
            new Guid("29002d65-df09-49e2-bd83-540dfe57a4fc");
    }
}
