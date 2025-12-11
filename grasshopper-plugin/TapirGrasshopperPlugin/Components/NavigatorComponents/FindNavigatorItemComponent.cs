using Grasshopper;
using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Text.RegularExpressions;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class FindNavigatorItemComponent : ArchicadAccessorComponent
    {
        public override string CommandName =>
            "GetDatabaseIdFromNavigatorItemId";

        public FindNavigatorItemComponent()
            : base(
                "FindNavigatorItem",
                "Finds a navigator item.",
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

            InText(
                "PathRegex",
                "The regular expression pattern for the path of the navigator item.");

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "Id",
                "Navigator item identifier.");

            OutTexts(
                "Prefix",
                "Navigator item prefix.");

            OutTexts(
                "Path",
                "Navigator item path.");

            OutTexts(
                "Name",
                "Navigator item name.");

            OutTexts(
                "Type",
                "Navigator item type.");

            OutGenerics(
                "SourceNavigatorItemId",
                "Navigator item source.");

            OutGenerics(
                "DatabaseId",
                "DatabaseId.");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            base.AddedToDocument(document);

            new NavigatorTreeTypeValueList().AddAsSource(
                this,
                0);
        }

        private List<DatabaseGuidWrapper> GetDatabaseIdsFromNavigatorItemIds(
            List<NavigatorGuidWrapper> navigatorItemIds)
        {
            if (!TryGetConvertedCadValues(
                    "GetDatabaseIdFromNavigatorItemId",
                    new { navigatorItemIds },
                    ToAddOn,
                    JHelp.Deserialize<DatabaseIdFromNavigatorWrapperResponse>,
                    out DatabaseIdFromNavigatorWrapperResponse response))
            {
                return new List<DatabaseGuidWrapper>();
            }

            return response.Databases;
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

            if (!da.TryGet(
                    2,
                    out string pathRegex))
            {
                return;
            }

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

            var navigatorItemIdTree = new DataTree<NavigatorGuidWrapper>();
            var navigatorItemPrefixTree = new DataTree<string>();
            var navigatorItemNameTree = new DataTree<string>();
            var navigatorItemPathTree = new DataTree<string>();
            var navigatorItemTypeTree = new DataTree<string>();
            var sourceNavigatorItemIdTree =
                new DataTree<NavigatorGuidWrapper>();

            response.GetItems(
                navigatorItemIdTree,
                navigatorItemPrefixTree,
                navigatorItemNameTree,
                navigatorItemPathTree,
                navigatorItemTypeTree,
                sourceNavigatorItemIdTree);

            var navigatorItemIdList = new List<NavigatorGuidWrapper>();
            var navigatorItemPrefixList = new List<string>();
            var navigatorItemNameList = new List<string>();
            var navigatorItemPathList = new List<string>();
            var navigatorItemTypeList = new List<string>();
            var sourceNavigatorItemIdList = new List<NavigatorGuidWrapper>();

            var re = new Regex(pathRegex);
            for (var i = 0; i < navigatorItemPathTree.BranchCount; i++)
            {
                var branch = navigatorItemPathTree.Branch(i);
                for (var j = 0; j < branch.Count; j++)
                {
                    if (re.IsMatch(branch[j]))
                    {
                        navigatorItemIdList.Add(
                            navigatorItemIdTree.Branch(i)[j]);
                        navigatorItemPrefixList.Add(
                            navigatorItemPrefixTree.Branch(i)[j]);
                        navigatorItemNameList.Add(
                            navigatorItemNameTree.Branch(i)[j]);
                        navigatorItemPathList.Add(branch[j]);
                        navigatorItemTypeList.Add(
                            navigatorItemTypeTree.Branch(i)[j]);
                        sourceNavigatorItemIdList.Add(
                            sourceNavigatorItemIdTree.Branch(i)[j]);
                    }
                }
            }

            da.SetDataList(
                0,
                navigatorItemIdList);
            da.SetDataList(
                1,
                navigatorItemPrefixList);
            da.SetDataList(
                2,
                navigatorItemNameList);
            da.SetDataList(
                3,
                navigatorItemPathList);
            da.SetDataList(
                4,
                navigatorItemTypeList);
            da.SetDataList(
                5,
                sourceNavigatorItemIdList);
            da.SetDataList(
                6,
                GetDatabaseIdsFromNavigatorItemIds(navigatorItemIdList));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.FindNavigatorItem;

        public override Guid ComponentGuid =>
            new Guid("d9162ee8-0d28-4dca-9c6f-19a0cceace23");
    }
}