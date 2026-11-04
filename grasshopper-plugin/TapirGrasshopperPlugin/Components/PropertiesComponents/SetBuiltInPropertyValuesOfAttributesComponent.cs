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
    public class SetBuiltInPropertyValuesOfAttributesComponent
        : ArchicadExecutorComponent
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

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> attributeWrappers))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<GH_ObjectWrapper> propertyWrappers))
            {
                return;
            }

            if (!da.TryGetList(
                    2,
                    out List<string> values))
            {
                return;
            }

            if (attributeWrappers.Count == 0 ||
                propertyWrappers.Count == 0 ||
                values.Count == 0)
            {
                return;
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
                return;
            }

            // The values of one attribute go into one item, so they are
            // written together. The results come attribute by attribute; the
            // row of each is kept to hand the messages out in the input order.
            var attributeItems = new Dictionary<string, JArray>();
            var attributeRows = new Dictionary<string, List<int>>();
            var attributeOrder = new List<string>();

            for (var i = 0; i < count; i++)
            {
                var attributeId = GuidObject<AttributeGuidObject>.CreateFromWrapper(
                    attributeWrappers.At(i));
                if (attributeId == null)
                {
                    this.AddError("Invalid attribute identifier in the AttributeGuids input.");
                    return;
                }

                var propertyId = GuidObject<PropertyGuidObject>.CreateFromWrapper(
                    propertyWrappers.At(i));
                if (propertyId == null)
                {
                    this.AddError("Invalid property identifier in the PropertyGuids input.");
                    return;
                }

                var key = attributeId.Guid.ToLowerInvariant();
                if (!attributeItems.ContainsKey(key))
                {
                    attributeItems[key] = new JArray();
                    attributeRows[key] = new List<int>();
                    attributeOrder.Add(key);
                }

                attributeItems[key].Add(
                    new JObject
                    {
                        ["propertyId"] = new JObject { ["guid"] = propertyId.Guid },
                        ["propertyValue"] = new JObject { ["value"] = values.At(i) }
                    });
                attributeRows[key].Add(i);
            }

            var attributes = new JArray();
            var rows = new List<int>();
            foreach (var key in attributeOrder)
            {
                attributes.Add(
                    new JObject
                    {
                        ["attributeId"] = new JObject { ["guid"] = key },
                        ["properties"] = attributeItems[key]
                    });
                rows.AddRange(attributeRows[key]);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["attributes"] = attributes },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var resultMessages = ExecutionResultsResponse.ErrorMessages(response);
            var errorMessages = new List<string>();
            for (var i = 0; i < count; i++)
            {
                errorMessages.Add("");
            }

            for (var k = 0; k < resultMessages.Count && k < rows.Count; k++)
            {
                errorMessages[rows[k]] = resultMessages[k];
            }

            da.SetDataList(
                0,
                errorMessages);
        }

        // Shares the icon of SetPropertyValuesOfAttributes.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetPropertyValuesOfAttributes;

        public override Guid ComponentGuid =>
            new Guid("5487e11a-8819-496d-8c76-50eed4225bf7");
    }
}
