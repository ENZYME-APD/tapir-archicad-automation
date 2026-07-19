using Grasshopper.Kernel;
using System;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetPenTablesComponent : AttributeDetailsJsonComponent
    {
        public override string CommandName => "GetPenTables";

        public GetPenTablesComponent()
            : base(
                "GetPenTables",
                "Get the details of the given Pen Table attributes.")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetPenTables;

        public override Guid ComponentGuid =>
            new Guid("0e011975-ca64-496f-b9ba-f223adf1d2f9");
    }
}
