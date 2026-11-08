using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Properties;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    // Sends one SetPropertyValuesOfAttributes request for every branch of the
    // inputs, so a tree input makes one undo step in Archicad (#203).
    public class SetPropertyValuesOfAttributesComponent
        : BatchedExecutorComponent<AttributePropertyValueObj>
    {
        public override string CommandName => "SetPropertyValuesOfAttributes";

        public SetPropertyValuesOfAttributesComponent()
            : base(
                "SetPropertyValuesOfAttributes",
                "Set the property values of the given attributes.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "AttributeGuids",
                "Identifiers of the attributes to modify.");

            InGenerics(
                "PropertyGuids",
                "Identifiers of the properties to set (input only 1 to use the same property for all attributes).");

            InTexts(
                "Values",
                "The new property values (input only 1 to use the same value for all attributes).");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each property value (empty when it succeeded).");
        }

        protected override bool TryGetBatchItems(
            IGH_DataAccess da,
            out List<AttributePropertyValueObj> items)
        {
            items = null;

            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> attributeWrappers))
            {
                return false;
            }

            if (!da.TryGetList(
                    1,
                    out List<GH_ObjectWrapper> propertyWrappers))
            {
                return false;
            }

            if (!da.TryGetList(
                    2,
                    out List<string> values))
            {
                return false;
            }

            if (propertyWrappers.Count != 1 &&
                propertyWrappers.Count != attributeWrappers.Count)
            {
                this.AddError(
                    "The size of the input PropertyGuids must be 1 or equal to the size of the input AttributeGuids.");
                return false;
            }

            if (values.Count != 1 &&
                values.Count != attributeWrappers.Count)
            {
                this.AddError(
                    "The size of the input Values must be 1 or equal to the size of the input AttributeGuids.");
                return false;
            }

            var branchItems = new List<AttributePropertyValueObj>();

            for (var i = 0; i < attributeWrappers.Count; i++)
            {
                var attributeId =
                    GuidObject<AttributeGuidObject>.CreateFromWrapper(attributeWrappers[i]);
                if (attributeId == null)
                {
                    this.AddError("Invalid attribute identifier in the AttributeGuids input.");
                    return false;
                }

                var propertyWrapper =
                    propertyWrappers[propertyWrappers.Count == 1 ? 0 : i];
                var propertyId =
                    GuidObject<PropertyGuidObject>.CreateFromWrapper(propertyWrapper);
                if (propertyId == null)
                {
                    this.AddError("Invalid property identifier in the PropertyGuids input.");
                    return false;
                }

                branchItems.Add(
                    new AttributePropertyValueObj
                    {
                        AttributeId = attributeId,
                        PropertyId = propertyId,
                        PropertyValue = new PropertyValueObj
                        {
                            Value = values[values.Count == 1 ? 0 : i]
                        }
                    });
            }

            items = branchItems;
            return true;
        }

        // The command refuses the same property of the same attribute given
        // twice; the branches used to be sent one after the other, so the
        // last value won.
        protected override string GetBatchItemKey(
            AttributePropertyValueObj item)
        {
            return $"{item.AttributeId?.Guid?.ToLowerInvariant()}|" +
                   $"{item.PropertyId?.Guid?.ToLowerInvariant()}";
        }

        protected override object CreateBatchParameters(
            List<AttributePropertyValueObj> items)
        {
            return new SetPropertyValuesOfAttributesParameters
            {
                AttributePropertyValues = items
            };
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetPropertyValuesOfAttributes;

        public override Guid ComponentGuid =>
            new Guid("cd030f8b-c843-4c22-8428-9cfdc7692612");
    }
}
