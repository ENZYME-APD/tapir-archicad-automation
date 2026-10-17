using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetLinksOfElementsComponent : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetLinksOfElements";

        public GetLinksOfElementsComponent()
            : base(
                "GetLinksOfElements",
                "Get the links created by LinkElements that start from each given element: the linked elements and the flags of the links, " +
                "one branch per queried element. Links pointing to an element are not returned; query the elements they start from.",
                GroupNames.ElementDetails)
        {
        }

        protected override string ResponseArrayKey => "linksOfElements";

        protected override void AddOutputs()
        {
            OutGenericTree(
                "LinkedElementGuids",
                "Identifiers of the elements the links of each queried element point to.");

            OutIntegerTree(
                "Flags",
                "Flags stored on each link by LinkElements, aligned with LinkedElementGuids.");

            OutTexts(
                "ErrorMessages",
                "Error message for each queried element (empty when successful).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var linkedElementGuids = new DataTree<object>();
            var flags = new DataTree<object>();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                linkedElementGuids.EnsurePath(path);
                flags.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");

                if (item?["links"] is JArray links)
                {
                    foreach (var link in links)
                    {
                        linkedElementGuids.Add(
                            ElementIdOf(link),
                            path);
                        flags.Add(
                            link.Value<int?>("flags") ?? 0,
                            path);
                    }
                }
            }

            da.SetDataTree(0, linkedElementGuids);
            da.SetDataTree(1, flags);
            da.SetDataList(2, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetLinksOfElements;

        public override Guid ComponentGuid =>
            new Guid("e2d6a9f4-3b7c-4e1d-8a5f-9c3b6d2e7f18");
    }
}
