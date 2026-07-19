using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ModifyDoorsComponent : JsonItemsExecutorComponent
    {
        public override string CommandName => "ModifyDoors";

        public ModifyDoorsComponent()
            : base(
                "ModifyDoors",
                "Modify Door elements based on the given parameters. Each input item is a JSON object matching the command's documented item schema, e.g. {\"elementId\":{\"guid\":\"...\"},\"position\":1.5}.",
                GroupNames.Elements,
                "doorsWithDetails",
                "ItemsData",
                "One JSON object per item, matching the command's documented item schema (see component description).")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyDoors;

        public override Guid ComponentGuid =>
            new Guid("c1b56fd7-26c6-4d9a-9902-5280aec836ae");
    }
}
