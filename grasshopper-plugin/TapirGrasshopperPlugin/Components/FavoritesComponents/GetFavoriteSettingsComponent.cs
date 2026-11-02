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
    public class GetFavoriteSettingsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetFavoriteSettings";

        public GetFavoriteSettingsComponent()
            : base(
                "GetFavoriteSettings",
                "Read the stored settings of the given favorites without applying them or placing an element: " +
                "the element type, the folder, the library part and GDL parameters, the marker (library part and " +
                "parameters) of Window and Door favorites, and the sub-elements. A favorite without a marker has no " +
                "marker, which is not an error. The other settings of the element (pens, layer, structure, etc.) " +
                "are not returned. Tree outputs have one branch per favorite.",
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

            OutTextTree(
                "Folders",
                "Folder hierarchy of each favorite in the Favorites palette, outermost folder first " +
                "(empty for the root).");

            OutTexts(
                "LibPartNames",
                "Name of the library part of each favorite, for the library part based types " +
                "(Object, Lamp, Window, Door, Skylight, Zone); null otherwise.");

            OutTextTree(
                "ParameterNames",
                "Name of each GDL parameter stored with the favorite.");

            OutTextTree(
                "ParameterValues",
                "Value of each GDL parameter stored with the favorite (arrays as JSON text).");

            OutTexts(
                "Markers",
                "The marker of each Window or Door favorite as JSON text: its libPart and parameters " +
                "(null when the favorite has no marker).");

            OutTextTree(
                "SubElements",
                "The sub-elements of each favorite as JSON text, one item per sub-element: its subType " +
                "(MainMarker or Other), elementType, libPart and parameters.");

            OutTexts(
                "Settings",
                "The whole settings of each favorite as JSON text, with every field of the GDL parameters " +
                "(name, displayName, index, type, dimension1, dimension2, value, valueDescription, " +
                "itemDescriptions and flags) and the library parts (name, parentUnID and ownUnID).");

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
            var folders = new DataTree<object>();
            var libPartNames = new List<object>();
            var parameterNames = new DataTree<object>();
            var parameterValues = new DataTree<object>();
            var markers = new List<object>();
            var subElements = new DataTree<object>();
            var settings = new List<object>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "favoriteSettings");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                folders.EnsurePath(path);
                parameterNames.EnsurePath(path);
                parameterValues.EnsurePath(path);
                subElements.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    elementTypes.Add(null);
                    libPartNames.Add(null);
                    markers.Add(null);
                    settings.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                elementTypes.Add(JsonOutputHelp.Scalar(item, "elementType"));
                libPartNames.Add(JsonOutputHelp.Scalar(item["libPart"], "name"));
                markers.Add(item["marker"]?.ToString(Formatting.None));
                settings.Add(item.ToString(Formatting.None));
                errors.Add("");

                if (item["folder"] is JArray folder)
                {
                    foreach (var name in folder)
                    {
                        folders.Add(name.ToString(), path);
                    }
                }

                if (item["parameters"] is JArray parameters)
                {
                    foreach (var parameter in parameters)
                    {
                        parameterNames.Add(
                            JsonOutputHelp.Scalar(parameter, "name"),
                            path);
                        parameterValues.Add(
                            ValueOf(parameter["value"]),
                            path);
                    }
                }

                if (item["subElements"] is JArray subElementItems)
                {
                    foreach (var subElement in subElementItems)
                    {
                        subElements.Add(
                            subElement.ToString(Formatting.None),
                            path);
                    }
                }
            }

            da.SetDataList(0, elementTypes);
            da.SetDataTree(1, folders);
            da.SetDataList(2, libPartNames);
            da.SetDataTree(3, parameterNames);
            da.SetDataTree(4, parameterValues);
            da.SetDataList(5, markers);
            da.SetDataTree(6, subElements);
            da.SetDataList(7, settings);
            da.SetDataList(8, errors);
        }

        // A scalar parameter value as it is; an array value as JSON text.
        private static object ValueOf(
            JToken value)
        {
            if (value == null || value.Type == JTokenType.Null)
            {
                return null;
            }
            if (value is JValue scalar)
            {
                return scalar.Value;
            }
            return value.ToString(Formatting.None);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetFavoriteSettings;

        public override Guid ComponentGuid =>
            new Guid("3f549718-039d-4259-92fe-6c47e6440b41");
    }
}
