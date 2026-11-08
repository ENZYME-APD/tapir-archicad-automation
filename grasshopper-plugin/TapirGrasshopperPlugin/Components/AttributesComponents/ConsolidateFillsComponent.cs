using System;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class ConsolidateFillsComponent : ConsolidationComponent
    {
        public override string CommandName => "ConsolidateFills";

        public ConsolidateFillsComponent()
            : base(
                "ConsolidateFills",
                "Runs the Fill Consolidation menu command of Archicad on the current window, as the menu does, and " +
                "reports whether it ran together with what Archicad answered. Available from Archicad 27.")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ConsolidateFills;

        public override Guid ComponentGuid =>
            new Guid("70ca1ddd-f9e3-4a7c-aee9-1650c0e5bd8e");
    }
}
