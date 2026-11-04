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
    public class SetPropertyValueOfAttributesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetPropertyValueOfAttributes";

        public SetPropertyValueOfAttributesComponent()
            : base(
                "SetPropertyValueOfAttributes",
                "Set one property value on several attributes (for example Building Materials) in one undo step. " +
                "If the property is not available for any of the given attributes, no attribute is changed.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "AttributeGuids",
                "The attributes to set the value on. The property must be available for every one of them.");

            InGeneric(
                "PropertyGuid",
                "The property to set.");

            InText(
                "Value",
                "The new value of the property, as its display string.");
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult.Message),
                ExecutionResult.Doc);
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

            var attributeIds = new JArray();
            foreach (var wrapper in attributeWrappers)
            {
                var id = GuidObject<AttributeGuidObject>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError("Invalid attribute identifier in the AttributeGuids input.");
                    return;
                }
                attributeIds.Add(
                    new JObject
                    {
                        ["attributeId"] = new JObject { ["guid"] = id.Guid }
                    });
            }

            if (!da.TryGet(
                    1,
                    out GH_ObjectWrapper propertyWrapper))
            {
                return;
            }

            var propertyId = GuidObject<PropertyGuidObject>.CreateFromWrapper(propertyWrapper);
            if (propertyId == null)
            {
                this.AddError("Invalid property identifier in the PropertyGuid input.");
                return;
            }

            if (!da.TryGet(
                    2,
                    out string value))
            {
                return;
            }

            var parameters = new JObject
            {
                ["attributeIds"] = attributeIds,
                ["propertyId"] = new JObject { ["guid"] = propertyId.Guid },
                ["propertyValue"] = new JObject { ["value"] = value }
            };

            // The command answers with one ExecutionResult: it succeeds or fails
            // as a whole, so there is nothing per item to hand out.
            if (!TryGetConvertedCadValues(
                    CommandName,
                    parameters,
                    ToAddOn,
                    ExecutionResult.Deserialize,
                    out ExecutionResult response))
            {
                return;
            }

            if (!response.Success)
            {
                this.AddError(response.Message());
            }

            da.SetData(0, response.Message());
        }

        // Shares the icon of SetPropertyValuesOfAttributes.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetPropertyValuesOfAttributes;

        public override Guid ComponentGuid =>
            new Guid("75d28987-7453-410c-91a7-33dbef2cac21");
    }
}
