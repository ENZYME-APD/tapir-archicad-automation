using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.DesignOptions;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.DesignOptionsComponents
{
    public class SetDesignOptionCombinationOfViewsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetDesignOptionCombinationOfViews";

        public SetDesignOptionCombinationOfViewsComponent()
            : base(
                "SetDesignOptionCombinationOfViews",
                "Set the design option combination the given views show, or make them show only the main model. Available from Archicad 27.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "NavigatorItemGuids",
                "Identifiers of the views (View Map navigator items).");

            InGenerics(
                "DesignOptionCombinationGuids",
                "Design option combination each view should show (parallel to NavigatorItemGuids). Leave it empty to make the views show only the main model.");
            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each view (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> navWrappers))
            {
                return;
            }

            var combinationWrappers = da.GetOptionals<GH_ObjectWrapper>(1);
            var mainModelOnly = combinationWrappers.Count == 0;

            if (!mainModelOnly && combinationWrappers.Count != navWrappers.Count)
            {
                this.AddError(
                    "The size of the inputs NavigatorItemGuids and DesignOptionCombinationGuids must be equal.");
                return;
            }

            var input = new SetDesignOptionCombinationOfViewsParameters
            {
                NavigatorItemIdsWithCombination = new List<NavigatorItemIdWithCombination>()
            };

            for (var i = 0; i < navWrappers.Count; i++)
            {
                var navigatorItemId = GuidObject<NavigatorGuid>.CreateFromWrapper(navWrappers[i]);
                if (navigatorItemId == null)
                {
                    this.AddError("Invalid navigator item identifier.");
                    return;
                }

                var item = new NavigatorItemIdWithCombination
                {
                    NavigatorItemId = navigatorItemId
                };

                if (mainModelOnly)
                {
                    item.MainModelOnly = true;
                }
                else
                {
                    item.DesignOptionCombinationId = GuidObject<DesignOptionCombinationGuid>
                        .CreateFromWrapper(combinationWrappers[i]);
                    if (item.DesignOptionCombinationId == null)
                    {
                        this.AddError(
                            "Invalid design option combination identifier in the DesignOptionCombinationGuids input.");
                        return;
                    }
                }

                input.NavigatorItemIdsWithCombination.Add(item);
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetActiveDesignOptionsInCombinations;

        public override Guid ComponentGuid =>
            new Guid("c58f2a73-91d6-4b3e-a7c0-2f4d6e8b1a39");
    }
}
