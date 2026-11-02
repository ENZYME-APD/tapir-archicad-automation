using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Analytical;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class CreateAnalyticalLoadCasesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateAnalyticalLoadCases";

        public CreateAnalyticalLoadCasesComponent()
            : base(
                "CreateAnalyticalLoadCases",
                "Creates structural analysis load cases in the given load groups in one undo step and returns their identifiers.",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Name of each load case to create.");

            InGenerics(
                "LoadGroupGuids",
                "Identifier of the load group of each load case (input only 1 to use the same load group for all).");

            InTexts(
                "ActionTypes",
                "Action type of each load case (input only 1 to use the same action type for all): " +
                AnalyticalLoadEnums.ActionTypes + ".");

            InTexts(
                "LoadTypes",
                "Load type of each load case (input only 1 to use the same load type for all): " +
                AnalyticalLoadEnums.LoadCaseLoadTypes + ".");

            InTexts(
                "Descriptions",
                "Description of each load case (input only 1 to use the same description for all). " +
                "Optional; empty when it is omitted.");

            InTexts(
                "Durations",
                "Duration of each load case (input only 1 to use the same duration for all): " +
                AnalyticalLoadEnums.Durations + ". Optional; not set when it is omitted.");

            SetOptionality(
                new[]
                {
                    4,
                    5
                });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "LoadCaseGuids",
                "Identifier of each created load case (null when it could not be created).");

            OutErrorMessages(
                "Error message of each load case (empty when it was created successfully).");
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

            if (!AnalyticalHelp.TryGetIds(
                    this,
                    da,
                    1,
                    "LoadGroupGuids",
                    out List<LoadGroupGuid> loadGroupIds))
            {
                return;
            }

            if (!da.TryGetList(
                    2,
                    out List<string> actionTypes) ||
                !da.TryGetList(
                    3,
                    out List<string> loadTypes))
            {
                return;
            }

            da.TryGetList(
                4,
                out List<string> descriptions);
            da.TryGetList(
                5,
                out List<string> durations);

            if (!AnalyticalHelp.CheckRequiredCount(this, loadGroupIds.Count, names.Count, "LoadGroupGuids", "Names") ||
                !AnalyticalHelp.CheckRequiredCount(this, actionTypes.Count, names.Count, "ActionTypes", "Names") ||
                !AnalyticalHelp.CheckRequiredCount(this, loadTypes.Count, names.Count, "LoadTypes", "Names") ||
                !AnalyticalHelp.CheckOptionalCount(this, descriptions.Count, names.Count, "Descriptions", "Names") ||
                !AnalyticalHelp.CheckOptionalCount(this, durations.Count, names.Count, "Durations", "Names"))
            {
                return;
            }

            var loadCases = new JArray();
            for (var i = 0; i < names.Count; i++)
            {
                var loadCase = new JObject
                {
                    ["name"] = names[i],
                    ["actionType"] = AnalyticalHelp.ItemFor(actionTypes, i),
                    ["loadGroupId"] = AnalyticalHelp.IdOf(AnalyticalHelp.ItemFor(loadGroupIds, i)),
                    ["loadType"] = AnalyticalHelp.ItemFor(loadTypes, i)
                };
                if (descriptions.Count > 0)
                {
                    loadCase["description"] = AnalyticalHelp.ItemFor(descriptions, i);
                }
                if (durations.Count > 0)
                {
                    loadCase["duration"] = AnalyticalHelp.ItemFor(durations, i);
                }
                loadCases.Add(loadCase);
            }

            SetCadValuesWithCreatedIds<LoadCaseGuid>(
                CommandName,
                new JObject { ["loadCases"] = loadCases },
                ToAddOn,
                da,
                "loadCaseIds",
                "loadCaseId");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateAnalyticalLoadCases;

        public override Guid ComponentGuid =>
            new Guid("75b884bd-43d5-406f-8ebd-32b7eedb00d4");
    }
}
