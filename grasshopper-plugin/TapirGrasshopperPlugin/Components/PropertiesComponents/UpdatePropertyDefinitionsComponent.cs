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
    public class UpdatePropertyDefinitionsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "UpdatePropertyDefinitions";

        public UpdatePropertyDefinitionsComponent()
            : base(
                "UpdatePropertyDefinitions",
                "Update existing Custom Property Definitions in place: the expressions, name, description, " +
                "group, default value, availability and enum options. Only the provided fields are changed; " +
                "the definitions keep their identifiers, so element values survive. " +
                "The tree inputs take one branch per property (or 1 branch to use the same for all).",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "PropertyGuids",
                "Identifiers of the property definitions to update.");

            inManager.AddTextParameter(
                "Expressions",
                "Expressions",
                "The new expression strings for each expression-based property (one branch per property). Optional.",
                GH_ParamAccess.tree);

            InTexts(
                "Names",
                "New name of each property (input only 1 to use the same value for all). Optional.");

            InTexts(
                "Descriptions",
                "New description of each property (input only 1 to use the same value for all). Optional.");

            InGenerics(
                "GroupGuids",
                "Identifier of the custom property group to move each property into (input only 1 to use the same group for all). Optional.");

            InTexts(
                "DefaultValues",
                "One JSON object per property with the new default value, e.g. " +
                "{\"basicDefaultValue\":{\"type\":\"string\",\"status\":\"normal\",\"value\":\"...\"}} or " +
                "{\"expressions\":[\"...\"]}. Input only 1 to use the same value for all. Optional.");

            InGenericTree(
                "AvailabilitySet",
                "Classification item identifiers that replace the availability list of each property. Optional.");

            InGenericTree(
                "AvailabilityAdd",
                "Classification item identifiers to add to the availability of each property. Optional.");

            InGenericTree(
                "AvailabilityRemove",
                "Classification item identifiers to remove from the availability of each property. Optional.");

            inManager.AddTextParameter(
                "EnumValuesToAdd",
                "EnumValuesToAdd",
                "Display texts of the enum options to add to each enumeration property. Existing options are kept. Optional.",
                GH_ParamAccess.tree);

            InGenericTree(
                "RenameEnumValueGuids",
                "Identifiers of the enum options to rename (see PossibleEnumValueGuids of AllProperties). Optional.");

            inManager.AddTextParameter(
                "RenameEnumValueTexts",
                "RenameEnumValueTexts",
                "New display texts of the enum options in RenameEnumValueGuids (same tree structure). Optional.",
                GH_ParamAccess.tree);

            InGenericTree(
                "RemoveEnumValueGuids",
                "Identifiers of the enum options to remove. Elements holding a removed option lose that value. Optional.");

            inManager.AddTextParameter(
                "EnumOrder",
                "EnumOrder",
                "Every option's display text, once, in the new order (applied after rename, remove and add). Optional.",
                GH_ParamAccess.tree);

            SetOptionality(new[] { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each property definition (empty when it succeeded).");
        }

        private static T GetAt<T>(
            List<T> values,
            int index)
            where T : class
        {
            if (values.Count == 0)
            {
                return null;
            }

            return values[values.Count == 1 ? 0 : index];
        }

        // The branch of the tree that belongs to the given property: the only
        // branch when the tree has one, the index-th otherwise. Null when the
        // tree is empty.
        private static List<T> GetBranchAt<T>(
            GH_Structure<T> tree,
            int index)
            where T : IGH_Goo
        {
            var branchCount = tree?.Branches.Count ?? 0;
            if (branchCount == 0)
            {
                return null;
            }

            return tree.Branches[branchCount == 1 ? 0 : index];
        }

        private static List<string> GetTextsAt(
            GH_Structure<GH_String> tree,
            int index)
        {
            var branch = GetBranchAt(tree, index);
            if (branch == null)
            {
                return null;
            }

            var texts = branch
                .Select(x => x?.Value)
                .Where(x => x != null)
                .ToList();
            return texts.Count == 0 ? null : texts;
        }

        private static bool TryGetGuidsAt<T>(
            GH_Structure<IGH_Goo> tree,
            int index,
            out List<T> guids)
            where T : GuidObject<T>, new()
        {
            guids = null;
            var branch = GetBranchAt(tree, index);
            if (branch == null)
            {
                return true;
            }

            var result = new List<T>();
            foreach (var goo in branch)
            {
                if (goo == null)
                {
                    continue;
                }

                var wrapper = goo as GH_ObjectWrapper ?? new GH_ObjectWrapper(goo);
                var id = GuidObject<T>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    return false;
                }
                result.Add(id);
            }

            guids = result.Count == 0 ? null : result;
            return true;
        }

        private static List<ClassificationItemGuidWrapper> ToClassificationItemIds(
            List<ClassificationGuid> guids)
        {
            return guids?
                .Select(x => new ClassificationItemGuidWrapper { ClassificationItemId = x })
                .ToList();
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> propertyWrappers))
            {
                return;
            }

            var propertyCount = propertyWrappers.Count;

            da.TryGetTree(1, out GH_Structure<GH_String> expressionsTree);
            da.TryGetList(2, out List<string> names);
            names = names ?? new List<string>();
            da.TryGetList(3, out List<string> descriptions);
            descriptions = descriptions ?? new List<string>();
            da.TryGetList(4, out List<GH_ObjectWrapper> groupWrappers);
            groupWrappers = groupWrappers ?? new List<GH_ObjectWrapper>();
            da.TryGetList(5, out List<string> defaultValues);
            defaultValues = defaultValues ?? new List<string>();
            da.TryGetTree(6, out GH_Structure<IGH_Goo> availabilitySetTree);
            da.TryGetTree(7, out GH_Structure<IGH_Goo> availabilityAddTree);
            da.TryGetTree(8, out GH_Structure<IGH_Goo> availabilityRemoveTree);
            da.TryGetTree(9, out GH_Structure<GH_String> enumValuesToAddTree);
            da.TryGetTree(10, out GH_Structure<IGH_Goo> renameEnumGuidsTree);
            da.TryGetTree(11, out GH_Structure<GH_String> renameEnumTextsTree);
            da.TryGetTree(12, out GH_Structure<IGH_Goo> removeEnumGuidsTree);
            da.TryGetTree(13, out GH_Structure<GH_String> enumOrderTree);

            foreach (var pair in new (string Name, int Count)[]
                     {
                         ("Expressions", expressionsTree?.Branches.Count ?? 0),
                         ("Names", names.Count),
                         ("Descriptions", descriptions.Count),
                         ("GroupGuids", groupWrappers.Count),
                         ("DefaultValues", defaultValues.Count),
                         ("AvailabilitySet", availabilitySetTree?.Branches.Count ?? 0),
                         ("AvailabilityAdd", availabilityAddTree?.Branches.Count ?? 0),
                         ("AvailabilityRemove", availabilityRemoveTree?.Branches.Count ?? 0),
                         ("EnumValuesToAdd", enumValuesToAddTree?.Branches.Count ?? 0),
                         ("RenameEnumValueGuids", renameEnumGuidsTree?.Branches.Count ?? 0),
                         ("RenameEnumValueTexts", renameEnumTextsTree?.Branches.Count ?? 0),
                         ("RemoveEnumValueGuids", removeEnumGuidsTree?.Branches.Count ?? 0),
                         ("EnumOrder", enumOrderTree?.Branches.Count ?? 0)
                     })
            {
                if (pair.Count > 1 &&
                    pair.Count != propertyCount)
                {
                    this.AddError(
                        $"The size (or branch count) of the input {pair.Name} must be 0, 1 or equal to the size of the input PropertyGuids.");
                    return;
                }
            }

            var input = new UpdatePropertyDefinitionsParameters
            {
                PropertyDefinitions = new List<PropertyDefinitionUpdate>()
            };

            for (var i = 0; i < propertyCount; i++)
            {
                var id = GuidObject<PropertyGuidObject>.CreateFromWrapper(propertyWrappers[i]);
                if (id == null)
                {
                    this.AddError(
                        "Invalid property identifier in the PropertyGuids input.");
                    return;
                }

                var update = new PropertyDefinitionUpdate
                {
                    PropertyId = id,
                    Expressions = GetTextsAt(expressionsTree, i),
                    Name = GetAt(names, i),
                    Description = GetAt(descriptions, i)
                };

                var groupWrapper = GetAt(groupWrappers, i);
                if (groupWrapper != null)
                {
                    update.GroupId = GuidObject<PropertyGroupGuidObject>.CreateFromWrapper(groupWrapper);
                    if (update.GroupId == null)
                    {
                        this.AddError(
                            "Invalid property group identifier in the GroupGuids input.");
                        return;
                    }
                }

                var defaultValue = GetAt(defaultValues, i);
                if (!string.IsNullOrEmpty(defaultValue))
                {
                    try
                    {
                        update.DefaultValue = JObject.Parse(defaultValue);
                    }
                    catch (Exception ex)
                    {
                        this.AddError(
                            $"Invalid JSON in the DefaultValues input: {ex.Message}");
                        return;
                    }
                }

                if (!TryGetGuidsAt(availabilitySetTree, i, out List<ClassificationGuid> availabilitySet) ||
                    !TryGetGuidsAt(availabilityAddTree, i, out List<ClassificationGuid> availabilityAdd) ||
                    !TryGetGuidsAt(availabilityRemoveTree, i, out List<ClassificationGuid> availabilityRemove))
                {
                    this.AddError(
                        "Invalid classification item identifier in an Availability input.");
                    return;
                }

                if (availabilitySet != null ||
                    availabilityAdd != null ||
                    availabilityRemove != null)
                {
                    update.Availability = new PropertyAvailabilityUpdate
                    {
                        Set = ToClassificationItemIds(availabilitySet),
                        Add = ToClassificationItemIds(availabilityAdd),
                        Remove = ToClassificationItemIds(availabilityRemove)
                    };
                }

                update.PossibleEnumValues = GetTextsAt(enumValuesToAddTree, i)?
                    .Select(x => new EnumValueToAddArrayItem
                    {
                        EnumValue = new EnumValueToAdd { DisplayValue = x }
                    })
                    .ToList();

                if (!TryGetGuidsAt(renameEnumGuidsTree, i, out List<EnumValueGuidObject> renameGuids) ||
                    !TryGetGuidsAt(removeEnumGuidsTree, i, out List<EnumValueGuidObject> removeGuids))
                {
                    this.AddError(
                        "Invalid enum value identifier in the RenameEnumValueGuids or RemoveEnumValueGuids input.");
                    return;
                }

                var renameTexts = GetTextsAt(renameEnumTextsTree, i);
                if ((renameGuids?.Count ?? 0) != (renameTexts?.Count ?? 0))
                {
                    this.AddError(
                        "The inputs RenameEnumValueGuids and RenameEnumValueTexts must have the same structure.");
                    return;
                }

                if (renameGuids != null)
                {
                    update.RenameEnumValues = renameGuids
                        .Select((x, j) => new EnumValueRename
                        {
                            EnumValueId = x,
                            DisplayValue = renameTexts[j]
                        })
                        .ToList();
                }

                update.RemoveEnumValues = removeGuids?
                    .Select(x => new EnumValueGuidWrapper { EnumValueId = x })
                    .ToList();

                update.EnumOrder = GetTextsAt(enumOrderTree, i);

                input.PropertyDefinitions.Add(update);
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.UpdatePropertyDefinitions;

        public override Guid ComponentGuid =>
            new Guid("1eaaba4e-9162-4784-8e34-893e05498b43");
    }
}
