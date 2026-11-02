using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Globalization;
using TapirGrasshopperPlugin.Components.MEPComponents;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class CreateLibraryPartsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateLibraryParts";

        // The text inputs of a library part after Names, with their key.
        private static readonly (string Input, string Key, string Description)[] TextInputs =
        {
            ("ParentIds", "parentId",
                "Parent subtype of each library part: its unique ID ('{main}-{revision}') or its main GUID, " +
                "as GetAvailableLibraryParts reports it. Either ParentIds or ParentNames is required"),
            ("ParentNames", "parentName",
                "Document name of the parent subtype of each library part, e.g. 'General Object' or 'General Label' " +
                "from the Archicad Library; used when ParentIds is not given"),
            ("Types", "type",
                "Type of each library part (Object, Lamp, Window, Door, Label, Zone, ...). " +
                "By default the type of the parent"),
            ("Paths", "path",
                "Relative path of the .gsm file of each library part inside the embedded library, " +
                "like the OutputPaths of AddFilesToEmbeddedLibrary. By default '<name>.gsm'")
        };

        private static readonly (string Input, string Key, string Description)[] ScriptInputs =
        {
            ("MasterScripts", "masterScript", "GDL master script"),
            ("Scripts2D", "script2D", "GDL 2D script"),
            ("Scripts3D", "script3D", "GDL 3D script"),
            ("ParameterScripts", "parameterScript", "GDL parameter script (value lists, locks, hiding)"),
            ("UIScripts", "uiScript", "GDL user interface script"),
            ("PropertiesScripts", "propertiesScript", "GDL properties (component listing) script")
        };

        private const int FirstNumberInput = 5;
        private const int FirstScriptInput = 7;
        private const int FirstParameterInput = FirstScriptInput + 6;
        private const int OverwriteExistingInput = FirstParameterInput + 5;

        public CreateLibraryPartsComponent()
            : base(
                "CreateLibraryParts",
                "Creates GDL library parts in the embedded library from their scripts (master, 2D, 3D, parameter, " +
                "interface, properties) and parameter definitions, under the given parent subtype. The new parts are " +
                "placeable right away, e.g. with CreateObjects. With OverwriteExisting an embedded library item already " +
                "existing on the path of a new part is replaced (Archicad 27 or newer).",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Name of each new library part, as shown in the library and used by CreateObjects as libraryPartName.");

            foreach (var input in TextInputs)
            {
                InTexts(
                    input.Input,
                    input.Description + " (input only 1 to use the same value for all). Optional.");
            }

            InNumbers(
                "A",
                "Default value of the A (X size) parameter of each library part " +
                "(input only 1 to use the same value for all). Optional, 1.0 by default.");

            InNumbers(
                "B",
                "Default value of the B (Y size) parameter of each library part " +
                "(input only 1 to use the same value for all). Optional, 1.0 by default.");

            foreach (var input in ScriptInputs)
            {
                InTexts(
                    input.Input,
                    $"The {input.Description} of each library part (input only 1 to use the same script for all). Optional.");
            }

            InTextTree(
                "ParameterNames",
                "GDL variable names (at most 31 characters) of the parameters of each library part besides A and B, " +
                "in the order they appear in the settings dialog (one branch per library part, or one branch for all). " +
                "Optional.");

            InTextTree(
                "ParameterTypes",
                "Type of each parameter, as GetGDLParametersOfElements reports it: Integer, Length, Angle, RealNumber, " +
                "LightSwitch, RGBColor, Intensity, LineType, Material, FillPattern, PenColor, String, Boolean, " +
                "Separator, Title, BuildingMaterial or Profile (same tree shape as ParameterNames).");

            InGenericTree(
                "ParameterValues",
                "Default value of each parameter: a number for the numeric and attribute index types, a text for String " +
                "and Title, true or false for Boolean, 'On' or 'Off' for LightSwitch " +
                "(same tree shape as ParameterNames). Optional.");

            InTextTree(
                "ParameterDescriptions",
                "Display name of each parameter in the settings dialog (same tree shape as ParameterNames). Optional.");

            InBooleanTree(
                "ParameterIsHidden",
                "Hide each parameter from the settings dialog (same tree shape as ParameterNames). Optional.");

            InBoolean(
                "OverwriteExisting",
                "Replace an embedded library item already existing on the path of a new library part " +
                "(Archicad 27 or newer).",
                false);

            var optional = new List<int>();
            for (var i = 1; i <= OverwriteExistingInput; i++)
            {
                optional.Add(i);
            }
            SetOptionality(optional.ToArray());
        }

        private void InTextTree(
            string name,
            string description)
        {
            inManager.AddTextParameter(
                name,
                name,
                description,
                GH_ParamAccess.tree);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "LibraryPartGuids",
                "Main GUID of each created library part, as GetAvailableLibraryParts reports it (empty when it failed).");

            OutTexts(
                "UnIds",
                "Unique ID ('{main}-{revision}') of each created library part, usable as ParentIds of further parts.");

            OutTexts(
                "Names",
                "Name of each created library part.");

            OutIntegers(
                "Indices",
                "Index of each created library part in the loaded libraries.");

            OutTexts(
                "Paths",
                "Relative path of the .gsm file of each created library part inside the embedded library; " +
                "DeleteEmbeddedLibraryItems takes it as path.");

            OutErrorMessages(
                "Error message of each library part (empty when it was created).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> names))
            {
                return;
            }

            var fields = new ElectricalItemFields(this, 0, names.Count);
            for (var i = 0; i < TextInputs.Length; i++)
            {
                if (!fields.AddTexts(da, 1 + i, TextInputs[i].Key))
                {
                    return;
                }
            }
            if (!fields.AddNumbers(da, FirstNumberInput, "a") ||
                !fields.AddNumbers(da, FirstNumberInput + 1, "b"))
            {
                return;
            }
            for (var i = 0; i < ScriptInputs.Length; i++)
            {
                if (!fields.AddTexts(da, FirstScriptInput + i, ScriptInputs[i].Key))
                {
                    return;
                }
            }

            da.TryGetTree(FirstParameterInput, out GH_Structure<GH_String> parameterNames);
            da.TryGetTree(FirstParameterInput + 1, out GH_Structure<GH_String> parameterTypes);
            da.TryGetTree(FirstParameterInput + 2, out GH_Structure<IGH_Goo> parameterValues);
            da.TryGetTree(FirstParameterInput + 3, out GH_Structure<GH_String> parameterDescriptions);
            da.TryGetTree(FirstParameterInput + 4, out GH_Structure<GH_Boolean> parameterIsHidden);

            var nameBranches = Branches(parameterNames);
            if (nameBranches.Count > 1 &&
                nameBranches.Count != names.Count)
            {
                this.AddError(
                    "The ParameterNames input must have 0, 1 or as many branches as the size of the input Names.");
                return;
            }

            var items = new JArray();
            for (var i = 0; i < names.Count; i++)
            {
                var item = new JObject { ["name"] = names[i] };
                fields.ApplyTo(item, i);

                if (nameBranches.Count > 0)
                {
                    var branch = nameBranches.Count == 1 ? 0 : i;
                    if (!TryGetParameters(
                            branch,
                            nameBranches[branch],
                            Branches(parameterTypes),
                            Branches(parameterValues),
                            Branches(parameterDescriptions),
                            Branches(parameterIsHidden),
                            out var parameters))
                    {
                        return;
                    }
                    item["parameters"] = parameters;
                }

                items.Add(item);
            }

            var input = new JObject { ["libraryParts"] = items };
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

            var guids = new List<object>();
            var unIds = new List<object>();
            var createdNames = new List<object>();
            var indices = new List<object>();
            var paths = new List<object>();
            var errors = new List<string>();
            foreach (var created in JsonOutputHelp.Items(response, "libraryParts"))
            {
                if (JsonOutputHelp.IsError(created))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(created));
                    guids.Add(null);
                    unIds.Add(null);
                    createdNames.Add(null);
                    indices.Add(null);
                    paths.Add(null);
                    continue;
                }

                errors.Add("");
                guids.Add(JsonOutputHelp.Scalar(created, "guid"));
                unIds.Add(JsonOutputHelp.Scalar(created, "unId"));
                createdNames.Add(JsonOutputHelp.Scalar(created, "name"));
                indices.Add(JsonOutputHelp.Scalar(created, "index"));
                paths.Add(JsonOutputHelp.Scalar(created, "path"));
            }

            da.SetDataList(0, guids);
            da.SetDataList(1, unIds);
            da.SetDataList(2, createdNames);
            da.SetDataList(3, indices);
            da.SetDataList(4, paths);
            da.SetDataList(5, errors);
        }

        private static List<List<T>> Branches<T>(
            GH_Structure<T> tree)
            where T : IGH_Goo
        {
            var result = new List<List<T>>();
            if (tree == null)
            {
                return result;
            }
            foreach (var branch in tree.Branches)
            {
                result.Add(branch);
            }
            return result;
        }

        // The branch of a parameter tree that belongs to the parameter names
        // of the given branch; its size must be 0 or that of the names.
        private bool TryGetBranch<T>(
            List<List<T>> branches,
            int branchIndex,
            int count,
            string inputName,
            out List<T> branch)
            where T : IGH_Goo
        {
            branch = branches.Count == 0
                ? new List<T>()
                : branches[branches.Count == 1 ? 0 : Math.Min(branchIndex, branches.Count - 1)];

            if (branches.Count > 1 && branchIndex >= branches.Count ||
                branch.Count != 0 && branch.Count != count)
            {
                this.AddError(
                    $"The {inputName} input must have the same tree shape as the ParameterNames input.");
                return false;
            }
            return true;
        }

        private bool TryGetParameters(
            int branchIndex,
            List<GH_String> parameterNames,
            List<List<GH_String>> typeBranches,
            List<List<IGH_Goo>> valueBranches,
            List<List<GH_String>> descriptionBranches,
            List<List<GH_Boolean>> isHiddenBranches,
            out JArray parameters)
        {
            parameters = null;
            var count = parameterNames.Count;
            if (!TryGetBranch(typeBranches, branchIndex, count, "ParameterTypes", out var types) ||
                !TryGetBranch(valueBranches, branchIndex, count, "ParameterValues", out var values) ||
                !TryGetBranch(descriptionBranches, branchIndex, count, "ParameterDescriptions", out var descriptions) ||
                !TryGetBranch(isHiddenBranches, branchIndex, count, "ParameterIsHidden", out var isHidden))
            {
                return false;
            }

            if (types.Count != count)
            {
                this.AddError("Give the type of each parameter in the ParameterTypes input.");
                return false;
            }

            parameters = new JArray();
            for (var j = 0; j < count; j++)
            {
                var type = types[j]?.Value;
                var parameter = new JObject
                {
                    ["name"] = parameterNames[j]?.Value,
                    ["type"] = type
                };

                var value = values.Count > 0 ? ValueOf(values[j], type) : null;
                if (value != null)
                {
                    parameter["value"] = value;
                }
                if (descriptions.Count > 0 && descriptions[j]?.Value != null)
                {
                    parameter["description"] = descriptions[j].Value;
                }
                if (isHidden.Count > 0 && isHidden[j] != null)
                {
                    parameter["isHidden"] = isHidden[j].Value;
                }

                parameters.Add(parameter);
            }
            return true;
        }

        // The default value as the parameter type expects it: a text for
        // String, Title and LightSwitch, a boolean for Boolean, a number
        // otherwise (a number given as text is converted).
        private static JToken ValueOf(
            IGH_Goo goo,
            string type)
        {
            var value = goo?.ScriptVariable();
            switch (value)
            {
                case null:
                    return null;
                case bool boolean:
                    return boolean;
                case int integer:
                    return integer;
                case double number:
                    return number;
                case string text:
                    if (type == "String" || type == "Title" || type == "LightSwitch")
                    {
                        return text;
                    }
                    if (type == "Boolean" && bool.TryParse(text, out var parsedBoolean))
                    {
                        return parsedBoolean;
                    }
                    if (double.TryParse(text, NumberStyles.Float, CultureInfo.InvariantCulture, out var parsedNumber))
                    {
                        return parsedNumber;
                    }
                    return text;
                default:
                    return value.ToString();
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateLibraryParts;

        public override Guid ComponentGuid =>
            new Guid("46c6b122-2de5-4d87-b06c-01c33967f945");
    }
}
