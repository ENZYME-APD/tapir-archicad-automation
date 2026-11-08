using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;
using TapirGrasshopperPlugin.Types.Generic;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Properties;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    // Sends one SetBuiltInPropertyValuesOfAttributes request for every branch
    // of the inputs, so a tree input makes one undo step in Archicad (#203).
    public class SetBuiltInPropertyValuesOfAttributesComponent
        : BatchedExecutorComponent<AttributePropertyValueObj>
    {
        public override string CommandName =>
            "SetBuiltInPropertyValuesOfAttributes";

        public SetBuiltInPropertyValuesOfAttributesComponent()
            : base(
                "SetBuiltInPropertyValuesOfAttributes",
                "Set built-in property values of attributes; the values of each attribute are written together. " +
                "Custom properties are refused, set them with SetPropertyValuesOfAttributes.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "AttributeGuids",
                "Identifier of the attribute of each value (input only 1 to set every value on the same attribute).");

            InGenerics(
                "PropertyGuids",
                "Identifier of the built-in property of each value (input only 1 to use the same property for " +
                "every value).");

            InTexts(
                "Values",
                "The new property values, as display strings (input only 1 to use the same value for every " +
                "property).");
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

            if (attributeWrappers.Count == 0 ||
                propertyWrappers.Count == 0 ||
                values.Count == 0)
            {
                return false;
            }

            // The longest list gives the number of values.
            var count = attributeWrappers.Count;
            var countInputName = "AttributeGuids";
            if (propertyWrappers.Count > count)
            {
                count = propertyWrappers.Count;
                countInputName = "PropertyGuids";
            }
            if (values.Count > count)
            {
                count = values.Count;
                countInputName = "Values";
            }

            if (!this.CheckSizes(
                    count,
                    countInputName,
                    ("AttributeGuids", attributeWrappers.Count),
                    ("PropertyGuids", propertyWrappers.Count),
                    ("Values", values.Count)))
            {
                return false;
            }

            var branchItems = new List<AttributePropertyValueObj>();

            for (var i = 0; i < count; i++)
            {
                var attributeId = GuidObject<AttributeGuidObject>.CreateFromWrapper(
                    attributeWrappers.At(i));
                if (attributeId == null)
                {
                    this.AddError("Invalid attribute identifier in the AttributeGuids input.");
                    return false;
                }

                var propertyId = GuidObject<PropertyGuidObject>.CreateFromWrapper(
                    propertyWrappers.At(i));
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
                            Value = values.At(i)
                        }
                    });
            }

            items = branchItems;
            return true;
        }

        // The command refuses a property given twice for one attribute; the
        // branches used to be sent one after the other, so the last value won.
        protected override string GetBatchItemKey(
            AttributePropertyValueObj item)
        {
            return $"{item.AttributeId?.Guid?.ToLowerInvariant()}|" +
                   $"{item.PropertyId?.Guid?.ToLowerInvariant()}";
        }

        // The values that follow each other on the same attribute go into one
        // entry, so they are written together. The command answers entry by
        // entry, so the results stay in the order of the items; an attribute
        // given again later gets an entry of its own, written in the same
        // undo step.
        protected override object CreateBatchParameters(
            List<AttributePropertyValueObj> items)
        {
            var attributes = new JArray();
            string lastKey = null;
            JObject entry = null;

            foreach (var item in items)
            {
                var key = item.AttributeId.Guid.ToLowerInvariant();
                if (entry == null || key != lastKey)
                {
                    entry = new JObject
                    {
                        ["attributeId"] = new JObject { ["guid"] = item.AttributeId.Guid },
                        ["properties"] = new JArray()
                    };
                    attributes.Add(entry);
                    entry = (JObject)attributes.Last;
                    lastKey = key;
                }

                ((JArray)entry["properties"]).Add(
                    new JObject
                    {
                        ["propertyId"] = new JObject { ["guid"] = item.PropertyId.Guid },
                        ["propertyValue"] = new JObject { ["value"] = item.PropertyValue.Value }
                    });
            }

            return new JObject { ["attributes"] = attributes };
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetBuiltInPropertyValuesOfAttributes;

        public override Guid ComponentGuid =>
            new Guid("5487e11a-8819-496d-8c76-50eed4225bf7");
    }
}
