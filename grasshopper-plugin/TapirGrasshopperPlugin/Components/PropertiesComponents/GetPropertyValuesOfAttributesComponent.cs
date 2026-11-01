using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
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
                "Get the property values of the given attributes for the given properties. " +
                "Value outputs have one branch per queried attribute, with one item per queried property. " +
                "With IncludeStatus it also returns whether each property is available, editable and visible " +
                "for the attribute (Tapir 1.7.3 or newer).",
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

            InBoolean(
                "IncludeStatus",
                "Also return whether each property is available, editable and visible for the attribute; " +
                "a property without an available value then gives a null value with its status instead of " +
                "an error. Default is false.");

            SetOptionality(2);
        }

        protected override void AddOutputs()
        {
            OutTextTree(
                "Values",
                "Property value of each attribute and property (one branch per attribute).");

            OutTextTree(
                "ValueErrorMessages",
                "Error message for each attribute and property (one branch per attribute; empty when successful).");

            OutTexts(
                "ErrorMessages",
                "Error message for each queried attribute (empty when successful).");

            OutBooleanTree(
                "IsAvailable",
                "Whether the property is available for the attribute, in the structure of the Values output. " +
                "Only filled when IncludeStatus is true.");

            OutBooleanTree(
                "IsEditable",
                "Whether the value of the property is editable for the attribute, in the structure of the " +
                "Values output. Only filled when IncludeStatus is true.");

            OutBooleanTree(
                "IsVisible",
                "Whether the property is visible for the attribute, in the structure of the Values output. " +
                "Only filled when IncludeStatus is true.");
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

            da.TryGet(
                2,
                out bool includeStatus);

            var parameters = JObject.FromObject(input);
            if (includeStatus)
            {
                parameters["includeStatus"] = true;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var values = new DataTree<object>();
            var valueErrors = new DataTree<object>();
            var errors = new List<string>();
            var isAvailable = new DataTree<object>();
            var isEditable = new DataTree<object>();
            var isVisible = new DataTree<object>();

            var items = JsonOutputHelp.Items(
                response,
                "propertyValuesForAttributes");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                values.EnsurePath(path);
                valueErrors.EnsurePath(path);
                if (includeStatus)
                {
                    isAvailable.EnsurePath(path);
                    isEditable.EnsurePath(path);
                    isVisible.EnsurePath(path);
                }

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");

                if (item["propertyValues"] is JArray propertyValues)
                {
                    foreach (var valueOrError in propertyValues)
                    {
                        if (includeStatus)
                        {
                            isAvailable.Add(
                                JsonOutputHelp.Scalar(valueOrError, "isAvailable"),
                                path);
                            isEditable.Add(
                                JsonOutputHelp.Scalar(valueOrError, "isEditable"),
                                path);
                            isVisible.Add(
                                JsonOutputHelp.Scalar(valueOrError, "isVisible"),
                                path);
                        }

                        if (JsonOutputHelp.IsError(valueOrError))
                        {
                            values.Add(null, path);
                            valueErrors.Add(
                                JsonOutputHelp.ErrorMessage(valueOrError),
                                path);
                            continue;
                        }

                        values.Add(
                            JsonOutputHelp.Scalar(
                                valueOrError["propertyValue"],
                                "value"),
                            path);
                        valueErrors.Add("", path);
                    }
                }
            }

            da.SetDataTree(0, values);
            da.SetDataTree(1, valueErrors);
            da.SetDataList(2, errors);
            da.SetDataTree(3, isAvailable);
            da.SetDataTree(4, isEditable);
            da.SetDataTree(5, isVisible);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetPropertyValuesOfAttributes;

        public override Guid ComponentGuid =>
            new Guid("7cdf4c9b-8e01-498f-8c10-d9066dc05f53");
    }
}
