using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using System.Linq;

namespace Tapir.Components.Utilities
{
    public class TreeBranchSelectorComponent : Component
    {
        /// <summary>
        /// Initializes a new instance of the TreeBranchSelectorComponent class.
        /// </summary>
        public TreeBranchSelectorComponent()
            : base("Tree Branch Selector", "BranchSel", 
                  "Selects a specific branch from a tree structure based on an index", "Utilities")
        {
        }

        /// <summary>
        /// Registers all the input parameters for this component.
        /// </summary>
        protected override void RegisterInputParams(GH_Component.GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter("Input Tree", "T", "Tree structure to select from", GH_ParamAccess.tree);
            pManager.AddIntegerParameter("Path Index", "I", "Integer for branch selection", GH_ParamAccess.item, 0);
        }

        /// <summary>
        /// Registers all the output parameters for this component.
        /// </summary>
        protected override void RegisterOutputParams(GH_Component.GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter("Selected Branch", "S", "List of items from the selected branch", GH_ParamAccess.list);
            pManager.AddGenericParameter("Other Branches", "O", "List of items from all other branches", GH_ParamAccess.list);
            pManager.AddIntegerParameter("Total Branches", "T", "Total number of branches in the tree", GH_ParamAccess.item);
        }

        /// <summary>
        /// This is the method that actually does the work.
        /// </summary>
        /// <param name="DA">The DA object is used to retrieve from inputs and store in outputs.</param>
        protected override void SolveInstance(IGH_DataAccess DA)
        {
            // Initialize outputs
            List<IGH_Goo> selectedBranch = new List<IGH_Goo>();
            List<IGH_Goo> otherBranches = new List<IGH_Goo>();
            int totalBranches = 0;
            string message = "";

            // Get input tree
            GH_Structure<IGH_Goo> inputTree = new GH_Structure<IGH_Goo>();
            if (!DA.GetDataTree(0, out inputTree))
            {
                message = "Error: Input tree is required";
                this.Message = message;
                DA.SetDataList(0, selectedBranch);
                DA.SetDataList(1, otherBranches);
                DA.SetData(2, totalBranches);
                return;
            }

            try
            {
                // Get all paths from the tree
                var paths = inputTree.Paths;
                totalBranches = paths.Count;

                if (totalBranches == 0)
                {
                    message = "Input tree is empty";
                    this.Message = message;
                    DA.SetDataList(0, selectedBranch);
                    DA.SetDataList(1, otherBranches);
                    DA.SetData(2, totalBranches);
                    return;
                }

                // Get path index
                int pathIndex = 0;
                DA.GetData(1, ref pathIndex);

                // Convert path_index to int and handle bounds
                pathIndex = pathIndex % totalBranches;
                if (pathIndex < 0)
                {
                    pathIndex += totalBranches;
                }

                // Get the path at the specified index
                var selectedPath = paths[pathIndex];

                // Get branch data at the selected path
                selectedBranch = inputTree.get_Branch(selectedPath).ToList();

                // Get all other branches
                for (int i = 0; i < totalBranches; i++)
                {
                    if (i != pathIndex)
                    {
                        var branchData = inputTree.get_Branch(paths[i]).ToList();
                        otherBranches.AddRange(branchData);
                    }
                }

                message = $"Selected branch {pathIndex} of {totalBranches} (Path: {selectedPath})\n" +
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
            DA.SetDataList(0, selectedBranch);
            DA.SetDataList(1, otherBranches);
            DA.SetData(2, totalBranches);

            // Update component message
            this.Message = message;
        }

        /// <summary>
        /// Provides an Icon for the component.
        /// </summary>
        protected override System.Drawing.Bitmap Icon
        {
            get
            {
                return null;
            }
        }

        /// <summary>
        /// Gets the unique ID for this component. Do not change this ID after release.
        /// </summary>
        public override Guid ComponentGuid
        {
            get { return new Guid("A2B11162-627B-455B-B9C0-963E76B36DC2"); }
        }
    }
}
