using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class CreateLayoutSubsetComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateLayoutSubset";

        public CreateLayoutSubsetComponent()
            : base(
                "CreateLayoutSubset",
                "Create Layout Book subsets.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Name of each new subset.");

            InGenerics(
                "ParentNavigatorItemGuids",
                "Identifiers of the parent navigator items (omit to create the subsets under the Layout Book root). Input only 1 to use the same parent for all subsets. Optional.");

            InTexts(
                "OwnPrefixes",
                "ID prefix of the subset. Input only 1 to use the same value for all subsets. Optional.");

            InTexts(
                "CustomNumbers",
                "Custom number of the subset. Input only 1 to use the same value for all subsets. Optional.");

            InTexts(
                "NumberingStyles",
                "Numbering style: Undefined, abc, ABC, 1, 01, 001, 0001 or noID. Input only 1 to use the same value for all subsets. Optional.");

            InIntegers(
                "StartAts",
                "Number to start the numbering at. Input only 1 to use the same value for all subsets. Optional.");

            InBooleans(
                "ContinueNumbering",
                "Continue the numbering of the parent. Input only 1 to use the same value for all subsets. Optional.");

            InBooleans(
                "UseUpperPrefixes",
                "Include the prefix of the parent. Input only 1 to use the same value for all subsets. Optional.");

            InBooleans(
                "IncludeToIDSequences",
                "Include the subset in the ID sequence. Input only 1 to use the same value for all subsets. Optional.");

            InBooleans(
                "CustomNumbering",
                "Use custom numbering. Input only 1 to use the same value for all subsets. Optional.");

            InBooleans(
                "AddOwnPrefixes",
                "Add the subset's own prefix to the IDs. Input only 1 to use the same value for all subsets. Optional.");

            SetOptionality(new[] { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 });
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "NavigatorItemGuids",
                "Identifiers of the created subsets (null for failed items).");

            OutTexts(
                "ErrorMessages",
                "Error message for each subset (empty when the subset was created successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> names))
            {
                return;
            }

            var subsetCount = names.Count;
            if (subsetCount == 0)
            {
                this.AddError("The Names input must contain at least one item.");
                return;
            }

            var items = new List<JObject>();
            for (var i = 0; i < subsetCount; i++)
            {
                items.Add(new JObject { ["name"] = names[i] });
            }

            da.TryGetList(
                1,
                out List<GH_ObjectWrapper> parentWrappers);
            parentWrappers = parentWrappers ?? new List<GH_ObjectWrapper>();
            if (parentWrappers.Count > 0)
            {
                if (parentWrappers.Count != 1 && parentWrappers.Count != subsetCount)
                {
                    this.AddError(
                        "The size of the input ParentNavigatorItemGuids must be 0, 1 or equal to the size of the input Names.");
                    return;
                }
                for (var i = 0; i < subsetCount; i++)
                {
                    var parentId = GuidObject<NavigatorGuid>.CreateFromWrapper(
                        parentWrappers[parentWrappers.Count == 1 ? 0 : i]);
                    if (parentId == null)
                    {
                        this.AddError("Invalid identifier in the ParentNavigatorItemGuids input.");
                        return;
                    }
                    items[i]["parentNavigatorItemId"] = new JObject { ["guid"] = parentId.Guid };
                }
            }

            if (!TryApplyList<string>(da, 2, "OwnPrefixes", items, "ownPrefix", v => v) ||
                !TryApplyList<string>(da, 3, "CustomNumbers", items, "customNumber", v => v) ||
                !TryApplyList<string>(da, 4, "NumberingStyles", items, "numberingStyle", v => v) ||
                !TryApplyList<int>(da, 5, "StartAts", items, "startAt", v => v) ||
                !TryApplyList<bool>(da, 6, "ContinueNumbering", items, "continueNumbering", v => v) ||
                !TryApplyList<bool>(da, 7, "UseUpperPrefixes", items, "useUpperPrefix", v => v) ||
                !TryApplyList<bool>(da, 8, "IncludeToIDSequences", items, "includeToIDSequence", v => v) ||
                !TryApplyList<bool>(da, 9, "CustomNumbering", items, "customNumbering", v => v) ||
                !TryApplyList<bool>(da, 10, "AddOwnPrefixes", items, "addOwnPrefix", v => v))
            {
                return;
            }

            var itemsArray = new JArray();
            foreach (var item in items)
            {
                itemsArray.Add(item);
            }
            var parameters = new JObject { ["subsetsData"] = itemsArray };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var navigatorItemGuids = new List<string>();
            var errors = new List<string>();
            if (response["navigatorItems"] is JArray results)
            {
                foreach (var result in results)
                {
                    if (result?["error"] != null)
                    {
                        errors.Add(result["error"]?["message"]?.ToString() ?? "");
                        navigatorItemGuids.Add(null);
                        continue;
                    }
                    errors.Add("");
                    navigatorItemGuids.Add(result?["navigatorItemId"]?["guid"]?.ToString());
                }
            }
            da.SetDataList(0, navigatorItemGuids);
            da.SetDataList(1, errors);
        }

        private bool TryApplyList<T>(
            IGH_DataAccess da,
            int inputIndex,
            string inputName,
            List<JObject> items,
            string jsonKey,
            Func<T, JToken> convert)
        {
            var values = new List<T>();
            da.GetDataList(inputIndex, values);
            if (values.Count == 0)
            {
                return true;
            }
            if (values.Count != 1 && values.Count != items.Count)
            {
                this.AddError(
                    $"The size of the input {inputName} must be 0, 1 or equal to the size of the input Names.");
                return false;
            }
            for (var i = 0; i < items.Count; i++)
            {
                items[i][jsonKey] = convert(values[values.Count == 1 ? 0 : i]);
            }
            return true;
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateLayoutSubset;

        public override Guid ComponentGuid =>
            new Guid("c44f0d60-2085-436c-9a53-bcfb9f7ad77f");
    }
}
