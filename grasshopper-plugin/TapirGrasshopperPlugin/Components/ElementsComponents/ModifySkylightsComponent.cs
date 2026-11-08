using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ModifySkylightsComponent : ModifyElementsComponentBase
    {
        public override string CommandName => "ModifySkylights";

        public ModifySkylightsComponent()
            : base(
                "ModifySkylights",
                "Modify Skylight elements. Only the connected optional inputs are changed on the elements. " +
                "The floor plan display, pens, line types, section fill, surface and the use-the-library-part's-" +
                "settings flags can be modified through the AdditionalSettings input, in the fields of the " +
                "ModifySkylights command. Not written: the owner roof or shell, the plane of a multi-plane roof " +
                "the skylight sits on and the library part's own parameters (use SetGDLParametersOfElements). " +
                "Archicad may set the elevation angle again from the owner.",
                GroupNames.ElementModification)
        {
        }

        protected override string ArrayKey => "skylightsWithDetails";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("AnchorPositions", "anchorPosition", FieldKind.Point2D, "Global XY coordinates of the skylight's anchor point; Archicad places the skylight on its owner roof or shell at this point."),
            new Field("AnchorPoints", "anchorPoint", FieldKind.Text, "Which point of the skylight AnchorPositions places: BottomCenter, BottomLeft, BottomRight, TopCenter, TopLeft or TopRight."),
            new Field("FixModes", "fixMode", FieldKind.Text, "What the skylight keeps when the angle or the height of its owner changes: Horizontal or Vertical."),
            new Field("Widths", "width", FieldKind.Number, "Width of the skylight."),
            new Field("Heights", "height", FieldKind.Number, "Height of the skylight."),
            new Field("AzimuthAngles", "azimuthAngle", FieldKind.Number, "Rotation angle of the skylight in the horizontal plane, measured clockwise, in radians."),
            new Field("ElevationAngles", "elevationAngle", FieldKind.Number, "Elevation angle of the owner's surface at the skylight, in radians."),
            new Field("Reflected", "reflected", FieldKind.Boolean, "Mirror the skylight.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifySkylights;

        public override Guid ComponentGuid =>
            new Guid("9a31a981-6118-42f0-a1ff-8ab5bb4cef87");
    }
}
