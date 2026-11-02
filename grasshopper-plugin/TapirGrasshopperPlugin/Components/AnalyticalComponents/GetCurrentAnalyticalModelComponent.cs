using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Types.Analytical;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class GetCurrentAnalyticalModelComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetCurrentAnalyticalModel";

        public GetCurrentAnalyticalModelComponent()
            : base(
                "GetCurrentAnalyticalModel",
                "Retrieves the Structural Analytical Model created for the current view. " +
                "Available from Archicad 27.",
                GroupNames.Analytical)
        {
        }

        protected override void AddOutputs()
        {
            OutGeneric(
                "AnalyticalModelGuid",
                "Identifier of the Structural Analytical Model of the current view.");
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
                AnalyticalHelp.IdFrom<AnalyticalModelGuid>(
                    response["analyticalModelId"]));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetCurrentAnalyticalModel;

        public override Guid ComponentGuid =>
            new Guid("50bd8d3f-f25d-419d-a22f-b9cfe6712cb1");
    }
}
