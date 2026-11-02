using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class Set3DCutawayStatusComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "Set3DCutawayStatus";

        public Set3DCutawayStatusComponent()
            : base(
                "Set3DCutawayStatus",
                "Turns 3D Cutaway on or off and shows or hides the cutting planes in the 3D window. " +
                "In Archicad 25-27 the 3D window may need to be open.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            inManager.AddBooleanParameter(
                "CutawayEnabled",
                "CutawayEnabled",
                "Turns 3D Cutaway on or off. Optional; left unchanged when not given."
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            inManager.AddBooleanParameter(
                "CuttingPlanesVisible",
                "CuttingPlanesVisible",
                "Shows or hides the cutting planes. Optional; left unchanged when not given."
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            SetOptionality(new[] { 0, 1 });
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            if (da.TryGet(
                    0,
                    out bool cutawayEnabled))
            {
                parameters["cutawayEnabled"] = cutawayEnabled;
            }

            if (da.TryGet(
                    1,
                    out bool cuttingPlanesVisible))
            {
                parameters["cuttingPlanesVisible"] = cuttingPlanesVisible;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            if ((bool?)response?["success"] != true)
            {
                this.AddError(
                    response?["error"]?["message"]?.ToString() ??
                    "Failed to change the 3D Cutaway status.");
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.Set3DCutawayStatus;

        public override Guid ComponentGuid =>
            new Guid("61db35da-04bb-44f0-92f6-d1ec01a33fc0");
    }
}
