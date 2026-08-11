using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateLineElementsComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateLineElements";

        public CreateLineElementsComponent()
            : base(
                "CreateLineElements",
                "Create 2D Line elements based on the given parameters.",
                GroupNames.ElementCreation)
        {
        }

        protected override string ArrayKey => "linesData";

        protected override bool HasAdditionalSettingsInput => false;

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("BegPoints", "begCoordinate", FieldKind.Point2D, "Beginning point of the line (only X and Y are used).", required: true),
            new Field("EndPoints", "endCoordinate", FieldKind.Point2D, "End point of the line (only X and Y are used).", required: true),
            new Field("LinePenIndices", "linePenIndex", FieldKind.Integer, "Pen index of the line."),
            new Field("LineTypeGuids", "lineTypeId", FieldKind.AttributeGuid, "Line type attribute of the line."),
            new Field("RoomSeparators", "roomSeparator", FieldKind.Boolean, "The line acts as a zone boundary."),
            new Field("LayerIndices", "layerIndex", FieldKind.Integer, "Layer attribute index to place the line on."),
            new Field("FloorIndices", "floorInd", FieldKind.Number, "Home story index of the line."),
            new Field("FavoriteNames", "favoriteName", FieldKind.Text, "Name of a favorite to base the new element on. Its settings are applied first, then the other inputs override them.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateLineElements;

        public override Guid ComponentGuid =>
            new Guid("9a70551b-e5a1-4feb-afa5-b16c6455b3c2");
    }
}
