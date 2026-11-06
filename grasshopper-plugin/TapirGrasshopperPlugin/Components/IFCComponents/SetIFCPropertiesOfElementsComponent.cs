using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Globalization;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.IFCComponents
{
    public class SetIFCPropertiesOfElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetIFCPropertiesOfElements";

        public SetIFCPropertiesOfElementsComponent()
            : base(
                "SetIFCPropertiesOfElements",
                "Sets local IFC properties on elements by creating or modifying single value " +
                "(IfcPropertySingleValue) properties in the given property sets. " +
                "Available only in Archicad 25, 26 and 27 for now.",
                GroupNames.IFC)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to set the IFC property of.");

            InTexts(
                "PropertySetNames",
                "Name of the property set, e.g. Pset_DoorCommon; custom property set names are accepted too " +
                "(input only 1 to use the same value for all).");

            InTexts(
                "PropertyNames",
                "Name of the property, e.g. FireRating (input only 1 to use the same value for all).");

            InTexts(
                "ValueTypes",
                "IFC type of the value, e.g. IfcLabel, IfcText, IfcIdentifier, IfcBoolean, IfcLogical, " +
                "IfcInteger, IfcReal or a measure type like IfcThermalTransmittanceMeasure " +
                "(input only 1 to use the same value for all).");

            InGenerics(
                "Values",
                "New value of the property: a boolean for IfcBoolean and IfcLogical (IfcLogical also accepts " +
                "Unknown), a number for the numeric types and a text for every other type. Texts holding a " +
                "number or a boolean are converted to the value type " +
                "(input only 1 to use the same value for all).");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when the IFC property was set successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> elementWrappers))
            {
                return;
            }

            var inputs = new List<string>[3];
            var inputNames = new[] { "PropertySetNames", "PropertyNames", "ValueTypes" };
            for (var inputIndex = 0; inputIndex < 3; inputIndex++)
            {
                if (!da.TryGetList(
                        inputIndex + 1,
                        out List<string> texts))
                {
                    return;
                }

                if (texts.Count != 1 &&
                    texts.Count != elementWrappers.Count)
                {
                    this.AddError(
                        $"The size of the input {inputNames[inputIndex]} must be 1 or equal to the size of the input ElementGuids.");
                    return;
                }

                inputs[inputIndex] = texts;
            }

            if (!da.TryGetList(
                    4,
                    out List<GH_ObjectWrapper> values))
            {
                return;
            }

            if (values.Count != 1 &&
                values.Count != elementWrappers.Count)
            {
                this.AddError(
                    "The size of the input Values must be 1 or equal to the size of the input ElementGuids.");
                return;
            }

            var propertyValues = new JArray();
            for (var i = 0; i < elementWrappers.Count; i++)
            {
                var elementId = GuidObject<ElementGuid>.CreateFromWrapper(elementWrappers[i]);
                if (elementId == null)
                {
                    this.AddError("Invalid element identifier in the ElementGuids input.");
                    return;
                }

                var valueType = Pick(inputs[2], i);
                var value = ToJsonValue(
                    Pick(values, i),
                    valueType);
                if (value == null)
                {
                    this.AddError(
                        "Invalid value in the Values input: it must be a number, a text or a boolean.");
                    return;
                }

                propertyValues.Add(
                    new JObject
                    {
                        ["elementId"] = new JObject { ["guid"] = elementId.Guid },
                        ["propertySetName"] = Pick(inputs[0], i),
                        ["propertyName"] = Pick(inputs[1], i),
                        ["type"] = valueType,
                        ["value"] = value
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["elementIFCPropertyValues"] = propertyValues },
                ToAddOn,
                da);
        }

        private static T Pick<T>(
            List<T> list,
            int index)
        {
            return list[list.Count == 1 ? 0 : index];
        }

        // Converts the value to the JSON type the add-on reads for the IFC value
        // type (the same grouping as the add-on's ParseIFCPropertyValue), so a
        // number typed into a panel works too. A value that cannot be converted
        // is sent as it is, and the add-on reports it on the element.
        private static JToken ToJsonValue(
            GH_ObjectWrapper wrapper,
            string valueType)
        {
            var value = wrapper?.Value;
            if (value is IGH_Goo goo && !(value is GH_ObjectWrapper))
            {
                value = goo.ScriptVariable();
            }

            JToken raw;
            switch (value)
            {
                case bool boolValue:
                    raw = boolValue;
                    break;
                case int intValue:
                    raw = intValue;
                    break;
                case long longValue:
                    raw = longValue;
                    break;
                case double doubleValue:
                    raw = doubleValue;
                    break;
                case string stringValue:
                    raw = stringValue;
                    break;
                default:
                    return null;
            }

            valueType = valueType ?? "";

            if (valueType == "IfcBoolean" || valueType == "IfcLogical")
            {
                if (value is string text)
                {
                    if (bool.TryParse(text.Trim(), out var parsed))
                    {
                        return parsed;
                    }
                }
                return raw;
            }

            if (valueType == "IfcInteger" ||
                valueType == "IfcCountMeasure" ||
                valueType == "IfcTimeStamp")
            {
                if (TryGetNumber(value, out var number) &&
                    number == Math.Floor(number) &&
                    Math.Abs(number) < 9e18)
                {
                    return (long)number;
                }
                return raw;
            }

            if (valueType == "IfcReal" ||
                (valueType.EndsWith("Measure") && valueType != "IfcDescriptiveMeasure"))
            {
                if (TryGetNumber(value, out var number))
                {
                    return number;
                }
                return raw;
            }

            if (value is string)
            {
                return raw;
            }

            return System.Convert.ToString(
                value,
                CultureInfo.InvariantCulture);
        }

        private static bool TryGetNumber(
            object value,
            out double number)
        {
            switch (value)
            {
                case int intValue:
                    number = intValue;
                    return true;
                case long longValue:
                    number = longValue;
                    return true;
                case double doubleValue:
                    number = doubleValue;
                    return true;
                case string text:
                    return double.TryParse(
                        text.Trim(),
                        NumberStyles.Float,
                        CultureInfo.InvariantCulture,
                        out number);
                default:
                    number = 0.0;
                    return false;
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetIFCPropertiesOfElements;

        public override Guid ComponentGuid =>
            new Guid("3e739dde-5a99-4566-aaf0-41ef44728bee");
    }
}
