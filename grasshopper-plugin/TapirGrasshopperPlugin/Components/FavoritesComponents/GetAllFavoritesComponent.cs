using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.FavoritesComponents
{
    public class GetAllFavoritesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAllFavorites";

        public GetAllFavoritesComponent()
            : base(
                "GetAllFavorites",
                "Get every Favorite of the project, or only those of the given element type, with its name and " +
                "element type. Available from Archicad 29.",
                GroupNames.Favorites)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "ElementType",
                "Only the Favorites of this element type are returned. Leave it empty to get every Favorite.");

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "FavoriteNames",
                "Name of each Favorite.");

            OutTexts(
                "ElementTypes",
                "Element type of each Favorite (null when Archicad cannot read the Favorite's element settings).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            if (da.TryGet(
                    0,
                    out string elementType) &&
                !string.IsNullOrEmpty(elementType))
            {
                parameters["elementType"] = elementType;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var names = new List<object>();
            var elementTypes = new List<object>();

            foreach (var favorite in JsonOutputHelp.Items(
                         response,
                         "favorites"))
            {
                names.Add(JsonOutputHelp.Scalar(favorite, "name"));
                elementTypes.Add(JsonOutputHelp.Scalar(favorite, "elementType"));
            }

            da.SetDataList(0, names);
            da.SetDataList(1, elementTypes);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAllFavorites;

        public override Guid ComponentGuid =>
            new Guid("c6efefb7-134b-4571-a3e4-3dcdcad09937");
    }
}
