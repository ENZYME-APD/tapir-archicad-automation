using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    // Shared code of the MEP port and preference table components.
    internal static class MEPComponentHelp
    {
        // Reads a list of guid texts (port ids, preference table guids) in
        // the hyphenated form the add-on accepts; an unconnected optional
        // input yields an empty list. Returns false on an invalid guid.
        public static bool TryGetGuidTexts(
            GH_Component component,
            IGH_DataAccess da,
            int index,
            out List<string> guids)
        {
            guids = new List<string>();
            da.TryGetList(index, out List<string> texts);
            foreach (var text in texts ?? new List<string>())
            {
                if (!Guid.TryParse(text, out var guid))
                {
                    component.AddError(
                        $"Invalid identifier in the {component.Params.Input[index].Name} input.");
                    return false;
                }
                guids.Add(guid.ToString("D"));
            }
            return true;
        }

        // The same for a single guid text.
        public static bool TryGetGuidText(
            GH_Component component,
            IGH_DataAccess da,
            int index,
            out string guid)
        {
            guid = null;
            string text = null;
            if (!da.GetData(index, ref text))
            {
                return false;
            }
            if (!Guid.TryParse(text, out var parsed))
            {
                component.AddError(
                    $"Invalid identifier in the {component.Params.Input[index].Name} input.");
                return false;
            }
            guid = parsed.ToString("D");
            return true;
        }

        // The message of a single ExecutionResult: empty when it succeeded,
        // null when the response has no such result.
        public static string ExecutionResultMessage(
            JToken result)
        {
            if (result == null || result.Type == JTokenType.Null)
            {
                return null;
            }
            return (bool?)result["success"] == true
                ? ""
                : result["error"]?["message"]?.ToString() ?? "Unknown error.";
        }
    }
}
