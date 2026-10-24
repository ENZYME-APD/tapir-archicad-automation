using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.DesignOptions;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.DesignOptionsComponents
{
    public class GetDesignOptionCombinationOfViewsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetDesignOptionCombinationOfViews";

        public GetDesignOptionCombinationOfViewsComponent()
            : base(
                "GetDesignOptionCombinationOfViews",
                "Get which design option combination the given views show. Available from Archicad 29.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "NavigatorItemGuids",
                "Identifiers of the views (View Map navigator items) to query.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "NavigatorItemGuids",
                "Identifiers of the queried views.");

            OutTexts(
                "Statuses",
                "What each view shows: MainModelOnly, Standard (a design option combination), Custom (a set of design options modified by the user) or Missing (a deleted combination).");

            OutTexts(
                "Names",
                "Names of the design option combination settings of the views.");

            OutGenerics(
                "DesignOptionCombinationGuids",
                "Identifiers of the design option combinations the views show (given only for the Standard status, when exactly one combination has the name).");

            OutErrorMessages(
                "Error message of each queried view (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out NavigatorItemsObject navigatorItems))
            {
                return;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    navigatorItems,
                    ToAddOn,
                    JHelp.Deserialize<GetDesignOptionCombinationOfViewsResponse>,
                    out GetDesignOptionCombinationOfViewsResponse response))
            {
                return;
            }

            var combinationOfViews =
                response.DesignOptionCombinationOfViews ?? new List<DesignOptionCombinationOfView>();

            da.SetDataList(
                0,
                combinationOfViews.Select(
                    v => v.NavigatorItemId == null
                        ? null
                        : new NavigatorGuidWrapper { NavigatorId = v.NavigatorItemId }));

            da.SetDataList(
                1,
                combinationOfViews.Select(v => v.Status));

            da.SetDataList(
                2,
                combinationOfViews.Select(v => v.Name));

            da.SetDataList(
                3,
                combinationOfViews.Select(v => v.DesignOptionCombinationId));

            da.SetDataList(
                4,
                combinationOfViews.Select(
                    v => v.Error == null
                        ? ""
                        : v.Error.Message ?? "Unknown error."));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetDesignOptionCombinations;

        public override Guid ComponentGuid =>
            new Guid("3e9b6d21-5c4a-4f87-b0d2-8a61c7f4e915");
    }
}
