using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    // Shared base for the attribute detail-getter commands. Their response
    // schemas are deeply nested and vary per attribute type (dash/line items,
    // profile geometry, composite skins, pen tables, ...), so the details are
    // returned as a JSON string that can be parsed downstream.
    public abstract class AttributeDetailsJsonComponent : ArchicadAccessorComponent
    {
        protected AttributeDetailsJsonComponent(
            string name,
            string description)
            : base(
                name,
                description,
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "AttributeGuids",
                "Identifiers of the attributes to query.");
        }

        protected override void AddOutputs()
        {
            OutText(
                "Details",
                "JSON object with the per-attribute details (or errors) of the queried attributes.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out AttributeGuidItemsObject input))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(input),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(
                0,
                response.ToString(Formatting.Indented));
        }
    }
}
