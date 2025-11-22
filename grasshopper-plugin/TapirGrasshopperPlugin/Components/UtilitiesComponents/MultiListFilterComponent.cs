using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using System.Linq;

namespace TapirGrasshopperPlugin.Components.UtilitiesComponents
{
    public class MultiListFilterComponent : Component
    {
        public static string Doc =>
            "Filters multiple lists based on specified indices";

        public MultiListFilterComponent()
            : base(
                "Multi-List Filter",
                "MLFilter",
                Doc,
                GroupNames.Utilities)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "List 1",
                "First list to filter.");

            InGenerics(
                "List 2",
                "Second list to filter.");

            InGenerics(
                "List 3",
                "Third list to filter.");

            InGenerics(
                "List 4",
                "Fourth list to filter.");

            InIntegers(
                "Index",
                "Index or indices to extract from lists");

            Params.Input[1].Optional = true;
            Params.Input[2].Optional = true;
            Params.Input[3].Optional = true;
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "Out 1",
                "Filtered output from List 1");

            OutGenerics(
                "Out 2",
                "Filtered output from List 2");

            OutGenerics(
                "Out 3",
                "Filtered output from List 3");

            OutGenerics(
                "Out 4",
                "Filtered output from List 4");
        }

        protected override void SolveInstance(
            IGH_DataAccess DA)
        {
            // Initialize outputs as empty lists
            var out1 = new List<IGH_Goo>();
            var out2 = new List<IGH_Goo>();
            var out3 = new List<IGH_Goo>();
            var out4 = new List<IGH_Goo>();
            var message = "Waiting for at least one input list.";

            // Collect input lists
            var list1 = new List<IGH_Goo>();
            var list2 = new List<IGH_Goo>();
            var list3 = new List<IGH_Goo>();
            var list4 = new List<IGH_Goo>();

            var hasList1 = DA.GetDataList(
                0,
                list1);
            var hasList2 = DA.GetDataList(
                1,
                list2);
            var hasList3 = DA.GetDataList(
                2,
                list3);
            var hasList4 = DA.GetDataList(
                3,
                list4);

            // Collect non-null input lists
            var inputLists = new List<List<IGH_Goo>>();
            if (hasList1 && list1.Count > 0)
            {
                inputLists.Add(list1);
            }

            if (hasList2 && list2.Count > 0)
            {
                inputLists.Add(list2);
            }

            if (hasList3 && list3.Count > 0)
            {
                inputLists.Add(list3);
            }

            if (hasList4 && list4.Count > 0)
            {
                inputLists.Add(list4);
            }

            // Process only if we have at least one non-empty input list
            if (inputLists.Count > 0)
            {
                // Validate lists have the same length
                var lengths = inputLists.Select(lst => lst.Count).ToList();
                if (!lengths.All(x => x == lengths[0]))
                {
                    // Format error message with line breaks
                    message =
                        $"Lists have different lengths:\n{string.Join(", ", lengths)}";
                }
                else
                {
                    // Get indices
                    var indices = new List<int>();
                    if (!DA.GetDataList(
                            4,
                            indices) || indices.Count == 0)
                    {
                        message = "No indices provided";
                    }
                    else
                    {
                        try
                        {
                            // Adjust negative indices to count from the end
                            for (var i = 0; i < indices.Count; i++)
                            {
                                if (indices[i] < 0)
                                {
                                    indices[i] = lengths[0] + indices[i];
                                }
                            }

                            // Filter all lists using the indices
                            var filteredLists = new List<List<IGH_Goo>>();
                            foreach (var lst in inputLists)
                            {
                                var filtered = new List<IGH_Goo>();
                                foreach (var idx in indices)
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
                                if (inputLists.Count >= 1 &&
                                    filteredLists.Count >= 1)
                                {
                                    out1 = new List<IGH_Goo>
                                    {
                                        filteredLists[0][0]
                                    };
                                }

                                if (inputLists.Count >= 2 &&
                                    filteredLists.Count >= 2)
                                {
                                    out2 = new List<IGH_Goo>
                                    {
                                        filteredLists[1][0]
                                    };
                                }

                                if (inputLists.Count >= 3 &&
                                    filteredLists.Count >= 3)
                                {
                                    out3 = new List<IGH_Goo>
                                    {
                                        filteredLists[2][0]
                                    };
                                }

                                if (inputLists.Count >= 4 &&
                                    filteredLists.Count >= 4)
                                {
                                    out4 = new List<IGH_Goo>
                                    {
                                        filteredLists[3][0]
                                    };
                                }
                            }
                            else
                            {
                                if (inputLists.Count >= 1 &&
                                    filteredLists.Count >= 1)
                                {
                                    out1 = filteredLists[0];
                                }

                                if (inputLists.Count >= 2 &&
                                    filteredLists.Count >= 2)
                                {
                                    out2 = filteredLists[1];
                                }

                                if (inputLists.Count >= 3 &&
                                    filteredLists.Count >= 3)
                                {
                                    out3 = filteredLists[2];
                                }

                                if (inputLists.Count >= 4 &&
                                    filteredLists.Count >= 4)
                                {
                                    out4 = filteredLists[3];
                                }
                            }

                            // Format success message with line breaks
                            if (indices.Count == 1)
                            {
                                message =
                                    $"Output: {indices.Count} value\nfrom {inputLists.Count} lists";
                            }
                            else
                            {
                                message =
                                    $"Output: {indices.Count} values\nfrom {inputLists.Count} lists";
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
                            message =
                                $"Index out of range.\nValid range is -{lengths[0]} to {lengths[0] - 1}";
                        }
                    }
                }
            }

            // Set outputs
            DA.SetDataList(
                0,
                out1);
            DA.SetDataList(
                1,
                out2);
            DA.SetDataList(
                2,
                out3);
            DA.SetDataList(
                3,
                out4);

            // Update component message
            Message = FormatMessage(message);
        }

        private string FormatMessage(
            string msg,
            int maxLength = 30)
        {
            var lines = new List<string>();
            var currentLine = "";

            foreach (var word in msg.Split(' '))
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

            return string.Join(
                "\n",
                lines);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MultiListFilter;

        public override Guid ComponentGuid =>
            new Guid("A6B11162-627B-455B-B9C0-963E76B36DC6");
    }
}