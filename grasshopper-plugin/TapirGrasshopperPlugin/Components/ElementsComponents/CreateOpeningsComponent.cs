using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateOpeningsComponent : JsonItemsExecutorComponent
    {
        public override string CommandName => "CreateOpenings";

        public CreateOpeningsComponent()
            : base(
                "CreateOpenings",
                "Create Opening elements in the given host elements. Each input item is a JSON object matching the command's documented item schema, e.g. {\"hostElementId\":{\"guid\":\"...\"},\"position\":{\"x\":0,\"y\":0,\"z\":1.0}}.",
                GroupNames.Elements,
                "openingsData",
                "ItemsData",
                "One JSON object per item, matching the command's documented item schema (see component description).")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateOpenings;

        public override Guid ComponentGuid =>
            new Guid("376250b5-de92-4346-a819-e10319d67961");
    }
}
