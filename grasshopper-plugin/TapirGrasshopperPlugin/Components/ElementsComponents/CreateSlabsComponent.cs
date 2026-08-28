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
                "The outline of each slab is a closed curve; its arc segments are kept as arcs.",
                GroupNames.ElementCreation)
        {
        }

        protected override string ArrayKey => "slabsData";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("Outlines", "polygonCoordinates", FieldKind.OutlineCurve, "The closed outline curve of each slab, one curve per slab. Line and arc segments are kept as they are; anything else is approximated with a polyline. Only X and Y are used.", required: true),
            new Field("Holes", "holes", FieldKind.HoleCurvesTree, "The closed curves of the voids in each slab, one branch per slab. A slab with no voids gets an empty branch."),
            new Field("Levels", "level", FieldKind.Number, "The Z coordinate of the reference plane of the slab.", required: true),
            new Field("Thicknesses", "thickness", FieldKind.Number, "Thickness of the slab."),
            new Field("ReferencePlaneLocations", "referencePlaneLocation", FieldKind.Text, "Reference plane location: Top, CoreTop, CoreBottom or Bottom.", valueList: () => new ReferencePlaneLocationValueList ()),
            new Field("FloorIndices", "floorIndex", FieldKind.Integer, "Home story index of the slab."),
            new Field("FavoriteNames", "favoriteName", FieldKind.Text, "Name of a favorite to base the new element on. Its settings are applied first, then the other inputs override them.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateSlabs;

        public override Guid ComponentGuid =>
            new Guid("6c7d2368-5d7f-4ef5-9313-03cb01d07ad5");
    }
}
