using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateZonesComponent : JsonItemsExecutorComponent
    {
        public override string CommandName => "CreateZones";

        public CreateZonesComponent()
            : base(
                "CreateZones",
                "Create Zone elements based on the given parameters. Each input item is a JSON object matching the command's documented item schema, e.g. {\"name\":\"Room\",\"numberStr\":\"01\",\"polygonCoordinates\":[{\"x\":0,\"y\":0},...]}.",
                GroupNames.Elements,
                "zonesData",
                "ItemsData",
                "One JSON object per item, matching the command's documented item schema (see component description).")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateZones;

        public override Guid ComponentGuid =>
            new Guid("efb3d7fd-67db-487d-8419-b05946060943");
    }
}
