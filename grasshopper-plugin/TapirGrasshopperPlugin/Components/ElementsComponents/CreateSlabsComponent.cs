using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateSlabsComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateSlabs";

        public CreateSlabsComponent()
            : base(
                "CreateSlabs",
                "Create Slab elements based on the given parameters. " +
                "Holes and arcs can be given through the AdditionalSettings input (polygonArcs, holes).",
                GroupNames.ElementCreation)
        {
        }

        protected override string ArrayKey => "slabsData";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("Polygons", "polygonCoordinates", FieldKind.PointsTree2D, "Outline points of each slab (one branch per slab, at least 3 points; only X and Y are used).", required: true, minPointsPerBranch: 3),
            new Field("Levels", "level", FieldKind.Number, "The Z coordinate of the reference plane of the slab.", required: true),
            new Field("Thicknesses", "thickness", FieldKind.Number, "Thickness of the slab."),
            new Field("ReferencePlaneLocations", "referencePlaneLocation", FieldKind.Text, "Reference plane location: Top, CoreTop, CoreBottom or Bottom.", valueList: () => new ReferencePlaneLocationValueList ()),
            new Field("FloorIndices", "floorIndex", FieldKind.Integer, "Home story index of the slab.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateSlabs;

        public override Guid ComponentGuid =>
            new Guid("6c7d2368-5d7f-4ef5-9313-03cb01d07ad5");
    }
}
