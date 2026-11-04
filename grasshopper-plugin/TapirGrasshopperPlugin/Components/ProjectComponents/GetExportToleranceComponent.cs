using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetExportToleranceComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetExportTolerance";

        public GetExportToleranceComponent()
            : base(
                "GetExportTolerance",
                "Gets the export tolerance: the largest distance allowed between an arc and the " +
                "straight segments that replace it when curves are segmented for export.",
                GroupNames.Project)
        {
        }

        protected override void AddOutputs()
        {
            outManager.AddNumberParameter(
                "ArcTolerance",
                "ArcTolerance",
                ("The largest distance allowed between an arc and the straight segments that replace " +
                 "it when Archicad segments curves for export, in meters.")
                    .WithTypeName("numberItem"),
                GH_ParamAccess.item);
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
                response.Value<double?>("arcTolerance"));
        }

        // Shares the icon of GetProjectPreferences until it gets its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetProjectPreferences;

        public override Guid ComponentGuid =>
            new Guid("5efbb878-8902-4456-bcff-0c4f87a24b84");
    }
}
