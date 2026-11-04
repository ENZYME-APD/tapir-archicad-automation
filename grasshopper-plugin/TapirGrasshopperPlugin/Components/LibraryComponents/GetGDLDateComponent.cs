using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class GetGDLDateComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetGDLDate";

        public GetGDLDateComponent()
            : base(
                "GetGDLDate",
                "Gets the date GDL scripts see, as the integer value the Archicad API returns.",
                GroupNames.Library)
        {
        }

        protected override void AddOutputs()
        {
            OutInteger(
                "Date",
                "The date GDL scripts see, as the integer value of the Archicad API. Pass it to SetGDLDate to restore it.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(0, (int?)response["date"]);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.IsUsableGDLString;

        public override Guid ComponentGuid =>
            new Guid("7dd8065d-0969-44d3-85c0-38ca1beaf7c7");
    }
}
