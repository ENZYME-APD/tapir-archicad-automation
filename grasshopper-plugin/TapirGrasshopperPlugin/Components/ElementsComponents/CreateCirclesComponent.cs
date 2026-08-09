using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateCirclesComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateCircles";

        public CreateCirclesComponent()
            : base(
                "CreateCircles",
                "Create 2D Circle elements based on the given parameters.",
                GroupNames.ElementCreation)
        {
        }

        protected override string ArrayKey => "circlesData";

        protected override bool HasAdditionalSettingsInput => false;

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("Origins", "origin", FieldKind.Point2D, "Center point of the circle (only X and Y are used).", required: true),
            new Field("Radii", "radius", FieldKind.Number, "Radius of the circle.", required: true),
            new Field("LinePenIndices", "linePenIndex", FieldKind.Integer, "Pen index of the circle."),
            new Field("LineTypeGuids", "lineTypeId", FieldKind.AttributeGuid, "Line type attribute of the circle."),
            new Field("RoomSeparators", "roomSeparator", FieldKind.Boolean, "The circle acts as a zone boundary."),
            new Field("LayerIndices", "layerIndex", FieldKind.Integer, "Layer attribute index to place the circle on."),
            new Field("FloorIndices", "floorInd", FieldKind.Number, "Home story index of the circle."),
            new Field("FavoriteNames", "favoriteName", FieldKind.Text, "Name of a favorite to base the new element on. Its settings are applied first, then the other inputs override them.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateCircles;

        public override Guid ComponentGuid =>
            new Guid("305ce2a2-33d1-4180-b927-003f29c93e89");
    }
}
