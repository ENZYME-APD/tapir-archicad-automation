using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.SolidOperationComponents
{
    public class GetSolidElementLinksComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetSolidElementLinks";

        public GetSolidElementLinksComponent()
            : base(
                "GetSolidElementLinks",
                "Get solid element operation links for each queried element, grouped by role (target or operator).",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to query.");
        }

        protected override void AddOutputs()
        {
            OutText(
                "SolidLinks",
                "JSON object with the solid element operation links of the queried elements.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(elements),
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
            Properties.Resources.GetSolidElementLinks;

        public override Guid ComponentGuid =>
            new Guid("d7c54912-2b06-4c8c-9535-35d6c6a25831");
    }
}
