using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.FavoritesComponents
{
    public class GetFavoriteDefaultsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetFavoriteDefaults";

        public GetFavoriteDefaultsComponent()
            : base(
                "GetFavoriteDefaults",
                "Read the element default stored in the given favorites through the Favorite manager, without " +
                "applying it or placing an element: the element type, the layer, the element ID, the folder, the " +
                "library part and GDL parameters, the marker of every element type that has one, and the " +
                "sub-elements with each of their kinds (Curtain Wall frames and panels, Stair treads and risers, " +
                "Railing posts and rails, markers ...). Available from Archicad 29. Tree outputs have one branch " +
                "per favorite.",
                GroupNames.Favorites)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "FavoriteNames",
                "Names of the favorites.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "ElementTypes",
                "Element type of each favorite.");

            OutIntegers(
                "LayerIndices",
                "Index of the layer each favorite places its elements on.");

            OutTexts(
                "ElementIDs",
                "Element ID stored with each favorite (null when it is empty).");

            OutTextTree(
                "Folders",
                "Folder hierarchy of each favorite in the Favorites palette, outermost folder first " +
                "(empty for the root).");

            OutTexts(
                "LibPartNames",
                "Name of the library part of each favorite, for the library part based types " +
                "(Object, Lamp, Window, Door, Skylight, Zone); null otherwise.");

            OutTexts(
                "Markers",
                "The marker of each favorite as JSON text: its elementType, libPart and parameters " +
                "(null when the favorite has no marker).");

            OutTextTree(
                "SubElementTypes",
                "Element type of each sub-element of the favorite.");

            OutTextTree(
                "SubElementKinds",
                "Kinds of each sub-element of the favorite (for example CWFrameClass or StairTread), " +
                "comma separated.");

            OutTextTree(
                "SubElements",
                "The sub-elements of each favorite as JSON text, one item per sub-element: its elementType, " +
                "subTypes, libPart and parameters.");

            OutTexts(
                "Defaults",
                "The whole element default of each favorite as JSON text, with every field of the GDL " +
                "parameters and the library parts.");

            OutErrorMessages(
                "Error message for each favorite (empty when it could be read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> favoriteNames))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["favorites"] = JArray.FromObject(favoriteNames) },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var elementTypes = new List<object>();
            var layerIndices = new List<object>();
            var elementIds = new List<object>();
            var folders = new DataTree<object>();
            var libPartNames = new List<object>();
            var markers = new List<object>();
            var subElementTypes = new DataTree<object>();
            var subElementKinds = new DataTree<object>();
            var subElements = new DataTree<object>();
            var defaults = new List<object>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "favoriteDefaults");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                folders.EnsurePath(path);
                subElementTypes.EnsurePath(path);
                subElementKinds.EnsurePath(path);
                subElements.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    elementTypes.Add(null);
                    layerIndices.Add(null);
                    elementIds.Add(null);
                    libPartNames.Add(null);
                    markers.Add(null);
                    defaults.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                elementTypes.Add(JsonOutputHelp.Scalar(item, "elementType"));
                layerIndices.Add(JsonOutputHelp.Scalar(item, "layerIndex"));
                elementIds.Add(JsonOutputHelp.Scalar(item, "elementId"));
                libPartNames.Add(JsonOutputHelp.Scalar(item["libPart"], "name"));
                markers.Add(item["marker"]?.ToString(Formatting.None));
                defaults.Add(item.ToString(Formatting.None));
                errors.Add("");

                if (item["folder"] is JArray folder)
                {
                    foreach (var name in folder)
                    {
                        folders.Add(name.ToString(), path);
                    }
                }

                if (item["subElements"] is JArray subElementItems)
                {
                    foreach (var subElement in subElementItems)
                    {
                        subElementTypes.Add(
                            JsonOutputHelp.Scalar(subElement, "elementType"),
                            path);
                        var kinds = new List<string>();
                        if (subElement["subTypes"] is JArray subTypes)
                        {
                            foreach (var subType in subTypes)
                            {
                                kinds.Add(subType.ToString());
                            }
                        }
                        subElementKinds.Add(string.Join(", ", kinds), path);
                        subElements.Add(
                            subElement.ToString(Formatting.None),
                            path);
                    }
                }
            }

            da.SetDataList(0, elementTypes);
            da.SetDataList(1, layerIndices);
            da.SetDataList(2, elementIds);
            da.SetDataTree(3, folders);
            da.SetDataList(4, libPartNames);
            da.SetDataList(5, markers);
            da.SetDataTree(6, subElementTypes);
            da.SetDataTree(7, subElementKinds);
            da.SetDataTree(8, subElements);
            da.SetDataList(9, defaults);
            da.SetDataList(10, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetFavoriteDefaults;

        public override Guid ComponentGuid =>
            new Guid("d5032425-f1bd-4b40-9131-b6deaa9066b7");
    }
}
