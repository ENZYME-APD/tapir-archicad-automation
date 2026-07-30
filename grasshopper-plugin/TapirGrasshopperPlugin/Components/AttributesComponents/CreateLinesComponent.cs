using System;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class CreateLinesComponent : AttributeCreateJsonComponent
    {
        public override string CommandName => "CreateLines";

        public CreateLinesComponent()
            : base(
                "CreateLines",
                "Create or overwrite Line attributes based on the given parameters.",
                "lineDataArray")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateLines;

        public override Guid ComponentGuid =>
            new Guid("5c7c63da-0da6-4be7-947e-dbba74c94b34");
    }
}
