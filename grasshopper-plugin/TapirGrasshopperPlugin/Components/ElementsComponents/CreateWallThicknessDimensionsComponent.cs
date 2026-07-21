using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateWallThicknessDimensionsComponent : JsonItemsExecutorComponent
    {
        public override string CommandName => "CreateWallThicknessDimensions";

        public CreateWallThicknessDimensionsComponent()
            : base(
                "CreateWallThicknessDimensions",
                "Create associative wall thickness dimensions for the given walls. Each input item is a JSON object matching the command's documented item schema, e.g. {\"wallId\":{\"guid\":\"...\"},\"linePosition\":{\"x\":0,\"y\":0}}.",
                GroupNames.Elements,
                "dimensionsData",
                "ItemsData",
                "One JSON object per item, matching the command's documented item schema (see component description).")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateWallThicknessDimensions;

        public override Guid ComponentGuid =>
            new Guid("21859652-daef-4d51-9fe4-3b634b8ab36f");
    }
}
