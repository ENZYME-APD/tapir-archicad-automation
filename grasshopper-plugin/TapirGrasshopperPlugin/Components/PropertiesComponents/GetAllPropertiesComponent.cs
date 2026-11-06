using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Properties;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class GetAllPropertiesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAllProperties";

        public GetAllPropertiesComponent()
            : base(
                "AllProperties",
                "Get all properties. Optionally only the given properties or property types, " +
                "and without the availability lists.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "PropertyGuids",
                "Returns only the definitions of these properties; identifiers not found in the project " +
                "are left out. All properties are returned when nothing is connected.");

            InTexts(
                "PropertyTypes",
                "Returns only the properties of these types: StaticBuiltIn, DynamicBuiltIn or Custom. " +
                "All types are returned when nothing is connected.");

            InBoolean(
                "IncludeAvailability",
                "Adds the availability list (the classification items a custom property is available for) " +
                "to each custom property. Default is true.",
                true);

            SetOptionality(
                new[]
                {
                    0,
                    1,
                    2
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
                "Descriptions",
                "Description of each property.");

            OutTexts(
                "DefaultValues",
                "The basic default value of each property as text (null for built-in, " +
                "expression-based properties and properties without a default).");

            OutGenericTree(
                "AvailabilityGuids",
                "Classification items each custom property is available for (one branch per property).");

            OutGenericTree(
                "EnumValueGuids",
                "Identifiers of the enum options of each enumeration property (one branch per property).");

            OutTextTree(
                "EnumValueTexts",
                "Display texts of the enum options of each enumeration property (one branch per property).");

            OutGenericTree(
                "DefaultEnumValueGuids",
                "Identifiers of the enum options the default of each custom enumeration property holds (one branch per property).");

            OutGenerics(
                "GroupIds",
                "Identifier of every property group, including empty ones.");

            OutTexts(
                "GroupNames",
                "Name of every property group, including empty ones.");

            OutTexts(
                "GroupDescriptions",
                "Description of every property group, including empty ones.");

            OutBooleans(
                "GroupIsCustom",
                "True for every custom property group, false for the built-in ones.");

            OutBooleans(
                "IsExpressionBased",
                "True for each property whose value is computed from an expression.");

            OutTextTree(
                "Expressions",
                "Expression strings of each expression-based property (one branch per property).");

            OutTexts(
                "PropertyTypes",
                "Type of each property: StaticBuiltIn, DynamicBuiltIn or Custom.");

            OutTexts(
                "CollectionTypes",
                "Collection type of each property: Undefined, Single, List, SingleChoiceEnumeration or " +
                "MultipleChoiceEnumeration.");

            OutTexts(
                "ValueTypes",
                "Value type of each property: Undefined, Integer, Real, String, Boolean or Guid.");

            OutTexts(
                "MeasureTypes",
                "Measure type of each property: Undefined, Default, Length, Area, Volume or Angle.");

            OutBooleans(
                "IsEditable",
                "True for each property whose value can be edited.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            if (da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> propertyWrappers) &&
                propertyWrappers.Count > 0)
            {
                var properties = new JArray();
                foreach (var wrapper in propertyWrappers)
                {
                    var id = GuidObject<PropertyGuidObject>.CreateFromWrapper(wrapper);
                    if (id == null)
                    {
                        this.AddError("Invalid property identifier in the PropertyGuids input.");
                        return;
                    }
                    properties.Add(
                        new JObject
                        {
                            ["propertyId"] = new JObject { ["guid"] = id.Guid }
                        });
                }
                parameters["properties"] = properties;
            }

            if (da.TryGetList(
                    1,
                    out List<string> propertyTypes) &&
                propertyTypes.Count > 0)
            {
                parameters["propertyTypes"] = JArray.FromObject(propertyTypes);
            }

            if (da.TryGet(
                    2,
                    out bool includeAvailability))
            {
                parameters["includeAvailability"] = includeAvailability;
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

            da.SetDataList(
                0,
                response.Properties.Select(x => x.PropertyId));

            da.SetDataList(
                1,
                response.Properties.Select(x => x.PropertyGroupName));

            da.SetDataList(
                2,
                response.Properties.Select(x => x.PropertyName));

            da.SetDataList(
                3,
                response.Properties.Select(x => StringHelp.Join(
                    x.PropertyGroupName,
                    x.PropertyName)));

            da.SetDataList(
                4,
                response.Properties.Select(x => x.PropertyGroupId));

            da.SetDataList(
                5,
                response.Properties.Select(x => x.PropertyDescription));

            da.SetDataList(
                6,
                response.Properties.Select(x => x.DefaultValueDisplay));

            var availabilityTree = new GH_Structure<GH_ObjectWrapper>();
            var enumValueGuidTree = new GH_Structure<GH_ObjectWrapper>();
            var enumValueTextTree = new GH_Structure<GH_String>();
            var defaultEnumValueTree = new GH_Structure<GH_ObjectWrapper>();
            for (var i = 0; i < response.Properties.Count; i++)
            {
                var property = response.Properties[i];
                var path = new GH_Path(i);
                availabilityTree.EnsurePath(path);
                enumValueGuidTree.EnsurePath(path);
                enumValueTextTree.EnsurePath(path);
                defaultEnumValueTree.EnsurePath(path);

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

                foreach (var item in property.DefaultEnumValueIds ?? new List<EnumValueGuidObject>())
                {
                    defaultEnumValueTree.Append(
                        new GH_ObjectWrapper(item),
                        path);
                }
            }

            da.SetDataTree(
                7,
                availabilityTree);

            da.SetDataTree(
                8,
                enumValueGuidTree);

            da.SetDataTree(
                9,
                enumValueTextTree);

            da.SetDataTree(
                10,
                defaultEnumValueTree);

            var groups = response.PropertyGroups ?? new List<PropertyGroupDetails>();

            da.SetDataList(
                11,
                groups.Select(x => x.PropertyGroupId));

            da.SetDataList(
                12,
                groups.Select(x => x.Name));

            da.SetDataList(
                13,
                groups.Select(x => x.Description));

            da.SetDataList(
                14,
                groups.Select(x => x.IsCustom));

            da.SetDataList(
                15,
                response.Properties.Select(x => x.IsExpressionBased));

            var expressionTree = new GH_Structure<GH_String>();
            for (var i = 0; i < response.Properties.Count; i++)
            {
                var path = new GH_Path(i);
                expressionTree.EnsurePath(path);
                foreach (var expression in response.Properties[i].Expressions ?? new List<string>())
                {
                    expressionTree.Append(
                        new GH_String(expression),
                        path);
                }
            }

            da.SetDataTree(
                16,
                expressionTree);

            da.SetDataList(
                17,
                response.Properties.Select(x => x.PropertyType));

            da.SetDataList(
                18,
                response.Properties.Select(x => x.PropertyCollectionType));

            da.SetDataList(
                19,
                response.Properties.Select(x => x.PropertyValueType));

            da.SetDataList(
                20,
                response.Properties.Select(x => x.PropertyMeasureType));

            da.SetDataList(
                21,
                response.Properties.Select(x => x.PropertyIsEditable));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AllProperties;

        public override Guid ComponentGuid =>
            new Guid("79f924e4-5b26-4efe-bfaf-0e82f9bb3821");
    }
}