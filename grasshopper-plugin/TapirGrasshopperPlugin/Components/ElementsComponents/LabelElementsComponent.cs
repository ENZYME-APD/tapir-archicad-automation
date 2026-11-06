using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class LabelElementsComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateLabels";

        public LabelElementsComponent()
            : base(
                "LabelElements",
                "Create Label elements for the given elements. " +
                "The text style, the symbol label settings, the leader line and multi-run content can be " +
                "given through the AdditionalSettings input (style, symbolStyle, leaderLine, runs).",
                GroupNames.ElementCreation)
        {
        }

        protected override string ArrayKey => "labelsData";

        protected override bool HasAdditionalSettingsInput => false;

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("ElementGuids", "parentElementId", FieldKind.ElementGuid, "Identifiers of the elements to create labels for.", required: true),
            new Field("Texts", "text", FieldKind.Text, "Content of the label (required for text labels)."),
            new Field("BegPoints", "begCoordinate", FieldKind.Point2D, "Beginning point of the label pointer line (only X and Y are used)."),
            new Field("MidPoints", "midCoordinate", FieldKind.Point2D, "Middle point of the label pointer line (only X and Y are used)."),
            new Field("EndPoints", "endCoordinate", FieldKind.Point2D, "End point of the label pointer line (only X and Y are used)."),
            new Field("FloorIndices", "floorInd", FieldKind.Number, "Home story index of the label."),
            new Field("FavoriteNames", "favoriteName", FieldKind.Text, "Name of a favorite to base the new element on. Its settings are applied first, then the other inputs override them.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        private static readonly List<Field> TrailingFieldDefinitions = new List<Field>
        {
            new Field("LabelClasses", "labelClass", FieldKind.Text, "Label class: Text or Symbol. When not given, the Label tool default decides, which can be either."),
            new Field("FontNames", "style.fontName", FieldKind.Text, "Font of text labels, by name (e.g. Arial), as the Font attributes carry it. An element whose font name no loaded font has fails. Symbol labels take it in symbolStyle.fontName through AdditionalSettings."),
            TrailingAdditionalSettingsField
        };

        protected override IReadOnlyList<Field> TrailingFields => TrailingFieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.LabelElements;

        public override Guid ComponentGuid =>
            new Guid("ecdb0a59-f928-4ed3-88e1-cd9aea737b39");
    }
}
