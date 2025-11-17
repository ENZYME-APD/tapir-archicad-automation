using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Favorites;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetFavoritesByTypeComponent : ArchicadAccessorComponent
    {
        public static string CommandName => "GetFavoritesByType";

        public GetFavoritesByTypeComponent()
            : base(
                CommandName,
                CommandName,
                FavoritesResponse.Doc,
                GroupNames.Favorites)
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
            pManager.AddTextParameter(
                "Type",
                "Type",
                "Element type.",
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
            IGH_DataAccess DA)
        {
            if (!DA.GetItem(
                    0,
                    out string eType))
            {
                return;
            }

            var jObject = JObject.FromObject(new ElementTypeObject(eType));

            if (!GetResponse(
                    CommandName,
                    jObject,
                    out FavoritesResponse response))
            {
                return;
            }

            DA.SetDataList(
                0,
                response.Favorites);
        }

        public override Guid ComponentGuid =>
            new Guid("2055b426-9c88-4b49-8ff4-4b03145e5b1c");
    }
}