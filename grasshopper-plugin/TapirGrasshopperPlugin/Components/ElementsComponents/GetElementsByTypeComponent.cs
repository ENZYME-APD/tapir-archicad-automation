using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Element;
using TapirGrasshopperPlugin.ResponseTypes.Navigator;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetElementsByTypeComponent : ArchicadAccessorComponent
    {
        public static string Doc => "Get all elements by type.";
        public override string CommandName => "GetElementsByType";

        public GetElementsByTypeComponent()
            : base(
                "ElementsByType",
                "Get all elements by type.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InText("ElementsType");

            InTexts(
                "Filters",
                defaultValue: nameof(ElementFilter.NoFilter));

            InGenerics("Databases");

            SetOptionality(
                new[]
                {
                    1,
                    2
                });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "List of element Guids matching the type and the filter.");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            base.AddedToDocument(document);

            new ElementTypeValueList().AddAsSource(
                this,
                0);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string eType))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<string> filters))
            {
                return;
            }

            var databases = DatabasesObject.Create(
                da,
                2);

            var input = new ElementsByTypeObj()
            {
                ElementType = eType,
                Filters =
                    filters is null || filters.Count == 0 ? null : filters,
                Databases =
                    databases is null || databases.Databases.Count == 0
                        ? null
                        : databases.Databases
            };

            if (!TryGetConvertedValues(
                    CommandName,
                    input,
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
            Properties.Resources.ElemsByType;

        public override Guid ComponentGuid =>
            new Guid("8075031e-b38d-4f3b-8e5c-8e740d13a091");
    }
}