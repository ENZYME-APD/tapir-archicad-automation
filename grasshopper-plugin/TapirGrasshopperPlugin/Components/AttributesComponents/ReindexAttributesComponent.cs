using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class ReindexAttributesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ReindexAttributes";

        public ReindexAttributesComponent()
            : base(
                "ReindexAttributes",
                "Moves fills, line types, surfaces and building materials to new indices, the way ReindexLayers " +
                "moves layers. Each attribute is recreated at its new index with a new attribute id (its name, " +
                "definition and folder are kept), every reference to the old index is rewritten (elements in every " +
                "view and layout, library part parameters, memo data, tool defaults, Favorites, other attributes, " +
                "graphic override rules), then the old attribute is deleted. If an element cannot be changed " +
                "(locked, not reserved in Teamwork), everything is put back and that attribute is not moved. " +
                "Elements from hotlinked modules and indices typed into GDL scripts are not changed. DryRun lists " +
                "every reference without changing anything. The whole call is one undo step. Available from " +
                "Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "AttributeType",
                "The type of the attributes to move: Fill, Line, Surface or BuildingMaterial.");

            InGenerics(
                "AttributeGuids",
                "Identifiers of the attributes to move, handled in the given order.");

            InIntegers(
                "NewIndices",
                "The new index of each attribute. It must not be used by another attribute of the type.");

            InBoolean(
                "DryRun",
                "Only list every reference to each attribute, without changing anything. Default is false.");

            InInteger(
                "MaxPlaceholders",
                "Every free index below the new index is filled with a temporary attribute first; an attribute " +
                "that needs more than this many is not moved. Default is 1000.",
                1000);

            SetOptionality(new[] { 3, 4 });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "AttributeGuids",
                "Each attribute at its new index (a new attribute id; with DryRun the current id; null when failed).");

            OutIntegers(
                "OldIndices",
                "Old index of each attribute.");

            OutIntegers(
                "NewIndices",
                "New index of each attribute.");

            OutIntegers(
                "Placeholders",
                "Number of temporary attributes needed for the free indices below the new index of each attribute.");

            OutGenericTree(
                "Elements",
                "Elements that refer to each attribute, rewritten unless DryRun (one branch per attribute).");

            OutGenericTree(
                "HotlinkElements",
                "Elements from hotlinked modules that refer to each attribute; they keep the old index (one branch " +
                "per attribute).");

            OutTextTree(
                "ToolDefaults",
                "Element types whose tool default refers to each attribute (one branch per attribute).");

            OutTextTree(
                "Favorites",
                "Names of the Favorites that refer to each attribute (one branch per attribute).");

            OutGenericTree(
                "ReferringAttributes",
                "Other attributes (composites, building materials, surfaces, MEP systems, profiles) that refer to " +
                "each attribute (one branch per attribute).");

            OutTextTree(
                "ReferringAttributeNames",
                "Type and name of each referring attribute, as \"Type: Name\" (in the structure of " +
                "ReferringAttributes).");

            OutTextTree(
                "OverrideRules",
                "Names of the graphic override rules that refer to each attribute (one branch per attribute).");

            OutGenericTree(
                "LayersShown",
                "Hidden or locked layers that were shown and unlocked for the move and put back afterwards (one " +
                "branch per attribute; empty with DryRun).");

            OutTextTree(
                "Warnings",
                "What keeps the old index or did not work after each attribute was moved (one branch per attribute).");

            OutErrorMessages(
                "Error message of each attribute (empty when it was moved).");
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
                    out List<GH_ObjectWrapper> attributeWrappers))
            {
                return;
            }

            if (!da.TryGetList(
                    2,
                    out List<int> newIndices))
            {
                return;
            }

            var attributes = ReindexOutputHelp.MovesOf(
                this,
                attributeWrappers,
                newIndices,
                "AttributeGuids");
            if (attributes == null)
            {
                return;
            }

            var parameters = new JObject
            {
                ["attributeType"] = attributeType,
                ["attributes"] = attributes,
                ["dryRun"] = da.GetOptional(
                    3,
                    false),
                ["maxPlaceholders"] = da.GetOptional(
                    4,
                    1000)
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
            var oldIndices = new List<object>();
            var resultIndices = new List<object>();
            var placeholders = new List<object>();
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
                    oldIndices.Add(null);
                    resultIndices.Add(null);
                    placeholders.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                attributeGuids.Add(ReindexOutputHelp.AttributeGuidOf(item));
                oldIndices.Add(JsonOutputHelp.Scalar(item, "oldIndex"));
                resultIndices.Add(JsonOutputHelp.Scalar(item, "newIndex"));
                placeholders.Add(JsonOutputHelp.Scalar(item, "placeholders"));
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
            da.SetDataList(1, oldIndices);
            da.SetDataList(2, resultIndices);
            da.SetDataList(3, placeholders);
            da.SetDataTree(4, elements);
            da.SetDataTree(5, hotlinkElements);
            da.SetDataTree(6, toolDefaults);
            da.SetDataTree(7, favorites);
            da.SetDataTree(8, referringAttributes);
            da.SetDataTree(9, referringAttributeNames);
            da.SetDataTree(10, overrideRules);
            da.SetDataTree(11, layersShown);
            da.SetDataTree(12, warnings);
            da.SetDataList(13, errors);
        }

        // Shares the icon of MoveAttributesAndFolders.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MoveAttributesAndFolders;

        public override Guid ComponentGuid =>
            new Guid("431f0b14-681d-40a2-8775-035d6c31fc54");
    }
}
