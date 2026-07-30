using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    // Shared base for the attribute detail-getter commands. Takes the list of
    // attribute identifiers, runs the command and hands the per-attribute
    // response items to the derived component, which exposes every response
    // field as a separate output. Outputs stay aligned with the input list:
    // error items produce nulls (lists) or empty branches (trees), and the
    // per-item error message is reported on the ErrorMessages output.
    public abstract class AttributeStructuredGetterComponent : ArchicadAccessorComponent
    {
        protected AttributeStructuredGetterComponent(
            string name,
            string description)
            : base(
                name,
                description,
                GroupNames.Attributes)
        {
        }

        protected abstract string ResponseArrayKey { get; }

        protected abstract void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items);

        protected override void AddInputs()
        {
            InGenerics(
                "AttributeGuids",
                "Identifiers of the attributes to query.");
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

            SetOutputs(
                da,
                JsonOutputHelp.Items(
                    response,
                    ResponseArrayKey));
        }

        // Builds the house-style guid wrapper for the attributeId of a data
        // item; null for error items.
        protected static AttributeGuidWrapper AttributeIdOf(
            JToken item)
        {
            var guid = JsonOutputHelp.WrappedGuidOf(
                item,
                "attributeId");
            if (guid == null)
            {
                return null;
            }
            return new AttributeGuidWrapper
            {
                AttributeId = new AttributeGuidObject { Guid = guid }
            };
        }

        // Builds a guid wrapper from a nested AttributeIdArrayItem field
        // ({"attributeId": {"guid": ...}}); null when absent.
        protected static AttributeGuidWrapper WrappedAttributeIdOf(
            JToken wrapper)
        {
            var guid = JsonOutputHelp.WrappedGuidOf(
                wrapper,
                "attributeId");
            if (guid == null)
            {
                return null;
            }
            return new AttributeGuidWrapper
            {
                AttributeId = new AttributeGuidObject { Guid = guid }
            };
        }
    }
}
