using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Element;

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
            InText(
                "Type",
                "Elements type.");

            InTexts(
                "Filter",
                "Elements filter.");

            InGenerics(
                "Databases",
                "Databases to find elements.");

            Params.Input[2].Optional = true;
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
            if (!da.TryGetItem(
                    0,
                    out string eType))
            {
                return;
            }

            if (!da.TryGetItems(
                    1,
                    out List<string> filters))
            {
                return;
            }

            var databases = DatabasesObj.Create(
                da,
                2);

            var elementsByType = new ElementsByTypeObj()
            {
                ElementType = eType,
                Filters =
                    filters is null || filters.Count == 0 ? null : filters,
                Databases =
                    databases is null || databases.Databases.Count == 0
                        ? null
                        : databases.Databases
            };

            if (!TryGetConvertedResponse(
                    CommandName,
                    elementsByType,
                    out ElementsObj elements))
            {
                return;
            }

            da.SetDataList(
                0,
                elements.Elements);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ElemsByType;

        public override Guid ComponentGuid =>
            new Guid("8075031e-b38d-4f3b-8e5c-8e740d13a091");
    }
}