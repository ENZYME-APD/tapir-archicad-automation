using Grasshopper.Kernel;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Favorites;
using TapirGrasshopperPlugin.ResponseTypes.Generic;

namespace TapirGrasshopperPlugin.Components.FavoritesComponents
{
    public class ApplyFavoritesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "ApplyFavoritesToElementDefaults";

        public ApplyFavoritesComponent()
            : base(
                "ApplyFavoritesToElementDefaults",
                "Apply the given favorites to element defaults.",
                GroupNames.Favorites)
        {
        }

        protected override void AddInputs()
        {
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
            if (!da.TryGetAndCreate(
                    0,
                    out FavoritesObj input))
            {
                return;
            }

            if (!TryGetConvertedResponse(
                    CommandName,
                    input,
                    out ExecutionResultsResponse response))
            {
                return;
            }

            da.SetDataList(
                0,
                response.ExecutionResults.Select(x => x.Message()));
        }

        public override Guid ComponentGuid =>
            new Guid("88c1bbe5-1b1c-49f3-9318-b7135eab31de");
    }
}