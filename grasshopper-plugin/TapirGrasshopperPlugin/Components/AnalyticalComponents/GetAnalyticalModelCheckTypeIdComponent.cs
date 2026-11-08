using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class GetAnalyticalModelCheckTypeIdComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAnalyticalModelCheckTypeId";

        public GetAnalyticalModelCheckTypeIdComponent()
            : base(
                "GetAnalyticalModelCheckTypeId",
                "Retrieves the type identifier of the Structural Analytical Model check. " +
                "Available from Archicad 29.",
                GroupNames.Analytical)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                "ModelCheckTypeGuid",
                "Type identifier of the Structural Analytical Model check.");
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
                JsonOutputHelp.GuidOf(response["modelCheckTypeId"]));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAnalyticalModelCheckTypeId;

        public override Guid ComponentGuid =>
            new Guid("58eb4104-1ea0-4396-9337-543b816b7180");
    }
}
