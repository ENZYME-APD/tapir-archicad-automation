using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Navigator;

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

        public override void AddedToDocument(
            GH_Document document)
        {
            base.AddedToDocument(document);

            new ElementFilterValueList().AddAsSource(
                this,
                0);
        }

        protected override void AddInputs()
        {
            InTexts(
                "Filters",
                defaultValue: nameof(ElementFilter.NoFilter));

            InGenerics(
                "Databases",
                "Databases to find elements in.");

            SetOptionality(
                new[]
                {
                    0,
                    1
                });
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
            if (!da.TryGetList(
                    0,
                    out List<string> filters))
            {
                return;
            }

            var databases = DatabasesObject.Create(
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

            if (!TryGetConvertedCadValues(
                    CommandName,
                    elementFilters,
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
            Properties.Resources.AllElems;

        public override Guid ComponentGuid =>
            new Guid("61085af7-4f11-49be-bd97-00effddf90af");
    }
}