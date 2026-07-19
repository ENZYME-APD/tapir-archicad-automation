using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateAssociativeDimensionsComponent : JsonItemsExecutorComponent
    {
        public override string CommandName => "CreateAssociativeDimensions";

        public CreateAssociativeDimensionsComponent()
            : base(
                "CreateAssociativeDimensions",
                "Create associative linear dimensions from explicit witness point references. Each input item is a JSON object matching the command's documented item schema, e.g. {\"witnessPoints\":[...],\"lineCoordinate\":{\"x\":0,\"y\":0}}.",
                GroupNames.Elements,
                "dimensionsData",
                "ItemsData",
                "One JSON object per item, matching the command's documented item schema (see component description).")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateAssociativeDimensions;

        public override Guid ComponentGuid =>
            new Guid("24a2fb04-121e-408a-9f20-bace552ec1b5");
    }
}
