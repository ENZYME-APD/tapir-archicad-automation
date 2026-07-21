using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreatePolylinesComponent : JsonItemsExecutorComponent
    {
        public override string CommandName => "CreatePolylines";

        public CreatePolylinesComponent()
            : base(
                "CreatePolylines",
                "Create Polyline elements based on the given parameters. Each input item is a JSON object matching the command's documented item schema, e.g. {\"coordinates\":[{\"x\":0,\"y\":0},...],\"floorIndex\":0}.",
                GroupNames.Elements,
                "polylinesData",
                "ItemsData",
                "One JSON object per item, matching the command's documented item schema (see component description).")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreatePolylines;

        public override Guid ComponentGuid =>
            new Guid("cfc22062-ef8d-4eb8-855d-170216cd129f");
    }
}
