using Grasshopper.Kernel;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetRenovationFiltersComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetRenovationFilters";

        public GetRenovationFiltersComponent()
            : base(
                "GetRenovationFilters",
                "Get the renovation filters of the project and the one the current window uses " +
                "(Document > Renovation > Renovation Filter).",
                GroupNames.Navigator)
        {
        }

        protected override void AddOutputs()
        {
            // The identifiers are texts, like the RenovationFilterGuid output of
            // ViewSettings and the RenovationFilterGuid input of SetViewSettings,
            // so a listed filter can be set on saved views directly.
            OutTexts(
                "RenovationFilterIds",
                "Identifier of each renovation filter. The same identifier is the RenovationFilterGuid of the ViewSettings and SetViewSettings components.");

            OutTexts(
                "Names",
                "Name of each renovation filter.");

            OutText(
                "ActiveRenovationFilterId",
                "Identifier of the renovation filter of the current window. Empty when it has none.");

            OutText(
                "ActiveRenovationFilterName",
                "Name of the renovation filter of the current window. Empty when it has none.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetConvertedCadValues(
                    CommandName,
                    null,
                    ToAddOn,
                    JHelp.Deserialize<RenovationFiltersResponse>,
                    out RenovationFiltersResponse response))
            {
                return;
            }

            da.SetDataList(
                0,
                response.RenovationFilters.Select(x => x.RenovationFilterId?.Guid));

            da.SetDataList(
                1,
                response.RenovationFilters.Select(x => x.Name));

            var activeId = response.ActiveRenovationFilterId?.Guid;
            if (string.IsNullOrEmpty(activeId))
            {
                return;
            }

            da.SetData(
                2,
                activeId);

            var active = response.RenovationFilters.FirstOrDefault(
                x => string.Equals(
                    x.RenovationFilterId?.Guid,
                    activeId,
                    StringComparison.OrdinalIgnoreCase));

            da.SetData(
                3,
                active?.Name);
        }

        // Shares the icon of ViewSettings until it has one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetViewSettings;

        public override Guid ComponentGuid =>
            new Guid("5c1e8f0a-9b7d-4e36-a2c4-7f3d0b8e6a91");
    }
}
