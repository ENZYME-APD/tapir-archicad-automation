using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateStairsComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateStairs";

        public CreateStairsComponent()
            : base(
                "CreateStairs",
                "Create Stair elements based on the given parameters.",
                GroupNames.ElementCreation)
        {
        }

        protected override string ArrayKey => "stairsData";

        protected override bool HasAdditionalSettingsInput => false;

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("BaseLines", "baseLinePoints", FieldKind.PointsTree2D, "Base line points of each stair (one branch per stair, at least 2 points; only X and Y are used).", required: true, minPointsPerBranch: 2),
            new Field("ZCoordinates", "zCoordinate", FieldKind.Number, "Bottom level of the stair.", required: true),
            new Field("TotalHeights", "totalHeight", FieldKind.Number, "Total height of the stair."),
            new Field("FlightWidths", "flightWidth", FieldKind.Number, "Width of the stair flight."),
            new Field("StepNums", "stepNum", FieldKind.Integer, "Number of steps."),
            new Field("RiserHeights", "riserHeight", FieldKind.Number, "Height of the risers."),
            new Field("TreadDepths", "treadDepth", FieldKind.Number, "Depth of the treads."),
            new Field("FloorIndices", "floorIndex", FieldKind.Integer, "Home story index of the stair.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateStairs;

        public override Guid ComponentGuid =>
            new Guid("9efe8c4e-9357-4ef0-9f07-61944965d1e8");
    }
}
