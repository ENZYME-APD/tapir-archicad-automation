using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class GetCurrentWindowDrawingScaleComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetCurrentWindowDrawingScale";

        public GetCurrentWindowDrawingScaleComponent()
            : base(
                "GetCurrentWindowDrawingScale",
                "Returns the drawing scale of the current (active) window, the scale shown in the bottom bar, " +
                "as its denominator: 100.0 for 1:100, 20.0 for 1:20. It reads the scale of the current database " +
                "(floor plan, section, elevation, interior elevation, detail, worksheet or 3D document window, " +
                "each of which has its own). Layouts always return 1.0.",
                GroupNames.General)
        {
        }

        protected override void AddOutputs()
        {
            outManager.AddNumberParameter(
                "Scale",
                "Scale",
                "The denominator of the drawing scale of the current window: 100.0 for 1:100."
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
                (double?)response["scale"]);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetCurrentWindowDrawingScale;

        public override Guid ComponentGuid =>
            new Guid("d86a9b58-5cfa-494e-a81b-2711d1d402b7");
    }
}
