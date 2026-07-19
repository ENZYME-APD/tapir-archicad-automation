using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetCalculationUnitsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetCalculationUnits";

        public GetCalculationUnitsComponent()
            : base(
                "GetCalculationUnits",
                "Get the project calculation units (length, area, volume, angle).",
                GroupNames.Project)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                "CalculationUnits",
                "JSON object describing the project's calculation units.");
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
            Properties.Resources.GetCalculationUnits;

        public override Guid ComponentGuid =>
            new Guid("42b28eda-a438-4d2b-94de-0ff00882251f");
    }
}
