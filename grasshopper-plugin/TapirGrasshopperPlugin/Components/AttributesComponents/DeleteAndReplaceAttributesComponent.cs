using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class DeleteAndReplaceAttributesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteAndReplaceAttributes";

        public DeleteAndReplaceAttributesComponent()
            : base(
                "DeleteAndReplaceAttributes",
                "Deletes fills, line types, surfaces or building materials and replaces each by another attribute " +
                "of the same type wherever it was used, like Delete and Replace in the Attribute Manager. Every " +
                "reference is rewritten to the replacement (elements in every view and layout, library part " +
                "parameters, memo data, tool defaults, Favorites, other attributes, graphic override rules). If an " +
                "element cannot be changed (locked, not reserved in Teamwork), everything is put back and that " +
                "attribute is not deleted. Elements from hotlinked modules and indices typed into GDL scripts are " +
                "not changed. DryRun lists every reference without changing anything. The whole call is one undo " +
                "step. Available from Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "AttributeType",
                "The type of the attributes to delete and of their replacements: Fill, Line, Surface or " +
                "BuildingMaterial.");

            InGenerics(
                "AttributeGuids",
                "Identifiers of the attributes to delete, handled in the given order.");

            InGenerics(
                "ReplacementAttributeGuids",
                "The attribute of the same type that replaces each deleted attribute wherever it is used. It must " +
                "be another attribute than the one to delete. Input only 1 to use the same replacement for all.");

            InBoolean(
                "DryRun",
                "Only list every reference to each attribute to delete, without changing anything. Default is " +
                "false.");

            SetOptionality(new[] { 3 });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "AttributeGuids",
                "Each deleted attribute (with DryRun, the attribute that would be deleted; null when failed).");

            OutGenerics(
                "ReplacementAttributeGuids",
                "The attribute that took the place of each deleted attribute.");

            OutGenericTree(
                "Elements",
                "Elements that referred to each deleted attribute and refer to its replacement now, unless DryRun " +
                "(one branch per attribute).");

            OutGenericTree(
                "HotlinkElements",
                "Elements from hotlinked modules that refer to each deleted attribute; they cannot be changed (one " +
                "branch per attribute).");

            OutTextTree(
                "ToolDefaults",
                "Element types whose tool default referred to each deleted attribute (one branch per attribute).");

            OutTextTree(
                "Favorites",
                "Names of the Favorites that referred to each deleted attribute (one branch per attribute).");

            OutGenericTree(
                "ReferringAttributes",
                "Other attributes (composites, building materials, surfaces, MEP systems, profiles) that referred " +
                "to each deleted attribute (one branch per attribute).");

            OutTextTree(
                "ReferringAttributeNames",
                "Type and name of each referring attribute, as \"Type: Name\" (in the structure of " +
                "ReferringAttributes).");

            OutTextTree(
                "OverrideRules",
                "Names of the graphic override rules that referred to each deleted attribute (one branch per " +
                "attribute).");

            OutGenericTree(
                "LayersShown",
                "Hidden or locked layers that were shown and unlocked for the change and put back afterwards (one " +
                "branch per attribute; empty with DryRun).");

            OutTextTree(
                "Warnings",
                "What still refers to each deleted attribute or could not be rewritten to the replacement (one " +
                "branch per attribute).");

            OutErrorMessages(
                "Error message of each attribute (empty when it was deleted and replaced).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string attributeType) ||
                string.IsNullOrEmpty(attributeType))
            {
                this.AddError("AttributeType is required.");
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<GH_ObjectWrapper> attributeWrappers) ||
                !da.TryGetList(
                    2,
                    out List<GH_ObjectWrapper> replacementWrappers))
            {
                return;
            }

            if (!this.CheckSizes(
                    attributeWrappers.Count,
                    "AttributeGuids",
                    ("ReplacementAttributeGuids", replacementWrappers.Count)))
            {
                return;
            }

            var attributes = new JArray();
            for (var i = 0; i < attributeWrappers.Count; i++)
            {
                var attributeId = GuidObject<AttributeGuidObject>.CreateFromWrapper(attributeWrappers[i]);
                if (attributeId == null)
                {
                    this.AddError("Invalid attribute identifier in the AttributeGuids input.");
                    return;
                }

                var replacementId = GuidObject<AttributeGuidObject>.CreateFromWrapper(replacementWrappers.At(i));
                if (replacementId == null)
                {
                    this.AddError("Invalid attribute identifier in the ReplacementAttributeGuids input.");
                    return;
                }

                attributes.Add(
                    new JObject
                    {
                        ["attributeId"] = new JObject { ["guid"] = attributeId.Guid },
                        ["replacementAttributeId"] = new JObject { ["guid"] = replacementId.Guid }
                    });
            }

            if (attributes.Count == 0)
            {
                return;
            }

            var parameters = new JObject
            {
                ["attributeType"] = attributeType,
                ["attributes"] = attributes,
                ["dryRun"] = da.GetOptional(
                    3,
                    false)
            };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var attributeGuids = new List<object>();
            var replacementGuids = new List<object>();
            var elements = new DataTree<object>();
            var hotlinkElements = new DataTree<object>();
            var toolDefaults = new DataTree<string>();
            var favorites = new DataTree<string>();
            var referringAttributes = new DataTree<object>();
            var referringAttributeNames = new DataTree<string>();
            var overrideRules = new DataTree<string>();
            var layersShown = new DataTree<object>();
            var warnings = new DataTree<string>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "attributes");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                elements.EnsurePath(path);
                hotlinkElements.EnsurePath(path);
                toolDefaults.EnsurePath(path);
                favorites.EnsurePath(path);
                referringAttributes.EnsurePath(path);
                referringAttributeNames.EnsurePath(path);
                overrideRules.EnsurePath(path);
                layersShown.EnsurePath(path);
                warnings.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    attributeGuids.Add(null);
                    replacementGuids.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                attributeGuids.Add(ReindexOutputHelp.AttributeGuidOf(item));
                replacementGuids.Add(ReplacementGuidOf(item));
                ReindexOutputHelp.AddElements(elements, item["elements"], path);
                ReindexOutputHelp.AddElements(hotlinkElements, item["hotlinkElements"], path);
                ReindexOutputHelp.AddTexts(toolDefaults, item["toolDefaults"], path);
                ReindexOutputHelp.AddTexts(favorites, item["favorites"], path);
                ReindexOutputHelp.AddTexts(overrideRules, item["overrideRules"], path);
                ReindexOutputHelp.AddTexts(warnings, item["warnings"], path);

                if (item["attributes"] is JArray referring)
                {
                    foreach (var attribute in referring)
                    {
                        referringAttributes.Add(
                            ReindexOutputHelp.AttributeGuidOf(attribute),
                            path);
                        referringAttributeNames.Add(
                            attribute["attributeType"]?.ToString() + ": " + attribute["name"]?.ToString(),
                            path);
                    }
                }

                if (item["layersShown"] is JArray layers)
                {
                    foreach (var layer in layers)
                    {
                        layersShown.Add(
                            ReindexOutputHelp.AttributeGuidOf(layer),
                            path);
                    }
                }

                errors.Add("");
            }

            da.SetDataList(0, attributeGuids);
            da.SetDataList(1, replacementGuids);
            da.SetDataTree(2, elements);
            da.SetDataTree(3, hotlinkElements);
            da.SetDataTree(4, toolDefaults);
            da.SetDataTree(5, favorites);
            da.SetDataTree(6, referringAttributes);
            da.SetDataTree(7, referringAttributeNames);
            da.SetDataTree(8, overrideRules);
            da.SetDataTree(9, layersShown);
            da.SetDataTree(10, warnings);
            da.SetDataList(11, errors);
        }

        private static AttributeGuidWrapper ReplacementGuidOf(
            JToken item)
        {
            var guid = JsonOutputHelp.WrappedGuidOf(
                item,
                "replacementAttributeId");
            return guid == null
                ? null
                : new AttributeGuidWrapper
                {
                    AttributeId = new AttributeGuidObject { Guid = guid }
                };
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteAttributes;

        public override Guid ComponentGuid =>
            new Guid("3d14db40-dc08-4ee1-a9cc-2d7c317489f2");
    }
}
