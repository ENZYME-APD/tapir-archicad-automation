using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ModifyLampsComponent : ModifyElementsComponentBase
    {
        public override string CommandName => "ModifyLamps";

        public ModifyLampsComponent()
            : base(
                "ModifyLamps",
                "Modify Lamp elements. Only the connected optional inputs are changed on the elements. " +
                "Section attributes, pens, visibility and further settings can be changed through the " +
                "AdditionalSettings input (sectionFillId, useObjectPens, visibility, etc.).",
                GroupNames.ElementModification,
                "lampsWithDetails",
                new List<Field>
                {
                    new Field("Points", "coordinates", FieldKind.Point3D, "New insertion point of the lamp."),
                    new Field("Dimensions", "dimensions", FieldKind.Point3D, "Dimensions of the lamp (X, Y and Z sizes)."),
                    new Field("Angles", "angle", FieldKind.Number, "Rotation angle in radians."),
                    new Field("Pens", "pen", FieldKind.Integer, "Pen index of the lamp."),
                    new Field("LineTypeGuids", "lineTypeId", FieldKind.AttributeGuid, "Line type attribute of the lamp."),
                    new Field("SurfaceGuids", "surfaceId", FieldKind.AttributeGuid, "Surface attribute override of the lamp."),
                    new Field("Reflected", "reflected", FieldKind.Boolean, "Mirror the lamp."),
                    new Field("LightIsOn", "lightIsOn", FieldKind.Boolean, "Turn the light of the lamp on or off.")
                })
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyLamps;

        public override Guid ComponentGuid =>
            new Guid("6540dda6-1336-4477-abb8-8d1349b0cb06");
    }
}
