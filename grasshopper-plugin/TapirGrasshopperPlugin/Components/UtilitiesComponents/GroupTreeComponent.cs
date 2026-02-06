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
        public GroupTreeComponent()
            : base(
                "GroupToTree",
                "Groups data items into a tree structure based on values",
                GroupNames.Utilities)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "DataList",
                "List of data items to group");

            InGenerics(
                "ValuesList",
                "List of values to group by");
        }

        protected override void AddOutputs()
        {
            OutGenericTree(
                "OutputTree",
                "Data tree with items grouped by values");

            OutGenerics(
                "UniqueValues",
                "List of unique values used for grouping");

            OutIntegers(
                "BranchCounts",
                "Number of items in each branch");
        }

        protected override void SolveInstance(
            IGH_DataAccess da)
        {
            // Initialize outputs
            var outputTree = new GH_Structure<IGH_Goo>();
            var uniqueValues = new List<IGH_Goo>();
            var branchCounts = new List<int>();

            // Get inputs
            var dataList = new List<IGH_Goo>();
            if (!da.GetDataList(
                    0,
                    dataList) || dataList.Count == 0)
            {
                Message = "Error: Both data list and values list are required";
                da.SetDataTree(
                    0,
                    outputTree);
                da.SetDataList(
                    1,
                    uniqueValues);
                da.SetDataList(
                    2,
                    branchCounts);
                return;
            }

            var valuesList = new List<IGH_Goo>();
            if (!da.GetDataList(
                    1,
                    valuesList) || valuesList.Count == 0)
            {
                Message = "Error: Both data list and values list are required";
                da.SetDataTree(
                    0,
                    outputTree);
                da.SetDataList(
                    1,
                    uniqueValues);
                da.SetDataList(
                    2,
                    branchCounts);
                return;
            }

            // Validate lists have same length
            if (dataList.Count != valuesList.Count)
            {
                Message =
                    $"Error: Lists have different lengths. Data: {dataList.Count}, Values: {valuesList.Count}";
                da.SetDataTree(
                    0,
                    outputTree);
                da.SetDataList(
                    1,
                    uniqueValues);
                da.SetDataList(
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
                    var valueKey = (valuesList[i] == null) ? "null" : valuesList[i].ToString();
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
                message += "Items per branch:";

                var maxItemsToDump = 10;
                var maxLineLength = 40;
                for (var i = 0; i < Math.Min(uniqueValues.Count, maxItemsToDump); i++)
                {
                    var valueKey = (uniqueValues[i] == null) ? "null" : uniqueValues[i].ToString();
                    var itemMessage = $"{valueKey}({branchCounts[i]})";
                    if (itemMessage.Length > maxLineLength)
                    {
                        itemMessage = itemMessage.Substring(0, maxLineLength);
                    }
                    message += "\n" + itemMessage;
                }
            }
            catch (Exception e)
            {
                message = $"Error: {e.Message}";
                outputTree = new GH_Structure<IGH_Goo>();
                uniqueValues.Clear();
                branchCounts.Clear();
            }

            // Set outputs
            da.SetDataTree(
                0,
                outputTree);
            da.SetDataList(
                1,
                uniqueValues);
            da.SetDataList(
                2,
                branchCounts);

            // Update component message
            Message = message;
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GroupToTree;

        public override Guid ComponentGuid =>
            new Guid("A7B11162-627B-455B-B9C0-963E76B36DC7");
    }
}