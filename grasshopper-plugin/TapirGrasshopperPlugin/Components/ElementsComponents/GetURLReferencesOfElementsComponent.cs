using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetURLReferencesOfElementsComponent
        : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetURLReferencesOfElements";

        public GetURLReferencesOfElementsComponent()
            : base(
                "GetURLReferencesOfElements",
                "Gets the web link (URL reference) of the given elements: a URL with a description, shown in " +
                "Archicad in the element's settings under Element Information > Link. An element without a " +
                "link gets the error APIERR_NOUSERDATA. The outputs are aligned with the input list of elements.",
                GroupNames.ElementDetails)
        {
        }

        protected override string ResponseArrayKey => "urlReferences";

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to get the web link of.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "URLs",
                "The URL of the link of each element (null for an error).");

            OutTexts(
                "Descriptions",
                "The description of the link of each element (null for an error).");

            OutErrorMessages(
                "Error message of each element (empty when successful), e.g. when it has no link.");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var urls = new List<string>();
            var descriptions = new List<string>();
            var errors = new List<string>();

            foreach (var item in items)
            {
                if (JsonOutputHelp.IsError(item))
                {
                    urls.Add(null);
                    descriptions.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                urls.Add(item?["url"]?.ToString());
                descriptions.Add(item?["description"]?.ToString());
                errors.Add("");
            }

            da.SetDataList(
                0,
                urls);
            da.SetDataList(
                1,
                descriptions);
            da.SetDataList(
                2,
                errors);
        }

        // Shares the icon of OpenWebsiteButton until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.OpenWebsiteButton;

        public override Guid ComponentGuid =>
            new Guid("4744043c-d846-49b1-8d7f-f669daa61a15");
    }
}
