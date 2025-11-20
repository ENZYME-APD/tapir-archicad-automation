using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class SetPropertyValuesOfElementsComponent
        : ArchicadExecutorComponent
    {
        public static string Doc => "Set property values of elements.";
        public override string CommandName => "SetPropertyValuesOfElements";

        public SetPropertyValuesOfElementsComponent()
            : base(
                "Set Property Values",
                "SetPropertyValues",
                Doc,
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "PropertyGuid",
                "The property Guid to set the value for.");

            InGenerics(
                "ElementGuids",
                "Elements Guids to set the value for.");

            InTexts(
                "Values",
                "Single value or list of values to set for the corresponding elements.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var propertyId = PropertyIdObj.Create(
                da,
                0);
            if (propertyId == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input PropertyGuid failed to collect data.");
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

            var values = new List<string>();
            if (!da.GetDataList(
                    2,
                    values))
            {
                return;
            }


            if (values.Count != 1 && elements.Elements.Count != values.Count)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "The count of Values must be 1 or the same as the count of ElementGuids.");
                return;
            }

            var elemPropertyValues = new ElementPropertyValuesObj();

            elemPropertyValues.ElementPropertyValues =
                new List<ElementPropertyValueObj>();

            for (var i = 0; i < elements.Elements.Count; i++)
            {
                var elementId = elements.Elements[i];
                var elemPropertyValue = new ElementPropertyValueObj()
                {
                    ElementId = elementId.ElementId,
                    PropertyId = propertyId,
                    PropertyValue = new PropertyValueObj()
                    {
                        Value = values.Count == 1
                            ? values[0]
                            : values[i]
                    }
                };
                elemPropertyValues.ElementPropertyValues.Add(elemPropertyValue);
            }

            GetResponse(
                CommandName,
                elemPropertyValues);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetPropertyValues;

        public override Guid ComponentGuid =>
            new Guid("5d2aa76e-4a59-4b58-a5ce-51878c1478d0");
    }
}