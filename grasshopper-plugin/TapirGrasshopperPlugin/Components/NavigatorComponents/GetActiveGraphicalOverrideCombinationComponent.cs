using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetActiveGraphicalOverrideCombinationComponent
        : ArchicadAccessorComponent
    {
        public override string CommandName =>
            "GetActiveGraphicalOverrideCombination";

        public GetActiveGraphicalOverrideCombinationComponent()
            : base(
                "ActiveGraphicalOverrideCombination",
                "Gets the graphic override combination applied to the current window of Archicad. " +
                "The outputs are empty when no combination is applied. Available from Archicad 30.",
                GroupNames.Navigator)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                "CombinationGuid",
                "Identifier of the graphic override combination of the current window. Empty when none is applied.");

            OutText(
                "Name",
                "Name of the graphic override combination of the current window. Empty when none is applied.");
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

            da.SetData(
                0,
                response?["combinationId"]?["guid"]?.ToString());

            da.SetData(
                1,
                response?["name"]?.ToString());
        }

        // Shares the icon of GetViewSettings until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetViewSettings;

        public override Guid ComponentGuid =>
            new Guid("6c1f0b8e-4d27-4a93-b5e1-2f8a7c3d9e14");
    }
}
