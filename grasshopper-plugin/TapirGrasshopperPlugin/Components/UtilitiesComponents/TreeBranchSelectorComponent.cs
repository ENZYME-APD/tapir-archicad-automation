using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.UtilitiesComponents
{
    public class TreeBranchSelectorComponent : Component
    {
        public TreeBranchSelectorComponent()
            : base(
                "TreeBranchSelector",
                "Selects a specific branch from a tree structure based on an index.",
                GroupNames.Utilities)
        {
        }

        protected override void AddInputs()
        {
            InGenericTree(
                "InputTree",
                "Tree structure to select from");

            InInteger(
                "PathIndex",
                "Integer for branch selection",
                0);
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "SelectedBranch",
                "List of items from the selected branch");

            OutGenerics(
                "OtherBranches",
                "List of items from all other branches");

            OutInteger(
                "TotalBrancheCount",
                "Total number of branches in the tree");
        }

        protected override void SolveInstance(
            IGH_DataAccess da)
        {
            // Initialize outputs
            var selectedBranch = new List<IGH_Goo>();
            var otherBranches = new List<IGH_Goo>();
            var totalBranches = 0;

            // Get input tree
            var inputTree = new GH_Structure<IGH_Goo>();
            if (!da.GetDataTree(
                    0,
                    out inputTree))
            {
                Message = "Error: Input tree is required";
                return;
            }

            var message = "";

            try
            {
                // Get all paths from the tree
                var paths = inputTree.Paths;
                totalBranches = paths.Count;

                if (totalBranches == 0)
                {
                    message = "Input tree is empty";
                    Message = message;
                    da.SetDataList(
                        0,
                        selectedBranch);
                    da.SetDataList(
                        1,
                        otherBranches);
                    da.SetData(
                        2,
                        totalBranches);
                    return;
                }

                // Get path index
                var pathIndex = 0;
                da.GetData(
                    1,
                    ref pathIndex);

                // Convert path_index to int and handle bounds
                pathIndex = pathIndex % totalBranches;
                if (pathIndex < 0)
                {
                    pathIndex += totalBranches;
                }

                // Get the path at the specified index
                var selectedPath = paths[pathIndex];

                // Get branch data at the selected path
                selectedBranch = inputTree.Branches[pathIndex];

                // Get all other branches
                for (var i = 0; i < totalBranches; i++)
                {
                    if (i != pathIndex)
                    {
                        otherBranches.AddRange(inputTree.Branches[i]);
                    }
                }

                message =
                    $"Selected branch {pathIndex} of {totalBranches} (Path: {selectedPath})\n" +
                    $"Selected branch: {selectedBranch.Count} items\n" +
                    $"Other branches: {otherBranches.Count} items";
            }
            catch (Exception e)
            {
                message = $"Error: {e.Message}";
                selectedBranch.Clear();
                otherBranches.Clear();
                totalBranches = 0;
            }

            // Set outputs
            da.SetDataList(
                0,
                selectedBranch);
            da.SetDataList(
                1,
                otherBranches);
            da.SetData(
                2,
                totalBranches);

            // Update component message
            Message = message;
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.TreeBranchSelector;

        public override Guid ComponentGuid =>
            new Guid("A2B11162-627B-455B-B9C0-963E76B36DC2");
    }
}