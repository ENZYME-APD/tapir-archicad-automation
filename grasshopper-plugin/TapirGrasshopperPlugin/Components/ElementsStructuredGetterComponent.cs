using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components
{
    // Shared base for accessor commands that take a list of elements and
    // expose every field of the per-element response items as a separate
    // output. Outputs stay aligned with the input list: error items produce
    // nulls (lists) or empty branches (trees), and the per-item error message
    // is reported on the ErrorMessages output.
    public abstract class ElementsStructuredGetterComponent : ArchicadAccessorComponent
    {
        protected ElementsStructuredGetterComponent(
            string name,
            string description,
            string subCategory)
            : base(
                name,
                description,
                subCategory)
        {
        }

        protected abstract string ResponseArrayKey { get; }

        protected abstract void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items);

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to query.");
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

            SetOutputs(
                da,
                JsonOutputHelp.Items(
                    response,
                    ResponseArrayKey));
        }

        // Builds the house-style guid wrapper for an elementId field of a data
        // item; null for error items or when absent.
        public static ElementGuidWrapper ElementIdOf(
            JToken item,
            string key = "elementId")
        {
            var guid = JsonOutputHelp.WrappedGuidOf(
                item,
                key);
            if (guid == null)
            {
                return null;
            }
            return new ElementGuidWrapper
            {
                ElementId = new ElementGuid { Guid = guid }
            };
        }
    }
}
