using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateMorphsComponent : JsonItemsExecutorComponent
    {
        public override string CommandName => "CreateMorphs";

        public CreateMorphsComponent()
            : base(
                "CreateMorphs",
                "Create Morph elements from simple box definitions. Each input item is a JSON object matching the command's documented item schema, e.g. {\"boxBegCoordinate\":{\"x\":0,\"y\":0,\"z\":0},\"boxEndCoordinate\":{\"x\":1,\"y\":1,\"z\":1}}.",
                GroupNames.Elements,
                "morphsData",
                "ItemsData",
                "One JSON object per item, matching the command's documented item schema (see component description).")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateMorphs;

        public override Guid ComponentGuid =>
            new Guid("494ff831-5664-4886-ad51-79848e07750f");
    }
}
