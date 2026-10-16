using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateColumnsComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateColumns";

        public CreateColumnsComponent()
            : base(
                "CreateColumns",
                "Create Column elements based on the given parameters. " +
                "Unlinked width and depth can be given through the AdditionalSettings input " +
                "(isWidthAndHeightLinked).",
                GroupNames.ElementCreation)
        {
        }

        protected override string ArrayKey => "columnsData";

        protected override bool HasAdditionalSettingsInput => false;

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("Points", "coordinates", FieldKind.Point3D, "Insertion points of the columns.", required: true),
            new Field("Heights", "height", FieldKind.Number, "Height of the column."),
            new Field("Widths", "width", FieldKind.Number, "Width of the column core."),
            new Field("Depths", "depth", FieldKind.Number, "Depth of the column core."),
            new Field("AxisRotationAngles", "axisRotationAngle", FieldKind.Number, "Rotation angle of the column axis in radians."),
            new Field("CoreAnchors", "coreAnchor", FieldKind.Text, "Core anchor: TopLeft, TopCenter, TopRight, MiddleLeft, Center, MiddleRight, BottomLeft, BottomCenter or BottomRight.", valueList: () => new AnchorPointValueList ()),
            new Field("FloorIndices", "floorIndex", FieldKind.Integer, "Home story index of the column."),
            new Field("FavoriteNames", "favoriteName", FieldKind.Text, "Name of a favorite to base the new element on. Its settings are applied first, then the other inputs override them.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        private static readonly List<Field> TrailingFieldDefinitions = new List<Field>
        {
            new Field("CircleBased", "circleBased", FieldKind.Boolean, "True for a round column cross section, false for rectangular (applied to all segments)."),
            new Field("BuildingMaterialGuids", "buildingMaterialId", FieldKind.AttributeGuid, "Building material attribute of the round or rectangular cross section (applied to all segments)."),
            new Field("ProfileGuids", "profileId", FieldKind.AttributeGuid, "Profile attribute; switches the cross section to this custom profile (applied to all segments)."),
            TrailingAdditionalSettingsField
        };

        protected override IReadOnlyList<Field> TrailingFields => TrailingFieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateColumns;

        public override Guid ComponentGuid =>
            new Guid("6d34d401-1119-497a-9014-ac05345973ff");
    }
}
