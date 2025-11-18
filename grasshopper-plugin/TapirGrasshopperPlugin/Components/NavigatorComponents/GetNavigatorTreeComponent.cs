using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
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
        public NavigatorIdObj NavigatorItemId;

        [JsonProperty("prefix")]
        public string Prefix;

        [JsonProperty("name")]
        public string Name;

        [JsonProperty("type")]
        public string Type;

        [JsonProperty("sourceNavigatorItemId")]
        public NavigatorIdObj SourceNavigatorItemId;

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
            DataTree<NavigatorIdItemObj> navigatorItemIdTree,
            DataTree<string> navigatorItemPrefixTree,
            DataTree<string> navigatorItemNameTree,
            DataTree<string> navigatorItemPathTree,
            DataTree<string> navigatorItemTypeTree,
            DataTree<NavigatorIdItemObj> sourceNavigatorItemIdTree)
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
            DataTree<NavigatorIdItemObj> navigatorItemIdTree,
            DataTree<string> navigatorItemPrefixTree,
            DataTree<string> navigatorItemNameTree,
            DataTree<string> navigatorItemPathTree,
            DataTree<string> navigatorItemTypeTree,
            DataTree<NavigatorIdItemObj> sourceNavigatorItemIdTree)
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
                    new NavigatorIdItemObj
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
                    new NavigatorIdItemObj
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
        public List<NavigatorIdItemObj> NavigatorItemIds;
    }

    public class GetDatabaseIdFromNavigatorItemIdOutput
    {
        [JsonProperty("databases")]
        public List<DatabaseIdItemObj> Databases;
    }

    public class GetNavigatorTreeComponent : ArchicadAccessorComponent
    {
        public static string Doc => "Get the tree structure of the navigator.";

        public override string CommandName => "GetNavigatorItemTree";

        public GetNavigatorTreeComponent()
            : base(
                "Navigator Tree",
                "NavigatorTree",
                Doc,
                GroupNames.Navigator)
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
            pManager.AddTextParameter(
                "TreeType",
                "TreeType",
                "The type of a navigator item tree.",
                GH_ParamAccess.item);
            pManager.AddTextParameter(
                "PublisherSetName",
                "PublisherSetName",
                "The name of the publisher set.",
                GH_ParamAccess.item,
                "");
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter(
                "NavigatorItemIdTree",
                "NavigatorItemIdTree",
                "Tree of Navigator item identifiers.",
                GH_ParamAccess.tree);
            pManager.AddTextParameter(
                "NavigatorItemPrefixTree",
                "NavigatorItemPrefixTree",
                "Tree of Navigator item prefixes.",
                GH_ParamAccess.tree);
            pManager.AddTextParameter(
                "NavigatorItemNameTree",
                "NavigatorItemNameTree",
                "Tree of Navigator item names.",
                GH_ParamAccess.tree);
            pManager.AddTextParameter(
                "NavigatorItemPathTree",
                "NavigatorItemPathTree",
                "Tree of Navigator item pathes.",
                GH_ParamAccess.tree);
            pManager.AddTextParameter(
                "NavigatorItemTypeTree",
                "NavigatorItemTypeTree",
                "Tree of Navigator item types.",
                GH_ParamAccess.tree);
            pManager.AddGenericParameter(
                "SourceNavigatorItemIdTree",
                "SourceNavigatorItemIdTree",
                "Tree of Navigator item sources.",
                GH_ParamAccess.tree);
            pManager.AddGenericParameter(
                "DatabaseIdTree",
                "DatabaseIdTree",
                "Tree of DatabaseIds of Navigator items.",
                GH_ParamAccess.tree);
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            base.AddedToDocument(document);

            new NavigatorTreeTypeValueList().AddAsSource(
                this,
                0);
        }

        private DataTree<DatabaseIdItemObj>
            GetDatabaseIdTreeFromNavigatorItemIdTree(
                DataTree<NavigatorIdItemObj> navItemIdTree)
        {
            var databaseIdTree = new DataTree<DatabaseIdItemObj>();

            var branches = new List<GH_Path>();
            var allItems = new List<NavigatorIdItemObj>();
            for (var i = 0; i < navItemIdTree.BranchCount; i++)
            {
                var path = navItemIdTree.Path(i);
                var items = navItemIdTree.Branch(path);
                foreach (var item in items)
                {
                    branches.Add(path);
                    allItems.Add(item);
                }
            }

            var input =
                new GetDatabaseIdFromNavigatorItemIdInput
                {
                    NavigatorItemIds = allItems
                };

            if (!GetConvertedResponse(
                    CommandName,
                    input,
                    out GetDatabaseIdFromNavigatorItemIdOutput response))
            {
                return databaseIdTree;
            }

            for (var i = 0; i < response.Databases.Count; i++)
            {
                var item = response.Databases[i];
                databaseIdTree.Add(
                    item,
                    branches[i]);
            }

            return databaseIdTree;
        }

        protected override void Solve(
            IGH_DataAccess DA)
        {
            var type = "";
            if (!DA.GetData(
                    0,
                    ref type))
            {
                return;
            }

            var name = "";
            if (!DA.GetData(
                    1,
                    ref name))
            {
                return;
            }

            var navigatorTreeId = new NavigatorTreeIdHolderObj
            {
                NavigatorTreeId = new NavigatorTreeIdObj
                {
                    Type = type,
                    Name = string.IsNullOrEmpty(name) ? null : name
                }
            };

            var navigatorTreeIdObj = JObject.FromObject(navigatorTreeId);
            var response = SendArchicadCommand(
                "GetNavigatorItemTree",
                navigatorTreeIdObj);
            if (!response.Succeeded)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    response.GetErrorMessage());
                return;
            }

            var navigatorTreeObj = response.Result.ToObject<NavigatorTreeObj>();
            var navigatorItemIdTree = new DataTree<NavigatorIdItemObj>();
            var navigatorItemPrefixTree = new DataTree<string>();
            var navigatorItemNameTree = new DataTree<string>();
            var navigatorItemPathTree = new DataTree<string>();
            var navigatorItemTypeTree = new DataTree<string>();
            var sourceNavigatorItemIdTree = new DataTree<NavigatorIdItemObj>();

            navigatorTreeObj.GetItems(
                navigatorItemIdTree,
                navigatorItemPrefixTree,
                navigatorItemNameTree,
                navigatorItemPathTree,
                navigatorItemTypeTree,
                sourceNavigatorItemIdTree);

            DA.SetDataTree(
                0,
                navigatorItemIdTree);
            DA.SetDataTree(
                1,
                navigatorItemPrefixTree);
            DA.SetDataTree(
                2,
                navigatorItemNameTree);
            DA.SetDataTree(
                3,
                navigatorItemPathTree);
            DA.SetDataTree(
                4,
                navigatorItemTypeTree);
            DA.SetDataTree(
                5,
                sourceNavigatorItemIdTree);
            DA.SetDataTree(
                6,
                GetDatabaseIdTreeFromNavigatorItemIdTree(navigatorItemIdTree));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.NavigatorTree;

        public override Guid ComponentGuid =>
            new Guid("b0173d9e-483d-4d8e-a0ac-606673c8c094");
    }
}