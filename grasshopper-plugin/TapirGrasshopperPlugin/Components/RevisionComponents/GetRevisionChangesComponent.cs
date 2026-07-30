using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.RevisionComponents
{
    public class GetRevisionChangesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetRevisionChanges";

        public GetRevisionChangesComponent()
            : base(
                "GetRevisionChanges",
                "Retrieve all changes of the project.",
                GroupNames.Revision)
        {
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Ids",
                "Id of each revision change.");

            OutTexts(
                "Descriptions",
                "Description of each revision change.");

            OutTexts(
                "LastModifiedTimes",
                "Last modification time of each revision change.");

            OutTexts(
                "ModifiedByUsers",
                "User who last modified each revision change.");

            OutBooleans(
                "IsIssued",
                "True if the revision change is issued.");

            OutBooleans(
                "IsArchived",
                "True if the revision change is archived.");

            OutTexts(
                "FirstRevisionIssueGuids",
                "Identifier of the first revision issue of each change.");

            OutTextTree(
                "CustomSchemeKeys",
                "Custom scheme key of each custom data field (one branch per change).");

            OutTextTree(
                "CustomSchemeValues",
                "Custom scheme value of each custom data field (one branch per change).");
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

            var ids = new List<object>();
            var descriptions = new List<object>();
            var lastModifiedTimes = new List<object>();
            var modifiedByUsers = new List<object>();
            var isIssued = new List<object>();
            var isArchived = new List<object>();
            var firstRevisionIssueGuids = new List<object>();
            var customSchemeKeys = new DataTree<object>();
            var customSchemeValues = new DataTree<object>();

            var items = JsonOutputHelp.Items(
                response,
                "revisionChanges");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                customSchemeKeys.EnsurePath(path);
                customSchemeValues.EnsurePath(path);

                ids.Add(JsonOutputHelp.Scalar(item, "id"));
                descriptions.Add(JsonOutputHelp.Scalar(item, "description"));
                lastModifiedTimes.Add(JsonOutputHelp.Scalar(item, "lastModifiedTime"));
                modifiedByUsers.Add(JsonOutputHelp.Scalar(item, "modifiedByUser"));
                isIssued.Add(JsonOutputHelp.Scalar(item, "isIssued"));
                isArchived.Add(JsonOutputHelp.Scalar(item, "isArchived"));
                firstRevisionIssueGuids.Add(
                    JsonOutputHelp.GuidOf(item["firstRevisionIssueId"]));

                if (item["customSchemeData"] is JArray customSchemeData)
                {
                    foreach (var field in customSchemeData)
                    {
                        customSchemeKeys.Add(
                            JsonOutputHelp.Scalar(field, "customSchemeKey"),
                            path);
                        customSchemeValues.Add(
                            JsonOutputHelp.Scalar(field, "customSchemeValue"),
                            path);
                    }
                }
            }

            da.SetDataList(0, ids);
            da.SetDataList(1, descriptions);
            da.SetDataList(2, lastModifiedTimes);
            da.SetDataList(3, modifiedByUsers);
            da.SetDataList(4, isIssued);
            da.SetDataList(5, isArchived);
            da.SetDataList(6, firstRevisionIssueGuids);
            da.SetDataTree(7, customSchemeKeys);
            da.SetDataTree(8, customSchemeValues);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetRevisionChanges;

        public override Guid ComponentGuid =>
            new Guid("335b9748-401a-4de8-8bb1-b6b83a7e5d17");
    }
}
