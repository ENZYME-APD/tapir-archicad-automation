using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Analytical;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class UpdateAnalyticalModelComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "UpdateAnalyticalModel";

        public UpdateAnalyticalModelComponent()
            : base(
                "UpdateAnalyticalModel",
                "Triggers an update of the given Structural Analytical Model, " +
                "or of the model of the current view when no model is given. " +
                "Available from Archicad 27.",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "AnalyticalModelGuid",
                "Identifier of the Structural Analytical Model to update. " +
                "Optional; the model of the current view is updated when it is omitted.");

            SetOptionality(0);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            if (da.TryGet(
                    0,
                    out GH_ObjectWrapper modelWrapper))
            {
                var modelId = GuidObject<AnalyticalModelGuid>.CreateFromWrapper(modelWrapper);
                if (modelId == null)
                {
                    this.AddError("Invalid identifier in the AnalyticalModelGuid input.");
                    return;
                }
                parameters["analyticalModelId"] = AnalyticalHelp.IdOf(modelId);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            AnalyticalHelp.ReportExecutionResult(
                this,
                response,
                "Failed to update the Structural Analytical Model.");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.UpdateAnalyticalModel;

        public override Guid ComponentGuid =>
            new Guid("9e5ba959-930f-4f3b-bb63-520eecf734bb");
    }
}
