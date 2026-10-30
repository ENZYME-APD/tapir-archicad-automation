using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.FavoritesComponents
{
    public class ApplyFavoriteToToolDefaultComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ApplyFavoriteToToolDefault";

        public ApplyFavoriteToToolDefaultComponent()
            : base(
                "ApplyFavoriteToToolDefault",
                "Make the settings of the given Favorite the settings of its tool. Available from Archicad 29.",
                GroupNames.Favorites)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "FavoriteName",
                "Name of the Favorite whose settings become the settings of its tool.");
        }

        protected override void AddOutputs()
        {
            OutText(
                "ElementType",
                "Element type of the tool whose settings changed.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string favoriteName) ||
                string.IsNullOrEmpty(favoriteName))
            {
                this.AddError("FavoriteName is required.");
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["favoriteName"] = favoriteName },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(
                0,
                response["elementType"]?.ToString() ?? "");
        }

        public override Guid ComponentGuid =>
            new Guid("9a14c8d2-e7b3-4c50-8f19-6b2d5e0a3c94");
    }
}
