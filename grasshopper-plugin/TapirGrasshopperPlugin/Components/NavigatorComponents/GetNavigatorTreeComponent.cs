using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Text.RegularExpressions;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class NavigatorTreeIdObj
    {
        [JsonProperty ("type")]
        public string Type;

        [JsonProperty ("name", NullValueHandling = NullValueHandling.Ignore)]
        public string Name;
    }

    public class NavigatorTreeIdHolderObj
    {
        [JsonProperty ("navigatorTreeId")]
        public NavigatorTreeIdObj NavigatorTreeId;
    }

    public class NavigatorItemObj
    {
        [JsonProperty ("navigatorItemId")]
        public NavigatorIdObj NavigatorItemId;

        [JsonProperty ("prefix")]
        public string Prefix;

        [JsonProperty ("name")]
        public string Name;

        [JsonProperty ("type")]
        public string Type;

        [JsonProperty ("sourceNavigatorItemId")]
        public NavigatorIdObj SourceNavigatorItemId;

        [JsonProperty ("children")]
        public List<NavigatorItemHolderObj> Children;
    }

    public class NavigatorItemHolderObj
    {
        [JsonProperty ("navigatorItem")]
        public NavigatorItemObj NavigatorItem;
    }

    public class RootNavigatorItemObj
    {
        [JsonProperty ("rootItem")]
        public NavigatorItemObj RootItem;
    }

    public class NavigatorTreeObj
    {
        [JsonProperty ("navigatorTree")]
        public RootNavigatorItemObj NavigatorTree;

        public void GetItems (
            DataTree<NavigatorIdItemObj> navigatorItemIdTree,
            DataTree<string> navigatorItemPrefixTree,
            DataTree<string> navigatorItemNameTree,
            DataTree<string> navigatorItemPathTree,
            DataTree<string> navigatorItemTypeTree,
            DataTree<NavigatorIdItemObj> sourceNavigatorItemIdTree)
        {
            AddChildren (NavigatorTree.RootItem, new GH_Path (0), "",
                navigatorItemIdTree,
                navigatorItemPrefixTree,
                navigatorItemNameTree,
                navigatorItemPathTree,
                navigatorItemTypeTree,
                sourceNavigatorItemIdTree);
        }

        private void AddChildren (NavigatorItemObj navItem, GH_Path path, string pathStr,
            DataTree<NavigatorIdItemObj> navigatorItemIdTree,
            DataTree<string> navigatorItemPrefixTree,
            DataTree<string> navigatorItemNameTree,
            DataTree<string> navigatorItemPathTree,
            DataTree<string> navigatorItemTypeTree,
            DataTree<NavigatorIdItemObj> sourceNavigatorItemIdTree)
        {
            if (navItem.Children == null || navItem.Children.Count == 0) {
                return;
            }

            for (int childIndex = 0; childIndex < navItem.Children.Count; childIndex++) {
                NavigatorItemObj childNavItem = navItem.Children[childIndex].NavigatorItem;

                navigatorItemIdTree.Add (new NavigatorIdItemObj () { NavigatorId = childNavItem.NavigatorItemId }, path);
                navigatorItemPrefixTree.Add (childNavItem.Prefix, path);
                navigatorItemNameTree.Add (childNavItem.Name, path);
                string fullName = string.IsNullOrEmpty (childNavItem.Prefix) || childNavItem.Prefix == childNavItem.Name ? childNavItem.Name : childNavItem.Prefix + " " + childNavItem.Name;
                string newPathStr = string.IsNullOrEmpty (pathStr) ? fullName : pathStr + "/" + fullName;
                navigatorItemPathTree.Add (newPathStr, path);
                navigatorItemTypeTree.Add (childNavItem.Type, path);
                sourceNavigatorItemIdTree.Add (new NavigatorIdItemObj () { NavigatorId = childNavItem.SourceNavigatorItemId }, path);

                GH_Path newPath = path.AppendElement (childIndex);

                AddChildren (childNavItem, newPath, newPathStr,
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
        [JsonProperty ("navigatorItemIds")]
        public List<NavigatorIdItemObj> NavigatorItemIds;
    }

    public class GetDatabaseIdFromNavigatorItemIdOutput
    {
        [JsonProperty ("databases")]
        public List<DatabaseIdItemObj> Databases;
    }

    public class GetNavigatorTreeComponent : ArchicadAccessorComponent
    {
        public GetNavigatorTreeComponent ()
          : base (
                "Navigator Tree",
                "NavigatorTree",
                "Get the tree structure of the navigator.",
                "Navigator"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddTextParameter ("TreeType", "TreeType", "The type of a navigator item tree.", GH_ParamAccess.item);
            pManager.AddTextParameter ("PublisherSetName", "PublisherSetName", "The name of the publisher set.", GH_ParamAccess.item, @default: "");
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("NavigatorItemIdTree", "NavigatorItemIdTree", "Tree of Navigator item identifiers.", GH_ParamAccess.tree);
            pManager.AddTextParameter ("NavigatorItemPrefixTree", "NavigatorItemPrefixTree", "Tree of Navigator item prefixes.", GH_ParamAccess.tree);
            pManager.AddTextParameter ("NavigatorItemNameTree", "NavigatorItemNameTree", "Tree of Navigator item names.", GH_ParamAccess.tree);
            pManager.AddTextParameter ("NavigatorItemPathTree", "NavigatorItemPathTree", "Tree of Navigator item pathes.", GH_ParamAccess.tree);
            pManager.AddTextParameter ("NavigatorItemTypeTree", "NavigatorItemTypeTree", "Tree of Navigator item types.", GH_ParamAccess.tree);
            pManager.AddGenericParameter ("SourceNavigatorItemIdTree", "SourceNavigatorItemIdTree", "Tree of Navigator item sources.", GH_ParamAccess.tree);
            pManager.AddGenericParameter ("DatabaseIdTree", "DatabaseIdTree", "Tree of DatabaseIds of Navigator items.", GH_ParamAccess.tree);
        }

        public override void AddedToDocument (GH_Document document)
        {
            base.AddedToDocument (document);

            new NavigatorTreeTypeValueList ().AddAsSource (this, 0);
        }

        private DataTree<DatabaseIdItemObj> GetDatabaseIdTreeFromNavigatorItemIdTree (DataTree<NavigatorIdItemObj> navItemIdTree)
        {
            DataTree<DatabaseIdItemObj> databaseIdTree = new DataTree<DatabaseIdItemObj> ();

            List<GH_Path> branches = new List<GH_Path> ();
            List<NavigatorIdItemObj> allItems = new List<NavigatorIdItemObj> ();
            for (int i = 0; i < navItemIdTree.BranchCount; i++) {
                GH_Path path = navItemIdTree.Path (i);
                List<NavigatorIdItemObj> items = navItemIdTree.Branch (path);
                foreach (NavigatorIdItemObj item in items) {
                    branches.Add (path);
                    allItems.Add (item);
                }
            }

            GetDatabaseIdFromNavigatorItemIdInput input = new GetDatabaseIdFromNavigatorItemIdInput () {
                NavigatorItemIds = allItems
            };
            JObject inputObj = JObject.FromObject (input);
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "GetDatabaseIdFromNavigatorItemId", inputObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return databaseIdTree;
            }
            GetDatabaseIdFromNavigatorItemIdOutput output = response.Result.ToObject<GetDatabaseIdFromNavigatorItemIdOutput> ();

            for (int i = 0; i < output.Databases.Count; i++) {
                DatabaseIdItemObj item = output.Databases[i];
                databaseIdTree.Add (item, branches[i]);
            }

            return databaseIdTree;
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            string type = "";
            if (!DA.GetData (0, ref type)) {
                return;
            }

            string name = "";
            if (!DA.GetData (1, ref name)) {
                return;
            }

            NavigatorTreeIdHolderObj navigatorTreeId = new NavigatorTreeIdHolderObj () {
                NavigatorTreeId = new NavigatorTreeIdObj () {
                    Type = type,
                    Name = string.IsNullOrEmpty (name) ? null : name
                }
            };

            JObject navigatorTreeIdObj = JObject.FromObject (navigatorTreeId);
            CommandResponse response = SendArchicadCommand ("GetNavigatorItemTree", navigatorTreeIdObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
            NavigatorTreeObj navigatorTreeObj = response.Result.ToObject<NavigatorTreeObj> ();
            DataTree<NavigatorIdItemObj> navigatorItemIdTree = new DataTree<NavigatorIdItemObj> ();
            DataTree<string> navigatorItemPrefixTree = new DataTree<string> ();
            DataTree<string> navigatorItemNameTree = new DataTree<string> ();
            DataTree<string> navigatorItemPathTree = new DataTree<string> ();
            DataTree<string> navigatorItemTypeTree = new DataTree<string> ();
            DataTree<NavigatorIdItemObj> sourceNavigatorItemIdTree = new DataTree<NavigatorIdItemObj> ();

            navigatorTreeObj.GetItems (
                navigatorItemIdTree,
                navigatorItemPrefixTree,
                navigatorItemNameTree,
                navigatorItemPathTree,
                navigatorItemTypeTree,
                sourceNavigatorItemIdTree);

            DA.SetDataTree (0, navigatorItemIdTree);
            DA.SetDataTree (1, navigatorItemPrefixTree);
            DA.SetDataTree (2, navigatorItemNameTree);
            DA.SetDataTree (3, navigatorItemPathTree);
            DA.SetDataTree (4, navigatorItemTypeTree);
            DA.SetDataTree (5, sourceNavigatorItemIdTree);
            DA.SetDataTree (6, GetDatabaseIdTreeFromNavigatorItemIdTree (navigatorItemIdTree));
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.NavigatorTree;

        public override Guid ComponentGuid => new Guid ("b0173d9e-483d-4d8e-a0ac-606673c8c094");
    }
}