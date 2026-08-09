using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateHatchesComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateHatches";

        public CreateHatchesComponent()
            : base(
                "CreateHatches",
                "Create 2D Hatch (Fill) elements based on the given parameters. " +
                "Outline arcs and the zone special area percent can be given through the " +
                "AdditionalSettings input (arcs, roomSpecial).",
                GroupNames.ElementCreation)
        {
        }

        protected override string ArrayKey => "hatchesData";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("Polygons", "coordinates", FieldKind.PointsTree2D, "Outline points of each hatch (one branch per hatch, at least 3 points; only X and Y are used).", required: true, minPointsPerBranch: 3),
            new Field("ContourPenIndices", "contourPenIndex", FieldKind.Integer, "Pen index of the hatch contour."),
            new Field("FillPenIndices", "fillPenIndex", FieldKind.Integer, "Pen index of the fill."),
            new Field("FillBackgroundPenIndices", "fillBackgroundPenIndex", FieldKind.Integer, "Background pen index of the fill."),
            new Field("FillGuids", "fillId", FieldKind.AttributeGuid, "Fill attribute of the hatch."),
            new Field("BuildingMaterialGuids", "buildingMaterialId", FieldKind.AttributeGuid, "Building material attribute of the hatch."),
            new Field("ShowAreas", "showArea", FieldKind.Boolean, "Show the area text of the hatch."),
            new Field("LayerIndices", "layerIndex", FieldKind.Integer, "Layer attribute index to place the hatch on."),
            new Field("FloorIndices", "floorInd", FieldKind.Number, "Home story index of the hatch."),
            new Field("FavoriteNames", "favoriteName", FieldKind.Text, "Name of a favorite to base the new element on. Its settings are applied first, then the other inputs override them.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateHatches;

        public override Guid ComponentGuid =>
            new Guid("3168be46-b824-40cc-9d3a-9c0ee75bfc16");
    }
}
