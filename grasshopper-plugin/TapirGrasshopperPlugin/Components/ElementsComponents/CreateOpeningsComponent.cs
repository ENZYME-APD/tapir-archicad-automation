using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateOpeningsComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateOpenings";

        public CreateOpeningsComponent()
            : base(
                "CreateOpenings",
                "Create Opening elements in the given owner elements (walls, slabs, beams or meshes).",
                GroupNames.ElementCreation,
                "openingsData",
                new List<Field>
                {
                    new Field("OwnerElementGuids", "ownerElementId", FieldKind.ElementGuid, "Identifiers of the elements to cut the openings into.", required: true),
                    new Field("BasePoints", "basePoint", FieldKind.Point3D, "Base point of the opening.", required: true),
                    new Field("Widths", "width", FieldKind.Number, "Width of the opening."),
                    new Field("Heights", "height", FieldKind.Number, "Height of the opening.")
                })
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateOpenings;

        public override Guid ComponentGuid =>
            new Guid("376250b5-de92-4346-a819-e10319d67961");
    }
}
