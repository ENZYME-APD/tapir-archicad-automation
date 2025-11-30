using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetAllElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAllElements";

        public GetAllElementsComponent()
            : base(
                "AllElements",
                "Get all elements.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InTexts("Filters");
            InGenerics("Databases");

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
            if (!da.TryGetItems(
                    1,
                    out List<string> filters))
            {
                return;
            }

            var databases = DatabasesObj.Create(
                da,
                1);

            var elementFilters = new ElementFiltersObj
            {
                Filters = filters.Count > 0 ? filters : null,
                Databases =
                    databases is null || databases.Databases.Count == 0
                        ? null
                        : databases.Databases
            };


            if (!TryGetConvertedValues(
                    CommandName,
                    elementFilters,
                    SendToAddOn,
                    JHelp.Deserialize<ElementsObj>,
                    out ElementsObj response))
            {
                return;
            }

            da.SetDataList(
                0,
                response.Elements);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AllElems;

        public override Guid ComponentGuid =>
            new Guid("61085af7-4f11-49be-bd97-00effddf90af");
    }
}