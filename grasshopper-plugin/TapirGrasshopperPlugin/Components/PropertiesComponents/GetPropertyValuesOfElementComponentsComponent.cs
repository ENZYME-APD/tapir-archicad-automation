using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Properties;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class GetPropertyValuesOfElementComponentsComponent
        : ArchicadAccessorComponent
    {
        public override string CommandName =>
            "GetPropertyValuesOfElementComponents";

        public GetPropertyValuesOfElementComponentsComponent()
            : base(
                "GetPropertyValuesOfElementComponents",
                "Get the property values of the given element components (from GetComponentsOfElements) for " +
                "the given properties: for example the building material, thickness, area and volume of each " +
                "skin of a composite. Value outputs have one branch per queried component, with one item per " +
                "queried property.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "PropertyGuids",
                "Identifiers of the properties to query.");

            InGenerics(
                "ElementGuids",
                ElementComponentHelp.ElementGuidsDescription);

            InTexts(
                "ComponentGuids",
                ElementComponentHelp.ComponentGuidsDescription);
        }

        protected override void AddOutputs()
        {
            OutTextTree(
                "Values",
                "Property value of each component and property (one branch per component).");

            OutTextTree(
                "ValueErrorMessages",
                "Error message for each component and property (one branch per component; empty when successful).");

            OutErrorMessages(
                "Error message for each queried component (empty when successful).");
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

            if (!ElementComponentHelp.TryGetElementComponents(
                    this,
                    da,
                    1,
                    2,
                    out JArray elementComponents))
            {
                return;
            }

            var parameters = new JObject
            {
                ["elementComponents"] = elementComponents,
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

            var values = new DataTree<object>();
            var valueErrors = new DataTree<object>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "propertyValuesForElementComponents");
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

                foreach (var valueOrError in JsonOutputHelp.Items(
                             item as JObject,
                             "propertyValues"))
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
                        JsonOutputHelp.Scalar(
                            valueOrError["propertyValue"],
                            "value"),
                        path);
                    valueErrors.Add("", path);
                }
            }

            da.SetDataTree(0, values);
            da.SetDataTree(1, valueErrors);
            da.SetDataList(2, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetPropertyValuesOfElementComponents;

        public override Guid ComponentGuid =>
            new Guid("69912fd4-454b-40fb-acc3-6c3e6d331f74");
    }
}
