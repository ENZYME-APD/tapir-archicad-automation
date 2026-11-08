using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetElementsByCriteriaComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetElementsByCriteria";

        public GetElementsByCriteriaComponent()
            : base(
                "ElementsByCriteria",
                "Get the elements that match all the given criteria: element types, home stories, layers, " +
                "element filters and property values. Every input is optional; with none given, every " +
                "element of the current database is listed.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "ElementTypes",
                "Optional. Keep only the elements of one of these types (connect an ElementsType value list).");

            InIntegers(
                "FloorIndices",
                "Optional. Keep only the elements whose home story has one of these indices.");

            InIntegers(
                "LayerIndices",
                "Optional. Keep only the elements on one of these layers (layer attribute indices).");

            InTexts(
                "Filters",
                defaultValue: nameof(ElementFilter.NoFilter),
                description:
                "Element filters an element must pass. Objects that belong to another element (zone stamps, " +
                "the parts of stairs, railings and curtain walls) are left out unless IncludeSubElemObjects " +
                "is given.");

            InTexts(
                "PropertyCriteria",
                "Optional. Property criteria, all of which must match. Each item is a JSON object, or a JSON " +
                "array of them, with the keys propertyId ({\"guid\": \"...\"}) or propertyName (with " +
                "propertyGroupName when the name is used in more than one group), operator (equals, " +
                "notEquals, contains, greaterThan, lessThan, greaterOrEqual, lessOrEqual, hasValue, " +
                "isUserUndefined, isNotAvailable, isDefault, hasCustomValue; default equals) and value (a " +
                "string; numbers in m, m², m³ and rad, with a decimal point). " +
                "Example: {\"propertyName\": \"Renovation Status\", \"value\": \"New\"}.");

            InBoolean(
                "CountOnly",
                "If true, only the number of matching elements is returned, without their Guids.");

            // No default value: an unconnected input must not limit.
            inManager.AddIntegerParameter(
                "Limit",
                "Limit",
                "Optional. The most element Guids to list (at least 1). Count is still the number of all matches."
                    .WithTypeName("integerItem"),
                GH_ParamAccess.item);

            SetOptionality(
                new[]
                {
                    0,
                    1,
                    2,
                    3,
                    4,
                    5,
                    6
                });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "The Guids of the matching elements (empty when CountOnly is true).");

            OutInteger(
                "Count",
                "The number of all matching elements, also when Limit lists fewer.");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            AddAsSource<ElementTypeValueList>(
                document,
                0);
        }

        private static List<T> NullIfEmpty<T>(
            List<T> list)
        {
            return list == null || list.Count == 0 ? null : list;
        }

        private static readonly HashSet<string> CriterionKeys = new HashSet<string>
        {
            "propertyId",
            "propertyName",
            "propertyGroupName",
            "operator",
            "value"
        };

        // Reads the PropertyCriteria items: each one a JSON object or an array of them.
        private bool TryParsePropertyCriteria(
            List<string> items,
            out List<PropertyCriterionObject> criteria)
        {
            criteria = new List<PropertyCriterionObject>();
            foreach (var item in items)
            {
                if (string.IsNullOrWhiteSpace(item))
                {
                    continue;
                }

                JToken token;
                try
                {
                    token = JToken.Parse(item);
                }
                catch (JsonException ex)
                {
                    this.AddError(
                        $"Invalid JSON in the PropertyCriteria input: {ex.Message}");
                    return false;
                }

                var objects = new List<JToken>();
                if (token is JArray array)
                {
                    objects.AddRange(array);
                }
                else
                {
                    objects.Add(token);
                }

                foreach (var obj in objects)
                {
                    if (!(obj is JObject criterion))
                    {
                        this.AddError(
                            "Each PropertyCriteria item must be a JSON object, or an array of JSON objects.");
                        return false;
                    }

                    foreach (var property in criterion.Properties())
                    {
                        if (!CriterionKeys.Contains(property.Name))
                        {
                            this.AddError(
                                $"Unknown key in the PropertyCriteria input: {property.Name}. The keys are " +
                                string.Join(", ", CriterionKeys) + ".");
                            return false;
                        }
                    }

                    try
                    {
                        criteria.Add(criterion.ToObject<PropertyCriterionObject>());
                    }
                    catch (JsonException ex)
                    {
                        this.AddError(
                            $"Invalid criterion in the PropertyCriteria input: {ex.Message}");
                        return false;
                    }
                }
            }

            return true;
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            // The optional inputs are empty lists when nothing is connected.
            da.TryGetList(
                0,
                out List<string> elementTypes);

            da.TryGetList(
                1,
                out List<int> floorIndices);

            da.TryGetList(
                2,
                out List<int> layerIndices);

            da.TryGetList(
                3,
                out List<string> filters);

            da.TryGetList(
                4,
                out List<string> criteriaItems);

            if (!TryParsePropertyCriteria(
                    criteriaItems,
                    out var propertyCriteria))
            {
                return;
            }

            var countOnly = false;
            da.GetData(
                5,
                ref countOnly);

            int? limit = null;
            var limitValue = 0;
            if (da.GetData(
                    6,
                    ref limitValue))
            {
                if (limitValue < 1)
                {
                    this.AddError("Limit must be at least 1.");
                    return;
                }

                limit = limitValue;
            }

            var input = new ElementsByCriteriaObject
            {
                Types = NullIfEmpty(elementTypes),
                FloorIndices = NullIfEmpty(floorIndices),
                LayerIndices = NullIfEmpty(layerIndices),
                Filters = NullIfEmpty(filters),
                PropertyCriteria = NullIfEmpty(propertyCriteria),
                CountOnly = countOnly ? true : (bool?)null,
                Limit = limit
            };

            if (!TryGetConvertedCadValues(
                    CommandName,
                    input,
                    ToAddOn,
                    JHelp.Deserialize<ElementsByCriteriaResponse>,
                    out ElementsByCriteriaResponse response))
            {
                return;
            }

            da.SetDataList(
                0,
                response.Elements ?? new List<ElementGuidWrapper>());

            da.SetData(
                1,
                response.Count);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.FilterElems;

        public override Guid ComponentGuid =>
            new Guid("5d0f3c8a-7e21-4b9d-a6c4-2f8e91b37d05");
    }
}
