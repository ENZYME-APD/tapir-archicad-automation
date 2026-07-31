using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateArcsComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateArcs";

        public CreateArcsComponent()
            : base(
                "CreateArcs",
                "Create 2D Arc elements based on the given parameters.",
                GroupNames.ElementCreation,
                "arcsData",
                new List<Field>
                {
                    new Field("Origins", "origin", FieldKind.Point2D, "Center point of the arc (only X and Y are used).", required: true),
                    new Field("Radii", "radius", FieldKind.Number, "Radius of the arc.", required: true),
                    new Field("BegAngles", "begAngle", FieldKind.Number, "Beginning angle of the arc in radians.", required: true),
                    new Field("EndAngles", "endAngle", FieldKind.Number, "End angle of the arc in radians.", required: true),
                    new Field("LinePenIndices", "linePenIndex", FieldKind.Integer, "Pen index of the arc."),
                    new Field("LineTypeGuids", "lineTypeId", FieldKind.AttributeGuid, "Line type attribute of the arc."),
                    new Field("RoomSeparators", "roomSeparator", FieldKind.Boolean, "The arc acts as a zone boundary."),
                    new Field("LayerIndices", "layerIndex", FieldKind.Integer, "Layer attribute index to place the arc on."),
                    new Field("FloorIndices", "floorInd", FieldKind.Number, "Home story index of the arc.")
                },
                addAdditionalSettings: false)
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateArcs;

        public override Guid ComponentGuid =>
            new Guid("8ee9327e-6cf9-4c9e-8951-428dae0a4030");
    }
}
