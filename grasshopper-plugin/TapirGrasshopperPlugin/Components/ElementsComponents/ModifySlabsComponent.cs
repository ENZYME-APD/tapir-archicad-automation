using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ModifySlabsComponent : JsonItemsExecutorComponent
    {
        public override string CommandName => "ModifySlabs";

        public ModifySlabsComponent()
            : base(
                "ModifySlabs",
                "Modify Slab elements based on the given parameters. Each input item is a JSON object matching the command's documented item schema, e.g. {\"elementId\":{\"guid\":\"...\"},\"level\":0.0}.",
                GroupNames.Elements,
                "slabsWithDetails",
                "ItemsData",
                "One JSON object per item, matching the command's documented item schema (see component description).")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifySlabs;

        public override Guid ComponentGuid =>
            new Guid("7d7adda2-6afd-4371-8f0d-854e82b65dd7");
    }
}
