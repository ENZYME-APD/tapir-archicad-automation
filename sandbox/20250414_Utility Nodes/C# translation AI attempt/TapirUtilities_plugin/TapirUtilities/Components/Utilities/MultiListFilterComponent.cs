using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using System.Linq;

namespace Tapir.Components.Utilities
{
    public class MultiListFilterComponent : Component
    {
        /// <summary>
        /// Initializes a new instance of the MultiListFilterComponent class.
        /// </summary>
        public MultiListFilterComponent()
            : base("Multi-List Filter", "MLFilter", 
                  "Filters multiple lists based on specified indices", "Utilities")
        {
        }

        /// <summary>
        /// Registers all the input parameters for this component.
        /// </summary>
        protected override void RegisterInputParams(GH_Component.GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter("List 1", "L1", "First list to filter", GH_ParamAccess.list);
            pManager.AddGenericParameter("List 2", "L2", "Second list to filter", GH_ParamAccess.list);
            pManager[1].Optional = true;
            pManager.AddGenericParameter("List 3", "L3", "Third list to filter", GH_ParamAccess.list);
            pManager[2].Optional = true;
            pManager.AddGenericParameter("List 4", "L4", "Fourth list to filter", GH_ParamAccess.list);
            pManager[3].Optional = true;
            pManager.AddIntegerParameter("Index", "I", "Index or indices to extract from lists", GH_ParamAccess.list);
        }

        /// <summary>
        /// Registers all the output parameters for this component.
        /// </summary>
        protected override void RegisterOutputParams(GH_Component.GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter("Out 1", "O1", "Filtered output from List 1", GH_ParamAccess.list);
            pManager.AddGenericParameter("Out 2", "O2", "Filtered output from List 2", GH_ParamAccess.list);
            pManager.AddGenericParameter("Out 3", "O3", "Filtered output from List 3", GH_ParamAccess.list);
            pManager.AddGenericParameter("Out 4", "O4", "Filtered output from List 4", GH_ParamAccess.list);
        }

        /// <summary>
        /// This is the method that actually does the work.
        /// </summary>
        /// <param name="DA">The DA object is used to retrieve from inputs and store in outputs.</param>
        protected override void SolveInstance(IGH_DataAccess DA)
        {
            // Initialize outputs as empty lists
            List<IGH_Goo> out1 = new List<IGH_Goo>();
            List<IGH_Goo> out2 = new List<IGH_Goo>();
            List<IGH_Goo> out3 = new List<IGH_Goo>();
            List<IGH_Goo> out4 = new List<IGH_Goo>();
            string message = "Waiting for at least one input list.";

            // Collect input lists
            List<IGH_Goo> list1 = new List<IGH_Goo>();
            List<IGH_Goo> list2 = new List<IGH_Goo>();
            List<IGH_Goo> list3 = new List<IGH_Goo>();
            List<IGH_Goo> list4 = new List<IGH_Goo>();

            bool hasList1 = DA.GetDataList(0, list1);
            bool hasList2 = DA.GetDataList(1, list2);
            bool hasList3 = DA.GetDataList(2, list3);
            bool hasList4 = DA.GetDataList(3, list4);

            // Collect non-null input lists
            List<List<IGH_Goo>> inputLists = new List<List<IGH_Goo>>();
            if (hasList1 && list1.Count > 0) inputLists.Add(list1);
            if (hasList2 && list2.Count > 0) inputLists.Add(list2);
            if (hasList3 && list3.Count > 0) inputLists.Add(list3);
            if (hasList4 && list4.Count > 0) inputLists.Add(list4);

            // Process only if we have at least one non-empty input list
            if (inputLists.Count > 0)
            {
                // Validate lists have the same length
                List<int> lengths = inputLists.Select(lst => lst.Count).ToList();
                if (!lengths.All(x => x == lengths[0]))
                {
                    // Format error message with line breaks
                    message = $"Lists have different lengths:\n{string.Join(", ", lengths)}";
                }
                else
                {
                    // Get indices
                    List<int> indices = new List<int>();
                    if (!DA.GetDataList(4, indices) || indices.Count == 0)
                    {
                        message = "No indices provided";
                    }
                    else
                    {
                        try
                        {
                            // Adjust negative indices to count from the end
                            for (int i = 0; i < indices.Count; i++)
                            {
                                if (indices[i] < 0)
                                {
                                    indices[i] = lengths[0] + indices[i];
                                }
                            }

                            // Filter all lists using the indices
                            List<List<IGH_Goo>> filteredLists = new List<List<IGH_Goo>>();
                            foreach (var lst in inputLists)
                            {
                                List<IGH_Goo> filtered = new List<IGH_Goo>();
                                foreach (int idx in indices)
                                {
                                    if (idx >= 0 && idx < lst.Count)
                                    {
                                        filtered.Add(lst[idx]);
                                    }
                                    else
                                    {
                                        throw new IndexOutOfRangeException();
                                    }
                                }
                                filteredLists.Add(filtered);
                            }

                            // If only one index was used, return single items instead of lists
                            if (indices.Count == 1)
                            {
                                if (inputLists.Count >= 1 && filteredLists.Count >= 1) out1 = new List<IGH_Goo> { filteredLists[0][0] };
                                if (inputLists.Count >= 2 && filteredLists.Count >= 2) out2 = new List<IGH_Goo> { filteredLists[1][0] };
                                if (inputLists.Count >= 3 && filteredLists.Count >= 3) out3 = new List<IGH_Goo> { filteredLists[2][0] };
                                if (inputLists.Count >= 4 && filteredLists.Count >= 4) out4 = new List<IGH_Goo> { filteredLists[3][0] };
                            }
                            else
                            {
                                if (inputLists.Count >= 1 && filteredLists.Count >= 1) out1 = filteredLists[0];
                                if (inputLists.Count >= 2 && filteredLists.Count >= 2) out2 = filteredLists[1];
                                if (inputLists.Count >= 3 && filteredLists.Count >= 3) out3 = filteredLists[2];
                                if (inputLists.Count >= 4 && filteredLists.Count >= 4) out4 = filteredLists[3];
                            }

                            // Format success message with line breaks
                            if (indices.Count == 1)
                            {
                                message = $"Output: {indices.Count} value\nfrom {inputLists.Count} lists";
                            }
                            else
                            {
                                message = $"Output: {indices.Count} values\nfrom {inputLists.Count} lists";
                            }

                            // Add additional details if needed
                            if (message.Length < 40)
                            {
                                message += $"\nof {lengths[0]} elements";
                            }
                        }
                        catch (IndexOutOfRangeException)
                        {
                            // Format error message with line breaks
                            message = $"Index out of range.\nValid range is -{lengths[0]} to {lengths[0]-1}";
                        }
                    }
                }
            }

            // Set outputs
            DA.SetDataList(0, out1);
            DA.SetDataList(1, out2);
            DA.SetDataList(2, out3);
            DA.SetDataList(3, out4);

            // Update component message
            this.Message = FormatMessage(message);
        }

        /// <summary>
        /// Helper function to format the message with line breaks.
        /// </summary>
        private string FormatMessage(string msg, int maxLength = 30)
        {
            List<string> lines = new List<string>();
            string currentLine = "";
            
            foreach (string word in msg.Split(' '))
            {
                if (currentLine.Length + word.Length + 1 <= maxLength)
                {
                    currentLine += word + " ";
                }
                else
                {
                    lines.Add(currentLine.Trim());
                    currentLine = word + " ";
                }
            }
            
            if (!string.IsNullOrEmpty(currentLine))
            {
                lines.Add(currentLine.Trim());
            }
            
            return string.Join("\n", lines);
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
            get { return new Guid("A6B11162-627B-455B-B9C0-963E76B36DC6"); }
        }
    }
}
