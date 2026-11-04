using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.DesignOptionsComponents
{
    public class SetHotlinkDesignOptionTablesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetHotlinkDesignOptionTables";

        public SetHotlinkDesignOptionTablesComponent()
            : base(
                "SetHotlinkDesignOptionTables",
                "Sets the filter mode, the active combination and the active option of sets in the design " +
                "option table of the given placed hotlink instances. What is not given keeps its current " +
                "value. The changes are applied in the order filter mode, active combination, active options. " +
                "Every input but HotlinkInstanceGuids holds nothing (unchanged), one item or branch (used for " +
                "every instance) or one item or branch per instance. Available from Archicad 29.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "HotlinkInstanceGuids",
                "The placed hotlink instances (Hotlink elements) to change the design option table of, for " +
                "example the Instances output of Hotlinks.");

            InTexts(
                "FilterModes",
                "Optional. Which elements of the hotlinked module are shown: MainModelOnly, FilterByOptions " +
                "or FilterByCombination.");

            InTexts(
                "ActiveDesignOptionCombinations",
                "Optional. The name of the combination to activate, one of the DesignOptionCombinations of " +
                "GetHotlinkDesignOptionTables. An empty text deactivates the active combination.");

            inManager.AddTextParameter(
                "SetNames",
                "SetNames",
                ("Optional. The design option sets to activate an option in, one branch per instance (or one " +
                 "branch for all). Sets that are not listed keep their active option.")
                    .WithTypeName("textTree"),
                GH_ParamAccess.tree);

            inManager.AddTextParameter(
                "DesignOptionNames",
                "DesignOptionNames",
                ("Optional. The option to activate in each set of SetNames, in the same tree structure; the " +
                 "previously active option of the set is deactivated. An empty text deactivates every option " +
                 "of the set.")
                    .WithTypeName("textTree"),
                GH_ParamAccess.tree);

            SetOptionality(new[] { 1, 2, 3, 4 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each hotlink instance (empty when its table was changed).");
        }

        // The item of a 0, 1 or N item input that belongs to the instance at
        // index; null when the input holds nothing.
        private static string ItemFor(
            List<string> items,
            int index)
        {
            if (items.Count == 0)
            {
                return null;
            }

            return items.Count == 1 ? items[0] : items[index];
        }

        private static List<string> BranchTexts(
            GH_Structure<GH_String> tree,
            int branchIndex)
        {
            var texts = new List<string>();
            foreach (var item in tree.Branches[branchIndex])
            {
                texts.Add(item?.Value ?? "");
            }

            return texts;
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> instanceWrappers))
            {
                return;
            }

            var count = instanceWrappers.Count;
            var filterModes = da.GetOptionals<string>(1);
            var combinations = da.GetOptionals<string>(2);

            GH_Structure<GH_String> setNames;
            if (!da.TryGetTree(
                    3,
                    out setNames) ||
                setNames == null)
            {
                setNames = new GH_Structure<GH_String>();
            }

            GH_Structure<GH_String> optionNames;
            if (!da.TryGetTree(
                    4,
                    out optionNames) ||
                optionNames == null)
            {
                optionNames = new GH_Structure<GH_String>();
            }

            var setBranchCount = setNames.IsEmpty ? 0 : setNames.Branches.Count;
            var optionBranchCount = optionNames.IsEmpty ? 0 : optionNames.Branches.Count;

            if ((filterModes.Count > 1 && filterModes.Count != count) ||
                (combinations.Count > 1 && combinations.Count != count) ||
                (setBranchCount > 1 && setBranchCount != count))
            {
                this.AddError(
                    "FilterModes, ActiveDesignOptionCombinations and SetNames must hold nothing, one item " +
                    "(branch) or one item (branch) per item of HotlinkInstanceGuids.");
                return;
            }

            if (setBranchCount != optionBranchCount)
            {
                this.AddError(
                    "SetNames and DesignOptionNames must have the same tree structure.");
                return;
            }

            var hotlinkInstances = new JArray();
            for (var i = 0; i < count; i++)
            {
                var elementId = GuidObject<ElementGuid>.CreateFromWrapper(instanceWrappers[i]);
                if (elementId == null)
                {
                    this.AddError(
                        "Invalid element identifier in the HotlinkInstanceGuids input.");
                    return;
                }

                var instance = new JObject
                {
                    ["elementId"] = new JObject { ["guid"] = elementId.Guid }
                };

                var filterMode = ItemFor(filterModes, i);
                if (!string.IsNullOrWhiteSpace(filterMode))
                {
                    instance["filterMode"] = filterMode.Trim();
                }

                var combination = ItemFor(combinations, i);
                if (combination != null)
                {
                    instance["activeDesignOptionCombination"] = combination;
                }

                if (setBranchCount > 0)
                {
                    var branchIndex = setBranchCount == 1 ? 0 : i;
                    var sets = BranchTexts(setNames, branchIndex);
                    var options = BranchTexts(optionNames, branchIndex);
                    if (sets.Count != options.Count)
                    {
                        this.AddError(
                            "SetNames and DesignOptionNames must have the same tree structure.");
                        return;
                    }

                    var activeDesignOptions = new JArray();
                    for (var j = 0; j < sets.Count; j++)
                    {
                        activeDesignOptions.Add(
                            new JObject
                            {
                                ["setName"] = sets[j],
                                ["designOptionName"] = options[j]
                            });
                    }

                    instance["activeDesignOptions"] = activeDesignOptions;
                }

                hotlinkInstances.Add(instance);
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["hotlinkInstances"] = hotlinkInstances },
                ToAddOn,
                da);
        }

        // Shares the icon of SetActiveDesignOptionsInCombinations until it gets its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetActiveDesignOptionsInCombinations;

        public override Guid ComponentGuid =>
            new Guid("dda82e1d-f36c-421d-aa2b-70e78c5a5ca7");
    }
}
