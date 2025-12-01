using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Navigator;

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

            InText("PublisherSetName");
        }

        protected override void AddOutputs()
        {
            OutGenericTree("NavigatorItemIds");
            OutTextTree("NavigatorItemPrefixes");
            OutTextTree("NavigatorItemNames");
            OutTextTree("NavigatorItemPaths");
            OutTextTree("NavigatorItemTypes");
            OutGenericTree("SourceNavigatorItemIds");
            OutGenericTree("DatabaseIds");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            base.AddedToDocument(document);

            new NavigatorTreeTypeValueList().AddAsSource(
                this,
                0);
        }

        private DataTree<DatabaseGuidItemObject>
            GetDatabaseIdTreeFromNavigatorItemIdTree(
                DataTree<NavigatorGuidItemObject> navItemIdTree)
        {
            var databaseIdTree = new DataTree<DatabaseGuidItemObject>();

            var branches = new List<GH_Path>();
            var allItems = new List<NavigatorGuidItemObject>();

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

            if (!TryGetConvertedValues(
                    CommandName,
                    input,
                    ToAddOn,
                    JHelp.Deserialize<GetDatabaseIdFromNavigatorItemIdOutput>,
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
            if (!da.TryGet(
                    0,
                    out string type))
            {
                return;
            }

            if (!da.TryGet(
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

            if (!TryGetConvertedValues(
                    "GetNavigatorItemTree",
                    navigatorTreeId,
                    ToArchicad,
                    JHelp.Deserialize<NavigatorTreeObj>,
                    out NavigatorTreeObj response))
            {
                return;
            }

            var navigatorItemIdTree = new DataTree<NavigatorGuidItemObject>();
            var navigatorItemPrefixTree = new DataTree<string>();
            var navigatorItemNameTree = new DataTree<string>();
            var navigatorItemPathTree = new DataTree<string>();
            var navigatorItemTypeTree = new DataTree<string>();
            var sourceNavigatorItemIdTree =
                new DataTree<NavigatorGuidItemObject>();

            response.GetItems(
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