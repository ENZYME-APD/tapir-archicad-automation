using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateAssociativeDimensionsOnSectionComponent : JsonItemsExecutorComponent
    {
        public override string CommandName => "CreateAssociativeDimensionsOnSection";

        public CreateAssociativeDimensionsOnSectionComponent()
            : base(
                "CreateAssociativeDimensionsOnSection",
                "Create associative linear dimensions on section elements using common wall, slab, beam, column and opening presets. Each input item is a JSON object matching the command's documented item schema, e.g. {\"databaseId\":{\"guid\":\"...\"},\"elements\":[...]}.",
                GroupNames.Elements,
                "dimensionsData",
                "ItemsData",
                "One JSON object per item, matching the command's documented item schema (see component description).")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateAssociativeDimensionsOnSection;

        public override Guid ComponentGuid =>
            new Guid("24155e1d-1c5a-4a7b-8a5a-d8e384c281d1");
    }
}
