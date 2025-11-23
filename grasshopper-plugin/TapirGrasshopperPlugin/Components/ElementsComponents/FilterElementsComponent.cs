using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class FilterElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "FilterElements";

        public FilterElementsComponent()
            : base(
                "FilterElements",
                "Filter elements.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Elements Guids to filter.");

            InTexts(
                "Filter",
                "Elements filter.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "List of element Guids matching the filter.");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            base.AddedToDocument(document);

            new ElementFilterValueList().AddAsSource(
                this,
                1);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!ElementsObj.TryCreate(
                    da,
                    0,
                    out ElementsObj elements))
            {
                return;
            }

            if (!da.TryGetItems(
                    1,
                    out List<string> filters)) { return; }

            var filterElements = new FilterElementsObj
            {
                Elements = elements.Elements, Filters = filters
            };

            if (filterElements.Filters == null)
            {
                da.SetDataList(
                    0,
                    filterElements.Elements);
                return;
            }

            if (!TryGetConvertedResponse(
                    CommandName,
                    filterElements,
                    out ElementsObj response))
            {
                return;
            }

            da.SetDataList(
                0,
                response.Elements);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.FilterElems;

        public override Guid ComponentGuid =>
            new Guid("133ab85c-53f7-466d-8271-31c5518085e2");
    }
}