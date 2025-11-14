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
    public class FindNavigatorItemComponent : ArchicadAccessorComponent
    {
        public FindNavigatorItemComponent ()
          : base (
                "FindNavigatorItem",
                "FindNavigatorItem",
                "Finds a navigator item.",
                "Navigator"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddTextParameter ("TreeType", "TreeType", "The type of a navigator item tree.", GH_ParamAccess.item);
            pManager.AddTextParameter ("PublisherSetName", "PublisherSetName", "The name of the publisher set.", GH_ParamAccess.item, @default: "");
            pManager.AddTextParameter ("PathRegex", "PathRegex", "The regular expression pattern for the path of the navigator item.", GH_ParamAccess.item);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("Id", "Id", "Navigator item identifier.", GH_ParamAccess.list);
            pManager.AddTextParameter ("Prefix", "Prefix", "Navigator item prefix.", GH_ParamAccess.list);
            pManager.AddTextParameter ("Name", "Name", "Navigator item name.", GH_ParamAccess.list);
            pManager.AddTextParameter ("Path", "Path", "Navigator item path.", GH_ParamAccess.list);
            pManager.AddTextParameter ("Type", "Type", "Navigator item type.", GH_ParamAccess.list);
            pManager.AddGenericParameter ("SourceNavigatorItemId", "SourceNavigatorItemId", "Navigator item source.", GH_ParamAccess.list);
            pManager.AddGenericParameter ("DatabaseId", "DatabaseId", "DatabaseId.", GH_ParamAccess.list);
        }

        public override void AddedToDocument (GH_Document document)
        {
            base.AddedToDocument (document);

            new NavigatorTreeTypeValueList ().AddAsSource (this, 0);
        }

        private List<DatabaseIdItemObj> GetDatabaseIdsFromNavigatorItemIds (List<NavigatorIdItemObj> navItemIds)
        {
            GetDatabaseIdFromNavigatorItemIdInput input = new GetDatabaseIdFromNavigatorItemIdInput () {
                NavigatorItemIds = navItemIds
            };
            JObject inputObj = JObject.FromObject (input);
            CommandResponse response = SendArchicadAddOnCommand ("GetDatabaseIdFromNavigatorItemId", inputObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return new List<DatabaseIdItemObj> ();
            }
            GetDatabaseIdFromNavigatorItemIdOutput output = response.Result.ToObject<GetDatabaseIdFromNavigatorItemIdOutput> ();
            return output.Databases;
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

            string pathRegex = "";
            if (!DA.GetData (2, ref pathRegex)) {
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

            List<NavigatorIdItemObj> navigatorItemIdList = new List<NavigatorIdItemObj> ();
            List<string> navigatorItemPrefixList = new List<string> ();
            List<string> navigatorItemNameList = new List<string> ();
            List<string> navigatorItemPathList = new List<string> ();
            List<string> navigatorItemTypeList = new List<string> ();
            List<NavigatorIdItemObj> sourceNavigatorItemIdList = new List<NavigatorIdItemObj> ();

            Regex re = new Regex(pathRegex);
            for (int i = 0; i < navigatorItemPathTree.BranchCount; i++) {
                List<string> branch = navigatorItemPathTree.Branch (i);
                for (int j = 0; j < branch.Count; j++) {
                    if (re.IsMatch (branch[j])) {
                        navigatorItemIdList.Add (navigatorItemIdTree.Branch (i)[j]);
                        navigatorItemPrefixList.Add (navigatorItemPrefixTree.Branch (i)[j]);
                        navigatorItemNameList.Add (navigatorItemNameTree.Branch (i)[j]);
                        navigatorItemPathList.Add (branch[j]);
                        navigatorItemTypeList.Add (navigatorItemTypeTree.Branch (i)[j]);
                        sourceNavigatorItemIdList.Add (sourceNavigatorItemIdTree.Branch (i)[j]);
                    }
                }
            }

            DA.SetDataList (0, navigatorItemIdList);
            DA.SetDataList (1, navigatorItemPrefixList);
            DA.SetDataList (2, navigatorItemNameList);
            DA.SetDataList (3, navigatorItemPathList);
            DA.SetDataList (4, navigatorItemTypeList);
            DA.SetDataList (5, sourceNavigatorItemIdList);
            DA.SetDataList (6, GetDatabaseIdsFromNavigatorItemIds (navigatorItemIdList));
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.FindNavigatorItem;

        public override Guid ComponentGuid => new Guid ("d9162ee8-0d28-4dca-9c6f-19a0cceace23");
    }
}