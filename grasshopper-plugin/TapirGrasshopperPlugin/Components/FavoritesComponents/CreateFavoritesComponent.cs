using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Favorites;
using TapirGrasshopperPlugin.Types.Generic;

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

            inManager.AddTextParameter(
                "Folders",
                "Folders",
                "Folder hierarchy in the Favorites palette to place each new favorite under, " +
                "outermost folder first (one branch per favorite, or 1 branch to use the same folder for all). " +
                "Optional; by default the favorites are placed at the root.",
                GH_ParamAccess.tree);

            SetOptionality(2);
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
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return;
            }

            if (!da.TryGetList(
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

            da.TryGetTree(
                2,
                out GH_Structure<GH_String> folders);
            var folderBranchCount = folders?.Branches.Count ?? 0;
            if (folderBranchCount > 1 &&
                folderBranchCount != favorites.Count)
            {
                this.AddError(
                    "The number of branches in the input Folders must be 0, 1 or equal to the size of the input Favorites.");
                return;
            }

            var input = new FavoritesFromElementsObject(
                elements.GuidWrappers,
                favorites);
            if (folderBranchCount > 0)
            {
                for (var i = 0; i < input.FavoritesFromElements.Count; i++)
                {
                    var folder = folders.Branches[folderBranchCount == 1 ? 0 : i]
                        .Where(x => x != null && !string.IsNullOrEmpty(x.Value))
                        .Select(x => x.Value)
                        .ToList();
                    if (folder.Count > 0)
                    {
                        input.FavoritesFromElements[i].Folder = folder;
                    }
                }
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    input,
                    ToAddOn,
                    ExecutionResultsResponse.Deserialize,
                    out ExecutionResultsResponse response))
            {
                return;
            }

            da.SetDataList(
                0,
                response.ExecutionResults.Select(x => x.Message()));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateFavorites;

        public override Guid ComponentGuid =>
            new Guid("412380d3-4833-4166-827d-67e80edf4dbf");
    }
}