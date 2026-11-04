using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Project;

namespace TapirGrasshopperPlugin.Components.DesignOptionsComponents
{
    public class GetHotlinkDesignOptionTableDetailsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetHotlinkDesignOptionTableDetails";

        public GetHotlinkDesignOptionTableDetailsComponent()
            : base(
                "GetHotlinkDesignOptionTableDetails",
                "Reads the design option table of hotlink nodes: the design option sets, options and " +
                "combinations of the hotlinked source file, and which of them the node shows. " +
                "Available from Archicad 29.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "HotlinkNodeIds",
                "The hotlink nodes, for example the HotlinkNodeIds output of Hotlinks or CreateHotlinkNodes.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "HotlinkNodeIds",
                "The node each design option table belongs to (null for failed items).");

            OutTexts(
                "FilterModes",
                "What each node shows: MainModelOnly, FilterByOptions or FilterByCombination.");

            OutGenericTree(
                "Sets",
                "The names of the design option sets of the source file, one branch per node.");

            OutGenericTree(
                "Options",
                "The names of the design options of each set, one branch {node;set} per set.");

            OutGenericTree(
                "ActiveOptions",
                "The name of the option the node shows from each set, parallel to Sets " +
                "(null when none is active).");

            OutGenericTree(
                "Combinations",
                "The names of the design option combinations of the source file, one branch per node.");

            OutTexts(
                "ActiveCombinations",
                "The name of the combination each node shows (null when none is active).");

            OutErrorMessages(
                "Error message of each node (empty when its table was read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> nodeIdWrappers))
            {
                return;
            }

            var hotlinkNodes = new JArray();
            foreach (var wrapper in nodeIdWrappers)
            {
                var nodeId = GuidObject<HotlinkNodeGuid>.CreateFromWrapper(wrapper);
                if (nodeId == null)
                {
                    this.AddError(
                        "Invalid hotlink node identifier in the HotlinkNodeIds input.");
                    return;
                }

                hotlinkNodes.Add(
                    new JObject
                    {
                        ["hotlinkNodeId"] = new JObject { ["guid"] = nodeId.Guid }
                    });
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["hotlinkNodes"] = hotlinkNodes },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var nodeIds = new List<object>();
            var filterModes = new List<string>();
            var sets = new DataTree<object>();
            var options = new DataTree<object>();
            var activeOptions = new DataTree<object>();
            var combinations = new DataTree<object>();
            var activeCombinations = new List<string>();
            var errorMessages = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "hotlinkDesignOptionTables");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                sets.EnsurePath(path);
                activeOptions.EnsurePath(path);
                combinations.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    nodeIds.Add(null);
                    filterModes.Add(null);
                    activeCombinations.Add(null);
                    errorMessages.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                var nodeGuid = JsonOutputHelp.GuidOf(item["nodeGuid"]);
                nodeIds.Add(
                    nodeGuid == null
                        ? null
                        : new HotlinkNodeGuid { Guid = nodeGuid });
                filterModes.Add(item["filterMode"]?.ToString());
                activeCombinations.Add(item["activeCombination"]?.ToString());
                errorMessages.Add("");

                if (item["sets"] is JArray designOptionSets)
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
                            set["activeOption"]?.ToString(),
                            path);

                        if (set["options"] is JArray optionNames)
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

                if (item["combinations"] is JArray combinationNames)
                {
                    foreach (var name in combinationNames)
                    {
                        combinations.Add(
                            name.ToString(),
                            path);
                    }
                }
            }

            da.SetDataList(0, nodeIds);
            da.SetDataList(1, filterModes);
            da.SetDataTree(2, sets);
            da.SetDataTree(3, options);
            da.SetDataTree(4, activeOptions);
            da.SetDataTree(5, combinations);
            da.SetDataList(6, activeCombinations);
            da.SetDataList(7, errorMessages);
        }

        // Shares the icon of GetDesignOptions until it gets its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetDesignOptions;

        public override Guid ComponentGuid =>
            new Guid("d87f1fbb-ffb0-40c9-bceb-de57baf785ff");
    }
}
