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
                "Create Stair elements based on the given parameters. The baseline is given as points " +
                "(BaseLines) or as a curve with arcs for curved stairs (BaseLineCurves, in the Optional " +
                "inputs of the context menu). The settings without an input " +
                "(the baseline position, the landings and winders, the locks, the extra treads, the " +
                "walking line, the numbering, the walking direction, and the tread, riser and structure " +
                "settings of the flights and the landings, the same ones ModifyStairs accepts) can be " +
                "given through the AdditionalSettings input, in the fields of the CreateStairs command.",
                GroupNames.ElementCreation)
        {
        }

        protected override string ArrayKey => "stairsData";

        protected override bool HasAdditionalSettingsInput => false;

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("BaseLines", "baseLinePoints", FieldKind.PointsTree2D, "Base line points of each stair (one branch per stair, at least 2 points; only X and Y are used). Leave it empty when the baselines are given as curves in BaseLineCurves.", required: true, minPointsPerBranch: 2),
            new Field("ZCoordinates", "zCoordinate", FieldKind.Number, "Bottom level of the stair.", required: true),
            new Field("TotalHeights", "totalHeight", FieldKind.Number, "Total height of the stair."),
            new Field("FlightWidths", "flightWidth", FieldKind.Number, "Width of the stair flight."),
            new Field("StepNums", "stepNum", FieldKind.Integer, "Number of steps."),
            new Field("RiserHeights", "riserHeight", FieldKind.Number, "Height of the risers."),
            new Field("TreadDepths", "treadDepth", FieldKind.Number, "Depth of the treads."),
            new Field("FloorIndices", "floorIndex", FieldKind.Integer, "Home story index of the stair."),
            new Field("FavoriteNames", "favoriteName", FieldKind.Text, "Name of a favorite to base the new element on. Its settings are applied first, then the other inputs override them."),
            new Field("FinishVisible", "finishVisible", FieldKind.Boolean, "If false, the tread/riser finishes are hidden and only the stair structure is modeled.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        private static readonly List<Field> TrailingFieldDefinitions = new List<Field>
        {
            TrailingAdditionalSettingsField,
            new Field("BaseLineCurves", "baseLinePoints", FieldKind.BaseLineCurve, BaseLineCurvesDescription, arcsJsonKey: "baseLineArcs")
        };

        // Also the description of SolveStairBaseline's BaseLineCurves input.
        internal const string BaseLineCurvesDescription =
            "The baseline of each stair as a curve, one curve per stair, instead of the BaseLines points: " +
            "a polyline, or a polycurve of lines and arcs for a curved or spiral stair. Its corners become " +
            "baseLinePoints and its arcs baseLineArcs; any other curve is approximated with a polyline. Only " +
            "X and Y are used. Archicad keeps the circle of an arc but may shorten the flight to its risers, " +
            "so the stair can be shorter than the curve. Optional.";

        protected override string FirstFieldAlternative => "BaseLineCurves";

        protected override IReadOnlyList<Field> TrailingFields => TrailingFieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateStairs;

        public override Guid ComponentGuid =>
            new Guid("9efe8c4e-9357-4ef0-9f07-61944965d1e8");
    }
}
