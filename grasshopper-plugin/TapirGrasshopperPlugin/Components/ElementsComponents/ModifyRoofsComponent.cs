using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ModifyRoofsComponent : JsonItemsExecutorComponent
    {
        public override string CommandName => "ModifyRoofs";

        public ModifyRoofsComponent()
            : base(
                "ModifyRoofs",
                "Modify multi-plane Roof elements based on the given parameters. Each input item is a JSON object matching the command's documented item schema, e.g. {\"elementId\":{\"guid\":\"...\"},...}.",
                GroupNames.Elements,
                "roofsWithDetails",
                "ItemsData",
                "One JSON object per item, matching the command's documented item schema (see component description).")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyRoofs;

        public override Guid ComponentGuid =>
            new Guid("fdf98319-28ae-4ae0-9e7d-8a3810e4ff5e");
    }
}
