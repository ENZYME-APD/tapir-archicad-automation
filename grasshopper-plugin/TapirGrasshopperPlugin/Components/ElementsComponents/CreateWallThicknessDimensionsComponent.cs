using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateWallThicknessDimensionsComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateWallThicknessDimensions";

        public CreateWallThicknessDimensionsComponent()
            : base(
                "CreateWallThicknessDimensions",
                "Create dimensions measuring the thickness of the given walls.",
                GroupNames.ElementCreation)
        {
        }

        protected override string ArrayKey => "dimensionsData";

        protected override bool HasAdditionalSettingsInput => false;

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("WallGuids", "wallId", FieldKind.ElementGuid, "Identifiers of the walls to dimension.", required: true),
            new Field("ReferencePoints", "referencePoint", FieldKind.Point2D, "Point on the wall where the thickness is measured (only X and Y are used).", required: true),
            new Field("Directions", "direction", FieldKind.Point2D, "Direction of the dimension line (only X and Y are used).", required: true)
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateWallThicknessDimensions;

        public override Guid ComponentGuid =>
            new Guid("21859652-daef-4d51-9fe4-3b634b8ab36f");
    }
}
