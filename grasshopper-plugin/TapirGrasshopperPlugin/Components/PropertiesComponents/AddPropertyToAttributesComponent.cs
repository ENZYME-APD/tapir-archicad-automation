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
    public class AddPropertyToAttributesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "AddPropertyToAttributes";

        public AddPropertyToAttributesComponent()
            : base(
                "AddPropertyToAttributes",
                "Make an existing custom property available for the given attributes (for example " +
                "Building Materials), in one undo step. Available in Archicad 25, 26 and 27 only.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "PropertyGuid",
                "An existing custom property. Create it first with CreatePropertyDefinitions if needed.");

            InGenerics(
                "AttributeGuids",
                "The attributes (for example Building Materials) to make the property available for.");
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
            if (!da.TryGet(
                    0,
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

            if (!da.TryGetList(
                    1,
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

            var parameters = new JObject
            {
                ["propertyId"] = new JObject { ["guid"] = propertyId.Guid },
                ["attributeIds"] = attributeIds
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

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AddPropertyToAttributes;

        public override Guid ComponentGuid =>
            new Guid("2ad0915f-c784-41bb-8c48-4e4180336322");
    }
}
