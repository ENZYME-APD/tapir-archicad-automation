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
    public class GetGraphicOverrideRuleGroupsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetGraphicOverrideRuleGroups";

        public GetGraphicOverrideRuleGroupsComponent()
            : base(
                "GetGraphicOverrideRuleGroups",
                "Gets all graphic override rule groups of the project with the rules they contain, " +
                "empty groups included. Available from Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "RuleGroupNames",
                "Name of each rule group.");

            OutGenericTree(
                "RuleGuids",
                "Rules of each rule group in their order (one branch per rule group).");
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

            var names = new List<string>();
            var ruleGuids = new DataTree<GraphicOverrideRuleGuidWrapper>();

            var ruleGroups = JsonOutputHelp.Items(
                response,
                "graphicOverrideRuleGroups");
            for (var i = 0; i < ruleGroups.Count; i++)
            {
                names.Add(ruleGroups[i]["name"]?.ToString());

                var path = new GH_Path(i);
                ruleGuids.EnsurePath(path);
                ruleGuids.AddRange(
                    GraphicOverrideHelp.RuleIdsOf(ruleGroups[i]["graphicOverrideRuleIds"]),
                    path);
            }

            da.SetDataList(0, names);
            da.SetDataTree(1, ruleGuids);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetGraphicOverrideRuleGroups;

        public override Guid ComponentGuid =>
            new Guid("1b0f04d1-b542-4c35-bf3e-c61dfcff6648");
    }
}
