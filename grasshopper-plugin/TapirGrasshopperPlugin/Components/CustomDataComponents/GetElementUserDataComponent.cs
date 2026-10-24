using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.CustomData;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.CustomDataComponents
{
    public class GetElementUserDataComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetElementUserData";

        public GetElementUserDataComponent()
            : base(
                "GetElementUserData",
                "Read the text values SetElementUserData stored under keys on elements. " +
                "Outputs have one branch per element, with one item per stored pair; " +
                "with a Keys input only the pairs with those keys, in the order the element holds them.",
                GroupNames.CustomData)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Elements to read the data of.");

            InTexts(
                "Keys",
                "Keys to read; a key an element does not hold is left out of its branch. " +
                "Leave empty to read every pair stored on each element.");

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutTextTree(
                "Keys",
                "Key of each stored value (one branch per element).");

            OutTextTree(
                "Values",
                "Stored value of each key (one branch per element).");

            OutErrorMessages(
                "Error message of each element (empty when its data was read; " +
                "an element without data of Tapir has an empty branch, not an error).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return;
            }

            // An unconnected or empty Keys input reads every pair: the command
            // does that when the keys parameter is left out.
            var requestedKeys = da.GetOptionals<string>(1);

            if (requestedKeys.Exists(string.IsNullOrEmpty))
            {
                this.AddError("Every key must be a non-empty text.");
                return;
            }

            var input = new GetElementUserDataParameters
            {
                Elements = elements.Elements,
                Keys = requestedKeys.Count > 0
                    ? requestedKeys
                    : null
            };

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(input),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var keys = new DataTree<string>();
            var values = new DataTree<string>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "elementUserData");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                keys.EnsurePath(path);
                values.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");

                if (item["userData"] is JArray pairs)
                {
                    foreach (var pair in pairs)
                    {
                        keys.Add(
                            pair["key"]?.ToString(),
                            path);
                        values.Add(
                            pair["value"]?.ToString(),
                            path);
                    }
                }
            }

            da.SetDataTree(0, keys);
            da.SetDataTree(1, values);
            da.SetDataList(2, errors);
        }

        // The element user data has no icon of its own yet; it shares the one
        // of the property value getter.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetPropertyValues;

        public override Guid ComponentGuid =>
            new Guid("d3a81c56-4f7b-4e2a-95c8-6b0e2d9f1a47");
    }
}
