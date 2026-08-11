using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateHotspotsComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateHotspots";

        public CreateHotspotsComponent()
            : base(
                "CreateHotspots",
                "Create 2D Hotspot elements based on the given parameters.",
                GroupNames.ElementCreation)
        {
        }

        protected override string ArrayKey => "hotspotsData";

        protected override bool HasAdditionalSettingsInput => false;

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("Positions", "position", FieldKind.Point2D, "Position of the hotspot (only X and Y are used).", required: true),
            new Field("Heights", "height", FieldKind.Number, "Height of the hotspot."),
            new Field("PenIndices", "penIndex", FieldKind.Integer, "Pen index of the hotspot."),
            new Field("LayerIndices", "layerIndex", FieldKind.Integer, "Layer attribute index to place the hotspot on."),
            new Field("FloorIndices", "floorInd", FieldKind.Number, "Home story index of the hotspot."),
            new Field("FavoriteNames", "favoriteName", FieldKind.Text, "Name of a favorite to base the new element on. Its settings are applied first, then the other inputs override them.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateHotspots;

        public override Guid ComponentGuid =>
            new Guid("f33c8d8f-0c7a-485d-8ffb-112d969c3fea");
    }
}
