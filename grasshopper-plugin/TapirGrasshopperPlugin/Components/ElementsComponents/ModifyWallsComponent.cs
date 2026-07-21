using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ModifyWallsComponent : JsonItemsExecutorComponent
    {
        public override string CommandName => "ModifyWalls";

        public ModifyWallsComponent()
            : base(
                "ModifyWalls",
                "Modify Wall elements based on the given parameters. Each input item is a JSON object matching the command's documented item schema, e.g. {\"elementId\":{\"guid\":\"...\"},\"height\":3.0}.",
                GroupNames.Elements,
                "wallsWithDetails",
                "ItemsData",
                "One JSON object per item, matching the command's documented item schema (see component description).")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyWalls;

        public override Guid ComponentGuid =>
            new Guid("e7f0fec3-1341-4535-bfa4-31ae03978c93");
    }
}
