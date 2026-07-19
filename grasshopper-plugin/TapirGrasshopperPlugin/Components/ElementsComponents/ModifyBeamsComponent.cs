using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ModifyBeamsComponent : JsonItemsExecutorComponent
    {
        public override string CommandName => "ModifyBeams";

        public ModifyBeamsComponent()
            : base(
                "ModifyBeams",
                "Modify Beam elements based on the given parameters. Each input item is a JSON object matching the command's documented item schema, e.g. {\"elementId\":{\"guid\":\"...\"},\"width\":0.3}.",
                GroupNames.Elements,
                "beamsWithDetails",
                "ItemsData",
                "One JSON object per item, matching the command's documented item schema (see component description).")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyBeams;

        public override Guid ComponentGuid =>
            new Guid("537c6ad9-0c85-48ec-9544-54e0bbcca0e8");
    }
}
