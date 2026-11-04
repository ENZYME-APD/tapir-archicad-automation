using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class SetOpeningSymbolParametersComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetOpeningSymbolParameters";

        // The keys of a parameter the command accepts.
        private static readonly ISet<string> ParameterKeys = new HashSet<string>
        {
            "name",
            "index1",
            "index2",
            "value"
        };

        // The keys GetOpeningSymbolParameters returns besides them; they are
        // left out, so its output can be given back as it is.
        private static readonly ISet<string> ReadOnlyKeys = new HashSet<string>
        {
            "type",
            "dimension1",
            "dimension2"
        };

        public SetOpeningSymbolParametersComponent()
            : base(
                "SetOpeningSymbolParameters",
                "Changes the GDL parameters of the cut and uncut floor plan symbols of Opening elements, in " +
                "the format of SetGDLParametersOfElements. Only the given parameters are changed. Available " +
                "from Archicad 29.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the Opening elements (made with the Opening tool) to change.");

            InTexts(
                "CutSymbolParameters",
                "Optional. The parameters of the cut symbol to change, as a JSON array of objects with name " +
                "and value, and for one item of an array parameter its 1-based index1 (and index2). The " +
                "output of GetOpeningSymbolParameters is accepted as it is. Input only 1 to use the same " +
                "parameters for all.");

            InTexts(
                "UnCutSymbolParameters",
                "Optional. The parameters of the uncut symbol to change, in the same format. Input only 1 " +
                "to use the same parameters for all.");

            SetOptionality(new[] { 1, 2 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each opening (empty when it was changed).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> elementWrappers) ||
                !this.TryGetElementIds(
                    elementWrappers,
                    "ElementGuids",
                    out var elementIds))
            {
                return;
            }

            var cutTexts = da.OptionalList<string>(1);
            var unCutTexts = da.OptionalList<string>(2);
            if (!this.CheckSizes(
                    elementIds.Count,
                    "ElementGuids",
                    ("CutSymbolParameters", cutTexts.Count),
                    ("UnCutSymbolParameters", unCutTexts.Count)))
            {
                return;
            }

            var items = new JArray();
            for (var i = 0; i < elementIds.Count; i++)
            {
                var item = new JObject { ["elementId"] = elementIds[i] };

                if (cutTexts.Count > 0)
                {
                    if (!TryParseParameters(
                            cutTexts.At(i),
                            "CutSymbolParameters",
                            out var cutParameters))
                    {
                        return;
                    }

                    if (cutParameters != null)
                    {
                        item["cutSymbolParameters"] = cutParameters;
                    }
                }

                if (unCutTexts.Count > 0)
                {
                    if (!TryParseParameters(
                            unCutTexts.At(i),
                            "UnCutSymbolParameters",
                            out var unCutParameters))
                    {
                        return;
                    }

                    if (unCutParameters != null)
                    {
                        item["unCutSymbolParameters"] = unCutParameters;
                    }
                }

                items.Add(item);
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["elements"] = items },
                ToAddOn,
                da);
        }

        // A JSON array of parameters; null for an empty text. The read-only
        // keys of GetOpeningSymbolParameters are dropped, other unknown keys
        // are an error.
        private bool TryParseParameters(
            string text,
            string inputName,
            out JArray parameters)
        {
            parameters = null;
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
                this.AddError($"{inputName} is not a valid JSON array: {ex.Message}");
                return false;
            }

            parameters = new JArray();
            foreach (var token in parsed)
            {
                if (!(token is JObject parameter))
                {
                    this.AddError($"Every item of {inputName} has to be a JSON object.");
                    return false;
                }

                var result = new JObject();
                foreach (var property in parameter.Properties())
                {
                    if (ParameterKeys.Contains(property.Name))
                    {
                        result[property.Name] = property.Value;
                    }
                    else if (!ReadOnlyKeys.Contains(property.Name))
                    {
                        this.AddError($"Unknown key \"{property.Name}\" in {inputName}.");
                        return false;
                    }
                }

                parameters.Add(result);
            }

            return true;
        }

        // Shares the icon of SetOpeningFloorPlanParameters until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetOpeningFloorPlanParameters;

        public override Guid ComponentGuid =>
            new Guid("64f126dd-4e71-45fe-a798-381acf905094");
    }
}
