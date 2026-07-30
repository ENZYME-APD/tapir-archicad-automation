using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.RevisionComponents
{
    // Shared base for the commands that return the revision changes of a list
    // of entities (layouts or elements). Change outputs have one branch per
    // queried entity; custom scheme outputs have one branch per entity and
    // change.
    public abstract class RevisionChangesOfEntitiesComponent : ArchicadAccessorComponent
    {
        protected RevisionChangesOfEntitiesComponent(
            string name,
            string description)
            : base(
                name,
                description,
                GroupNames.Revision)
        {
        }

        protected override void AddOutputs()
        {
            OutTextTree(
                "ChangeIds",
                "Id of each revision change (one branch per queried entity).");

            OutTextTree(
                "ChangeDescriptions",
                "Description of each revision change (one branch per queried entity).");

            OutTextTree(
                "ChangeLastModifiedTimes",
                "Last modification time of each revision change (one branch per queried entity).");

            OutTextTree(
                "ChangeModifiedByUsers",
                "User who last modified each revision change (one branch per queried entity).");

            OutBooleanTree(
                "ChangeIsIssued",
                "True if the revision change is issued (one branch per queried entity).");

            OutBooleanTree(
                "ChangeIsArchived",
                "True if the revision change is archived (one branch per queried entity).");

            OutTextTree(
                "ChangeFirstRevisionIssueGuids",
                "Identifier of the first revision issue of each change (one branch per queried entity).");

            OutTextTree(
                "ChangeCustomSchemeKeys",
                "Custom scheme key of each custom data field (one branch per entity and change).");

            OutTextTree(
                "ChangeCustomSchemeValues",
                "Custom scheme value of each custom data field (one branch per entity and change).");

            OutTexts(
                "ErrorMessages",
                "Error message for each queried entity (empty when successful).");
        }

        protected void SetChangeOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var changeIds = new DataTree<object>();
            var changeDescriptions = new DataTree<object>();
            var changeLastModifiedTimes = new DataTree<object>();
            var changeModifiedByUsers = new DataTree<object>();
            var changeIsIssued = new DataTree<object>();
            var changeIsArchived = new DataTree<object>();
            var changeFirstRevisionIssueGuids = new DataTree<object>();
            var changeCustomSchemeKeys = new DataTree<object>();
            var changeCustomSchemeValues = new DataTree<object>();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                changeIds.EnsurePath(path);
                changeDescriptions.EnsurePath(path);
                changeLastModifiedTimes.EnsurePath(path);
                changeModifiedByUsers.EnsurePath(path);
                changeIsIssued.EnsurePath(path);
                changeIsArchived.EnsurePath(path);
                changeFirstRevisionIssueGuids.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");

                if (item["revisionChanges"] is JArray changes)
                {
                    for (var j = 0; j < changes.Count; j++)
                    {
                        var change = changes[j];
                        changeIds.Add(
                            JsonOutputHelp.Scalar(change, "id"),
                            path);
                        changeDescriptions.Add(
                            JsonOutputHelp.Scalar(change, "description"),
                            path);
                        changeLastModifiedTimes.Add(
                            JsonOutputHelp.Scalar(change, "lastModifiedTime"),
                            path);
                        changeModifiedByUsers.Add(
                            JsonOutputHelp.Scalar(change, "modifiedByUser"),
                            path);
                        changeIsIssued.Add(
                            JsonOutputHelp.Scalar(change, "isIssued"),
                            path);
                        changeIsArchived.Add(
                            JsonOutputHelp.Scalar(change, "isArchived"),
                            path);
                        changeFirstRevisionIssueGuids.Add(
                            JsonOutputHelp.GuidOf(change["firstRevisionIssueId"]),
                            path);

                        var changePath = new GH_Path(i, j);
                        changeCustomSchemeKeys.EnsurePath(changePath);
                        changeCustomSchemeValues.EnsurePath(changePath);
                        if (change["customSchemeData"] is JArray customSchemeData)
                        {
                            foreach (var field in customSchemeData)
                            {
                                changeCustomSchemeKeys.Add(
                                    JsonOutputHelp.Scalar(field, "customSchemeKey"),
                                    changePath);
                                changeCustomSchemeValues.Add(
                                    JsonOutputHelp.Scalar(field, "customSchemeValue"),
                                    changePath);
                            }
                        }
                    }
                }
            }

            da.SetDataTree(0, changeIds);
            da.SetDataTree(1, changeDescriptions);
            da.SetDataTree(2, changeLastModifiedTimes);
            da.SetDataTree(3, changeModifiedByUsers);
            da.SetDataTree(4, changeIsIssued);
            da.SetDataTree(5, changeIsArchived);
            da.SetDataTree(6, changeFirstRevisionIssueGuids);
            da.SetDataTree(7, changeCustomSchemeKeys);
            da.SetDataTree(8, changeCustomSchemeValues);
            da.SetDataList(9, errors);
        }
    }
}
