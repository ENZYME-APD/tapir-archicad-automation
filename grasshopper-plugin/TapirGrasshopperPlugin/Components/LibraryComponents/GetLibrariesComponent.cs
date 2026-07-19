using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class GetLibrariesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetLibraries";

        public GetLibrariesComponent()
            : base(
                "GetLibraries",
                "Get the list of loaded libraries.",
                GroupNames.Library)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                "Libraries",
                "JSON object with the list of project libraries.");
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

            da.SetData(
                0,
                response.ToString(Formatting.Indented));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetLibraries;

        public override Guid ComponentGuid =>
            new Guid("d5a0af65-47c5-43ba-b6c5-9900feedce9d");
    }
}
