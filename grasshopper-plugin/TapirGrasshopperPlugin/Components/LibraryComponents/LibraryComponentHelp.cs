using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    // Shared code of the library components.
    internal static class LibraryComponentHelp
    {
        // The commands that answer with a single ExecutionResult
        // ({"success": true} or {"success": false, "error": {...}}): empty
        // text when it succeeded, the error message otherwise.
        public static string ExecutionResultMessage(
            JObject response)
        {
            if ((bool?)response?["success"] == true)
            {
                return "";
            }

            return response?["error"]?["message"]?.ToString() ??
                   "Unknown error.";
        }

        // The type identifier of a library part custom section: an integer,
        // or a four-character code as text ('ABCD'), which is turned into its
        // integer value with the first character in the highest byte.
        public static bool TryGetSectionId(
            GH_Component component,
            IGH_DataAccess da,
            int index,
            out long sectionId)
        {
            sectionId = 0;
            if (!da.TryGet(
                    index,
                    out GH_ObjectWrapper wrapper) ||
                wrapper?.Value == null)
            {
                return false;
            }

            var value = wrapper.Value is IGH_Goo goo
                ? goo.ScriptVariable()
                : wrapper.Value;

            // sectionId is an unsigned 32-bit value in the schema, so it is kept in a long
            // and sent unchanged; a code starting with a character >= 0x80 stays positive.
            switch (value)
            {
                case int integer when integer >= 0:
                    sectionId = integer;
                    return true;
                case double number when Math.Abs(number - Math.Round(number)) < 1e-9 &&
                                        number >= 0 && number <= uint.MaxValue:
                    sectionId = (long)Math.Round(number);
                    return true;
                case string text when uint.TryParse(text, out var parsed):
                    sectionId = parsed;
                    return true;
                case string code when code.Length == 4 && code.All(c => c <= 0xFF):
                    var id = 0u;
                    foreach (var character in code)
                    {
                        id = (id << 8) | character;
                    }
                    sectionId = id;
                    return true;
            }

            component.AddError(
                $"The {component.Params.Input[index].Name} input must be an integer from 0 to 4294967295 or a four-character code.");
            return false;
        }
    }
}
