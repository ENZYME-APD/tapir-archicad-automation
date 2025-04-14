using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using System.Linq;

namespace TapirGrasshopperPlugin.Components.UtilitiesComponents
{
    public class TreeBranchSelectorComponent : Component
    {
        public TreeBranchSelectorComponent ()
            : base (
                "Tree Branch Selector",
                "BranchSel",
                "Selects a specific branch from a tree structure based on an index",
                "Utilities"
            )
        {
        }

        protected override void RegisterInputParams (GH_Component.GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("Input Tree", "T", "Tree structure to select from", GH_ParamAccess.tree);
            pManager.AddIntegerParameter ("Path Index", "I", "Integer for branch selection", GH_ParamAccess.item, 0);
        }

        protected override void RegisterOutputParams (GH_Component.GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("Selected Branch", "S", "List of items from the selected branch", GH_ParamAccess.list);
            pManager.AddGenericParameter ("Other Branches", "O", "List of items from all other branches", GH_ParamAccess.list);
            pManager.AddIntegerParameter ("Total Branches", "T", "Total number of branches in the tree", GH_ParamAccess.item);
        }

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            // Initialize outputs
            List<IGH_Goo> selectedBranch = new List<IGH_Goo> ();
            List<IGH_Goo> otherBranches = new List<IGH_Goo> ();
            int totalBranches = 0;

            // Get input tree
            GH_Structure<IGH_Goo> inputTree = new GH_Structure<IGH_Goo> ();
            if (!DA.GetDataTree (0, out inputTree)) {
                this.Message = "Error: Input tree is required";
                return;
            }

            string message = "";

            try {
                // Get all paths from the tree
                var paths = inputTree.Paths;
                totalBranches = paths.Count;

                if (totalBranches == 0) {
                    message = "Input tree is empty";
                    this.Message = message;
                    DA.SetDataList (0, selectedBranch);
                    DA.SetDataList (1, otherBranches);
                    DA.SetData (2, totalBranches);
                    return;
                }

                // Get path index
                int pathIndex = 0;
                DA.GetData (1, ref pathIndex);

                // Convert path_index to int and handle bounds
                pathIndex = pathIndex % totalBranches;
                if (pathIndex < 0) {
                    pathIndex += totalBranches;
                }

                // Get the path at the specified index
                var selectedPath = paths[pathIndex];

                // Get branch data at the selected path
                selectedBranch = inputTree.Branches[pathIndex];

                // Get all other branches
                for (int i = 0; i < totalBranches; i++) {
                    if (i != pathIndex) {
                        otherBranches.AddRange (inputTree.Branches[i]);
                    }
                }

                message = $"Selected branch {pathIndex} of {totalBranches} (Path: {selectedPath})\n" +
                          $"Selected branch: {selectedBranch.Count} items\n" +
                          $"Other branches: {otherBranches.Count} items";
            } catch (Exception e) {
                message = $"Error: {e.Message}";
                selectedBranch.Clear ();
                otherBranches.Clear ();
                totalBranches = 0;
            }

            // Set outputs
            DA.SetDataList (0, selectedBranch);
            DA.SetDataList (1, otherBranches);
            DA.SetData (2, totalBranches);

            // Update component message
            this.Message = message;
        }

        protected override System.Drawing.Bitmap Icon => Properties.Resources.GH_TreeBranchSelector;

        public override Guid ComponentGuid => new Guid ("A2B11162-627B-455B-B9C0-963E76B36DC2");
    }
}
