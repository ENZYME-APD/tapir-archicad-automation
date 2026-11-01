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
                "Gets the links created by LinkElements that start from the given elements: the linked element " +
                "and the flags of each link. Links pointing to an element are not returned; query the " +
                "elements they start from. Returns an error item for an element that does not exist.",
                GroupNames.ElementDetails)
        {
        }

        protected override string ResponseArrayKey => "linksOfElements";

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "The elements whose outgoing links are returned.");
        }

        protected override void AddOutputs()
        {
            OutGenericTree(
                "LinkedElementGuids",
                "The element each link points to (one branch per input element).");

            OutIntegerTree(
                "Flags",
                "The flags stored on each link by LinkElements (one branch per input element).");

            OutErrorMessages(
                "Error message of each element (empty when successful).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var linkedElements = new DataTree<object>();
            var flags = new DataTree<object>();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                linkedElements.EnsurePath(path);
                flags.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");
                if (!(item["links"] is JArray links))
                {
                    continue;
                }

                foreach (var link in links)
                {
                    linkedElements.Add(
                        ElementIdOf(link),
                        path);
                    flags.Add(
                        JsonOutputHelp.Scalar(
                            link,
                            "flags"),
                        path);
                }
            }

            da.SetDataTree(
                0,
                linkedElements);
            da.SetDataTree(
                1,
                flags);
            da.SetDataList(
                2,
                errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetLinksOfElements;

        public override Guid ComponentGuid =>
            new Guid("e7d44924-1ae7-4b6f-81dd-686ffe3cdee3");
    }
}
