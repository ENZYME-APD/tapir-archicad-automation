using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetElementUserDataComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetElementUserData";

        public GetElementUserDataComponent()
            : base(
                "GetElementUserData",
                "Reads the values stored on elements by SetElementUserData: every key of each element, or only the given keys.",
                GroupNames.ElementDetails)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to read.");

            InTexts(
                "Keys",
                "Only the pairs with these keys are returned. Optional; every pair is returned when not given.");

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutTextTree(
                "Keys",
                "The keys stored on each element, one branch per element.");

            OutTextTree(
                "Values",
                "The value of each key, in the same structure as Keys.");

            OutErrorMessages(
                "Error message of each element (empty when its data was read).");
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

            var parameters = JObject.FromObject(elements);

            if (da.TryGetList(
                    1,
                    out List<string> keys) &&
                keys.Count > 0)
            {
                parameters["keys"] = new JArray(keys);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var keysTree = new DataTree<string>();
            var valuesTree = new DataTree<string>();
            var errorMessages = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "elementUserData");
            for (var i = 0; i < items.Count; i++)
            {
                var path = new GH_Path(i);
                keysTree.EnsurePath(path);
                valuesTree.EnsurePath(path);

                if (JsonOutputHelp.IsError(items[i]))
                {
                    errorMessages.Add(JsonOutputHelp.ErrorMessage(items[i]));
                    continue;
                }

                if (items[i]["userData"] is JArray userData)
                {
                    foreach (var pair in userData)
                    {
                        keysTree.Add(
                            pair["key"]?.ToString(),
                            path);
                        valuesTree.Add(
                            pair["value"]?.ToString(),
                            path);
                    }
                }

                errorMessages.Add("");
            }

            da.SetDataTree(
                0,
                keysTree);

            da.SetDataTree(
                1,
                valuesTree);

            da.SetDataList(
                2,
                errorMessages);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetElementUserData;

        public override Guid ComponentGuid =>
            new Guid("be94b882-d701-4a67-b095-6e38db3364d3");
    }
}
