using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateAssociativeDimensionsOnSectionComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateAssociativeDimensionsOnSection";

        public CreateAssociativeDimensionsOnSectionComponent()
            : base(
                "CreateAssociativeDimensionsOnSection",
                "Create associative dimensions on a section database based on a preset. " +
                "Further options (skinBorderIndices, beginPlane, totalSizePlane, placeOnTop) " +
                "can be given through the AdditionalSettings input.",
                GroupNames.ElementCreation)
        {
        }

        protected override string ArrayKey => "dimensionsData";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("SectionElementGuids", "sectionElementId", FieldKind.ElementGuid, "Identifiers of the section elements to dimension on.", required: true),
            new Field("ReferencePoints", "referencePoint", FieldKind.Point2D, "Placement point of the dimension line in the section database (only X and Y are used).", required: true),
            new Field("Presets", "preset", FieldKind.Text, "Dimensioning preset: WallCompositeFaces, WallSkinBorders, SlabCompositeFaces, SlabSkinBorders, BeamOrColumnRefLineEndPoints, BeamOrColumnBoundingBoxCorners, DoorWindowWallHoleCorners or DoorWindowModelHotspots.", required: true, valueList: () => new DimensioningPresetValueList ()),
            new Field("Directions", "direction", FieldKind.Point2D, "Direction of the dimension line (only X and Y are used).")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateAssociativeDimensionsOnSection;

        public override Guid ComponentGuid =>
            new Guid("24155e1d-1c5a-4a7b-8a5a-d8e384c281d1");
    }
}
