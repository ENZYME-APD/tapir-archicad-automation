using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class RedrawCurrentDatabaseComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "RedrawCurrentDatabase";

        public RedrawCurrentDatabaseComponent()
            : base(
                "RedrawCurrentDatabase",
                "Redraws the current database, even when its window is in the background. Only the " +
                "visible region of its window is updated.",
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
                    "Failed to redraw the current database.");
            }
        }

        // Shares the icon of RebuildCurrentDatabase until it gets its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RebuildCurrentDatabase;

        public override Guid ComponentGuid =>
            new Guid("82bf003a-d7c3-45d2-9c76-9bf6c245cffd");
    }
}
