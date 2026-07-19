using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateDoorsComponent : JsonItemsExecutorComponent
    {
        public override string CommandName => "CreateDoors";

        public CreateDoorsComponent()
            : base(
                "CreateDoors",
                "Create Door elements in host walls based on the given parameters. Each input item is a JSON object matching the command's documented item schema, e.g. {\"wallId\":{\"guid\":\"...\"},\"libraryPartName\":\"Door 26\",\"position\":1.0}.",
                GroupNames.Elements,
                "doorsData",
                "ItemsData",
                "One JSON object per item, matching the command's documented item schema (see component description).")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateDoors;

        public override Guid ComponentGuid =>
            new Guid("7eadb251-9dac-496d-98a6-8bba8e41e6c6");
    }
}
