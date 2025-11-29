using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Navigator;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetNavigatorTreeComponent : ArchicadAccessorComponent
    {
        public static string Doc => "Get the tree structure of the navigator.";

        public override string CommandName =>
            "GetDatabaseIdFromNavigatorItemId";

        public GetNavigatorTreeComponent()
            : base(
                "NavigatorTree",
                "Get the tree structure of the navigator.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "TreeType",
                "The type of a navigator item tree.");

            InText(
                "PublisherSetName",
                "The name of the publisher set.");
        }

        protected override void AddOutputs()
        {
            OutGenericTree(
                "NavigatorItemIds",
                "Navigator item identifiers.");

            OutTextTree(
                "NavigatorItemPrefixes",
                "Navigator item prefixes.");

            OutTextTree(
                "NavigatorItemNames",
                "Navigator item names.");

            OutTextTree(
                "NavigatorItemPaths",
                "Navigator item paths.");

            OutTextTree(
                "NavigatorItemTypes",
                "Navigator item types.");

            OutGenericTree(
                "SourceNavigatorItemIds",
                "Navigator item sources.");

            OutGenericTree(
                "DatabaseIds",
                "DatabaseIds of Navigator items.");
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

            if (!TryGetConvertedResponse(
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
            IGH_DataAccess da)
        {
            if (!da.TryGetItem(
                    0,
                    out string type))
            {
                return;
            }

            if (!da.TryGetItem(
                    1,
                    out string name))
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
            CommandResponse response = SendArchicadCommand(
                "GetNavigatorItemTree",
                navigatorTreeIdObj);
            if (!response.Succeeded)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    response.GetErrorMessage());
                return;
            }

            NavigatorTreeObj navigatorTreeObj =
                response.Result.ToObject<NavigatorTreeObj>();

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

            da.SetDataTree(
                0,
                navigatorItemIdTree);
            da.SetDataTree(
                1,
                navigatorItemPrefixTree);
            da.SetDataTree(
                2,
                navigatorItemNameTree);
            da.SetDataTree(
                3,
                navigatorItemPathTree);
            da.SetDataTree(
                4,
                navigatorItemTypeTree);
            da.SetDataTree(
                5,
                sourceNavigatorItemIdTree);
            da.SetDataTree(
                6,
                GetDatabaseIdTreeFromNavigatorItemIdTree(navigatorItemIdTree));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.NavigatorTree;

        public override Guid ComponentGuid =>
            new Guid("b0173d9e-483d-4d8e-a0ac-606673c8c094");
    }
}