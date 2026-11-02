using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.RevisionComponents
{
    public class ModifyRevisionIssuesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ModifyRevisionIssues";

        public ModifyRevisionIssuesComponent()
            : base(
                "ModifyRevisionIssues",
                "Modify the ID, the description or the custom scheme fields of the given revision issues. " +
                "Only provided fields are changed. Issued revision issues may refuse the change.",
                GroupNames.Revision)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "RevisionIssueGuids",
                "Identifiers of the revision issues to modify (see GetRevisionIssues).");

            InTexts(
                "NewIds",
                "New ID of each issue (input only 1 to use the same ID for all). Optional.");

            InTexts(
                "NewDescriptions",
                "New description of each issue (input only 1 to use the same description for all). Optional.");

            InTexts(
                "CustomSchemeFields",
                "Key or name of each custom scheme field to set, the same fields for every issue " +
                "(see the CustomSchemeFieldKeys and CustomSchemeFieldNames outputs of GetRevisionIssues). Optional.");

            InTexts(
                "CustomSchemeValues",
                "Value of each custom scheme field: one per field to use the same values for every issue, " +
                "or one per field and issue, listed issue by issue. Optional.");

            SetOptionality(new[] { 1, 2, 3, 4 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each revision issue (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> issueGuids))
            {
                return;
            }

            da.TryGetList(1, out List<string> newIds);
            newIds = newIds ?? new List<string>();
            da.TryGetList(2, out List<string> newDescriptions);
            newDescriptions = newDescriptions ?? new List<string>();
            da.TryGetList(3, out List<string> customSchemeFields);
            customSchemeFields = customSchemeFields ?? new List<string>();
            da.TryGetList(4, out List<string> customSchemeValues);
            customSchemeValues = customSchemeValues ?? new List<string>();

            foreach (var pair in new (string Name, int Count)[]
                     {
                         ("NewIds", newIds.Count),
                         ("NewDescriptions", newDescriptions.Count)
                     })
            {
                if (pair.Count > 1 &&
                    pair.Count != issueGuids.Count)
                {
                    this.AddError(
                        $"The size of the input {pair.Name} must be 0, 1 or equal to the size of the input RevisionIssueGuids.");
                    return;
                }
            }

            if (customSchemeFields.Count == 0 &&
                customSchemeValues.Count > 0)
            {
                this.AddError(
                    "The input CustomSchemeValues needs the fields to set in the input CustomSchemeFields.");
                return;
            }

            // One value per field (shared by every issue), or one per field and issue.
            var valuesPerIssue =
                customSchemeValues.Count != customSchemeFields.Count &&
                customSchemeValues.Count == customSchemeFields.Count * issueGuids.Count;
            if (customSchemeFields.Count > 0 &&
                customSchemeValues.Count != customSchemeFields.Count &&
                !valuesPerIssue)
            {
                this.AddError(
                    "The size of the input CustomSchemeValues must be equal to the size of the input CustomSchemeFields, " +
                    "or to that size multiplied by the size of the input RevisionIssueGuids.");
                return;
            }

            var issues = new JArray();
            for (var i = 0; i < issueGuids.Count; i++)
            {
                if (string.IsNullOrWhiteSpace(issueGuids[i]) ||
                    !Guid.TryParse(issueGuids[i], out _))
                {
                    this.AddError("Invalid identifier in the RevisionIssueGuids input: " + issueGuids[i]);
                    return;
                }

                var issue = new JObject
                {
                    ["revisionIssueId"] = new JObject { ["guid"] = issueGuids[i] }
                };
                if (newIds.Count > 0)
                {
                    issue["id"] = newIds[newIds.Count == 1 ? 0 : i];
                }
                if (newDescriptions.Count > 0)
                {
                    issue["description"] = newDescriptions[newDescriptions.Count == 1 ? 0 : i];
                }
                if (customSchemeFields.Count > 0)
                {
                    var customSchemeData = new JArray();
                    for (var j = 0; j < customSchemeFields.Count; j++)
                    {
                        var field = customSchemeFields[j];
                        var value = customSchemeValues[valuesPerIssue ? i * customSchemeFields.Count + j : j];
                        var entry = new JObject();
                        entry[Guid.TryParse(field, out _) ? "customSchemeKey" : "customSchemeName"] = field;
                        entry["customSchemeValue"] = value;
                        customSchemeData.Add(entry);
                    }
                    issue["customSchemeData"] = customSchemeData;
                }
                issues.Add(issue);
            }

            var parameters = new JObject { ["revisionIssues"] = issues };

            SetCadValuesWithErrorMessages(
                CommandName,
                parameters,
                ToAddOn,
                da);
        }

        // Shares the icon of GetRevisionIssues until the component gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetRevisionIssues;

        public override Guid ComponentGuid =>
            new Guid("c7937cd8-662a-4d53-9a5b-47a0183811ad");
    }
}
