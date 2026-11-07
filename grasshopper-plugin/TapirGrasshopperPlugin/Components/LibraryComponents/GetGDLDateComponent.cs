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
                "Gets the date GDL scripts see: year, month, day, hour, minute and second.",
                GroupNames.Library)
        {
        }

        private static readonly string[] DateComponents =
        {
            "Year", "Month", "Day", "Hour", "Minute", "Second"
        };

        protected override void AddOutputs()
        {
            foreach (var name in DateComponents)
            {
                OutInteger(
                    name,
                    $"The {name.ToLowerInvariant()} of the date GDL scripts see. Pass it to SetGDLDate to restore the date.");
            }
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

            for (int i = 0; i < DateComponents.Length; i++)
            {
                da.SetData(
                    i,
                    (int?)response["date"]?[DateComponents[i].ToLowerInvariant()]);
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetGDLDate;

        public override Guid ComponentGuid =>
            new Guid("7dd8065d-0969-44d3-85c0-38ca1beaf7c7");
    }
}
