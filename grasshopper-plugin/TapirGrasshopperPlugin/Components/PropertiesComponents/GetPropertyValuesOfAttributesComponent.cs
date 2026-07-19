using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Properties;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class GetPropertyValuesOfAttributesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetPropertyValuesOfAttributes";

        public GetPropertyValuesOfAttributesComponent()
            : base(
                "GetPropertyValuesOfAttributes",
                "Get the property values of the given attributes for the given properties.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "AttributeGuids",
                "Identifiers of the attributes to query.");

            InGenerics(
                "PropertyGuids",
                "Identifiers of the properties to query.");
        }

        protected override void AddOutputs()
        {
            OutText(
                "PropertyValues",
                "JSON object with the property values of the given attributes.");
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

            var input = new GetPropertyValuesOfAttributesParameters
            {
                AttributeIds = new List<AttributeGuidWrapper>(),
                Properties = new List<PropertyGuidWrapper>()
            };

            foreach (var wrapper in attributeWrappers)
            {
                var id = GuidObject<AttributeGuidObject>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError("Invalid attribute identifier in the AttributeGuids input.");
                    return;
                }
                input.AttributeIds.Add(
                    new AttributeGuidWrapper { AttributeId = id });
            }

            foreach (var wrapper in propertyWrappers)
            {
                var id = GuidObject<PropertyGuidObject>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError("Invalid property identifier in the PropertyGuids input.");
                    return;
                }
                input.Properties.Add(
                    new PropertyGuidWrapper { PropertyId = id });
            }

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(input),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(
                0,
                response.ToString(Formatting.Indented));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetPropertyValuesOfAttributes;

        public override Guid ComponentGuid =>
            new Guid("7cdf4c9b-8e01-498f-8c10-d9066dc05f53");
    }
}
