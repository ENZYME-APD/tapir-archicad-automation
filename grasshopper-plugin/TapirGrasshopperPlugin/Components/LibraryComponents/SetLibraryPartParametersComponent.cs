using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class SetLibraryPartParametersComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetLibraryPartParameters";

        // The keys of a parameter the command accepts.
        private static readonly ISet<string> ParameterKeys = new HashSet<string>
        {
            "name",
            "index1",
            "index2",
            "value"
        };

        // The keys GetGDLParametersOfLibraryParts returns besides them; they
        // are left out, so its output can be given back as it is.
        private static readonly ISet<string> ReadOnlyKeys = new HashSet<string>
        {
            "displayName",
            "index",
            "type",
            "dimension1",
            "dimension2",
            "valueDescription",
            "itemDescriptions",
            "flags"
        };

        public SetLibraryPartParametersComponent()
            : base(
                "SetLibraryPartParameters",
                "Sets the parameter values, and optionally the parameter descriptions, of loaded library parts, in " +
                "the format of SetGDLParametersOfElements. Only the given parameters are changed; with Merge, the " +
                "changed list is merged into the parameters of the part instead of replacing them. A library part " +
                "that is not editable, or with an invalid value given, is left unchanged. Available from " +
                "Archicad 28.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Name of each loaded library part file with its extension, as the fileName of " +
                "GetAvailableLibraryParts (e.g. 'My Door.gsm').");

            InTexts(
                "Parameters",
                "The parameters to change in each library part, as a JSON array of objects with name and value, " +
                "and for one item of an array parameter its 1-based index1 (and index2). The Parameters output of " +
                "GetGDLParametersOfLibraryParts is accepted as it is. Input only 1 to use the same parameters for " +
                "all.");

            InTexts(
                "Descriptions",
                "The new descriptions of all parameters of each library part, as a JSON array of texts, one for " +
                "each parameter in the order of its parameter list. The current descriptions by default. Input " +
                "only 1 to use the same for all. Optional.");

            InBooleans(
                "Merge",
                "True to merge the changed parameter list into the parameters of the library part instead of " +
                "replacing them. False by default. Input only 1 to use the same for all. Optional.");

            SetOptionality(new[] { 2, 3 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each library part (empty when its parameters were set).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> names) ||
                !da.TryGetList(
                    1,
                    out List<string> parameterTexts))
            {
                return;
            }

            var descriptionTexts = da.OptionalList<string>(2);
            var merges = da.OptionalList<bool>(3);
            if (!this.CheckSizes(
                    names.Count,
                    "Names",
                    ("Parameters", parameterTexts.Count),
                    ("Descriptions", descriptionTexts.Count),
                    ("Merge", merges.Count)))
            {
                return;
            }

            var libraryParts = new JArray();
            for (var i = 0; i < names.Count; i++)
            {
                if (!TryParseParameters(
                        parameterTexts.At(i),
                        out var parameters))
                {
                    return;
                }

                var libraryPart = new JObject
                {
                    ["name"] = names[i],
                    ["parameters"] = parameters
                };

                if (descriptionTexts.Count > 0 &&
                    !string.IsNullOrWhiteSpace(descriptionTexts.At(i)))
                {
                    if (!TryParseDescriptions(
                            descriptionTexts.At(i),
                            out var descriptions))
                    {
                        return;
                    }

                    libraryPart["descriptions"] = descriptions;
                }

                if (merges.Count > 0)
                {
                    libraryPart["merge"] = merges.At(i);
                }

                libraryParts.Add(libraryPart);
            }

            if (libraryParts.Count == 0)
            {
                return;
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["libraryParts"] = libraryParts },
                ToAddOn,
                da);
        }

        // A JSON array of parameters; an empty array for an empty text. The
        // read-only keys of GetGDLParametersOfLibraryParts are dropped, and so
        // are its parameters without a value (Dictionary parameters); other
        // unknown keys are an error.
        private bool TryParseParameters(
            string text,
            out JArray parameters)
        {
            parameters = new JArray();
            if (string.IsNullOrWhiteSpace(text))
            {
                return true;
            }

            JArray parsed;
            try
            {
                parsed = JArray.Parse(text);
            }
            catch (JsonException ex)
            {
                this.AddError($"Parameters is not a valid JSON array: {ex.Message}");
                return false;
            }

            foreach (var token in parsed)
            {
                if (!(token is JObject parameter))
                {
                    this.AddError("Every item of Parameters has to be a JSON object.");
                    return false;
                }

                var result = new JObject();
                var readBack = false;
                foreach (var property in parameter.Properties())
                {
                    if (ParameterKeys.Contains(property.Name))
                    {
                        result[property.Name] = property.Value;
                    }
                    else if (ReadOnlyKeys.Contains(property.Name))
                    {
                        readBack = true;
                    }
                    else
                    {
                        this.AddError($"Unknown key \"{property.Name}\" in Parameters.");
                        return false;
                    }
                }

                if (readBack && result["value"] == null)
                {
                    continue;
                }

                parameters.Add(result);
            }

            return true;
        }

        // A JSON array of texts.
        private bool TryParseDescriptions(
            string text,
            out JArray descriptions)
        {
            descriptions = null;
            try
            {
                descriptions = JArray.Parse(text);
            }
            catch (JsonException ex)
            {
                this.AddError($"Descriptions is not a valid JSON array: {ex.Message}");
                return false;
            }

            foreach (var description in descriptions)
            {
                if (description.Type != JTokenType.String)
                {
                    this.AddError("Every item of Descriptions has to be a text.");
                    return false;
                }
            }

            return true;
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetElementGDLs;

        public override Guid ComponentGuid =>
            new Guid("65cd6c97-d940-43fc-bab8-69885c9b269f");
    }
}
