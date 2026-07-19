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
    public class UpdateFavoritesFromElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "UpdateFavoritesFromElements";

        public UpdateFavoritesFromElementsComponent()
            : base(
                "UpdateFavoritesFromElements",
                "Update the given existing Favorites from the given elements.",
                GroupNames.Favorites)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to update the Favorites from.");

            InTexts(
                "FavoriteNames",
                "Names of the existing Favorites to update.");
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

            if (favoriteNames.Count != elementWrappers.Count)
            {
                this.AddError(
                    "The size of the inputs ElementGuids and FavoriteNames must be equal.");
                return;
            }

            var input = new UpdateFavoritesFromElementsParameters
            {
                FavoritesFromElements = new List<FavoriteForElement>()
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

                input.FavoritesFromElements.Add(
                    new FavoriteForElement
                    {
                        ElementId = elementId,
                        Favorite = favoriteNames[i]
                    });
            }

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.UpdateFavoritesFromElements;

        public override Guid ComponentGuid =>
            new Guid("103ce5e8-b10e-49a0-b1c9-7ccdbb2333f7");
    }
}
