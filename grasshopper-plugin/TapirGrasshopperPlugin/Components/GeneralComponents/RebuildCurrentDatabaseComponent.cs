using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class RebuildCurrentDatabaseComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "RebuildCurrentDatabase";

        public RebuildCurrentDatabaseComponent()
            : base(
                "RebuildCurrentDatabase",
                "Rebuilds the current database, the one the API works on (see ChangeWindow). " +
                "Requires Archicad 27 or later.",
                GroupNames.General)
        {
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

            if ((bool?)response?["success"] != true)
            {
                this.AddError(
                    response?["error"]?["message"]?.ToString() ??
                    "Failed to rebuild the current database.");
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RebuildCurrentDatabase;

        public override Guid ComponentGuid =>
            new Guid("dcdc72dd-f093-44af-81cf-2ce9a6074aaa");
    }
}
