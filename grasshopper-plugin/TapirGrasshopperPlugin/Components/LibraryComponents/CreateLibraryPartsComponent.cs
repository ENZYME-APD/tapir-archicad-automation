using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    // Creates one library part per solve; Grasshopper runs the component once
    // per item of the item inputs, so a list of names creates a library part
    // for each, while the parameter lists apply to every one of them.
    public class CreateLibraryPartsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateLibraryParts";

        private const int NameInput = 0;
        private const int ParentIdInput = 1;
        private const int ParentNameInput = 2;
        private const int TypeInput = 3;
        private const int PathInput = 4;
        private const int AInput = 5;
        private const int BInput = 6;
        private const int MasterScriptInput = 7;
        private const int Script2DInput = 8;
        private const int Script3DInput = 9;
        private const int ParameterScriptInput = 10;
        private const int UIScriptInput = 11;
        private const int PropertiesScriptInput = 12;
        private const int ParameterNamesInput = 13;
        private const int ParameterTypesInput = 14;
        private const int ParameterValuesInput = 15;
        private const int ParameterDescriptionsInput = 16;
        private const int ParameterHiddenInput = 17;
        private const int OverwriteExistingInput = 18;

        private const int NameOutput = 0;
        private const int UnIdOutput = 1;
        private const int GuidOutput = 2;
        private const int IndexOutput = 3;
        private const int PathOutput = 4;
        private const int ErrorMessageOutput = 5;

        public CreateLibraryPartsComponent()
            : base(
                "CreateLibraryParts",
                "Create a GDL library part in the embedded library from its scripts and parameter " +
                "definitions, so it can be placed right away (for example with CreateObjects by its name). " +
                "Either ParentId or ParentName is required. The parameters besides A and B are given as " +
                "parallel lists. A list of names creates one library part per name.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Name",
                "Name of the new library part, as shown in the library and used by CreateObjects as LibraryPartName.");

            InText(
                "ParentId",
                "Parent subtype of the new library part: its unique ID ('{main}-{revision}') or its main guid, " +
                "as GetAvailableLibraryParts reports it. Either ParentId or ParentName is required.");

            InText(
                "ParentName",
                "Document name of the parent subtype, e.g. 'General Object' or 'General Label' from the " +
                "Archicad Library; used when ParentId is not given.");

            InText(
                "Type",
                "Optional. Type of the new library part (e.g. Object, Lamp, Label). By default the type of " +
                "the parent, and Object otherwise.");

            InText(
                "Path",
                "Optional. Relative path of the .gsm file inside the embedded library, the same kind of path " +
                "AddFilesToEmbeddedLibrary takes as OutputPaths. By default '<Name>.gsm'.");

            InNumber(
                "A",
                "Default value of the A (X size) parameter.",
                1.0);

            InNumber(
                "B",
                "Default value of the B (Y size) parameter.",
                1.0);

            InText(
                "MasterScript",
                "Optional. GDL master script.");

            InText(
                "Script2D",
                "Optional. GDL 2D script.");

            InText(
                "Script3D",
                "Optional. GDL 3D script.");

            InText(
                "ParameterScript",
                "Optional. GDL parameter script (value lists, locks, hiding).");

            InText(
                "UIScript",
                "Optional. GDL user interface script.");

            InText(
                "PropertiesScript",
                "Optional. GDL properties (component listing) script.");

            InTexts(
                "ParameterNames",
                "Optional. GDL variable name of each parameter besides A and B (at most 31 characters), " +
                "in the order they appear in the settings dialog. Only simple (non-array) parameters can be defined.");

            InTexts(
                "ParameterTypes",
                "Type of each parameter, named as GetGDLParametersOfElements reports it: Integer, Length, Angle, " +
                "RealNumber, LightSwitch, RGBColor, Intensity, LineType, Material, FillPattern, PenColor, String, " +
                "Boolean, Separator, Title, BuildingMaterial or Profile. One per ParameterNames item.");

            InGenerics(
                "ParameterValues",
                "Optional. Default value of each parameter: a number for the numeric and attribute index types, " +
                "a text for String and Title, a boolean for Boolean, 'On' or 'Off' for LightSwitch. " +
                "Empty, or one per ParameterNames item (null leaves the default of the type).");

            InTexts(
                "ParameterDescriptions",
                "Optional. Display name of each parameter in the settings dialog. Empty, or one per ParameterNames item.");

            InBooleans(
                "ParameterHidden",
                "Optional. True hides the parameter from the settings dialog. Empty, or one per ParameterNames item.");

            InBoolean(
                "OverwriteExisting",
                "Delete an embedded library item already existing on the path of the new library part first, " +
                "so the new one replaces the loaded library part (Archicad 27 or newer). " +
                "When false, creating on an existing path fails.",
                false);

            SetOptionality(
                new[]
                {
                    ParentIdInput,
                    ParentNameInput,
                    TypeInput,
                    PathInput,
                    MasterScriptInput,
                    Script2DInput,
                    Script3DInput,
                    ParameterScriptInput,
                    UIScriptInput,
                    PropertiesScriptInput,
                    ParameterNamesInput,
                    ParameterTypesInput,
                    ParameterValuesInput,
                    ParameterDescriptionsInput,
                    ParameterHiddenInput
                });
        }

        protected override void AddOutputs()
        {
            OutText(
                "Name",
                "Name of the created library part.");

            OutText(
                "UnId",
                "Unique ID of the created library part ('{main}-{revision}'), usable as ParentId of further library parts.");

            OutText(
                "Guid",
                "Main guid of the created library part, as GetAvailableLibraryParts reports it.");

            OutInteger(
                "Index",
                "Index of the created library part in the loaded libraries.");

            OutText(
                "Path",
                "Relative path of the .gsm file inside the embedded library; DeleteEmbeddedLibraryItems takes it as Paths.");

            OutText(
                "ErrorMessage",
                "Error message when the library part could not be created (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    NameInput,
                    out string name))
            {
                return;
            }

            var libraryPart = new JObject
            {
                ["name"] = name
            };

            SetTextWhenGiven(da, ParentIdInput, libraryPart, "parentId");
            SetTextWhenGiven(da, ParentNameInput, libraryPart, "parentName");
            if (libraryPart["parentId"] == null && libraryPart["parentName"] == null)
            {
                this.AddError(
                    "Either ParentId or ParentName is required.");
                return;
            }

            SetTextWhenGiven(da, TypeInput, libraryPart, "type");
            SetTextWhenGiven(da, PathInput, libraryPart, "path");

            libraryPart["a"] = da.GetOptional(AInput, 1.0);
            libraryPart["b"] = da.GetOptional(BInput, 1.0);

            SetTextWhenGiven(da, MasterScriptInput, libraryPart, "masterScript");
            SetTextWhenGiven(da, Script2DInput, libraryPart, "script2D");
            SetTextWhenGiven(da, Script3DInput, libraryPart, "script3D");
            SetTextWhenGiven(da, ParameterScriptInput, libraryPart, "parameterScript");
            SetTextWhenGiven(da, UIScriptInput, libraryPart, "uiScript");
            SetTextWhenGiven(da, PropertiesScriptInput, libraryPart, "propertiesScript");

            if (!TryGetParameters(
                    da,
                    out JArray parameters))
            {
                return;
            }

            if (parameters.Count > 0)
            {
                libraryPart["parameters"] = parameters;
            }

            var input = new JObject
            {
                ["libraryParts"] = new JArray(libraryPart)
            };

            // Only sent when set, like AddFilesToEmbeddedLibrary does.
            if (da.GetOptional(OverwriteExistingInput, false))
            {
                input["overwriteExisting"] = true;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    input,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var items = JsonOutputHelp.Items(response, "libraryParts");
            if (items.Count == 0)
            {
                this.AddError(
                    $"The {CommandName} command answered without a result.");
                return;
            }

            var item = items[0];
            if (JsonOutputHelp.IsError(item))
            {
                var errorMessage = JsonOutputHelp.ErrorMessage(item);
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Warning,
                    errorMessage);
                da.SetData(ErrorMessageOutput, errorMessage);
                return;
            }

            da.SetData(NameOutput, JsonOutputHelp.Scalar(item, "name"));
            da.SetData(UnIdOutput, JsonOutputHelp.Scalar(item, "unId"));
            da.SetData(GuidOutput, JsonOutputHelp.Scalar(item, "guid"));
            da.SetData(IndexOutput, JsonOutputHelp.Scalar(item, "index"));
            da.SetData(PathOutput, JsonOutputHelp.Scalar(item, "path"));
            da.SetData(ErrorMessageOutput, "");
        }

        // Sets the field from an optional text input; an empty text, like a
        // missing one, leaves the field out, so the Add-On uses its default.
        private static void SetTextWhenGiven(
            IGH_DataAccess da,
            int inputIndex,
            JObject target,
            string key)
        {
            if (da.TryGet(
                    inputIndex,
                    out string value) &&
                !string.IsNullOrEmpty(value))
            {
                target[key] = value;
            }
        }

        // The parameter definitions from the parallel lists. ParameterTypes
        // must have one item per name; the other lists may be empty.
        private bool TryGetParameters(
            IGH_DataAccess da,
            out JArray parameters)
        {
            parameters = new JArray();

            var names = da.GetOptionals<string>(ParameterNamesInput);
            var types = da.GetOptionals<string>(ParameterTypesInput);
            var values = da.GetOptionals<IGH_Goo>(ParameterValuesInput);
            var descriptions = da.GetOptionals<string>(ParameterDescriptionsInput);
            var hidden = da.GetOptionals<bool>(ParameterHiddenInput);

            if (types.Count != names.Count)
            {
                this.AddError(
                    "The size of the inputs ParameterNames and ParameterTypes must be equal.");
                return false;
            }

            if ((values.Count != 0 && values.Count != names.Count) ||
                (descriptions.Count != 0 && descriptions.Count != names.Count) ||
                (hidden.Count != 0 && hidden.Count != names.Count))
            {
                this.AddError(
                    "The inputs ParameterValues, ParameterDescriptions and ParameterHidden must be empty " +
                    "or have the size of ParameterNames.");
                return false;
            }

            for (var i = 0; i < names.Count; i++)
            {
                var parameter = new JObject
                {
                    ["name"] = names[i],
                    ["type"] = types[i]
                };

                if (i < values.Count && values[i] != null)
                {
                    if (!TryConvertParameterValue(
                            types[i],
                            values[i],
                            out JToken value))
                    {
                        this.AddError(
                            $"The value of the parameter '{names[i]}' cannot be converted to a {types[i]} value.");
                        return false;
                    }

                    if (value != null)
                    {
                        parameter["value"] = value;
                    }
                }

                if (i < descriptions.Count && !string.IsNullOrEmpty(descriptions[i]))
                {
                    parameter["description"] = descriptions[i];
                }

                if (i < hidden.Count && hidden[i])
                {
                    parameter["isHidden"] = true;
                }

                parameters.Add(parameter);
            }

            return true;
        }

        // Converts the generic input to the JSON type the Add-On reads for the
        // parameter type, so a number written in a panel still works as a number.
        private static bool TryConvertParameterValue(
            string type,
            IGH_Goo goo,
            out JToken value)
        {
            value = null;
            switch (type)
            {
                case "String":
                case "Title":
                    value = goo is GH_String text
                        ? text.Value
                        : goo.ToString();
                    return true;
                case "Boolean":
                    if (GH_Convert.ToBoolean(
                            goo,
                            out bool flag,
                            GH_Conversion.Both))
                    {
                        value = flag;
                        return true;
                    }
                    return false;
                case "LightSwitch":
                    if (goo is GH_String lightSwitch)
                    {
                        value = lightSwitch.Value;
                        return true;
                    }
                    if (GH_Convert.ToBoolean(
                            goo,
                            out bool isOn,
                            GH_Conversion.Both))
                    {
                        value = isOn ? "On" : "Off";
                        return true;
                    }
                    return false;
                case "Separator":
                    // A separator has no value.
                    return true;
                default:
                    if (GH_Convert.ToDouble(
                            goo,
                            out double number,
                            GH_Conversion.Both))
                    {
                        value = number;
                        return true;
                    }
                    return false;
            }
        }

        // Shares the CreateObjects icon until the component gets its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateObjects;

        public override Guid ComponentGuid =>
            new Guid("6b0f3c2e-9d71-4a8b-b3e4-2f5c7d19a0c4");
    }
}
