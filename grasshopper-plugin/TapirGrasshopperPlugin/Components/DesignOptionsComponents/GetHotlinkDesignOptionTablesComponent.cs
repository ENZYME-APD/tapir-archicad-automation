using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Project;

namespace TapirGrasshopperPlugin.Components.DesignOptionsComponents
{
    public class GetHotlinkDesignOptionTablesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetHotlinkDesignOptionTables";

        public GetHotlinkDesignOptionTablesComponent()
            : base(
                "GetHotlinkDesignOptionTables",
                "Retrieves the design option table of the given hotlink nodes and placed hotlink instances: " +
                "the filter mode (main model only, by options or by combination), the design option sets, " +
                "options and combinations of the hotlinked module and which of them are active. The outputs " +
                "list the nodes first, then the instances. Available from Archicad 29.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "HotlinkNodeIds",
                "Optional. Hotlink nodes to read the design option table of, for example the HotlinkNodeIds " +
                "output of Hotlinks.");

            InGenerics(
                "HotlinkInstanceGuids",
                "Optional. Placed hotlink instances (Hotlink elements) to read the design option table of, " +
                "for example the Instances output of Hotlinks.");

            SetOptionality(new[] { 0, 1 });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "HotlinkNodeIds",
                "The hotlink node each table belongs to; for an instance, the node it is placed from " +
                "(null for failed items).");

            OutTexts(
                "FilterModes",
                "Which elements of the hotlinked module are shown: MainModelOnly, FilterByOptions, " +
                "FilterByCombination or Unknown.");

            OutGenericTree(
                "DesignOptionCombinations",
                "The names of the design option combinations of the hotlinked module, one branch per table.");

            OutTexts(
                "ActiveDesignOptionCombinations",
                "The name of the active combination of each table (null when none is active).");

            OutGenericTree(
                "DesignOptionSets",
                "The names of the design option sets of the hotlinked module, one branch per table.");

            OutGenericTree(
                "DesignOptions",
                "The names of the design options of each set, one branch {table;set} per set.");

            OutGenericTree(
                "ActiveDesignOptions",
                "The name of the active design option of each set, parallel to DesignOptionSets " +
                "(null when no option of the set is active).");

            OutErrorMessages(
                "Error message of each hotlink (empty when its table was read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var hotlinks = new JArray();

            foreach (var wrapper in da.GetOptionals<GH_ObjectWrapper>(0))
            {
                var nodeId = GuidObject<HotlinkNodeGuid>.CreateFromWrapper(wrapper);
                if (nodeId == null)
                {
                    this.AddError(
                        "Invalid hotlink node identifier in the HotlinkNodeIds input.");
                    return;
                }

                hotlinks.Add(
                    new JObject
                    {
                        ["hotlinkNodeId"] = new JObject { ["guid"] = nodeId.Guid }
                    });
            }

            foreach (var wrapper in da.GetOptionals<GH_ObjectWrapper>(1))
            {
                var elementId = GuidObject<ElementGuid>.CreateFromWrapper(wrapper);
                if (elementId == null)
                {
                    this.AddError(
                        "Invalid element identifier in the HotlinkInstanceGuids input.");
                    return;
                }

                hotlinks.Add(
                    new JObject
                    {
                        ["elementId"] = new JObject { ["guid"] = elementId.Guid }
                    });
            }

            if (hotlinks.Count == 0)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Warning,
                    "Connect HotlinkNodeIds or HotlinkInstanceGuids.");
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["hotlinks"] = hotlinks },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var nodeIds = new List<object>();
            var filterModes = new List<string>();
            var combinations = new DataTree<object>();
            var activeCombinations = new List<string>();
            var sets = new DataTree<object>();
            var options = new DataTree<object>();
            var activeOptions = new DataTree<object>();
            var errorMessages = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "hotlinkDesignOptionTables");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                combinations.EnsurePath(path);
                sets.EnsurePath(path);
                activeOptions.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    nodeIds.Add(null);
                    filterModes.Add(null);
                    activeCombinations.Add(null);
                    errorMessages.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                var nodeGuid = JsonOutputHelp.GuidOf(item["hotlinkNodeId"]);
                nodeIds.Add(
                    nodeGuid == null
                        ? null
                        : new HotlinkNodeGuid { Guid = nodeGuid });
                filterModes.Add(item["filterMode"]?.ToString());
                activeCombinations.Add(item["activeDesignOptionCombination"]?.ToString());
                errorMessages.Add("");

                if (item["designOptionCombinations"] is JArray combinationNames)
                {
                    foreach (var name in combinationNames)
                    {
                        combinations.Add(
                            name.ToString(),
                            path);
                    }
                }

                if (item["designOptionSets"] is JArray designOptionSets)
                {
                    for (var j = 0; j < designOptionSets.Count; j++)
                    {
                        var set = designOptionSets[j];
                        var setPath = new GH_Path(i, j);
                        options.EnsurePath(setPath);

                        sets.Add(
                            set["name"]?.ToString(),
                            path);
                        activeOptions.Add(
                            set["activeDesignOption"]?.ToString(),
                            path);

                        if (set["designOptions"] is JArray optionNames)
                        {
                            foreach (var name in optionNames)
                            {
                                options.Add(
                                    name.ToString(),
                                    setPath);
                            }
                        }
                    }
                }
            }

            da.SetDataList(0, nodeIds);
            da.SetDataList(1, filterModes);
            da.SetDataTree(2, combinations);
            da.SetDataList(3, activeCombinations);
            da.SetDataTree(4, sets);
            da.SetDataTree(5, options);
            da.SetDataTree(6, activeOptions);
            da.SetDataList(7, errorMessages);
        }

        // Shares the icon of GetDesignOptions until it gets its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetDesignOptions;

        public override Guid ComponentGuid =>
            new Guid("838b7d60-6702-414d-96c8-ccba1447cf82");
    }
}
