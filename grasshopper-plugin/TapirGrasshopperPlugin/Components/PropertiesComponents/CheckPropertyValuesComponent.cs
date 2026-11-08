using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Properties;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class CheckPropertyValuesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "CheckPropertyValues";

        public CheckPropertyValuesComponent()
            : base(
                "CheckPropertyValues",
                "Check whether the given values are acceptable for the given property definitions, without " +
                "changing anything: whether each value can be converted to the type of its property and " +
                "Archicad accepts it for the property definition.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "PropertyGuids",
                "The property definition each value is checked against.");

            InTexts(
                "Values",
                "The values to check, as text (as SetPropertyValues takes them). Input only 1 property or " +
                "only 1 value to check it with every item of the other input.");
        }

        protected override void AddOutputs()
        {
            OutBooleans(
                "IsValid",
                "Whether each value is acceptable for its property (null when the property was not found).");

            OutErrorMessages(
                "Error message for each value (empty when it could be checked; why it could not be converted " +
                "or checked otherwise).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> propertyWrappers) ||
                !da.TryGetList(
                    1,
                    out List<string> values))
            {
                return;
            }

            var propertyIds = new List<PropertyGuidObject>();
            foreach (var wrapper in propertyWrappers)
            {
                var id = GuidObject<PropertyGuidObject>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError("Invalid property identifier in the PropertyGuids input.");
                    return;
                }
                propertyIds.Add(id);
            }

            if (propertyIds.Count == 0 ||
                values.Count == 0)
            {
                return;
            }

            var count = Math.Max(
                propertyIds.Count,
                values.Count);
            if (!ElementInputHelp.CheckOneOrMany(
                    this,
                    propertyIds,
                    count,
                    "PropertyGuids") ||
                !ElementInputHelp.CheckOneOrMany(
                    this,
                    values,
                    count,
                    "Values"))
            {
                return;
            }

            var propertyValues = new JArray();
            for (var i = 0; i < count; i++)
            {
                propertyValues.Add(
                    new JObject
                    {
                        ["propertyId"] = new JObject
                        {
                            ["guid"] = ElementInputHelp.ItemAt(propertyIds, i).Guid
                        },
                        ["propertyValue"] = new JObject
                        {
                            ["value"] = ElementInputHelp.ItemAt(values, i) ?? ""
                        }
                    });
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["propertyValues"] = propertyValues },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var isValid = new List<object>();
            var errors = new List<string>();
            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "results"))
            {
                isValid.Add(
                    JsonOutputHelp.Scalar(
                        item,
                        "isValid"));
                errors.Add(JsonOutputHelp.ErrorMessage(item));
            }

            da.SetDataList(0, isValid);
            da.SetDataList(1, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CheckPropertyValues;

        public override Guid ComponentGuid =>
            new Guid("e8ddc60a-079b-4781-9512-f165b0242656");
    }
}
