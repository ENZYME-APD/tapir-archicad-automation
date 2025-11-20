using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.UtilitiesComponents
{
    public class TreeBranchSelectorComponent : Component
    {
        public static string Doc =>
            "Selects a specific branch from a tree structure based on an index.";

        public TreeBranchSelectorComponent()
            : base(
                "Tree Branch Selector",
                "BranchSel",
                Doc,
                GroupNames.Utilities)
        {
        }

        protected override void AddInputs()
        {
            InGenericTree(
                "Input Tree",
                "Tree structure to select from");

            InInteger(
                "Path Index",
                "Integer for branch selection",
                0);
        }


        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter(
                "Selected Branch",
                "S",
                "List of items from the selected branch",
                GH_ParamAccess.list);
            pManager.AddGenericParameter(
                "Other Branches",
                "O",
                "List of items from all other branches",
                GH_ParamAccess.list);
            pManager.AddIntegerParameter(
                "Total Branches",
                "T",
                "Total number of branches in the tree",
                GH_ParamAccess.item);
        }

        protected override void SolveInstance(
            IGH_DataAccess DA)
        {
            // Initialize outputs
            var selectedBranch = new List<IGH_Goo>();
            var otherBranches = new List<IGH_Goo>();
            var totalBranches = 0;

            // Get input tree
            var inputTree = new GH_Structure<IGH_Goo>();
            if (!DA.GetDataTree(
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
                    DA.SetDataList(
                        0,
                        selectedBranch);
                    DA.SetDataList(
                        1,
                        otherBranches);
                    DA.SetData(
                        2,
                        totalBranches);
                    return;
                }

                // Get path index
                var pathIndex = 0;
                DA.GetData(
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
            DA.SetDataList(
                0,
                selectedBranch);
            DA.SetDataList(
                1,
                otherBranches);
            DA.SetData(
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