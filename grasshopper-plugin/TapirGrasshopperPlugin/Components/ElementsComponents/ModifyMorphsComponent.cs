using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ModifyMorphsComponent : JsonItemsExecutorComponent
    {
        public override string CommandName => "ModifyMorphs";

        public ModifyMorphsComponent()
            : base(
                "ModifyMorphs",
                "Modify Morph elements based on the given parameters. Each input item is a JSON object matching the command's documented item schema, e.g. {\"elementId\":{\"guid\":\"...\"},...}.",
                GroupNames.Elements,
                "morphsWithDetails",
                "ItemsData",
                "One JSON object per item, matching the command's documented item schema (see component description).")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyMorphs;

        public override Guid ComponentGuid =>
            new Guid("a4f78db1-8069-463d-a287-ae9a64b7e1a1");
    }
}
