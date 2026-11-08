using System;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class ConsolidateLinesComponent : ConsolidationComponent
    {
        public override string CommandName => "ConsolidateLines";

        public ConsolidateLinesComponent()
            : base(
                "ConsolidateLines",
                "Runs the Line Consolidation menu command of Archicad on the current window, as the menu does, and " +
                "reports whether it ran together with what Archicad answered. Available from Archicad 27.")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ConsolidateLines;

        public override Guid ComponentGuid =>
            new Guid("5e3d148f-c124-4836-bfad-4d5759fab626");
    }
}
