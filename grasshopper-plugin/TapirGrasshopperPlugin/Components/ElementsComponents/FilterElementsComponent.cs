using Grasshopper.Kernel;
using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class FilterElementsObj : AcceptsElementFilters
    {
        [JsonProperty("elements")]
        public List<ElementIdItemObj> Elements;

        [JsonProperty(
            "filters",
            NullValueHandling = NullValueHandling.Ignore)]
        private List<string> filters;

        [JsonIgnore]
        public List<string> Filters
        {
            get => filters;
            set => filters = AcceptElementFilters(value);
        }
    }

    public class FilterElementsComponent : ArchicadAccessorComponent
    {
        public static string Doc => "Filter elements.";
        public override string CommandName => "FilterElements";

        public FilterElementsComponent()
            : base(
                "Filter Elements",
                "FilterElems",
                Doc,
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            AddGenerics(
                "ElementGuids",
                "Elements Guids to filter.");

            AddTexts(
                "Filter",
                "Elements filter.",
                new List<string> { ElementFilter.NoFilter.ToString() });
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter(
                "ElementGuids",
                "ElementGuids",
                "List of element Guids matching the filter.",
                GH_ParamAccess.list);
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
            IGH_DataAccess DA)
        {
            ElementsObj inputElements = ElementsObj.Create(
                DA,
                0);
            if (inputElements == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input ElementGuids failed to collect data.");
                return;
            }

            List<string> filters = new();
            if (!DA.GetDataList(
                    1,
                    filters))
            {
                return;
            }

            FilterElementsObj filterElements = new()
            {
                Elements = inputElements.Elements, Filters = filters
            };

            if (filterElements.Filters == null)
            {
                DA.SetDataList(
                    0,
                    filterElements.Elements);
                return;
            }

            if (!GetConvertedResponse(
                    CommandName,
                    filterElements,
                    out ElementsObj elements))
            {
                return;
            }

            DA.SetDataList(
                0,
                elements.Elements);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.FilterElems;

        public override Guid ComponentGuid =>
            new("133ab85c-53f7-466d-8271-31c5518085e2");
    }
}