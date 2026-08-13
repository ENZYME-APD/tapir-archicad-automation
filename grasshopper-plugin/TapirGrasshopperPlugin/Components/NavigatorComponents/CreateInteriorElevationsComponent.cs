using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Components.ElementsComponents;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class CreateInteriorElevationsComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateInteriorElevations";

        public CreateInteriorElevationsComponent()
            : base(
                "CreateInteriorElevations",
                "Create Interior Elevation elements on the floor plan. Every consecutive pair " +
                "of the given points becomes one segment with its own viewpoint, so the corner " +
                "points of a room give an interior elevation of that room.",
                GroupNames.Navigator)
        {
        }

        protected override string ArrayKey => "interiorElevationsData";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("NodePoints", "nodeCoordinates", FieldKind.PointsTree2D, "Corner points of each interior elevation (one branch per elevation, at least 2 points; only X and Y are used). Repeat the first point at the end to close the chain.", required: true, minPointsPerBranch: 2),
            new Field("Depths", "depth", FieldKind.Number, "How far each segment looks."),
            new Field("Names", "name", FieldKind.Text, "Name of the interior elevation. Each segment is named after it."),
            new Field("Ids", "id", FieldKind.Text, "ID string of the interior elevation."),
            new Field("FloorIndices", "floorIndex", FieldKind.Integer, "Home story index of the interior elevation.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateInteriorElevations;

        public override Guid ComponentGuid =>
            new Guid("3f2b8c41-6d5a-4e9b-97c2-1a0e5d8b4f36");
    }
}
