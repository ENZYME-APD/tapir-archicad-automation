using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Properties;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class SearchPropertiesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "SearchProperties";

        public SearchPropertiesComponent()
            : base(
                "SearchProperties",
                "Search the properties by a part of their name or group name, optionally only those of a " +
                "property type, value type or collection type. Every input is optional; with none given, " +
                "every property is listed.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Query",
                "Optional. Lists the properties whose name or group name contains this text (case-insensitive).");

            InText(
                "PropertyGroup",
                "Optional. Lists only the properties of the groups whose name contains this text (case-insensitive).");

            InText(
                "PropertyType",
                "Optional. Lists only the properties of this type: StaticBuiltIn, DynamicBuiltIn or Custom.");

            InText(
                "ValueType",
                "Optional. Lists only the properties of this value type: Integer, Real, String, Boolean or Guid.");

            InText(
                "CollectionType",
                "Optional. Lists only the properties of this collection type: Single, List, " +
                "SingleChoiceEnumeration or MultipleChoiceEnumeration.");

            InBoolean(
                "IncludeEnumValues",
                "If true, EnumValueGuids and EnumValueTexts list the options of each enumeration property. " +
                "Default is false.");

            InBoolean(
                "IncludeAvailability",
                "If true, AvailabilityGuids lists the classification items each custom property is available " +
                "for. Default is false.");

            // No default value: an unconnected input must not limit.
            inManager.AddIntegerParameter(
                "Limit",
                "Limit",
                "Optional. The most properties to list (at least 1).".WithTypeName("integerItem"),
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
                    6,
                    7
                });
        }

        protected override void AddOutputs()
        {
            OutGenerics("PropertyIds");
            OutTexts("PropertyGroupNames");
            OutTexts("PropertyNames");

            OutTexts(
                "FullNames",
                "Full names containing the joined group and property name.");

            OutGenerics(
                "PropertyGroupIds",
                "Identifier of the group of each property.");

            OutTexts(
                "PropertyTypes",
                "Type of each property: StaticBuiltIn, DynamicBuiltIn or Custom.");

            OutTexts(
                "ValueTypes",
                "Value type of each property: Undefined, Integer, Real, String, Boolean or Guid.");

            OutTexts(
                "CollectionTypes",
                "Collection type of each property: Undefined, Single, List, SingleChoiceEnumeration or " +
                "MultipleChoiceEnumeration.");

            OutTexts(
                "MeasureTypes",
                "Measure type of each property: Undefined, Default, Length, Area, Volume or Angle.");

            OutTexts(
                "Descriptions",
                "Description of each property.");

            OutGenericTree(
                "AvailabilityGuids",
                "Classification items each custom property is available for (one branch per property; empty " +
                "unless IncludeAvailability is true).");

            OutGenericTree(
                "EnumValueGuids",
                "Identifiers of the options of each enumeration property (one branch per property; empty " +
                "unless IncludeEnumValues is true).");

            OutTextTree(
                "EnumValueTexts",
                "Display texts of the options of each enumeration property (one branch per property; empty " +
                "unless IncludeEnumValues is true).");
        }

        private static void AddText(
            IGH_DataAccess da,
            int index,
            JObject parameters,
            string key)
        {
            if (da.TryGet(
                    index,
                    out string value) &&
                !string.IsNullOrWhiteSpace(value))
            {
                parameters[key] = value.Trim();
            }
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            AddText(da, 0, parameters, "query");
            AddText(da, 1, parameters, "propertyGroup");
            AddText(da, 2, parameters, "propertyType");
            AddText(da, 3, parameters, "valueType");
            AddText(da, 4, parameters, "collectionType");

            if (da.TryGet(
                    5,
                    out bool includeEnumValues) &&
                includeEnumValues)
            {
                parameters["includeEnumValues"] = true;
            }

            if (da.TryGet(
                    6,
                    out bool includeAvailability) &&
                includeAvailability)
            {
                parameters["includeAvailability"] = true;
            }

            if (da.TryGet(
                    7,
                    out int limit))
            {
                if (limit < 1)
                {
                    this.AddError("Limit must be at least 1.");
                    return;
                }

                parameters["limit"] = limit;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    parameters,
                    ToAddOn,
                    JHelp.Deserialize<AllProperties>,
                    out AllProperties response))
            {
                return;
            }

            var properties = response.Properties ?? new List<PropertyDetailsObj>();

            da.SetDataList(
                0,
                properties.Select(x => x.PropertyId));

            da.SetDataList(
                1,
                properties.Select(x => x.PropertyGroupName));

            da.SetDataList(
                2,
                properties.Select(x => x.PropertyName));

            da.SetDataList(
                3,
                properties.Select(x => StringHelp.Join(
                    x.PropertyGroupName,
                    x.PropertyName)));

            da.SetDataList(
                4,
                properties.Select(x => x.PropertyGroupId));

            da.SetDataList(
                5,
                properties.Select(x => x.PropertyType));

            da.SetDataList(
                6,
                properties.Select(x => x.PropertyValueType));

            da.SetDataList(
                7,
                properties.Select(x => x.PropertyCollectionType));

            da.SetDataList(
                8,
                properties.Select(x => x.PropertyMeasureType));

            da.SetDataList(
                9,
                properties.Select(x => x.PropertyDescription));

            var availabilityTree = new GH_Structure<GH_ObjectWrapper>();
            var enumValueGuidTree = new GH_Structure<GH_ObjectWrapper>();
            var enumValueTextTree = new GH_Structure<GH_String>();
            for (var i = 0; i < properties.Count; i++)
            {
                var property = properties[i];
                var path = new GH_Path(i);
                availabilityTree.EnsurePath(path);
                enumValueGuidTree.EnsurePath(path);
                enumValueTextTree.EnsurePath(path);

                foreach (var item in property.Availability ?? new List<ClassificationItemGuidWrapper>())
                {
                    availabilityTree.Append(
                        new GH_ObjectWrapper(item.ClassificationItemId),
                        path);
                }

                foreach (var item in property.PossibleEnumValues ?? new List<PossibleEnumValueItem>())
                {
                    enumValueGuidTree.Append(
                        new GH_ObjectWrapper(EnumValueGuidObject.FromString(item.EnumValue?.Guid)),
                        path);
                    enumValueTextTree.Append(
                        new GH_String(item.EnumValue?.DisplayValue),
                        path);
                }
            }

            da.SetDataTree(
                10,
                availabilityTree);

            da.SetDataTree(
                11,
                enumValueGuidTree);

            da.SetDataTree(
                12,
                enumValueTextTree);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.PropertyByName;

        public override Guid ComponentGuid =>
            new Guid("b8e4a2d6-31c7-4f05-9a8e-6d2c74f1e93b");
    }
}
