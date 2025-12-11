using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Favorites;

namespace TapirGrasshopperPlugin.Components.FavoritesComponents
{
    public class GetFavoritesByTypeComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetFavoritesByType";

        public GetFavoritesByTypeComponent()
            : base(
                "FavoritesByType",
                "Returns a list of the names of all Favorites with the given element type.",
                GroupNames.Favorites)
        {
        }

        protected override void AddInputs()
        {
            InText("ElementsType");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                nameof(Favorites),
                "Names of the stored Favorites.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string input))
            {
                return;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    new ElementTypeObject(input),
                    ToAddOn,
                    JHelp.Deserialize<FavoritesObject>,
                    out FavoritesObject response))
            {
                return;
            }

            da.SetDataList(
                0,
                response.Favorites);
        }

        public override Guid ComponentGuid =>
            new Guid("2055b426-9c88-4b49-8ff4-4b03145e5b1c");
    }
}