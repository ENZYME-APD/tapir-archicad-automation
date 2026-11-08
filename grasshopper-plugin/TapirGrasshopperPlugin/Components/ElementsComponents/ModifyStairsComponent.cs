using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ModifyStairsComponent : ModifyElementsComponentBase
    {
        public override string CommandName => "ModifyStairs";

        public ModifyStairsComponent()
            : base(
                "ModifyStairs",
                "Modify Stair elements in place, keeping their baseline and position. Only the connected " +
                "optional inputs are changed on the elements. Archicad solves the stair again within its " +
                "baseline, so it may adjust the step values; read them back with GetDetailsOfElements. The " +
                "tread and riser settings and the structure of the flights and landings (treads, risers, " +
                "structures) can be modified through the AdditionalSettings input, in the fields of the " +
                "ModifyStairs command.",
                GroupNames.ElementModification)
        {
        }

        protected override string ArrayKey => "stairsWithDetails";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("FlightWidths", "flightWidth", FieldKind.Number, "The width of the flights."),
            new Field("TotalHeights", "totalHeight", FieldKind.Number, "The height of the stair relative to its bottom."),
            new Field("StepNumbers", "stepNum", FieldKind.Integer, "The number of steps (risers)."),
            new Field("RiserHeights", "riserHeight", FieldKind.Number, "The height of each riser."),
            new Field("TreadDepths", "treadDepth", FieldKind.Number, "The length of each going."),
            new Field("TreadDepthLocked", "treadDepthLocked", FieldKind.Boolean, "Whether the going lengths are locked."),
            new Field("TotalHeightLocked", "totalHeightLocked", FieldKind.Boolean, "Whether the total height is locked."),
            new Field("FinishVisible", "finishVisible", FieldKind.Boolean, "Whether the tread and riser finishes of the stair are shown.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyStairs;

        public override Guid ComponentGuid =>
            new Guid("49403588-c047-4820-b2e6-4eceeefd1ffb");
    }
}
