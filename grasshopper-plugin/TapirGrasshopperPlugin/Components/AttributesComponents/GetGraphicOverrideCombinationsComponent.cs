using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GraphicOverrides;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetGraphicOverrideCombinationsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetGraphicOverrideCombinations";

        public GetGraphicOverrideCombinationsComponent()
            : base(
                "GetGraphicOverrideCombinations",
                "Gets all graphic override combinations of the project with the rules they contain. " +
                "Available from Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "CombinationGuids",
                "Identifier of each graphic override combination.");

            OutTexts(
                "Names",
                "Name of each combination.");

            OutGenericTree(
                "RuleGuids",
                "Rules of each combination in priority order (one branch per combination).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var combinationGuids = new List<GraphicOverrideCombinationGuidWrapper>();
            var names = new List<string>();
            var ruleGuids = new DataTree<GraphicOverrideRuleGuidWrapper>();

            var combinations = JsonOutputHelp.Items(
                response,
                "graphicOverrideCombinations");
            for (var i = 0; i < combinations.Count; i++)
            {
                var combination = combinations[i];
                combinationGuids.Add(
                    GraphicOverrideHelp.CombinationIdOf(
                        combination["graphicOverrideCombinationId"]));
                names.Add(combination["name"]?.ToString());

                var path = new GH_Path(i);
                ruleGuids.EnsurePath(path);
                ruleGuids.AddRange(
                    GraphicOverrideHelp.RuleIdsOf(combination["graphicOverrideRuleIds"]),
                    path);
            }

            da.SetDataList(0, combinationGuids);
            da.SetDataList(1, names);
            da.SetDataTree(2, ruleGuids);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetGraphicOverrideCombinations;

        public override Guid ComponentGuid =>
            new Guid("a8b20a64-7970-4f86-b6e7-c6fc79cb965a");
    }
}
