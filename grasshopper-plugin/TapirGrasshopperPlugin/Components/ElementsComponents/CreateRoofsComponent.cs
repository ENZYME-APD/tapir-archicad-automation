using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateRoofsComponent : JsonItemsExecutorComponent
    {
        public override string CommandName => "CreateRoofs";

        public CreateRoofsComponent()
            : base(
                "CreateRoofs",
                "Create multi-plane Roof elements based on footprint, level and roof profile data. Each input item is a JSON object matching the command's documented item schema, e.g. {\"level\":3.0,\"polygonCoordinates\":[{\"x\":0,\"y\":0},...]}.",
                GroupNames.Elements,
                "roofsData",
                "ItemsData",
                "One JSON object per item, matching the command's documented item schema (see component description).")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateRoofs;

        public override Guid ComponentGuid =>
            new Guid("7bff9b96-91ba-434b-b052-fa88c2744ce6");
    }
}
