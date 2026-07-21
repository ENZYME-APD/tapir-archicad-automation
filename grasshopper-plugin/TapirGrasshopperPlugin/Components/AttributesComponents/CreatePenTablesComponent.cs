using System;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class CreatePenTablesComponent : AttributeCreateJsonComponent
    {
        public override string CommandName => "CreatePenTables";

        public CreatePenTablesComponent()
            : base(
                "CreatePenTables",
                "Create or overwrite Pen Table attributes based on the given parameters.",
                "penTableDataArray")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreatePenTables;

        public override Guid ComponentGuid =>
            new Guid("c39adf73-f300-4fd4-ac2f-ffa5d17c8309");
    }
}
