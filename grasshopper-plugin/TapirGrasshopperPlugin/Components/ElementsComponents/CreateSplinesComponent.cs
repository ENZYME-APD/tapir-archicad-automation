using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateSplinesComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateSplines";

        public CreateSplinesComponent()
            : base(
                "CreateSplines",
                "Create 2D Spline elements based on the given parameters. " +
                "Only auto-smoothed curves are supported (bezier handles are calculated by Archicad).",
                GroupNames.ElementCreation,
                "splinesData",
                new List<Field>
                {
                    new Field("Splines", "coordinates", FieldKind.PointsTree2D, "Points of each spline (one branch per spline, at least 3 points; only X and Y are used; do not repeat the first point for closed splines).", required: true, minPointsPerBranch: 3),
                    new Field("Closed", "closed", FieldKind.Boolean, "The spline is a closed curve."),
                    new Field("LinePenIndices", "linePenIndex", FieldKind.Integer, "Pen index of the spline."),
                    new Field("LineTypeGuids", "lineTypeId", FieldKind.AttributeGuid, "Line type attribute of the spline."),
                    new Field("RoomSeparators", "roomSeparator", FieldKind.Boolean, "The spline acts as a zone boundary."),
                    new Field("LayerIndices", "layerIndex", FieldKind.Integer, "Layer attribute index to place the spline on."),
                    new Field("FloorIndices", "floorInd", FieldKind.Number, "Home story index of the spline.")
                },
                addAdditionalSettings: false)
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateSplines;

        public override Guid ComponentGuid =>
            new Guid("e5858c3e-cfdc-4bec-bafb-30fd68ab31cb");
    }
}
