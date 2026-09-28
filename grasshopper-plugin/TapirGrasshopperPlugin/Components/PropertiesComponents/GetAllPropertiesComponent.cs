using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Properties;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class GetAllPropertiesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAllProperties";

        public GetAllPropertiesComponent()
            : base(
                "AllProperties",
                "Get all properties.",
                GroupNames.Properties)
        {
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
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetConvertedCadValues(
                    CommandName,
                    null,
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
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AllProperties;

        public override Guid ComponentGuid =>
            new Guid("79f924e4-5b26-4efe-bfaf-0e82f9bb3821");
    }
}