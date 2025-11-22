using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetAllElementsComponent : ArchicadAccessorComponent
    {
        public static string Doc => "Get all elements.";
        public override string CommandName => "GetAllElements";

        public GetAllElementsComponent()
            : base(
                "All Elements",
                "AllElems",
                Doc,
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Filter",
                "Elements filter.",
                new List<string> { ElementFilter.NoFilter.ToString() });

            InGenerics(
                "Databases",
                "Databases to find elements.");

            Params.Input[1].Optional = true;
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "List of element Guids matching the filter.");
        }


        protected override void Solve(
            IGH_DataAccess da)
        {
            var filters = new List<string>();
            if (!da.GetDataList(
                    0,
                    filters))
            {
                return;
            }

            var databases = DatabasesObj.Create(
                da,
                1);

            var elementFilters = new ElementFiltersObj()
            {
                Filters = filters.Count > 0 ? filters : null,
                Databases =
                    databases is null || databases.Databases.Count == 0
                        ? null
                        : databases.Databases
            };

            if (!GetConvertedResponse(
                    CommandName,
                    elementFilters,
                    out ElementsObj elements))
            {
                return;
            }

            da.SetDataList(
                0,
                elements.Elements);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AllElems;

        public override Guid ComponentGuid =>
            new Guid("61085af7-4f11-49be-bd97-00effddf90af");
    }
}