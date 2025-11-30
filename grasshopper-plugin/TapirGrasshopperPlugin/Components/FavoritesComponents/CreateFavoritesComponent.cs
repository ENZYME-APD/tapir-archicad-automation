using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Element;
using TapirGrasshopperPlugin.ResponseTypes.Favorites;
using TapirGrasshopperPlugin.ResponseTypes.Generic;

namespace TapirGrasshopperPlugin.Components.FavoritesComponents
{
    public class CreateFavoritesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "CreateFavoritesFromElements";

        public CreateFavoritesComponent()
            : base(
                "CreateFavoritesFromElements",
                "Create favorites from the given elements.",
                GroupNames.Favorites)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Elements Guids to get detailList for.");

            InTexts(
                nameof(Favorites),
                "A list of favorite names.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                nameof(ExecutionResultsResponse.ExecutionResults),
                ExecutionResult.Doc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!ElementsObj.TryCreate(
                    da,
                    0,
                    out ElementsObj elements))
            {
                return;
            }

            if (!da.TryGetItems(
                    1,
                    out List<string> favorites))
            {
                return;
            }

            if (favorites.Count != elements.Elements.Count)
            {
                this.AddError("Element to Favorite count mismatch!");
                return;
            }

            if (!TryGetConvertedValues(
                    CommandName,
                    new FavoritesFromElementsObj(
                        elements.Ids,
                        favorites),
                    SendToAddOn,
                    ExecutionResultsResponse.Deserialize,
                    out ExecutionResultsResponse response))
            {
                return;
            }

            da.SetDataList(
                0,
                response.ExecutionResults.Select(x => x.Message()));
        }

        public override Guid ComponentGuid =>
            new Guid("412380d3-4833-4166-827d-67e80edf4dbf");
    }
}