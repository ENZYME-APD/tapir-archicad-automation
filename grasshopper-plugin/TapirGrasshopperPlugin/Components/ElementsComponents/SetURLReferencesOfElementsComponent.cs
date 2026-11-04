using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class SetURLReferencesOfElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetURLReferencesOfElements";

        public SetURLReferencesOfElementsComponent()
            : base(
                "SetURLReferencesOfElements",
                "Sets the web link (URL reference) of the given elements: a URL with a description, shown in " +
                "Archicad in the element's settings under Element Information > Link. Fails with " +
                "APIERR_MOREUSER for an element that already holds a link.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to set the web link of.");

            InTexts(
                "URLs",
                "The URL of the link, e.g. https://www.graphisoft.com; at most 511 bytes in UTF-8 (input " +
                "only 1 to use the same URL for all).");

            InTexts(
                "Descriptions",
                "Optional. The description of the link, e.g. GRAPHISOFT Home Page; at most 255 bytes in " +
                "UTF-8. Empty when not given (input only 1 to use the same description for all).");

            SetOptionality(2);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when its link was set).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> elementWrappers) ||
                !this.TryGetElementIds(
                    elementWrappers,
                    "ElementGuids",
                    out var elementIds))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<string> urls) ||
                !ElementInputHelp.CheckOneOrMany(
                    this,
                    urls,
                    elementIds.Count,
                    "URLs"))
            {
                return;
            }

            var descriptions = da.OptionalList<string>(2);
            if (!this.CheckSizes(
                    elementIds.Count,
                    "ElementGuids",
                    ("Descriptions", descriptions.Count)))
            {
                return;
            }

            var items = new JArray();
            for (var i = 0; i < elementIds.Count; i++)
            {
                var item = new JObject
                {
                    ["elementId"] = elementIds[i],
                    ["url"] = urls.At(i) ?? ""
                };

                if (descriptions.Count > 0)
                {
                    item["description"] = descriptions.At(i) ?? "";
                }

                items.Add(item);
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["elementsWithURLReferences"] = items },
                ToAddOn,
                da);
        }

        // Shares the icon of OpenWebsiteButton until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.OpenWebsiteButton;

        public override Guid ComponentGuid =>
            new Guid("20caae5c-b249-499e-94cb-a6777b884840");
    }
}
