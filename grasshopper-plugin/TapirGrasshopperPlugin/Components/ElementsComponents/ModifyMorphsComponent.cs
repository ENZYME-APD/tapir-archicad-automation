using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ModifyMorphsComponent : ModifyElementsComponentBase
    {
        public override string CommandName => "ModifyMorphs";

        public ModifyMorphsComponent()
            : base(
                "ModifyMorphs",
                "Modify Morph elements. Only the connected optional inputs are changed on the elements. " +
                "The body geometry, floor plan display and texture settings can be modified through the " +
                "AdditionalSettings input (body, showContour, displayOption, coverFillType, etc.).",
                GroupNames.ElementModification,
                "morphsWithDetails",
                new List<Field>
                {
                    new Field("Translations", "translation", FieldKind.Point3D, "Translation vector applied to the morph."),
                    new Field("RotationDegreesZ", "rotationDegreesZ", FieldKind.Number, "Rotation around the vertical axis in degrees."),
                    new Field("Levels", "level", FieldKind.Number, "Base level of the morph relative to the home story."),
                    new Field("BuildingMaterialGuids", "buildingMaterialId", FieldKind.AttributeGuid, "Building material attribute of the morph body."),
                    new Field("SurfaceGuids", "surfaceId", FieldKind.AttributeGuid, "Surface attribute override of the morph body."),
                    new Field("CastShadows", "castShadow", FieldKind.Boolean, "The morph casts shadow in 3D."),
                    new Field("ReceiveShadows", "receiveShadow", FieldKind.Boolean, "The morph receives shadow in 3D.")
                })
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyMorphs;

        public override Guid ComponentGuid =>
            new Guid("a4f78db1-8069-463d-a287-ae9a64b7e1a1");
    }
}
