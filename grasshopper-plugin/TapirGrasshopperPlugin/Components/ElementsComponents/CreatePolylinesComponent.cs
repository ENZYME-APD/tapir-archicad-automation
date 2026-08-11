using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreatePolylinesComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreatePolylines";

        public CreatePolylinesComponent()
            : base(
                "CreatePolylines",
                "Create Polyline elements based on the given parameters. " +
                "Arcs can be given through the AdditionalSettings input (arcs).",
                GroupNames.ElementCreation)
        {
        }

        protected override string ArrayKey => "polylinesData";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("Polylines", "coordinates", FieldKind.PointsTree2D, "Points of each polyline (one branch per polyline, at least 2 points; only X and Y are used).", required: true, minPointsPerBranch: 2),
            new Field("LayerIndices", "layerIndex", FieldKind.Integer, "Index of the layer of the polyline."),
            new Field("LinePenIndices", "linePenIndex", FieldKind.Integer, "Pen index of the polyline."),
            new Field("LineTypeIndices", "lineTypeIndex", FieldKind.Integer, "Line type index of the polyline."),
            new Field("RoomSeparators", "roomSeparator", FieldKind.Boolean, "The polyline acts as a room separator."),
            new Field("FavoriteNames", "favoriteName", FieldKind.Text, "Name of a favorite to base the new element on. Its settings are applied first, then the other inputs override them.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreatePolylines;

        public override Guid ComponentGuid =>
            new Guid("cfc22062-ef8d-4eb8-855d-170216cd129f");
    }
}
