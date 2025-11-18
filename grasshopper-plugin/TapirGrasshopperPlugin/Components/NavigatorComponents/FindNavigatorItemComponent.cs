using Grasshopper;
using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Text.RegularExpressions;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class FindNavigatorItemComponent : ArchicadAccessorComponent
    {
        public static string Doc => "Finds a navigator item.";

        public override string CommandName =>
            "GetDatabaseIdFromNavigatorItemId";

        public FindNavigatorItemComponent()
            : base(
                "FindNavigatorItem",
                "FindNavigatorItem",
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
                @default: "");
            pManager.AddTextParameter(
                "PathRegex",
                "PathRegex",
                "The regular expression pattern for the path of the navigator item.",
                GH_ParamAccess.item);
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter(
                "Id",
                "Id",
                "Navigator item identifier.",
                GH_ParamAccess.list);
            pManager.AddTextParameter(
                "Prefix",
                "Prefix",
                "Navigator item prefix.",
                GH_ParamAccess.list);
            pManager.AddTextParameter(
                "Name",
                "Name",
                "Navigator item name.",
                GH_ParamAccess.list);
            pManager.AddTextParameter(
                "Path",
                "Path",
                "Navigator item path.",
                GH_ParamAccess.list);
            pManager.AddTextParameter(
                "Type",
                "Type",
                "Navigator item type.",
                GH_ParamAccess.list);
            pManager.AddGenericParameter(
                "SourceNavigatorItemId",
                "SourceNavigatorItemId",
                "Navigator item source.",
                GH_ParamAccess.list);
            pManager.AddGenericParameter(
                "DatabaseId",
                "DatabaseId",
                "DatabaseId.",
                GH_ParamAccess.list);
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            base.AddedToDocument(document);

            new NavigatorTreeTypeValueList().AddAsSource(
                this,
                0);
        }

        private List<DatabaseIdItemObj> GetDatabaseIdsFromNavigatorItemIds(
            List<NavigatorIdItemObj> navItemIds)
        {
            var input =
                new GetDatabaseIdFromNavigatorItemIdInput()
                {
                    NavigatorItemIds = navItemIds
                };
            var inputObj = JObject.FromObject(input);
            var response = SendArchicadAddOnCommand(
                "GetDatabaseIdFromNavigatorItemId",
                inputObj);
            if (!response.Succeeded)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    response.GetErrorMessage());
                return new List<DatabaseIdItemObj>();
            }

            var output = response.Result
                .ToObject<GetDatabaseIdFromNavigatorItemIdOutput>();
            return output.Databases;
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

            var pathRegex = "";
            if (!DA.GetData(
                    2,
                    ref pathRegex))
            {
                return;
            }

            var navigatorTreeId = new NavigatorTreeIdHolderObj()
            {
                NavigatorTreeId = new NavigatorTreeIdObj()
                {
                    Type = type,
                    Name = string.IsNullOrEmpty(name) ? null : name
                }
            };

            if (!GetConvertedResponse(
                    CommandName,
                    navigatorTreeId,
                    out NavigatorTreeObj response))
            {
                return;
            }

            var navigatorItemIdTree = new DataTree<NavigatorIdItemObj>();
            var navigatorItemPrefixTree = new DataTree<string>();
            var navigatorItemNameTree = new DataTree<string>();
            var navigatorItemPathTree = new DataTree<string>();
            var navigatorItemTypeTree = new DataTree<string>();
            var sourceNavigatorItemIdTree = new DataTree<NavigatorIdItemObj>();

            response.GetItems(
                navigatorItemIdTree,
                navigatorItemPrefixTree,
                navigatorItemNameTree,
                navigatorItemPathTree,
                navigatorItemTypeTree,
                sourceNavigatorItemIdTree);

            var navigatorItemIdList = new List<NavigatorIdItemObj>();
            var navigatorItemPrefixList = new List<string>();
            var navigatorItemNameList = new List<string>();
            var navigatorItemPathList = new List<string>();
            var navigatorItemTypeList = new List<string>();
            var sourceNavigatorItemIdList = new List<NavigatorIdItemObj>();

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

            DA.SetDataList(
                0,
                navigatorItemIdList);
            DA.SetDataList(
                1,
                navigatorItemPrefixList);
            DA.SetDataList(
                2,
                navigatorItemNameList);
            DA.SetDataList(
                3,
                navigatorItemPathList);
            DA.SetDataList(
                4,
                navigatorItemTypeList);
            DA.SetDataList(
                5,
                sourceNavigatorItemIdList);
            DA.SetDataList(
                6,
                GetDatabaseIdsFromNavigatorItemIds(navigatorItemIdList));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.FindNavigatorItem;

        public override Guid ComponentGuid =>
            new Guid("d9162ee8-0d28-4dca-9c6f-19a0cceace23");
    }
}