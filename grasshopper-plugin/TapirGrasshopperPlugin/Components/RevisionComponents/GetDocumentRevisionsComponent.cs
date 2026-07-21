using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.RevisionComponents
{
    public class GetDocumentRevisionsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetDocumentRevisions";

        public GetDocumentRevisionsComponent()
            : base(
                "GetDocumentRevisions",
                "Retrieve all document revisions of the project.",
                GroupNames.Revision)
        {
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "RevisionGuids",
                "Identifier of each document revision.");

            OutTexts(
                "Ids",
                "Id of each document revision.");

            OutTexts(
                "FinalIds",
                "Final id of each document revision.");

            OutTexts(
                "OwnerUsers",
                "Owner user of each document revision.");

            OutTexts(
                "Statuses",
                "Status of each document revision: Actual or Issued.");

            OutTextTree(
                "ChangeIds",
                "Ids of the changes of each document revision (one branch per revision).");

            OutTexts(
                "LayoutIds",
                "Layout id of each document revision.");

            OutGenerics(
                "LayoutDatabaseGuids",
                "Layout database identifier of each document revision.");

            OutTexts(
                "LayoutNames",
                "Layout name of each document revision.");

            OutTexts(
                "MasterLayoutNames",
                "Master layout name of each document revision.");

            OutNumberList(
                "LayoutWidths",
                "Layout width of each document revision in mm.");

            OutNumberList(
                "LayoutHeights",
                "Layout height of each document revision in mm.");

            OutTexts(
                "SubsetIds",
                "Subset id of each document revision.");

            OutTexts(
                "SubsetNames",
                "Subset name of each document revision.");

            OutTexts(
                "LayoutOwnerUsers",
                "Owner user of the layout of each document revision.");

            OutTextTree(
                "LayoutCustomSchemeKeys",
                "Custom scheme key of each layout custom data field (one branch per revision).");

            OutTextTree(
                "LayoutCustomSchemeValues",
                "Custom scheme value of each layout custom data field (one branch per revision).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var revisionGuids = new List<object>();
            var ids = new List<object>();
            var finalIds = new List<object>();
            var ownerUsers = new List<object>();
            var statuses = new List<object>();
            var changeIds = new DataTree<object>();
            var layoutIds = new List<object>();
            var layoutDatabaseGuids = new List<object>();
            var layoutNames = new List<object>();
            var masterLayoutNames = new List<object>();
            var layoutWidths = new List<object>();
            var layoutHeights = new List<object>();
            var subsetIds = new List<object>();
            var subsetNames = new List<object>();
            var layoutOwnerUsers = new List<object>();
            var layoutCustomSchemeKeys = new DataTree<object>();
            var layoutCustomSchemeValues = new DataTree<object>();

            var items = JsonOutputHelp.Items(
                response,
                "documentRevisions");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                changeIds.EnsurePath(path);
                layoutCustomSchemeKeys.EnsurePath(path);
                layoutCustomSchemeValues.EnsurePath(path);

                revisionGuids.Add(JsonOutputHelp.GuidOf(item["revisionId"]));
                ids.Add(JsonOutputHelp.Scalar(item, "id"));
                finalIds.Add(JsonOutputHelp.Scalar(item, "finalId"));
                ownerUsers.Add(JsonOutputHelp.Scalar(item, "ownerUser"));
                statuses.Add(JsonOutputHelp.Scalar(item, "status"));

                if (item["changes"] is JArray changes)
                {
                    foreach (var change in changes)
                    {
                        changeIds.Add(
                            JsonOutputHelp.Scalar(change, "id"),
                            path);
                    }
                }

                var layoutInfo = item["layoutInfo"];
                layoutIds.Add(JsonOutputHelp.Scalar(layoutInfo, "id"));
                var databaseGuid = JsonOutputHelp.WrappedGuidOf(
                    layoutInfo,
                    "databaseId");
                layoutDatabaseGuids.Add(
                    databaseGuid == null
                        ? null
                        : new DatabaseGuidWrapper
                        {
                            DatabaseId = new DatabaseGuidObject { Guid = databaseGuid }
                        });
                layoutNames.Add(JsonOutputHelp.Scalar(layoutInfo, "name"));
                masterLayoutNames.Add(JsonOutputHelp.Scalar(layoutInfo, "masterLayoutName"));
                layoutWidths.Add(JsonOutputHelp.Scalar(layoutInfo, "width"));
                layoutHeights.Add(JsonOutputHelp.Scalar(layoutInfo, "height"));
                subsetIds.Add(JsonOutputHelp.Scalar(layoutInfo, "subsetId"));
                subsetNames.Add(JsonOutputHelp.Scalar(layoutInfo, "subsetName"));
                layoutOwnerUsers.Add(JsonOutputHelp.Scalar(layoutInfo, "ownerUser"));

                if (layoutInfo?["customSchemeData"] is JArray customSchemeData)
                {
                    foreach (var field in customSchemeData)
                    {
                        layoutCustomSchemeKeys.Add(
                            JsonOutputHelp.Scalar(field, "customSchemeKey"),
                            path);
                        layoutCustomSchemeValues.Add(
                            JsonOutputHelp.Scalar(field, "customSchemeValue"),
                            path);
                    }
                }
            }

            da.SetDataList(0, revisionGuids);
            da.SetDataList(1, ids);
            da.SetDataList(2, finalIds);
            da.SetDataList(3, ownerUsers);
            da.SetDataList(4, statuses);
            da.SetDataTree(5, changeIds);
            da.SetDataList(6, layoutIds);
            da.SetDataList(7, layoutDatabaseGuids);
            da.SetDataList(8, layoutNames);
            da.SetDataList(9, masterLayoutNames);
            da.SetDataList(10, layoutWidths);
            da.SetDataList(11, layoutHeights);
            da.SetDataList(12, subsetIds);
            da.SetDataList(13, subsetNames);
            da.SetDataList(14, layoutOwnerUsers);
            da.SetDataTree(15, layoutCustomSchemeKeys);
            da.SetDataTree(16, layoutCustomSchemeValues);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetDocumentRevisions;

        public override Guid ComponentGuid =>
            new Guid("866db71d-ff77-4077-9bde-ad08ae5c6822");
    }
}
