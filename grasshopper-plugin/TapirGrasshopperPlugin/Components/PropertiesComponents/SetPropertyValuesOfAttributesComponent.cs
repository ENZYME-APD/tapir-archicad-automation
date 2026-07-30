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
    public class SetPropertyValuesOfAttributesComponent : ArchicadExecutorComponent
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

            if (propertyWrappers.Count != 1 &&
                propertyWrappers.Count != attributeWrappers.Count)
            {
                this.AddError(
                    "The size of the input PropertyGuids must be 1 or equal to the size of the input AttributeGuids.");
                return;
            }

            if (values.Count != 1 &&
                values.Count != attributeWrappers.Count)
            {
                this.AddError(
                    "The size of the input Values must be 1 or equal to the size of the input AttributeGuids.");
                return;
            }

            var input = new SetPropertyValuesOfAttributesParameters
            {
                AttributePropertyValues = new List<AttributePropertyValueObj>()
            };

            for (var i = 0; i < attributeWrappers.Count; i++)
            {
                var attributeId =
                    GuidObject<AttributeGuidObject>.CreateFromWrapper(attributeWrappers[i]);
                if (attributeId == null)
                {
                    this.AddError("Invalid attribute identifier in the AttributeGuids input.");
                    return;
                }

                var propertyWrapper =
                    propertyWrappers[propertyWrappers.Count == 1 ? 0 : i];
                var propertyId =
                    GuidObject<PropertyGuidObject>.CreateFromWrapper(propertyWrapper);
                if (propertyId == null)
                {
                    this.AddError("Invalid property identifier in the PropertyGuids input.");
                    return;
                }

                input.AttributePropertyValues.Add(
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

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetPropertyValuesOfAttributes;

        public override Guid ComponentGuid =>
            new Guid("cd030f8b-c843-4c22-8428-9cfdc7692612");
    }
}
