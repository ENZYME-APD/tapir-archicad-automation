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
                "Gets the attributes in use, as Archicad reports them for the usedAttributes JSON query: parsed into " +
                "UsedAttributes when the report is a JSON object, otherwise as text in Raw. Helps to find the " +
                "attributes DeleteAttributes can remove before a handover. Available from Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                "UsedAttributes",
                "The report as JSON text, when Archicad answered with a JSON object. Its layout is not documented by " +
                "the Archicad API.");

            OutText(
                "Raw",
                "The report as text, when it is not a JSON object.");
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
