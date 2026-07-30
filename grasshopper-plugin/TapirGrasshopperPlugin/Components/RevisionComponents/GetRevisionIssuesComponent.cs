using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.RevisionComponents
{
    public class GetRevisionIssuesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetRevisionIssues";

        public GetRevisionIssuesComponent()
            : base(
                "GetRevisionIssues",
                "Retrieve all issues of the project.",
                GroupNames.Revision)
        {
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "RevisionIssueGuids",
                "Identifier of each revision issue.");

            OutTexts(
                "Ids",
                "Id of each revision issue.");

            OutTexts(
                "Descriptions",
                "Description of each revision issue.");

            OutTexts(
                "IssueTimes",
                "Issue time of each revision issue.");

            OutTexts(
                "IssuedByUsers",
                "User who issued each revision issue.");

            OutBooleans(
                "OverrideRevisionIds",
                "True if the revision id of all included layouts is overridden.");

            OutBooleans(
                "CreateNewRevisions",
                "True if a new revision is created in all included layouts.");

            OutBooleans(
                "IsIssued",
                "True if the revision issue is issued.");

            OutIntegers(
                "MarkersVisibleSinceIndices",
                "Index the change markers are visible since.");

            OutTextTree(
                "DocumentRevisionGuids",
                "Identifiers of the document revisions of each issue (one branch per issue).");

            OutTextTree(
                "CustomSchemeKeys",
                "Custom scheme key of each custom data field (one branch per issue).");

            OutTextTree(
                "CustomSchemeValues",
                "Custom scheme value of each custom data field (one branch per issue).");
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

            var issueGuids = new List<object>();
            var ids = new List<object>();
            var descriptions = new List<object>();
            var issueTimes = new List<object>();
            var issuedByUsers = new List<object>();
            var overrideRevisionIds = new List<object>();
            var createNewRevisions = new List<object>();
            var isIssued = new List<object>();
            var markersVisibleSinceIndices = new List<object>();
            var documentRevisionGuids = new DataTree<object>();
            var customSchemeKeys = new DataTree<object>();
            var customSchemeValues = new DataTree<object>();

            var items = JsonOutputHelp.Items(
                response,
                "revisionIssues");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                documentRevisionGuids.EnsurePath(path);
                customSchemeKeys.EnsurePath(path);
                customSchemeValues.EnsurePath(path);

                issueGuids.Add(JsonOutputHelp.GuidOf(item["revisionIssueId"]));
                ids.Add(JsonOutputHelp.Scalar(item, "id"));
                descriptions.Add(JsonOutputHelp.Scalar(item, "description"));
                issueTimes.Add(JsonOutputHelp.Scalar(item, "issueTime"));
                issuedByUsers.Add(JsonOutputHelp.Scalar(item, "issuedByUser"));
                overrideRevisionIds.Add(
                    JsonOutputHelp.Scalar(item, "overrideRevisionIDOfAllIncludedLayouts"));
                createNewRevisions.Add(
                    JsonOutputHelp.Scalar(item, "createNewRevisionInAllIncludedLayouts"));
                isIssued.Add(JsonOutputHelp.Scalar(item, "isIssued"));
                markersVisibleSinceIndices.Add(
                    JsonOutputHelp.Scalar(item, "markersVisibleSinceIndex"));

                if (item["documentRevisions"] is JArray documentRevisions)
                {
                    foreach (var revision in documentRevisions)
                    {
                        documentRevisionGuids.Add(
                            JsonOutputHelp.GuidOf(revision["revisionId"]),
                            path);
                    }
                }

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

            da.SetDataList(0, issueGuids);
            da.SetDataList(1, ids);
            da.SetDataList(2, descriptions);
            da.SetDataList(3, issueTimes);
            da.SetDataList(4, issuedByUsers);
            da.SetDataList(5, overrideRevisionIds);
            da.SetDataList(6, createNewRevisions);
            da.SetDataList(7, isIssued);
            da.SetDataList(8, markersVisibleSinceIndices);
            da.SetDataTree(9, documentRevisionGuids);
            da.SetDataTree(10, customSchemeKeys);
            da.SetDataTree(11, customSchemeValues);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetRevisionIssues;

        public override Guid ComponentGuid =>
            new Guid("93ff2a2d-6342-48ab-9986-9a77cf1b1e09");
    }
}
