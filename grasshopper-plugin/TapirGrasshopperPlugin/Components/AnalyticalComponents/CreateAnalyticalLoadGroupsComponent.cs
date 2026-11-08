using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Analytical;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class CreateAnalyticalLoadGroupsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateAnalyticalLoadGroups";

        public CreateAnalyticalLoadGroupsComponent()
            : base(
                "CreateAnalyticalLoadGroups",
                "Creates structural analysis load groups in one undo step and returns their identifiers.",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Name of each load group to create.");

            InTexts(
                "GroupTypes",
                "Type of each load group (input only 1 to use the same type for all): " +
                AnalyticalLoadEnums.GroupTypes + ".");

            InTexts(
                "Relations",
                "How the load cases of each load group are combined (input only 1 to use the same relation for all): " +
                AnalyticalLoadEnums.Relations + ".");

            InTexts(
                "LoadTypes",
                "Load type of each load group as free text (input only 1 to use the same load type for all). " +
                "Optional; empty when it is omitted.");

            SetOptionality(3);
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "LoadGroupGuids",
                "Identifier of each created load group (null when it could not be created).");

            OutErrorMessages(
                "Error message of each load group (empty when it was created successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> names) ||
                !da.TryGetList(
                    1,
                    out List<string> groupTypes) ||
                !da.TryGetList(
                    2,
                    out List<string> relations))
            {
                return;
            }

            da.TryGetList(
                3,
                out List<string> loadTypes);

            if (!AnalyticalHelp.CheckRequiredCount(this, groupTypes.Count, names.Count, "GroupTypes", "Names") ||
                !AnalyticalHelp.CheckRequiredCount(this, relations.Count, names.Count, "Relations", "Names") ||
                !AnalyticalHelp.CheckOptionalCount(this, loadTypes.Count, names.Count, "LoadTypes", "Names"))
            {
                return;
            }

            var loadGroups = new JArray();
            for (var i = 0; i < names.Count; i++)
            {
                var loadGroup = new JObject
                {
                    ["name"] = names[i],
                    ["groupType"] = AnalyticalHelp.ItemFor(groupTypes, i),
                    ["relation"] = AnalyticalHelp.ItemFor(relations, i)
                };
                if (loadTypes.Count > 0)
                {
                    loadGroup["loadType"] = AnalyticalHelp.ItemFor(loadTypes, i);
                }
                loadGroups.Add(loadGroup);
            }

            SetCadValuesWithCreatedIds<LoadGroupGuid>(
                CommandName,
                new JObject { ["loadGroups"] = loadGroups },
                ToAddOn,
                da,
                "loadGroupIds",
                "loadGroupId");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateAnalyticalLoadGroups;

        public override Guid ComponentGuid =>
            new Guid("7d430ef3-98a5-4cff-bb43-989d763128e7");
    }
}
