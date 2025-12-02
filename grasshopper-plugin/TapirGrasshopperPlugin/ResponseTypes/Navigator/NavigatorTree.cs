using Grasshopper;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.ResponseTypes.Navigator
{
    public class NavigatorTreeIdObj
    {
        [JsonProperty("type")]
        public string Type;

        [JsonProperty(
            "name",
            NullValueHandling = NullValueHandling.Ignore)]
        public string Name;
    }

    public class NavigatorTreeIdHolderObj
    {
        [JsonProperty("navigatorTreeId")]
        public NavigatorTreeIdObj NavigatorTreeId;
    }

    public class NavigatorItemObj
    {
        [JsonProperty("navigatorItemId")]
        public NavigatorGuid NavigatorItemId;

        [JsonProperty("prefix")]
        public string Prefix;

        [JsonProperty("name")]
        public string Name;

        [JsonProperty("type")]
        public string Type;

        [JsonProperty("sourceNavigatorItemId")]
        public NavigatorGuid SourceNavigatorItemId;

        [JsonProperty("children")]
        public List<NavigatorItemHolderObj> Children;
    }

    public class NavigatorItemHolderObj
    {
        [JsonProperty("navigatorItem")]
        public NavigatorItemObj NavigatorItem;
    }

    public class RootNavigatorItemObj
    {
        [JsonProperty("rootItem")]
        public NavigatorItemObj RootItem;
    }

    public class NavigatorTreeObj
    {
        [JsonProperty("navigatorTree")]
        public RootNavigatorItemObj NavigatorTree;

        public void GetItems(
            DataTree<NavigatorGuidWrapper> navigatorItemIdTree,
            DataTree<string> navigatorItemPrefixTree,
            DataTree<string> navigatorItemNameTree,
            DataTree<string> navigatorItemPathTree,
            DataTree<string> navigatorItemTypeTree,
            DataTree<NavigatorGuidWrapper> sourceNavigatorItemIdTree)
        {
            AddChildren(
                NavigatorTree.RootItem,
                new GH_Path(0),
                "",
                navigatorItemIdTree,
                navigatorItemPrefixTree,
                navigatorItemNameTree,
                navigatorItemPathTree,
                navigatorItemTypeTree,
                sourceNavigatorItemIdTree);
        }

        private void AddChildren(
            NavigatorItemObj navItem,
            GH_Path path,
            string pathStr,
            DataTree<NavigatorGuidWrapper> navigatorItemIdTree,
            DataTree<string> navigatorItemPrefixTree,
            DataTree<string> navigatorItemNameTree,
            DataTree<string> navigatorItemPathTree,
            DataTree<string> navigatorItemTypeTree,
            DataTree<NavigatorGuidWrapper> sourceNavigatorItemIdTree)
        {
            if (navItem.Children == null || navItem.Children.Count == 0)
            {
                return;
            }

            for (var childIndex = 0;
                 childIndex < navItem.Children.Count;
                 childIndex++)
            {
                var childNavItem = navItem.Children[childIndex].NavigatorItem;

                navigatorItemIdTree.Add(
                    new NavigatorGuidWrapper
                    {
                        NavigatorId = childNavItem.NavigatorItemId
                    },
                    path);

                navigatorItemPrefixTree.Add(
                    childNavItem.Prefix,
                    path);

                navigatorItemNameTree.Add(
                    childNavItem.Name,
                    path);

                var fullName =
                    string.IsNullOrEmpty(childNavItem.Prefix) ||
                    childNavItem.Prefix == childNavItem.Name
                        ? childNavItem.Name
                        : childNavItem.Prefix + " " + childNavItem.Name;

                var newPathStr = string.IsNullOrEmpty(pathStr)
                    ? fullName
                    : pathStr + "/" + fullName;

                navigatorItemPathTree.Add(
                    newPathStr,
                    path);

                navigatorItemTypeTree.Add(
                    childNavItem.Type,
                    path);

                sourceNavigatorItemIdTree.Add(
                    new NavigatorGuidWrapper
                    {
                        NavigatorId = childNavItem.SourceNavigatorItemId
                    },
                    path);

                var newPath = path.AppendElement(childIndex);

                AddChildren(
                    childNavItem,
                    newPath,
                    newPathStr,
                    navigatorItemIdTree,
                    navigatorItemPrefixTree,
                    navigatorItemNameTree,
                    navigatorItemPathTree,
                    navigatorItemTypeTree,
                    sourceNavigatorItemIdTree);
            }
        }
    }

    public class GetDatabaseIdFromNavigatorItemIdInput
    {
        [JsonProperty("navigatorItemIds")]
        public List<NavigatorGuidWrapper> NavigatorItemIds;
    }

    public class GetDatabaseIdFromNavigatorItemIdOutput
    {
        [JsonProperty("databases")]
        public List<DatabaseGuidWrapper> Databases;
    }
}