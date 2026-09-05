using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class ChangeDrawingLinkComponent : CreateElementsComponentBase
    {
        public override string CommandName => "ChangeDrawingLink";

        public ChangeDrawingLinkComponent()
            : base(
                "ChangeDrawingLink",
                "Relink existing Drawing elements to different source navigator items. " +
                "Relinking replaces the Drawing element, so the ElementGuids output contains " +
                "the NEW identifier of each Drawing.",
                GroupNames.Navigator)
        {
        }

        protected override string ArrayKey => "drawingsWithNewLinks";

        protected override bool HasAdditionalSettingsInput => false;

        // Relinking replaces existing Drawings instead of creating new
        // elements, so the Tapir GH metadata inputs make no sense here.
        protected override bool SupportsElementMetadata => false;

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("DrawingGuids", "elementId", FieldKind.ElementGuid, "Identifiers of the Drawing elements to relink.", required: true),
            new Field("NavigatorItemGuids", "navigatorItemId", FieldKind.ElementGuid, "Identifiers of the navigator items to link the Drawings to.", required: true),
            new Field("LayoutDatabaseGuids", "layoutDatabaseId", FieldKind.ElementGuid, "Identifiers of the layout databases containing the Drawings.", required: true)
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ChangeDrawingLink;

        public override Guid ComponentGuid =>
            new Guid("5faa7cc1-c02a-4c8a-ac9c-3dd917e4d3e1");
    }
}
