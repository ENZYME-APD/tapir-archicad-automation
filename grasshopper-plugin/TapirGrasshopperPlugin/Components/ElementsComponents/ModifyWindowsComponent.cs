using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ModifyWindowsComponent : JsonItemsExecutorComponent
    {
        public override string CommandName => "ModifyWindows";

        public ModifyWindowsComponent()
            : base(
                "ModifyWindows",
                "Modify Window elements based on the given parameters. Each input item is a JSON object matching the command's documented item schema, e.g. {\"elementId\":{\"guid\":\"...\"},\"position\":1.5}.",
                GroupNames.Elements,
                "windowsWithDetails",
                "ItemsData",
                "One JSON object per item, matching the command's documented item schema (see component description).")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyWindows;

        public override Guid ComponentGuid =>
            new Guid("45a03e3b-0ebd-4f98-a50b-aa6653878ca7");
    }
}
