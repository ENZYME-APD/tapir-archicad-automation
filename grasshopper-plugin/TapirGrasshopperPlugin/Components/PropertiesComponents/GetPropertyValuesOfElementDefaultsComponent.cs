using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Components.ElementsComponents;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Properties;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class GetPropertyValuesOfElementDefaultsComponent
        : ArchicadAccessorComponent
    {
        public override string CommandName =>
            "GetPropertyValuesOfElementDefaults";

        public GetPropertyValuesOfElementDefaultsComponent()
            : base(
                "GetPropertyValuesOfDefaults",
                "Get property values of the tool defaults (the settings new elements get) of element types. " +
                "Outputs have one branch per queried element type, with one item per queried property.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "PropertyGuids",
                "The property Guids to get the value for.");

            InTexts(
                "ElementTypes",
                "Element types whose defaults to get the value for.");
        }

        protected override void AddOutputs()
        {
            OutTextTree(
                "Values",
                "The property values of the defaults (one branch per element type; null when failed).");

            OutTextTree(
                "ValueErrorMessages",
                "Error message for each element type and property (one branch per element type; empty when successful).");

            OutTexts(
                "ErrorMessages",
                "Error message for each queried element type (empty when successful).");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            AddAsSource<ElementTypeValueList>(
                document,
                1);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out PropertiesObject properties))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<string> elementTypes))
            {
                return;
            }

            var parameters = new JObject
            {
                ["elementTypes"] = JArray.FromObject(elementTypes),
                ["properties"] = JArray.FromObject(properties.Properties)
            };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var values = new DataTree<string>();
            var valueErrors = new DataTree<string>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "propertyValuesForElementDefaults");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                values.EnsurePath(path);
                valueErrors.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");

                if (item["propertyValues"] is JArray propertyValues)
                {
                    foreach (var valueOrError in propertyValues)
                    {
                        if (JsonOutputHelp.IsError(valueOrError))
                        {
                            values.Add(null, path);
                            valueErrors.Add(
                                JsonOutputHelp.ErrorMessage(valueOrError),
                                path);
                            continue;
                        }

                        values.Add(
                            valueOrError["propertyValue"]?["value"]?.ToString(),
                            path);
                        valueErrors.Add("", path);
                    }
                }
            }

            da.SetDataTree(0, values);
            da.SetDataTree(1, valueErrors);
            da.SetDataList(2, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetPropertyValues;

        public override Guid ComponentGuid =>
            new Guid("7c3e91a4-5d2b-4f86-9b1e-0a6d8c4f2e37");
    }
}
