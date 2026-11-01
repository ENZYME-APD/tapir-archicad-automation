using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class SetMarkerParametersComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetMarkerParametersOfElements";

        public SetMarkerParametersComponent()
            : base(
                "SetMarkerParameters",
                "Sets a GDL parameter of the marker of the given Window and Door elements. The opening's own " +
                "settings and the other marker parameters are left as they are. To set several parameters of " +
                "the same opening, list the opening once for each parameter.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the Window and Door elements to set the marker parameter of.");

            InTexts(
                "ParameterNames",
                "Name of the marker parameter to set, see MarkerParameters " +
                "(input only 1 to use the same value for all). Optional when ParameterIndices is given.");

            InGenerics(
                "Values",
                "New value of the marker parameter: a number for the numeric types (Integer, Length, Angle, " +
                "RealNumber, PenColor, LineType, Material, FillPattern, BuildingMaterial, Profile, RGBColor, " +
                "Intensity), a text for String and Title, a boolean for Boolean and On/Off for LightSwitch. " +
                "Array parameters are not supported (input only 1 to use the same value for all).");

            InIntegers(
                "ParameterIndices",
                "Index of the marker parameter to set, as GetMarkerParametersOfElements reports it, instead of " +
                "its name (input only 1 to use the same value for all). Optional; when given, ParameterNames " +
                "is not used.");

            SetOptionality(new[] { 1, 3 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when the marker parameter was set successfully).");
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

            var parameterNames = da.OptionalList<string>(1);
            var parameterIndices = da.OptionalList<int>(3);
            if (parameterNames.Count == 0 &&
                parameterIndices.Count == 0)
            {
                this.AddError("Give the marker parameters in ParameterNames or ParameterIndices.");
                return;
            }

            if (!da.TryGetList(
                    2,
                    out List<GH_ObjectWrapper> values))
            {
                return;
            }

            if (parameterIndices.Count == 0 &&
                parameterNames.Count != 1 &&
                parameterNames.Count != elementWrappers.Count)
            {
                this.AddError(
                    "The size of the input ParameterNames must be 1 or equal to the size of the input ElementGuids.");
                return;
            }

            if (parameterIndices.Count > 1 &&
                parameterIndices.Count != elementWrappers.Count)
            {
                this.AddError(
                    "The size of the input ParameterIndices must be 0, 1 or equal to the size of the input ElementGuids.");
                return;
            }

            if (values.Count != 1 &&
                values.Count != elementWrappers.Count)
            {
                this.AddError(
                    "The size of the input Values must be 1 or equal to the size of the input ElementGuids.");
                return;
            }

            var elementsWithMarkerParameters = new JArray();
            for (var i = 0; i < elementWrappers.Count; i++)
            {
                var elementId = GuidObject<ElementGuid>.CreateFromWrapper(elementWrappers[i]);
                if (elementId == null)
                {
                    this.AddError("Invalid element identifier in the ElementGuids input.");
                    return;
                }

                var markerParameter = new JObject();
                if (parameterIndices.Count > 0)
                {
                    markerParameter["index"] = parameterIndices.At(i);
                }
                else
                {
                    var parameterName = parameterNames.At(i);
                    if (string.IsNullOrEmpty(parameterName))
                    {
                        this.AddError("Empty parameter name in the ParameterNames input.");
                        return;
                    }

                    markerParameter["name"] = parameterName;
                }

                var value = ToJsonValue(values[values.Count == 1 ? 0 : i]);
                if (value == null)
                {
                    this.AddError(
                        "Invalid value in the Values input: it must be a number, a text or a boolean.");
                    return;
                }

                markerParameter["value"] = value;
                elementsWithMarkerParameters.Add(
                    new JObject
                    {
                        ["elementId"] = new JObject { ["guid"] = elementId.Guid },
                        ["markerParameters"] = new JArray { markerParameter }
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["elementsWithMarkerParameters"] = elementsWithMarkerParameters },
                ToAddOn,
                da);
        }

        // The add-on reads the value with the type of the marker parameter. An
        // integer type (Integer, PenColor, ...) needs an integer JSON number, so
        // whole numbers are sent as integers; the real types accept them too.
        private static JToken ToJsonValue(
            GH_ObjectWrapper wrapper)
        {
            var value = wrapper?.Value;
            if (value is IGH_Goo goo && !(value is GH_ObjectWrapper))
            {
                value = goo.ScriptVariable();
            }

            switch (value)
            {
                case bool boolValue:
                    return boolValue;
                case int intValue:
                    return intValue;
                case long longValue:
                    return longValue;
                case double doubleValue:
                    return Math.Abs(doubleValue) < 1e15 &&
                           doubleValue == Math.Floor(doubleValue)
                        ? (JToken)(long)doubleValue
                        : doubleValue;
                case string stringValue:
                    return stringValue;
                default:
                    return null;
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetMarkerParametersOfElements;

        public override Guid ComponentGuid =>
            new Guid("756e4d35-d060-465b-a795-3084efbd6941");
    }
}
