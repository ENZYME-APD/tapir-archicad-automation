using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

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

            InTextWithDefault(
                "PublisherSetName",
                defaultValue: "");

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutGenericTree("Ids");
            OutTextTree("Prefixes");
            OutTextTree("Names");
            OutTextTree("Paths");
            OutTextTree("Types");
            OutGenericTree("SourceIds");
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

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string type))
            {
                return;
            }

            var name = da.GetOptional(
                1,
                "");

            if (!TryGetConvertedCadValues(
                    "GetNavigatorItemTree",
                    new
                    {
                        navigatorTreeId = new NavigatorTreeIdObj
                        {
                            Type = type,
                            Name = string.IsNullOrEmpty(name)
                                ? null
                                : name
                        }
                    },
                    ToArchicad,
                    JHelp.Deserialize<NavigatorTreeObj>,
                    out NavigatorTreeObj response))
            {
                return;
            }

            var idTree = new DataTree<NavigatorGuidWrapper>();
            var prefixTree = new DataTree<string>();
            var nameTree = new DataTree<string>();
            var pathTree = new DataTree<string>();
            var typeTree = new DataTree<string>();
            var sourceTree = new DataTree<NavigatorGuidWrapper>();

            response.GetItems(
                idTree,
                prefixTree,
                nameTree,
                pathTree,
                typeTree,
                sourceTree);

            da.SetDataTree(
                0,
                idTree);
            da.SetDataTree(
                1,
                prefixTree);
            da.SetDataTree(
                2,
                nameTree);
            da.SetDataTree(
                3,
                pathTree);
            da.SetDataTree(
                4,
                typeTree);
            da.SetDataTree(
                5,
                sourceTree);
            da.SetDataTree(
                6,
                GetDatabaseIdTree(idTree));
        }

        private DataTree<DatabaseGuidWrapper> GetDatabaseIdTree(
            DataTree<NavigatorGuidWrapper> wrapperTree)
        {
            var tree = new DataTree<DatabaseGuidWrapper>();

            var branches = new List<GH_Path>();
            var navigatorItemIds = new List<NavigatorGuidWrapper>();

            for (var i = 0; i < wrapperTree.BranchCount; i++)
            {
                var path = wrapperTree.Path(i);
                var items = wrapperTree.Branch(path);

                foreach (var item in items)
                {
                    branches.Add(path);
                    navigatorItemIds.Add(item);
                }
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    new { navigatorItemIds },
                    ToAddOn,
                    JHelp.Deserialize<DatabaseIdFromNavigatorWrapperResponse>,
                    out DatabaseIdFromNavigatorWrapperResponse response))
            {
                return tree;
            }

            for (var i = 0; i < response.Databases.Count; i++)
            {
                var item = response.Databases[i];
                tree.Add(
                    item,
                    branches[i]);
            }

            return tree;
        }


        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.NavigatorTree;

        public override Guid ComponentGuid =>
            new Guid("b0173d9e-483d-4d8e-a0ac-606673c8c094");
    }
}