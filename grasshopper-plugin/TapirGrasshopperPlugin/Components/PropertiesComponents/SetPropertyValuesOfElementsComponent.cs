using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Properties;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class SetPropertyValuesOfElementsComponent
        : ArchicadExecutorComponent
    {
        public override string CommandName => "SetPropertyValuesOfElements";

        public SetPropertyValuesOfElementsComponent()
            : base(
                "SetPropertyValues",
                "Set property values of elements.",
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
            if (!da.TryCreate(
                    0,
                    out PropertyGuidObject propertyId))
            {
                return;
            }

            if (!da.TryCreateFromList(
                    1,
                    out ElementsObject elements))
            {
                return;
            }

            if (!da.TryGetList(
                    2,
                    out List<string> values))
            {
                return;
            }

            if (values.Count != 1 && elements.Elements.Count != values.Count)
            {
                this.AddError(
                    "The count of Values must be 1 or the same as the count of ElementGuids.");
                return;
            }

            var input = new ElementPropertyValuesObj
            {
                ElementPropertyValues = new List<ElementPropertyValueObj>()
            };

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
                input.ElementPropertyValues.Add(elemPropertyValue);
            }

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetPropertyValues;

        public override Guid ComponentGuid =>
            new Guid("5d2aa76e-4a59-4b58-a5ce-51878c1478d0");
    }
}