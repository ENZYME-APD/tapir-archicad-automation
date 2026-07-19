using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateStairsComponent : JsonItemsExecutorComponent
    {
        public override string CommandName => "CreateStairs";

        public CreateStairsComponent()
            : base(
                "CreateStairs",
                "Create Stair elements based on the given baseline and parameters. Each input item is a JSON object matching the command's documented item schema, e.g. {\"baseline\":[{\"x\":0,\"y\":0},...],\"width\":1.2}.",
                GroupNames.Elements,
                "stairsData",
                "ItemsData",
                "One JSON object per item, matching the command's documented item schema (see component description).")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateStairs;

        public override Guid ComponentGuid =>
            new Guid("9efe8c4e-9357-4ef0-9f07-61944965d1e8");
    }
}
