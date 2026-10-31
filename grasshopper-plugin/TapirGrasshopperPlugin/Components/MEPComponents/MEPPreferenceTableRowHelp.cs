using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    // Shared code of the preference table row components: the row keys and
    // values are given as one JSON object per row, as the commands take them.
    internal static class MEPPreferenceTableRowHelp
    {
        public static readonly string[] KeyNames =
        {
            "shape",
            "index",
            "referenceId",
            "angle",
            "mainAxisReferenceId",
            "branchAxisReferenceId",
            "branchAxisAngle",
            "beginReferenceId",
            "endReferenceId"
        };

        public static readonly string[] ValueNames =
        {
            "value",
            "diameter",
            "wallThickness",
            "radius",
            "mainAxisDiameter",
            "branchAxisDiameter",
            "length",
            "branchOffset",
            "connectionLength",
            "beginDiameter",
            "endDiameter",
            "angle",
            "description"
        };

        public const string TableGuidDescription =
            "Guid of the preference table, as returned by GetMEPPreferenceTables (the branch, elbow and transition " +
            "tables with IncludeFittingTables): a pipe or circular duct segment table, a branch or elbow table of the " +
            "Piping or Ventilation domain, or, from Archicad 29, a transition table. Archicad 27: any preference table, " +
            "as returned by GetMEPPreferenceTableContainers.";

        public const string KeysDescription =
            "The key of each row as a JSON object (a plain integer is taken as referenceId). " +
            "Segment tables: {\"referenceId\": 20}. Elbow tables: referenceId and angle. Branch tables: " +
            "mainAxisReferenceId, branchAxisReferenceId and branchAxisAngle. Transition tables: beginReferenceId and " +
            "endReferenceId. The angles are as GetMEPPreferenceTables returns them; Archicad 28 finds the rows by their " +
            "reference ids only and rejects a key with an angle. Archicad 27: shape (Circular or Rectangular) and index.";

        public const string ValuesDescription =
            "The values of each row as a JSON object, lengths in meters. Segment tables: diameter, wallThickness and " +
            "description. Elbow tables: diameter, radius and description. Branch tables: mainAxisDiameter, " +
            "branchAxisDiameter, length, branchOffset, connectionLength and description. Pipe transition tables: " +
            "beginDiameter, endDiameter, length and description. Duct transition tables: beginDiameter, endDiameter, " +
            "angle and description. Archicad 27: value and description.";

        public static JObject TableId(
            string guid)
        {
            return new JObject { ["guid"] = guid };
        }

        public static bool TryGetKeys(
            GH_Component component,
            IGH_DataAccess da,
            int index,
            out List<JObject> keys)
        {
            return TryGetObjects(component, da, index, KeyNames, true, out keys);
        }

        public static bool TryGetValues(
            GH_Component component,
            IGH_DataAccess da,
            int index,
            out List<JObject> values)
        {
            return TryGetObjects(component, da, index, ValueNames, false, out values);
        }

        // Parses one JSON object per item of a text list input and checks
        // that it names only the known fields; an unconnected optional input
        // yields an empty list.
        private static bool TryGetObjects(
            GH_Component component,
            IGH_DataAccess da,
            int index,
            string[] names,
            bool integerIsReferenceId,
            out List<JObject> objects)
        {
            objects = new List<JObject>();
            var inputName = component.Params.Input[index].Name;
            da.TryGetList(index, out List<string> texts);
            foreach (var text in texts ?? new List<string>())
            {
                if (integerIsReferenceId &&
                    int.TryParse(text?.Trim(), out var referenceId))
                {
                    objects.Add(new JObject { ["referenceId"] = referenceId });
                    continue;
                }

                JObject item;
                try
                {
                    item = JObject.Parse(text ?? "");
                }
                catch (JsonReaderException ex)
                {
                    component.AddError($"Invalid JSON object in the {inputName} input: {ex.Message}");
                    return false;
                }

                var unknown = item.Properties()
                    .Select(property => property.Name)
                    .Where(name => !names.Contains(name))
                    .ToList();
                if (unknown.Count > 0)
                {
                    component.AddError(
                        $"Unknown field {string.Join(", ", unknown)} in the {inputName} input; " +
                        $"the known fields are {string.Join(", ", names)}.");
                    return false;
                }

                objects.Add(item);
            }
            return true;
        }

        // The per-row results of the Set and Add commands: the index of the
        // row (null when the command gives none) and the error message.
        public static void SetRowResults(
            IGH_DataAccess da,
            JObject response,
            int indicesOutput,
            int errorsOutput)
        {
            var indices = new List<object>();
            var errors = new List<string>();
            foreach (var result in JsonOutputHelp.Items(response, "executionResults"))
            {
                if ((bool?)result?["success"] == true)
                {
                    indices.Add(JsonOutputHelp.Scalar(result, "index"));
                    errors.Add("");
                    continue;
                }
                indices.Add(null);
                errors.Add(result?["error"]?["message"]?.ToString() ?? "Unknown error.");
            }

            da.SetDataList(indicesOutput, indices);
            da.SetDataList(errorsOutput, errors);
        }

        // Values given 1 (for all rows) or once per row.
        public static bool CheckCount(
            GH_Component component,
            int count,
            int keyCount,
            string inputName)
        {
            if (count > 1 && count != keyCount)
            {
                component.AddError(
                    $"The size of the input {inputName} must be 1 or equal to the size of the input Keys.");
                return false;
            }
            return true;
        }
    }
}
