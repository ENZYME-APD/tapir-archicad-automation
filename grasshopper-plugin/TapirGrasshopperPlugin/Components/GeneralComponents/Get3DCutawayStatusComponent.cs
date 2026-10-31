using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class Get3DCutawayStatusComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "Get3DCutawayStatus";

        public Get3DCutawayStatusComponent()
            : base(
                "Get3DCutawayStatus",
                "Returns whether 3D Cutaway is on and whether the cutting planes are shown in the 3D window. " +
                "In Archicad 25-27 the 3D window may need to be open.",
                GroupNames.General)
        {
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "CutawayEnabled",
                "Whether 3D Cutaway is on in the 3D window.");

            OutBoolean(
                "CuttingPlanesVisible",
                "Whether the cutting planes are shown in the 3D window.");
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
                (bool?)response["cutawayEnabled"]);

            da.SetData(
                1,
                (bool?)response["cuttingPlanesVisible"]);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.Get3DCutawayStatus;

        public override Guid ComponentGuid =>
            new Guid("9339c6b4-d2be-4ef0-884f-45966bfed35d");
    }
}
