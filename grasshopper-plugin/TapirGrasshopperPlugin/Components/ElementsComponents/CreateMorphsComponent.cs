using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateMorphsComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateMorphs";

        public CreateMorphsComponent()
            : base(
                "CreateMorphs",
                "Create box shaped Morph elements based on the given parameters. " +
                "Either the Sizes input or a body given through the AdditionalSettings input is required. " +
                "Custom geometry, floor plan display and texture settings can be given through " +
                "AdditionalSettings (body, displayOption, coverFillType, etc.).",
                GroupNames.ElementCreation,
                "morphsData",
                new List<Field>
                {
                    new Field("BasePoints", "basePoint", FieldKind.Point3D, "Base point of each morph.", required: true),
                    new Field("Sizes", "size", FieldKind.Point3D, "Size of the box shaped morph (X, Y and Z dimensions)."),
                    new Field("Levels", "level", FieldKind.Number, "Base level of the morph relative to the home story."),
                    new Field("BuildingMaterialGuids", "buildingMaterialId", FieldKind.AttributeGuid, "Building material attribute of the morph body."),
                    new Field("SurfaceGuids", "surfaceId", FieldKind.AttributeGuid, "Surface attribute of the morph body."),
                    new Field("CastShadows", "castShadow", FieldKind.Boolean, "The morph casts shadow in 3D."),
                    new Field("ReceiveShadows", "receiveShadow", FieldKind.Boolean, "The morph receives shadow in 3D."),
                    new Field("FloorIndices", "floorIndex", FieldKind.Integer, "Home story index of the morph.")
                })
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateMorphs;

        public override Guid ComponentGuid =>
            new Guid("494ff831-5664-4886-ad51-79848e07750f");
    }
}
