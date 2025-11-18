using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Favorites;

namespace TapirGrasshopperPlugin.Components.FavoritesComponents
{
    public class GetFavoritesByTypeComponent : ArchicadAccessorComponent
    {
        public static string Doc =>
            "Returns a list of the names of all favorites with the given element type.";

        public override string CommandName => "GetFavoritesByType";

        public GetFavoritesByTypeComponent()
            : base(
                "Get Favorites by Type",
                "Get Favorites by Type",
                Doc,
                GroupNames.Favorites)
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
            pManager.AddTextParameter(
                "Type",
                "Type",
                "Elements type.",
                GH_ParamAccess.item);
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddTextParameter(
                nameof(Favorites),
                "",
                "",
                GH_ParamAccess.list);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.GetItem(
                    0,
                    out string eType))
            {
                return;
            }

            if (!GetConvertedResponse(
                    CommandName,
                    new ElementTypeObject(eType),
                    out FavoritesResponse response))
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