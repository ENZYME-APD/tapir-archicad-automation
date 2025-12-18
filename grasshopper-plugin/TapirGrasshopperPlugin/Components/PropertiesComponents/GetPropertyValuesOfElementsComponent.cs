using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Properties;

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
            if (!da.TryCreateFromList(
                    0,
                    out PropertiesObject properties))
            {
                return;
            }

            if (!da.TryCreateFromList(
                    1,
                    out ElementsObject elements))
            {
                return;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    new
                    {
                        elements = elements.Elements,
                        properties = properties.Properties
                    },
                    ToAddOn,
                    JHelp.Deserialize<PropertyValuesForElements>,
                    out PropertyValuesForElements response))
            {
                return;
            }

            var elementIds = new DataTree<ElementGuidWrapper>();
            var values = new DataTree<string>();

            for (var i = 0; i < response.PropertyValuesOrErrors.Count; i++)
            {
                var propertyValues = response.PropertyValuesOrErrors[i];

                if (propertyValues.PropertyValuesOrErrors == null)
                {
                    throw new Exception(
                        $"No property found for {elements.Elements[i]}!");
                }

                for (var pIndex = 0;
                     pIndex < propertyValues.PropertyValuesOrErrors.Count;
                     pIndex++)
                {
                    var path = new GH_Path(pIndex);
                    var valueOrError =
                        propertyValues.PropertyValuesOrErrors[pIndex];

                    elementIds.Add(
                        elements.Elements[i],
                        path);

                    values.Add(
                        valueOrError.PropertyValue?.Value,
                        path);
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