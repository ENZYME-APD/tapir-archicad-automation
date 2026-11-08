using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetUnusedAttributesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetUnusedAttributes";

        internal const string TypesDescription =
            "Optional. The attribute types: layer, fill, surface, buildingMaterial, composite, profile, " +
            "lineType, penTable, zoneCategory (the names of the AttributeType list, e.g. Line or BuildingMaterial, " +
            "work too). All of them when empty.";

        public GetUnusedAttributesComponent()
            : base(
                "GetUnusedAttributes",
                "Lists the unused layers, fills, surfaces, building materials, composites, profiles, line types, " +
                "pen tables and zone categories, for the given types or all of them. Every reference is " +
                "collected (elements of the floor plan and of every view and layout, tool defaults, Favorites, " +
                "other attributes, graphic override rules); indices typed into GDL scripts and the pen tables " +
                "saved with views are not found. Read only. Available from Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Types",
                TypesDescription);

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "AttributeTypes",
                "The type of each unused attribute.");

            OutGenerics(
                "AttributeGuids",
                "The identifier of each unused attribute.");

            OutIntegers(
                "Indices",
                "The index of each unused attribute.");

            OutTexts(
                "Names",
                "The name of each unused attribute.");

            OutTexts(
                "CheckedTypes",
                "The checked attribute types, one item per type.");

            OutIntegers(
                "AttributeCounts",
                "The number of attributes of each checked type in the project.");

            OutIntegers(
                "UsedCounts",
                "The number of attributes of each checked type that are in use, as this command counts them.");

            OutIntegers(
                "ArchicadUsedCounts",
                "The number of attributes of each checked type in use as Archicad itself counts them " +
                "(see GetUsedAttributes); null when Archicad's report does not name the type. When it differs " +
                "from UsedCounts, check the list before deleting.");

            OutTexts(
                "Warnings",
                "What could not be checked; then a listed attribute may still be used.");
        }

        // The command takes the types in lower camel case; the AttributeType value list
        // gives them capitalized and calls line types Line.
        internal static JArray ToTypesParameter(
            List<string> types)
        {
            var result = new JArray();
            foreach (var type in types)
            {
                var trimmed = type?.Trim();
                if (string.IsNullOrEmpty(trimmed))
                {
                    continue;
                }

                if (trimmed.Equals("line", StringComparison.OrdinalIgnoreCase))
                {
                    trimmed = "lineType";
                }

                result.Add(char.ToLowerInvariant(trimmed[0]) + trimmed.Substring(1));
            }

            return result;
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();
            var types = ToTypesParameter(da.GetOptionals<string>(0));
            if (types.Count > 0)
            {
                parameters["types"] = types;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var attributeTypes = new List<string>();
            var attributeGuids = new List<AttributeGuidObject>();
            var indices = new List<object>();
            var names = new List<string>();
            var checkedTypes = new List<string>();
            var attributeCounts = new List<object>();
            var usedCounts = new List<object>();
            var archicadUsedCounts = new List<object>();

            foreach (var typeItem in JsonOutputHelp.Items(response, "attributeTypes"))
            {
                var type = typeItem["type"]?.ToString();
                checkedTypes.Add(type);
                attributeCounts.Add(JsonOutputHelp.Scalar(typeItem, "attributeCount"));
                usedCounts.Add(JsonOutputHelp.Scalar(typeItem, "usedCount"));
                archicadUsedCounts.Add(JsonOutputHelp.Scalar(typeItem, "archicadUsedCount"));

                if (!(typeItem["unusedAttributes"] is JArray unusedAttributes))
                {
                    continue;
                }

                foreach (var attribute in unusedAttributes)
                {
                    attributeTypes.Add(type);
                    attributeGuids.Add(attribute["attributeId"]?.ToObject<AttributeGuidObject>());
                    indices.Add(JsonOutputHelp.Scalar(attribute, "index"));
                    names.Add(attribute["name"]?.ToString());
                }
            }

            da.SetDataList(0, attributeTypes);
            da.SetDataList(1, attributeGuids);
            da.SetDataList(2, indices);
            da.SetDataList(3, names);
            da.SetDataList(4, checkedTypes);
            da.SetDataList(5, attributeCounts);
            da.SetDataList(6, usedCounts);
            da.SetDataList(7, archicadUsedCounts);
            da.SetDataList(8, Warnings(response));
        }

        internal static List<string> Warnings(
            JObject response)
        {
            var warnings = new List<string>();
            if (response?["warnings"] is JArray items)
            {
                foreach (var item in items)
                {
                    warnings.Add(item?.ToString());
                }
            }

            return warnings;
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetUnusedAttributes;

        public override Guid ComponentGuid =>
            new Guid("7d2e94b1-5c3a-4f86-a0e7-19b4c6d8e253");
    }
}
