using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ModifyObjectsComponent : ModifyElementsComponentBase
    {
        public override string CommandName => "ModifyObjects";

        public ModifyObjectsComponent()
            : base(
                "ModifyObjects",
                "Modify Object elements. Only the connected optional inputs are changed on the elements. " +
                "Section attributes, pens, visibility and further settings can be changed through the " +
                "AdditionalSettings input (sectionFillId, useObjectPens, visibility, etc.).",
                GroupNames.ElementModification)
        {
        }

        protected override string ArrayKey => "objectsWithDetails";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("Points", "coordinates", FieldKind.Point3D, "New insertion point of the object."),
            new Field("Dimensions", "dimensions", FieldKind.Point3D, "Dimensions of the object (X, Y and Z sizes)."),
            new Field("Angles", "angle", FieldKind.Number, "Rotation angle in radians."),
            new Field("Pens", "pen", FieldKind.Integer, "Pen index of the object."),
            new Field("LineTypeGuids", "lineTypeId", FieldKind.AttributeGuid, "Line type attribute of the object."),
            new Field("SurfaceGuids", "surfaceId", FieldKind.AttributeGuid, "Surface attribute override of the object."),
            new Field("Reflected", "reflected", FieldKind.Boolean, "Mirror the object.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyObjects;

        public override Guid ComponentGuid =>
            new Guid("43430b2f-333f-4d11-9837-8b89f09eb3b9");
    }
}
