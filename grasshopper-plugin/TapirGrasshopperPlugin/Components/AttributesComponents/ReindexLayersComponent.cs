using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class ReindexLayersComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ReindexLayers";

        public ReindexLayersComponent()
            : base(
                "ReindexLayers",
                "Moves layers to new indices. Archicad cannot renumber an attribute, so each layer is recreated " +
                "at its new index with a new attribute id, gets the old layer's state in every layer combination, " +
                "and every element of the old layer (floor plan, views and layouts) and the tool defaults on it are " +
                "moved to it; then the old layer is deleted. A layer with an element that cannot be moved (locked, " +
                "not reserved in Teamwork, from a hotlinked module) is left as it was. Favorites that use the layer " +
                "are not updated. Sections, elevations, interior elevations and 3D documents show the layer's model " +
                "elements again when they are rebuilt (ViewsToRebuild). DryRun lists what would change. The whole " +
                "call is one undo step. Available from Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "LayerGuids",
                "Identifiers of the layers to move, handled in the given order.");

            InIntegers(
                "NewIndices",
                "The new index of each layer. It must not be used by another layer; index 1 (the Archicad Layer) " +
                "cannot be used or moved.");

            InBoolean(
                "DryRun",
                "Only list what would change, without changing anything. Default is false.");

            InInteger(
                "MaxPlaceholders",
                "Every free index below the new index is filled with a temporary layer first (about 20 ms each); " +
                "a layer that needs more than this many is not moved. Default is 1000.",
                1000);

            SetOptionality(new[] { 2, 3 });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "LayerGuids",
                "Each layer at its new index (a new attribute id; with DryRun the current id; null when failed).");

            OutIntegers(
                "OldIndices",
                "Old index of each layer.");

            OutIntegers(
                "NewIndices",
                "New index of each layer.");

            OutGenericTree(
                "Elements",
                "Elements on each layer, moved to the new layer unless DryRun (one branch per layer).");

            OutGenericTree(
                "BlockedElements",
                "Only with DryRun: elements on each layer that cannot be moved because they come from a hotlinked " +
                "module (one branch per layer).");

            OutTextTree(
                "ToolDefaults",
                "Element types whose tool default is on each layer (one branch per layer).");

            OutIntegers(
                "Placeholders",
                "Number of temporary layers needed for the free indices below the new index of each layer.");

            OutTextTree(
                "Warnings",
                "What did not work after each layer was moved (one branch per layer).");

            OutGenericTree(
                "ViewsToRebuild",
                "Databases (sections, elevations, interior elevations, 3D documents) that show elements of each " +
                "layer and have to be rebuilt to show them on the new layer (one branch per layer).");

            OutErrorMessages(
                "Error message of each layer (empty when it was moved).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> layerWrappers))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<int> newIndices))
            {
                return;
            }

            var layers = ReindexOutputHelp.MovesOf(
                this,
                layerWrappers,
                newIndices,
                "LayerGuids");
            if (layers == null)
            {
                return;
            }

            var parameters = new JObject
            {
                ["layers"] = layers,
                ["dryRun"] = da.GetOptional(
                    2,
                    false),
                ["maxPlaceholders"] = da.GetOptional(
                    3,
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

            var layerGuids = new List<object>();
            var oldIndices = new List<object>();
            var resultIndices = new List<object>();
            var elements = new DataTree<object>();
            var blockedElements = new DataTree<object>();
            var toolDefaults = new DataTree<string>();
            var placeholders = new List<object>();
            var warnings = new DataTree<string>();
            var viewsToRebuild = new DataTree<object>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "layers");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                elements.EnsurePath(path);
                blockedElements.EnsurePath(path);
                toolDefaults.EnsurePath(path);
                warnings.EnsurePath(path);
                viewsToRebuild.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    layerGuids.Add(null);
                    oldIndices.Add(null);
                    resultIndices.Add(null);
                    placeholders.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                layerGuids.Add(ReindexOutputHelp.AttributeGuidOf(item));
                oldIndices.Add(JsonOutputHelp.Scalar(item, "oldIndex"));
                resultIndices.Add(JsonOutputHelp.Scalar(item, "newIndex"));
                placeholders.Add(JsonOutputHelp.Scalar(item, "placeholders"));
                ReindexOutputHelp.AddElements(elements, item["elements"], path);
                ReindexOutputHelp.AddElements(blockedElements, item["blockedElements"], path);
                ReindexOutputHelp.AddTexts(toolDefaults, item["toolDefaults"], path);
                ReindexOutputHelp.AddTexts(warnings, item["warnings"], path);

                if (item["viewsToRebuild"] is JArray views)
                {
                    foreach (var view in views)
                    {
                        var guid = JsonOutputHelp.WrappedGuidOf(
                            view,
                            "databaseId");
                        viewsToRebuild.Add(
                            guid == null
                                ? null
                                : new DatabaseGuidWrapper
                                {
                                    DatabaseId = new DatabaseGuidObject { Guid = guid }
                                },
                            path);
                    }
                }

                errors.Add("");
            }

            da.SetDataList(0, layerGuids);
            da.SetDataList(1, oldIndices);
            da.SetDataList(2, resultIndices);
            da.SetDataTree(3, elements);
            da.SetDataTree(4, blockedElements);
            da.SetDataTree(5, toolDefaults);
            da.SetDataList(6, placeholders);
            da.SetDataTree(7, warnings);
            da.SetDataTree(8, viewsToRebuild);
            da.SetDataList(9, errors);
        }

        // Shares the icon of SetLayers.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetLayers;

        public override Guid ComponentGuid =>
            new Guid("35f16d3f-beff-408e-aafc-ca6a3ad5a8d4");
    }
}
