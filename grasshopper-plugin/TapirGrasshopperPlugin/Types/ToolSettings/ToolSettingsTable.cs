using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace TapirGrasshopperPlugin.Types.ToolSettings
{
    // One setting of an Archicad tool, as listed in ToolSettings/<Tool>.json:
    // its keys in the Add-On's JSON, and the label, panel (group) and heading
    // inside the panel (subgroup) of its control in the tool's settings dialog.
    public class ToolSettingsField
    {
        public string Get;
        public string Set;
        public bool GetFromElement;
        public string Label;
        public string Group;
        public string Subgroup;

        // The name the field has without labels: the key the Add-On takes,
        // or the one it returns when the field cannot be written.
        public string ApiName => Set ?? Get;

        public string Key(
            bool useLabels)
        {
            return useLabels && !string.IsNullOrEmpty(Label) ? Label : ApiName;
        }
    }

    // A ToolSettings/<Tool>.json table, embedded in the plugin (see the
    // README in that folder). The tool components build their JSON inputs
    // and outputs from it: one per group, keyed by label or by API name.
    public class ToolSettingsTable
    {
        public string Tool { get; private set; }
        public string ElementType { get; private set; }
        public List<ToolSettingsField> Fields { get; } = new List<ToolSettingsField>();

        // The groups in the order of the table, i.e. of the dialog.
        public List<string> Groups => Fields
            .Where(f => !string.IsNullOrEmpty(f.Group))
            .Select(f => f.Group)
            .Distinct()
            .ToList();

        private readonly Dictionary<string, ToolSettingsField> _byKey =
            new Dictionary<string, ToolSettingsField>(StringComparer.OrdinalIgnoreCase);

        public static ToolSettingsTable Load(
            string tool)
        {
            var assembly = typeof(ToolSettingsTable).Assembly;
            var resourceName = "ToolSettings." + tool + ".json";
            using (var stream = assembly.GetManifestResourceStream(resourceName))
            {
                if (stream == null)
                {
                    throw new InvalidOperationException(
                        $"The tool settings table {resourceName} is not embedded in the plugin.");
                }

                using (var reader = new StreamReader(stream))
                {
                    return Parse(JObject.Parse(reader.ReadToEnd()));
                }
            }
        }

        public static ToolSettingsTable Parse(
            JObject json)
        {
            var table = new ToolSettingsTable
            {
                Tool = json["tool"]?.ToString(),
                ElementType = json["elementType"]?.ToString()
            };

            foreach (var item in json["fields"] as JArray ?? new JArray())
            {
                var field = new ToolSettingsField
                {
                    Get = item["get"]?.Type == JTokenType.String ? item["get"].ToString() : null,
                    Set = item["set"]?.Type == JTokenType.String ? item["set"].ToString() : null,
                    GetFromElement = item["getFrom"]?.ToString() == "element",
                    Label = item["label"]?.Type == JTokenType.String ? item["label"].ToString() : null,
                    Group = item["group"]?.Type == JTokenType.String ? item["group"].ToString() : null,
                    Subgroup = item["subgroup"]?.Type == JTokenType.String ? item["subgroup"].ToString() : null
                };
                table.Fields.Add(field);
            }

            // A field is found by its label and by both its API names, so a
            // JSON written with labels, with API names, or read back from
            // GetDetailsOfElements all work. Labels are unique per tool
            // (tools/check_tool_settings.py checks that).
            foreach (var field in table.Fields)
            {
                foreach (var key in new[] { field.Label, field.Set, field.Get })
                {
                    if (!string.IsNullOrEmpty(key) && !table._byKey.ContainsKey(key))
                    {
                        table._byKey[key] = field;
                    }
                }
            }

            return table;
        }

        public ToolSettingsField Find(
            string key)
        {
            return key != null && _byKey.TryGetValue(key, out var field) ? field : null;
        }

        public IEnumerable<ToolSettingsField> FieldsOf(
            string group)
        {
            return Fields.Where(f => f.Group == group);
        }
    }
}
