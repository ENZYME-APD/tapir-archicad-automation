using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class SetActiveRenovationFilterComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetActiveRenovationFilter";

        public SetActiveRenovationFilterComponent()
            : base(
                "SetActiveRenovationFilter",
                "Set the renovation filter of the current window " +
                "(Document > Renovation > Renovation Filter), by identifier or by name.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "RenovationFilterId",
                "Identifier of the renovation filter, as listed by GetRenovationFilters. " +
                "Takes precedence over Name when both are given.");

            InText(
                "Name",
                "Name of the renovation filter. Used when RenovationFilterId is empty.");

            SetOptionality(
                new[]
                {
                    0,
                    1
                });
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult.Message),
                ExecutionResult.Doc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var renovationFilterText = da.GetOptional<string>(
                0,
                null);
            var name = da.GetOptional<string>(
                1,
                null);

            var parameters = new SetActiveRenovationFilterParameters();

            if (!string.IsNullOrWhiteSpace(renovationFilterText))
            {
                parameters.RenovationFilterId =
                    RenovationFilterGuid.FromString(renovationFilterText);
                if (parameters.RenovationFilterId == null)
                {
                    this.AddError("Invalid identifier in the RenovationFilterId input.");
                    return;
                }
            }
            else if (!string.IsNullOrEmpty(name))
            {
                parameters.Name = name;
            }
            else
            {
                this.AddError("Either RenovationFilterId or Name is required.");
                return;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    parameters,
                    ToAddOn,
                    ExecutionResult.Deserialize,
                    out ExecutionResult response))
            {
                return;
            }

            if (!response.Success)
            {
                this.AddError(response.Message());
            }

            da.SetData(
                0,
                response.Message());
        }

        // Shares the icon of SetViewSettings until it has one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetViewSettings;

        public override Guid ComponentGuid =>
            new Guid("e2b7a4d1-3f58-4c0e-9d6b-1a8c5e7f2b43");
    }
}
