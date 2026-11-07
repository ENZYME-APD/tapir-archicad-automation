using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetUsedAttributesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetUsedAttributes";

        public GetUsedAttributesComponent()
            : base(
                "GetUsedAttributes",
                "Gets how many attributes of each type are in use (layers, lines, fills, composites, surfaces, ...), " +
                "as Archicad reports them for the usedAttributes JSON query: the counts in UsedAttributes, the report " +
                "as text in Raw. Available from Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                "UsedAttributes",
                "The number of attributes in use per attribute type, as JSON text, e.g. {\"Layer\": 16, \"Line\": 9}. " +
                "Empty when the report could not be read.");

            OutText(
                "Raw",
                "The report as Archicad wrote it, e.g. Layer: 16, LineType: 9.");
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

            var usedAttributes = response["usedAttributes"];
            da.SetData(
                0,
                usedAttributes == null || usedAttributes.Type == JTokenType.Null
                    ? null
                    : usedAttributes.ToString(Formatting.Indented));

            da.SetData(
                1,
                response["raw"]?.ToString());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetUsedAttributes;

        public override Guid ComponentGuid =>
            new Guid("fb80fb7f-4ba0-4226-b623-0eba078bcc9c");
    }
}
