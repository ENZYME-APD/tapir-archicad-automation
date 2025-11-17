using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class GetPropertyValuesOfElementsComponent
        : ArchicadAccessorComponent
    {
        public GetPropertyValuesOfElementsComponent()
            : base(
                "Get Property Values",
                "GetPropertyValues",
                "Get property values of elements.",
                "Properties")
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter(
                "PropertyGuids",
                "PropertyGuids",
                "The property Guids to get the value for.",
                GH_ParamAccess.list);
            pManager.AddGenericParameter(
                "ElementGuids",
                "ElementGuids",
                "Element Guids to get the value for.",
                GH_ParamAccess.list);
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter(
                "ElementGuids",
                "ElementGuids",
                "Element Guids the property is available for.",
                GH_ParamAccess.tree);
            pManager.AddTextParameter(
                "Values",
                "Values",
                "The property values of the elements.",
                GH_ParamAccess.tree);
        }

        protected override void Solve(
            IGH_DataAccess DA)
        {
            var properties = PropertiesObj.Create(
                DA,
                0);
            if (properties == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input PropertyGuids failed to collect data.");
                return;
            }

            var elements = ElementsObj.Create(
                DA,
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

            var elementsAndPropertyIdsObj =
                JObject.FromObject(elementsAndPropertyIds);
            var response = SendArchicadAddOnCommand(
                "GetPropertyValuesOfElements",
                elementsAndPropertyIdsObj);
            if (!response.Succeeded)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    response.GetErrorMessage());
                return;
            }

            var propertyValuesForElements =
                response.Result.ToObject<PropertyValuesForElements>();
            var elementIds = new DataTree<ElementIdItemObj>();
            var values = new DataTree<string>();
            for (var elementIndex = 0;
                 elementIndex < propertyValuesForElements.PropertyValuesOrErrors
                     .Count;
                 elementIndex++)
            {
                var propertyValuesOrError =
                    propertyValuesForElements.PropertyValuesOrErrors[
                        elementIndex];
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

            DA.SetDataTree(
                0,
                elementIds);
            DA.SetDataTree(
                1,
                values);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetPropertyValues;

        public override Guid ComponentGuid =>
            new Guid("c2a0a175-5cbc-4ec3-b5e2-744cd1be280d");
    }
}