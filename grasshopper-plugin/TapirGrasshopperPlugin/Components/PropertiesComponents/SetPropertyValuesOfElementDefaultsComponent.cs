using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Components.ElementsComponents;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Properties;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    // Sends one SetPropertyValuesOfElementDefaults request for every branch
    // of the inputs, so a tree input makes one undo step in Archicad (#203).
    public class SetPropertyValuesOfElementDefaultsComponent
        : BatchedExecutorComponent<ElementDefaultPropertyValueObj>
    {
        public override string CommandName =>
            "SetPropertyValuesOfElementDefaults";

        public SetPropertyValuesOfElementDefaultsComponent()
            : base(
                "SetPropertyValuesOfDefaults",
                "Set property values of the tool defaults (the settings new elements get) of element types, " +
                "so every element placed afterwards gets them.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "PropertyGuid",
                "The property Guid to set the value for.");

            InTexts(
                "ElementTypes",
                "Element types whose defaults to set the value for.");

            InTexts(
                "Values",
                "Single value or list of values to set for the corresponding element types.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element type (empty when it succeeded).");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            AddAsSource<ElementTypeValueList>(
                document,
                1);
        }

        protected override bool TryGetBatchItems(
            IGH_DataAccess da,
            out List<ElementDefaultPropertyValueObj> items)
        {
            items = null;

            if (!da.TryCreate(
                    0,
                    out PropertyGuidObject propertyId))
            {
                return false;
            }

            if (!da.TryGetList(
                    1,
                    out List<string> elementTypes))
            {
                return false;
            }

            if (!da.TryGetList(
                    2,
                    out List<string> values))
            {
                return false;
            }

            if (values.Count != 1 && elementTypes.Count != values.Count)
            {
                this.AddError(
                    "The count of Values must be 1 or the same as the count of ElementTypes.");
                return false;
            }

            items = new List<ElementDefaultPropertyValueObj>();

            for (var i = 0; i < elementTypes.Count; i++)
            {
                items.Add(
                    new ElementDefaultPropertyValueObj
                    {
                        ElementType = elementTypes[i],
                        PropertyId = propertyId,
                        PropertyValue = new PropertyValueObj
                        {
                            Value = values.Count == 1
                                ? values[0]
                                : values[i]
                        }
                    });
            }

            return true;
        }

        // The branches used to be sent one after the other, so the last value
        // of an element type and property won.
        protected override string GetBatchItemKey(
            ElementDefaultPropertyValueObj item)
        {
            return $"{item.ElementType}|" +
                   $"{item.PropertyId?.Guid?.ToLowerInvariant()}";
        }

        protected override object CreateBatchParameters(
            List<ElementDefaultPropertyValueObj> items)
        {
            return new SetPropertyValuesOfElementDefaultsParameters
            {
                ElementDefaultPropertyValues = items
            };
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetPropertyValues;

        public override Guid ComponentGuid =>
            new Guid("e4b8215f-93c7-4a0d-8f61-2d7b9e5c13a8");
    }
}
