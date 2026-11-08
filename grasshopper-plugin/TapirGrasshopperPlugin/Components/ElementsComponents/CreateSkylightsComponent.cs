using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateSkylightsComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateSkylights";

        public CreateSkylightsComponent()
            : base(
                "CreateSkylights",
                "Create Skylight elements in the given owner roofs or shells, at a floor plan anchor position. " +
                "The settings not given come from the Skylight tool defaults or the favorite. The floor plan " +
                "display, pens, line types, section fill, surface and the use-the-library-part's-settings flags " +
                "can be given through the AdditionalSettings input, in the fields of the CreateSkylights command.",
                GroupNames.ElementCreation)
        {
        }

        protected override string ArrayKey => "skylightsData";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("OwnerGuids", "ownerId", FieldKind.ElementGuid, "Identifiers of the roofs (single- or multi-plane) or shells to place the skylights in.", required: true),
            new Field("AnchorPositions", "anchorPosition", FieldKind.Point2D, "Global XY coordinates of the skylight's anchor point; Archicad places the skylight on its owner roof or shell at this point.", required: true),
            new Field("AnchorPoints", "anchorPoint", FieldKind.Text, "Which point of the skylight AnchorPositions places: BottomCenter, BottomLeft, BottomRight, TopCenter, TopLeft or TopRight."),
            new Field("FixModes", "fixMode", FieldKind.Text, "What the skylight keeps when the angle or the height of its owner changes: Horizontal or Vertical."),
            new Field("Widths", "width", FieldKind.Number, "Width of the skylight."),
            new Field("Heights", "height", FieldKind.Number, "Height of the skylight."),
            new Field("AzimuthAngles", "azimuthAngle", FieldKind.Number, "Rotation angle of the skylight in the horizontal plane, measured clockwise, in radians."),
            new Field("ElevationAngles", "elevationAngle", FieldKind.Number, "Elevation angle of the owner's surface at the skylight, in radians. Archicad may set it again from the owner."),
            new Field("Reflected", "reflected", FieldKind.Boolean, "Mirror the skylight."),
            new Field("LibraryPartNames", "libraryPartName", FieldKind.Text, "Localized name of a skylight library part to place instead of the tool default's."),
            new Field("FavoriteNames", "favoriteName", FieldKind.Text, "Name of the Skylight favorite to use as a base for the skylight.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateSkylights;

        public override Guid ComponentGuid =>
            new Guid("6e288ea2-fee8-49a3-8a19-7859a86a0e29");
    }
}
