using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Favorites;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.FavoritesComponents
{
    public class ApplyFavoritesToElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ApplyFavoritesToElements";

        public ApplyFavoritesToElementsComponent()
            : base(
                "ApplyFavoritesToElements",
                "Apply the given Favorites to the given elements, or one Favorite (FavoriteName) to the given elements " +
                "and/or to the elements of its type that match a filter.",
                GroupNames.Favorites)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to apply the Favorites to. " +
                "With FavoriteName it is optional: without it the Favorite is applied to every element of its type that matches the filter.");

            InTexts(
                "FavoriteNames",
                "Names of the Favorites to apply (input only 1 to apply the same Favorite to all elements). " +
                "Leave it empty when FavoriteName is given.");

            InBoolean(
                "ApplySettings",
                "Apply the Favorite's settings-type parameters (structure, materials, pens, etc. - never geometry).",
                true);

            InBoolean(
                "ApplyClassifications",
                "Apply the Favorite's classifications.",
                true);

            InBoolean(
                "ApplyCategories",
                "Apply the Favorite's element categories (e.g. IFC categories).",
                true);

            InBoolean(
                "ApplyProperties",
                "Apply the Favorite's property values.",
                true);

            InTexts(
                "KeepFields",
                "Optional fields of the target element to keep, copied back after the Favorite has been applied: " +
                "id, layerIndex, width, height, sillHeight, referenceLineLocation, topOffset, relativeTopStory. " +
                "A field the element's type does not have is ignored.");

            // The inputs below come after the existing ones, so saved
            // definitions keep their wiring.
            InText(
                "FavoriteName",
                "Optional. The name of one Favorite to apply, read only once, instead of FavoriteNames: " +
                "to the elements of ElementGuids, or, without them, to every element of the Favorite's type " +
                "that matches the filter below. Needs ElementGuids or a filter.");

            InTexts(
                "ElementTypes",
                "Optional filter for FavoriteName: apply it only to the elements of one of these types " +
                "(connect an ElementsType value list).");

            InIntegers(
                "FloorIndices",
                "Optional filter for FavoriteName: apply it only to the elements whose home story has one of these indices.");

            InIntegers(
                "LayerIndices",
                "Optional filter for FavoriteName: apply it only to the elements on one of these layers (layer attribute indices).");

            SetOptionality(
                new[]
                {
                    0,
                    1,
                    6,
                    7,
                    8,
                    9,
                    10
                });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when it succeeded). With FavoriteName and ElementGuids, " +
                "one per element of ElementGuids (an element that does not match the filter gets an error); " +
                "with FavoriteName and the filter alone, one per element the Favorite was applied to.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            da.TryGetList(
                0,
                out List<GH_ObjectWrapper> elementWrappers);

            da.TryGetList(
                1,
                out List<string> favoriteNames);

            da.TryGet(2, out bool applySettings);
            da.TryGet(3, out bool applyClassifications);
            da.TryGet(4, out bool applyCategories);
            da.TryGet(5, out bool applyProperties);

            da.TryGetList(6, out List<string> keepFields);

            da.TryGet(7, out string favoriteName);
            da.TryGetList(8, out List<string> types);
            da.TryGetList(9, out List<int> floorIndices);
            da.TryGetList(10, out List<int> layerIndices);
            var filter = ElementTypeFloorLayerFilter.FromLists(
                types,
                floorIndices,
                layerIndices);

            var elementIds = new List<ElementGuid>();
            for (var i = 0; i < elementWrappers.Count; i++)
            {
                var elementId = GuidObject<ElementGuid>.CreateFromWrapper(elementWrappers[i]);
                if (elementId == null)
                {
                    this.AddError(
                        "Invalid element identifier in the ElementGuids input.");
                    return;
                }

                elementIds.Add(elementId);
            }

            var input = new ApplyFavoritesToElementsParameters
            {
                ApplySettings = applySettings,
                ApplyClassifications = applyClassifications,
                ApplyCategories = applyCategories,
                ApplyProperties = applyProperties,
                KeepFields = keepFields != null && keepFields.Count > 0 ? keepFields : null
            };

            if (!string.IsNullOrEmpty(favoriteName))
            {
                // One Favorite for many elements.
                if (favoriteNames.Count > 0)
                {
                    this.AddError(
                        "Give either FavoriteNames or FavoriteName, not both.");
                    return;
                }

                if (elementIds.Count == 0 && filter == null)
                {
                    this.AddError(
                        "Give ElementGuids or a filter (ElementTypes, FloorIndices, LayerIndices) with FavoriteName, " +
                        "so that it is not applied to the whole project by accident.");
                    return;
                }

                input.Favorite = favoriteName;
                input.Filter = filter;
                if (elementIds.Count > 0)
                {
                    input.Elements = new List<ElementGuidWrapper>();
                    foreach (var elementId in elementIds)
                    {
                        input.Elements.Add(
                            new ElementGuidWrapper
                            {
                                ElementId = elementId
                            });
                    }
                }
            }
            else
            {
                if (filter != null)
                {
                    this.AddError(
                        "The filter (ElementTypes, FloorIndices, LayerIndices) only works with FavoriteName.");
                    return;
                }

                if (elementIds.Count == 0 || favoriteNames.Count == 0)
                {
                    this.AddError(
                        "Give ElementGuids and FavoriteNames, or FavoriteName.");
                    return;
                }

                if (favoriteNames.Count != 1 &&
                    favoriteNames.Count != elementIds.Count)
                {
                    this.AddError(
                        "The size of the input FavoriteNames must be 1 or equal to the size of the input ElementGuids.");
                    return;
                }

                input.FavoritesToApply = new List<FavoriteForElement>();
                for (var i = 0; i < elementIds.Count; i++)
                {
                    input.FavoritesToApply.Add(
                        new FavoriteForElement
                        {
                            ElementId = elementIds[i],
                            Favorite = favoriteNames[favoriteNames.Count == 1 ? 0 : i]
                        });
                }
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ApplyFavoritesToElements;

        public override Guid ComponentGuid =>
            new Guid("aca17605-6883-45d5-aa39-2882128e104e");
    }
}
