using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ModifyColumnsComponent : JsonItemsExecutorComponent
    {
        public override string CommandName => "ModifyColumns";

        public ModifyColumnsComponent()
            : base(
                "ModifyColumns",
                "Modify Column elements based on the given parameters. Each input item is a JSON object matching the command's documented item schema, e.g. {\"elementId\":{\"guid\":\"...\"},\"height\":3.0}.",
                GroupNames.Elements,
                "columnsWithDetails",
                "ItemsData",
                "One JSON object per item, matching the command's documented item schema (see component description).")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyColumns;

        public override Guid ComponentGuid =>
            new Guid("84cd940f-67e4-4b41-ad3e-b6a7af0eafa7");
    }
}
