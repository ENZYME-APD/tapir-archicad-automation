using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetDimensionDataComponent : ElementsJsonAccessorComponent
    {
        public override string CommandName => "GetDimensionData";

        public GetDimensionDataComponent()
            : base(
                "GetDimensionData",
                "Get witness point data (coordinates, measured values) from existing dimension chains.",
                GroupNames.Elements,
                "DimensionData")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetDimensionData;

        public override Guid ComponentGuid =>
            new Guid("2190e829-344e-4dd1-a748-7aafa8b695d9");
    }
}
