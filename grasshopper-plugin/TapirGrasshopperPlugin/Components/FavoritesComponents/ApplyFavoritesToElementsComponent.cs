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
                "Apply the given Favorites to the given elements.",
                GroupNames.Favorites)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to apply the Favorites to.");

            InTexts(
                "FavoriteNames",
                "Names of the Favorites to apply (input only 1 to apply the same Favorite to all elements).");

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
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> elementWrappers))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<string> favoriteNames))
            {
                return;
            }

            if (favoriteNames.Count != 1 &&
                favoriteNames.Count != elementWrappers.Count)
            {
                this.AddError(
                    "The size of the input FavoriteNames must be 1 or equal to the size of the input ElementGuids.");
                return;
            }

            da.TryGet(2, out bool applySettings);
            da.TryGet(3, out bool applyClassifications);
            da.TryGet(4, out bool applyCategories);
            da.TryGet(5, out bool applyProperties);

            var input = new ApplyFavoritesToElementsParameters
            {
                FavoritesToApply = new List<FavoriteForElement>(),
                ApplySettings = applySettings,
                ApplyClassifications = applyClassifications,
                ApplyCategories = applyCategories,
                ApplyProperties = applyProperties
            };

            for (var i = 0; i < elementWrappers.Count; i++)
            {
                var elementId = GuidObject<ElementGuid>.CreateFromWrapper(elementWrappers[i]);
                if (elementId == null)
                {
                    this.AddError(
                        "Invalid element identifier in the ElementGuids input.");
                    return;
                }

                input.FavoritesToApply.Add(
                    new FavoriteForElement
                    {
                        ElementId = elementId,
                        Favorite = favoriteNames[favoriteNames.Count == 1 ? 0 : i]
                    });
            }

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ApplyFavoritesToElements;

        public override Guid ComponentGuid =>
            new Guid("aca17605-6883-45d5-aa39-2882128e104e");
    }
}
