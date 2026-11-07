using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ModifyAssociativeDimensionsComponent : ModifyElementsComponentBase
    {
        public override string CommandName => "ModifyAssociativeDimensions";

        public ModifyAssociativeDimensionsComponent()
            : base(
                "ModifyAssociativeDimensions",
                "Change the settings of existing linear dimensions (the Dimension tool's elements, also in " +
                "sections). Only the connected optional inputs are changed on the elements; the text and witness " +
                "line settings are applied to every witness point. The level an Elevation dimension measures from " +
                "can be changed through the AdditionalSettings input, in the fields of the " +
                "ModifyAssociativeDimensions command. Not written: the witness points, the dimension line position " +
                "and direction (use MoveElements), the marker size and the text content and position.",
                GroupNames.ElementModification)
        {
        }

        protected override string ArrayKey => "dimensionsWithDetails";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("Appearances", "appearance", FieldKind.Text, "Dimension type: Linear, Cumulative, Baseline or Elevation."),
            new Field("LinePens", "linePen", FieldKind.Integer, "Pen of the dimension and witness lines."),
            new Field("MarkerTypes", "marker.type", FieldKind.Text, "Marker type at the witness points, named after API_MarkerID: CrossLine, EmptyCircle, SlashLine60, OpenArrow30, ClosedArrow30, FullArrow30, SlashLine45, CrossCircle, OpenArrow90, ClosedArrow90, FullArrow90, FullCircle, PepitaCircle, BandArrow, OpenArrow60, ClosedArrow60, FullArrow60 or SlashLine75."),
            new Field("MarkerPens", "marker.pen", FieldKind.Integer, "Pen of the markers."),
            new Field("TextHeights", "textHeight", FieldKind.Number, "Height of the dimension texts in millimeters (paper size)."),
            new Field("TextPens", "textPen", FieldKind.Integer, "Pen of the dimension texts."),
            new Field("TextFrames", "textFrame", FieldKind.Boolean, "Frame around the dimension texts."),
            new Field("WitnessForms", "defaultWitnessForm", FieldKind.Text, "Witness line form: None, Small, Large or Fix."),
            new Field("WitnessValues", "defaultWitnessVal", FieldKind.Number, "Value that goes with the witness line form, in meters (for Fix the length of the witness lines).")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyAssociativeDimensions;

        public override Guid ComponentGuid =>
            new Guid("19ca48a9-16f8-4408-8a3b-154660be095f");
    }
}
