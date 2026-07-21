using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateWindowsComponent : JsonItemsExecutorComponent
    {
        public override string CommandName => "CreateWindows";

        public CreateWindowsComponent()
            : base(
                "CreateWindows",
                "Create Window elements in host walls based on the given parameters. Each input item is a JSON object matching the command's documented item schema, e.g. {\"wallId\":{\"guid\":\"...\"},\"libraryPartName\":\"Window 26\",\"position\":1.0}.",
                GroupNames.Elements,
                "windowsData",
                "ItemsData",
                "One JSON object per item, matching the command's documented item schema (see component description).")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateWindows;

        public override Guid ComponentGuid =>
            new Guid("b981af9d-167a-45ce-a7ec-143bc5e53970");
    }
}
