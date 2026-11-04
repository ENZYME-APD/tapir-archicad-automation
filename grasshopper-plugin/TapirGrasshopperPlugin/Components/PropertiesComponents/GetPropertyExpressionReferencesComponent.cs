using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Properties;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class GetPropertyExpressionReferencesComponent
        : ArchicadAccessorComponent
    {
        public override string CommandName =>
            "GetPropertyExpressionReferences";

        public GetPropertyExpressionReferencesComponent()
            : base(
                "GetPropertyExpressionReferences",
                "Get the strings that refer to the given properties in property expressions.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "PropertyGuids",
                "Identifiers of the properties.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "References",
                "The string that refers to each property in a property expression (null when failed).");

            OutErrorMessages(
                "Error message for each property (empty when successful).");
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

            var parameters = new JObject
            {
                ["propertyIds"] = JArray.FromObject(properties.Properties)
            };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var references = new List<object>();
            var errors = new List<string>();

            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "references"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    references.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                references.Add(JsonOutputHelp.Scalar(item, "reference"));
                errors.Add("");
            }

            da.SetDataList(0, references);
            da.SetDataList(1, errors);
        }

        // Shares the icon of GetPropertyValues.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetPropertyValues;

        public override Guid ComponentGuid =>
            new Guid("f930265b-9643-484f-ab99-abc16f489171");
    }
}
