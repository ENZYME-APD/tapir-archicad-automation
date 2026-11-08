using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class FilterElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "FilterElements";

        public FilterElementsComponent()
            : base(
                "FilterElements",
                "Filter elements by Archicad's flag filters, layer, home story, element type and hotlink. An element is kept when it matches all the given criteria.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Element Guids to filter.");

            InTexts(
                "Filters",
                defaultValue: nameof(ElementFilter.NoFilter),
                description: "Element filter.");

            InIntegers(
                "LayerIndices",
                "Optional. Keep only the elements on one of these layers (layer attribute indices).");

            InIntegers(
                "FloorIndices",
                "Optional. Keep only the elements whose home story has one of these indices.");

            InTexts(
                "ElementTypes",
                "Optional. Keep only the elements of one of these types (connect an ElementsType value list).");

            // No default value: an unconnected input must not filter.
            inManager.AddBooleanParameter(
                "FromHotlink",
                "FromHotlink",
                ("Optional. True keeps only the elements that came in through a placed hotlink (they are " +
                 "read-only), false only the project's own elements. Both are kept when not given.")
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            InGenerics(
                "HotlinkGuids",
                "Optional. Keep only the elements that came in through one of these placed hotlink instances " +
                "(elements of type Hotlink; the hotlinkId field of GetDetailsOfElements).");

            SetOptionality(
                new[]
                {
                    1,
                    2,
                    3,
                    4,
                    5,
                    6
                });
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
            AddAsSource<ElementFilterValueList>(
                document,
                1);
        }

        private static List<T> NullIfEmpty<T>(
            List<T> list)
        {
            return list == null || list.Count == 0 ? null : list;
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

            if (!da.TryGetList(
                    1,
                    out List<string> filters))
            {
                return;
            }

            // The optional criteria are empty lists when nothing is connected.
            da.TryGetList(
                2,
                out List<int> layerIndices);

            da.TryGetList(
                3,
                out List<int> floorIndices);

            da.TryGetList(
                4,
                out List<string> elementTypes);

            bool? fromHotlink = null;
            var fromHotlinkValue = false;
            if (da.GetData(
                    5,
                    ref fromHotlinkValue))
            {
                fromHotlink = fromHotlinkValue;
            }

            da.TryGetList(
                6,
                out List<GH_ObjectWrapper> hotlinkWrappers);
            List<ElementGuid> hotlinkIds = null;
            if (hotlinkWrappers != null && hotlinkWrappers.Count > 0)
            {
                hotlinkIds = ElementInputHelp.ElementGuids(
                    this,
                    hotlinkWrappers,
                    "HotlinkGuids");
                if (hotlinkIds == null)
                {
                    return;
                }
            }

            var filterElements = new FilterElementsObject
            {
                Elements = elements.Elements,
                Filters = filters,
                LayerIndices = NullIfEmpty(layerIndices),
                FloorIndices = NullIfEmpty(floorIndices),
                ElementTypes = NullIfEmpty(elementTypes),
                FromHotlink = fromHotlink,
                HotlinkIds = hotlinkIds
            };

            if (!filterElements.HasCriteria)
            {
                da.SetDataList(
                    0,
                    filterElements.Elements);

                return;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    filterElements,
                    ToAddOn,
                    JHelp.Deserialize<ElementsObject>,
                    out ElementsObject response))
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
