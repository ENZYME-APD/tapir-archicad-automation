using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using System;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class GetPropertyValuesOfElementsComponent
        : ArchicadAccessorComponent
    {
        public static string Doc => "Get property values of elements.";
        public override string CommandName => "GetPropertyValuesOfElements";

        public GetPropertyValuesOfElementsComponent()
            : base(
                "GetPropertyValues",
                "Get property values of elements.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "PropertyGuids",
                "The property Guids to get the value for.");

            InGenerics(
                "ElementGuids",
                "Elements Guids to get the value for.");
        }

        protected override void AddOutputs()
        {
            OutGenericTree(
                "ElementGuids",
                "Elements Guids the property is available for.");

            OutTextTree(
                "Values",
                "The property values of the elements.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var properties = PropertiesObj.Create(
                da,
                0);

            if (properties == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input PropertyGuids failed to collect data.");
                return;
            }

            var elements = ElementsObj.Create(
                da,
                1);

            if (elements == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input ElementGuids failed to collect data.");
                return;
            }

            var elementsAndPropertyIds = new ElementsAndPropertyIdsObj()
            {
                Elements = elements.Elements,
                PropertyIds = properties.Properties
            };

            if (!GetConvertedResponse(
                    CommandName,
                    elementsAndPropertyIds,
                    out PropertyValuesForElements response))
            {
                return;
            }

            var elementIds = new DataTree<ElementIdItemObj>();
            var values = new DataTree<string>();

            for (var elementIndex = 0;
                 elementIndex < response.PropertyValuesOrErrors.Count;
                 elementIndex++)
            {
                var propertyValuesOrError =
                    response.PropertyValuesOrErrors[elementIndex];
                if (propertyValuesOrError.PropertyValuesOrErrors == null)
                {
                    continue;
                }

                for (var propertyIndex = 0;
                     propertyIndex < propertyValuesOrError
                         .PropertyValuesOrErrors.Count;
                     propertyIndex++)
                {
                    var propertyValueOrError =
                        propertyValuesOrError.PropertyValuesOrErrors[
                            propertyIndex];
                    if (propertyValueOrError.PropertyValue == null)
                    {
                        continue;
                    }

                    elementIds.Add(
                        elements.Elements[elementIndex],
                        new GH_Path(propertyIndex));
                    values.Add(
                        propertyValueOrError.PropertyValue.Value,
                        new GH_Path(propertyIndex));
                }
            }

            da.SetDataTree(
                0,
                elementIds);
            da.SetDataTree(
                1,
                values);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetPropertyValues;

        public override Guid ComponentGuid =>
            new Guid("c2a0a175-5cbc-4ec3-b5e2-744cd1be280d");
    }
}