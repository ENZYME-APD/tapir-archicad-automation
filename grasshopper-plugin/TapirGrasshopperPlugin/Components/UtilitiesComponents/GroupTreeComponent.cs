using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using System.Linq;

namespace TapirGrasshopperPlugin.Components.UtilitiesComponents
{
    public class GroupTreeComponent : Component
    {
        public static string Doc =>
            "Groups data items into a tree structure based on values";

        public GroupTreeComponent()
            : base(
                "Group to Tree",
                "GroupTree",
                Doc,
                GroupNames.Utilities)
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter(
                "Data List",
                "Data",
                "List of data items to group",
                GH_ParamAccess.list);
            pManager.AddGenericParameter(
                "Values List",
                "Values",
                "List of values to group by",
                GH_ParamAccess.list);
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter(
                "Output Tree",
                "Tree",
                "Data tree with items grouped by values",
                GH_ParamAccess.tree);
            pManager.AddGenericParameter(
                "Unique Values",
                "UValues",
                "List of unique values used for grouping",
                GH_ParamAccess.list);
            pManager.AddIntegerParameter(
                "Branch Counts",
                "C",
                "Number of items in each branch",
                GH_ParamAccess.list);
        }

        protected override void SolveInstance(
            IGH_DataAccess DA)
        {
            // Initialize outputs
            var outputTree = new GH_Structure<IGH_Goo>();
            var uniqueValues = new List<IGH_Goo>();
            var branchCounts = new List<int>();

            // Get inputs
            var dataList = new List<IGH_Goo>();
            if (!DA.GetDataList(
                    0,
                    dataList) || dataList.Count == 0)
            {
                Message = "Error: Both data list and values list are required";
                DA.SetDataTree(
                    0,
                    outputTree);
                DA.SetDataList(
                    1,
                    uniqueValues);
                DA.SetDataList(
                    2,
                    branchCounts);
                return;
            }

            var valuesList = new List<IGH_Goo>();
            if (!DA.GetDataList(
                    1,
                    valuesList) || valuesList.Count == 0)
            {
                Message = "Error: Both data list and values list are required";
                DA.SetDataTree(
                    0,
                    outputTree);
                DA.SetDataList(
                    1,
                    uniqueValues);
                DA.SetDataList(
                    2,
                    branchCounts);
                return;
            }

            // Validate lists have same length
            if (dataList.Count != valuesList.Count)
            {
                Message =
                    $"Error: Lists have different lengths. Data: {dataList.Count}, Values: {valuesList.Count}";
                DA.SetDataTree(
                    0,
                    outputTree);
                DA.SetDataList(
                    1,
                    uniqueValues);
                DA.SetDataList(
                    2,
                    branchCounts);
                return;
            }

            var message = "";

            try
            {
                // Create dictionary to store unique values and their data
                var groups = new Dictionary<string, List<IGH_Goo>>();
                var valueMap = new Dictionary<string, IGH_Goo>();

                // Group data by values
                for (var i = 0; i < dataList.Count; i++)
                {
                    var valueKey = valuesList[i].ToString();
                    if (!groups.ContainsKey(valueKey))
                    {
                        groups[valueKey] = new List<IGH_Goo>();
                        valueMap[valueKey] = valuesList[i];
                    }

                    groups[valueKey].Add(dataList[i]);
                }

                // Sort groups by values and create unique values list
                var sortedItems = groups.Keys.OrderBy(k => k).ToList();

                foreach (var key in sortedItems)
                {
                    uniqueValues.Add(valueMap[key]);
                    branchCounts.Add(groups[key].Count);
                }

                // Add branches to tree
                for (var i = 0; i < sortedItems.Count; i++)
                {
                    var path = new GH_Path(i);
                    outputTree.AppendRange(
                        groups[sortedItems[i]],
                        path);
                }

                // Format message
                message =
                    $"Created {groups.Count} branches from {dataList.Count} items\n";
                message += "Items per branch: \n";

                // Format items per branch with wrapping
                var branchInfo = new List<string>();
                for (var i = 0; i < uniqueValues.Count; i++)
                {
                    branchInfo.Add($"{uniqueValues[i]}({branchCounts[i]})");
                }

                var itemsMessage = string.Join(
                    ", ",
                    branchInfo);
                var maxLineLength = 40;
                var wrappedLines = WrapText(
                    itemsMessage,
                    maxLineLength);

                message += string.Join(
                    "\n",
                    wrappedLines);
            }
            catch (Exception e)
            {
                message = $"Error: {e.Message}";
                outputTree = new GH_Structure<IGH_Goo>();
                uniqueValues.Clear();
                branchCounts.Clear();
            }

            // Set outputs
            DA.SetDataTree(
                0,
                outputTree);
            DA.SetDataList(
                1,
                uniqueValues);
            DA.SetDataList(
                2,
                branchCounts);

            // Update component message
            Message = message;
        }

        private List<string> WrapText(
            string text,
            int maxLength)
        {
            var lines = new List<string>();
            var currentLine = "";

            foreach (var item in text.Split(
                         new[]
                         {
                             ", "
                         },
                         StringSplitOptions.None))
            {
                if (currentLine.Length + item.Length + 2 <=
                    maxLength) // +2 for ", "
                {
                    currentLine += item + ", ";
                }
                else
                {
                    lines.Add(
                        currentLine.TrimEnd(
                            ' ',
                            ','));
                    currentLine = item + ", ";
                }
            }

            if (!string.IsNullOrEmpty(currentLine))
            {
                lines.Add(
                    currentLine.TrimEnd(
                        ' ',
                        ','));
            }

            return lines;
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GroupToTree;

        public override Guid ComponentGuid =>
            new Guid("A7B11162-627B-455B-B9C0-963E76B36DC7");
    }
}