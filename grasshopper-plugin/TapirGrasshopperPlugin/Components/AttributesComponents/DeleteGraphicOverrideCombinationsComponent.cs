using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GraphicOverrides;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class DeleteGraphicOverrideCombinationsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteGraphicOverrideCombinations";

        public DeleteGraphicOverrideCombinationsComponent()
            : base(
                "DeleteGraphicOverrideCombinations",
                "Deletes graphic override combinations. Their rules are kept. Available from Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "CombinationGuids",
                "Identifiers of the combinations to delete.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each combination (empty when it was deleted).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> combinationWrappers))
            {
                return;
            }

            var combinationIds = new JArray();
            foreach (var wrapper in combinationWrappers)
            {
                var combinationId = GraphicOverrideHelp.CombinationIdObject(wrapper);
                if (combinationId == null)
                {
                    this.AddError("Invalid combination identifier in the CombinationGuids input.");
                    return;
                }

                combinationIds.Add(new JObject { ["graphicOverrideCombinationId"] = combinationId });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["graphicOverrideCombinationIds"] = combinationIds },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteGraphicOverrideCombinations;

        public override Guid ComponentGuid =>
            new Guid("9126a48c-99cc-425a-b302-2232cfa28bf7");
    }
}
