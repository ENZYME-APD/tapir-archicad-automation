using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Properties;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    // Sends one SetPropertyValuesOfElements request for every branch of the
    // inputs, so a tree input makes one undo step in Archicad (#203).
    public class SetPropertyValuesOfElementsComponent
        : BatchedExecutorComponent<ElementPropertyValueObj>
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
                "Single value or list of values to set for the corresponding elements. " +
                "Not needed for the elements whose ValueStatus is Default or UserUndefined.");

            InTexts(
                "ValueStatuses",
                "Optional single status or list of statuses for the corresponding elements: " +
                "Value (the default) sets the value from Values, Default resets the element's value " +
                "to the default value of the property, UserUndefined makes the value undefined.");

            SetOptionality(new[] { 2, 3 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each property value (empty when it succeeded).");
        }

        protected override bool TryGetBatchItems(
            IGH_DataAccess da,
            out List<ElementPropertyValueObj> items)
        {
            items = null;

            if (!da.TryCreate(
                    0,
                    out PropertyGuidObject propertyId))
            {
                return false;
            }

            if (!da.TryCreateFromList(
                    1,
                    out ElementsObject elements))
            {
                return false;
            }

            da.TryGetList(
                2,
                out List<string> values);

            da.TryGetList(
                3,
                out List<string> statuses);

            var elementCount = elements.Elements.Count;

            if (statuses.Count > 1 && statuses.Count != elementCount)
            {
                this.AddError(
                    "The count of ValueStatuses must be 0, 1 or the same as the count of ElementGuids.");
                return false;
            }

            var statusOfElements = new List<string>();
            for (var i = 0; i < elementCount; i++)
            {
                var status = statuses.Count == 0
                    ? "Value"
                    : statuses.Count == 1
                        ? statuses[0]
                        : statuses[i];
                if (string.IsNullOrEmpty(status))
                {
                    status = "Value";
                }

                if (status != "Value" && status != "Default" && status != "UserUndefined")
                {
                    this.AddError(
                        "ValueStatuses must be Value, Default or UserUndefined.");
                    return false;
                }

                statusOfElements.Add(status);
            }

            if (statusOfElements.Contains("Value") &&
                values.Count != 1 && values.Count != elementCount)
            {
                this.AddError(
                    "The count of Values must be 1 or the same as the count of ElementGuids.");
                return false;
            }

            items = new List<ElementPropertyValueObj>();

            for (var i = 0; i < elementCount; i++)
            {
                var elementId = elements.Elements[i];
                var elemPropertyValue = new ElementPropertyValueObj()
                {
                    ElementId = elementId.ElementId,
                    PropertyId = propertyId
                };

                if (statusOfElements[i] == "Value")
                {
                    elemPropertyValue.PropertyValue = new PropertyValueObj()
                    {
                        Value = values.Count == 1
                            ? values[0]
                            : values[i]
                    };
                }
                else
                {
                    elemPropertyValue.ValueStatus = statusOfElements[i];
                }

                items.Add(elemPropertyValue);
            }

            return true;
        }

        // The command keeps the first value of an element and property given
        // twice; the branches used to be sent one after the other, so the
        // last one won.
        protected override string GetBatchItemKey(
            ElementPropertyValueObj item)
        {
            return $"{item.ElementId?.Guid?.ToLowerInvariant()}|" +
                   $"{item.PropertyId?.Guid?.ToLowerInvariant()}";
        }

        protected override object CreateBatchParameters(
            List<ElementPropertyValueObj> items)
        {
            return new ElementPropertyValuesObj
            {
                ElementPropertyValues = items
            };
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetPropertyValues;

        public override Guid ComponentGuid =>
            new Guid("5d2aa76e-4a59-4b58-a5ce-51878c1478d0");
    }
}