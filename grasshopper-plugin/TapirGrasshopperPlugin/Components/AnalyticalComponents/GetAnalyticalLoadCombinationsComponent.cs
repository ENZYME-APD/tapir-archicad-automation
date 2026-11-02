using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Analytical;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class GetAnalyticalLoadCombinationsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAnalyticalLoadCombinations";

        public GetAnalyticalLoadCombinationsComponent()
            : base(
                "GetAnalyticalLoadCombinations",
                "Retrieves the structural analysis load combinations of the project with all their fields and load factors. " +
                "The load factor outputs have one branch per load combination.",
                GroupNames.Analytical)
        {
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "LoadCombinationGuids",
                "Identifier of each load combination.");

            OutTexts(
                "Names",
                "Name of each load combination.");

            OutTexts(
                "Descriptions",
                "Description of each load combination.");

            OutTexts(
                "Categories",
                "Category of each load combination: " + AnalyticalLoadEnums.Categories + ".");

            OutTexts(
                "NationalStandards",
                "National standard of each load combination (null when it is not set): " +
                AnalyticalLoadEnums.NationalStandards + ".");

            OutTexts(
                "Types",
                "Type of each load combination (null when it is not set; Archicad 27 and newer): " +
                AnalyticalLoadEnums.CombinationTypes + ".");

            OutGenericTree(
                "LoadCaseGuids",
                "Identifier of each load case of the combinations (one branch per load combination).");

            OutNumbers(
                "Factors",
                "Factor of each load case of the combinations (one branch per load combination; null when it is not set).");

            OutNumbers(
                "Multipliers",
                "Multiplier of each load case of the combinations (one branch per load combination).");

            OutErrorMessages(
                "Error message of each load combination (empty when it could be read).");
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

            var loadCombinationGuids = new List<object>();
            var names = new List<object>();
            var descriptions = new List<object>();
            var categories = new List<object>();
            var nationalStandards = new List<object>();
            var types = new List<object>();
            var loadCaseGuids = new DataTree<object>();
            var factors = new DataTree<object>();
            var multipliers = new DataTree<object>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "loadCombinations");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                loadCaseGuids.EnsurePath(path);
                factors.EnsurePath(path);
                multipliers.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    loadCombinationGuids.Add(null);
                    names.Add(null);
                    descriptions.Add(null);
                    categories.Add(null);
                    nationalStandards.Add(null);
                    types.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                loadCombinationGuids.Add(
                    AnalyticalHelp.IdFrom<LoadCombinationGuid>(item["loadCombinationId"]));
                names.Add(JsonOutputHelp.Scalar(item, "name"));
                descriptions.Add(JsonOutputHelp.Scalar(item, "description"));
                categories.Add(JsonOutputHelp.Scalar(item, "category"));
                nationalStandards.Add(JsonOutputHelp.Scalar(item, "nationalStandard"));
                types.Add(JsonOutputHelp.Scalar(item, "type"));
                errors.Add("");

                if (item["loadFactors"] is JArray loadFactors)
                {
                    foreach (var loadFactor in loadFactors)
                    {
                        loadCaseGuids.Add(
                            AnalyticalHelp.IdFrom<LoadCaseGuid>(loadFactor["loadCaseId"]),
                            path);
                        factors.Add(
                            JsonOutputHelp.Scalar(loadFactor, "factor"),
                            path);
                        multipliers.Add(
                            JsonOutputHelp.Scalar(loadFactor, "multiplier"),
                            path);
                    }
                }
            }

            da.SetDataList(0, loadCombinationGuids);
            da.SetDataList(1, names);
            da.SetDataList(2, descriptions);
            da.SetDataList(3, categories);
            da.SetDataList(4, nationalStandards);
            da.SetDataList(5, types);
            da.SetDataTree(6, loadCaseGuids);
            da.SetDataTree(7, factors);
            da.SetDataTree(8, multipliers);
            da.SetDataList(9, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAnalyticalLoadCombinations;

        public override Guid ComponentGuid =>
            new Guid("843ef1fc-4033-4811-bc0f-d34c9b43f756");
    }
}
