using Grasshopper.Kernel;
using System;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetLinesComponent : AttributeDetailsJsonComponent
    {
        public override string CommandName => "GetLines";

        public GetLinesComponent()
            : base(
                "GetLines",
                "Get the details of the given Line attributes.")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetLines;

        public override Guid ComponentGuid =>
            new Guid("d27a0625-35bb-407b-82e0-6a8017c589e2");
    }
}
