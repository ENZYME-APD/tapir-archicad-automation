using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateTextsComponent : JsonItemsExecutorComponent
    {
        public override string CommandName => "CreateTexts";

        public CreateTextsComponent()
            : base(
                "CreateTexts",
                "Create standalone Text elements based on the given parameters. Each input item is a JSON object matching the command's documented item schema, e.g. {\"text\":\"Hello\",\"coordinate\":{\"x\":0,\"y\":0},\"height\":2.0}.",
                GroupNames.Elements,
                "textsData",
                "ItemsData",
                "One JSON object per item, matching the command's documented item schema (see component description).")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateTexts;

        public override Guid ComponentGuid =>
            new Guid("ea685cf3-04f6-42c4-a363-4bdbba034ba4");
    }
}
