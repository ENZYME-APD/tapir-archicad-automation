using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.CustomData;

namespace TapirGrasshopperPlugin.Components.CustomDataComponents
{
    public class GetProjectDataComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetProjectData";

        public GetProjectDataComponent()
            : base(
                "GetProjectData",
                "Read the text values SetProjectData stored under keys in the project. " +
                "With no Keys input every stored key is listed with its value.",
                GroupNames.CustomData)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Keys",
                "Keys to read, one output item per key in this order. " +
                "Leave empty to list every key stored in the project.");

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Keys",
                "Key of each value.");

            OutTexts(
                "Values",
                "Stored value of each key (null when nothing is stored under it).");

            OutErrorMessages(
                "Error message of each key (empty when it was read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            // An unconnected or empty Keys input lists every stored key: the
            // command does that when the keys parameter is left out.
            da.TryGetList(
                0,
                out List<string> requestedKeys);
            requestedKeys = requestedKeys ?? new List<string>();

            if (requestedKeys.Exists(string.IsNullOrEmpty))
            {
                this.AddError("Every key must be a non-empty text.");
                return;
            }

            var input = new ProjectDataKeysParameters
            {
                Keys = requestedKeys.Count > 0
                    ? requestedKeys
                    : null
            };

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(input),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var keys = new List<string>();
            var values = new List<string>();
            var errors = new List<string>();

            // One item per requested key, in the input order, or one per
            // stored key when nothing was requested. An error item carries no
            // key, so the requested one is handed out in its place.
            var items = JsonOutputHelp.Items(
                response,
                "projectData");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                if (JsonOutputHelp.IsError(item))
                {
                    keys.Add(
                        i < requestedKeys.Count
                            ? requestedKeys[i]
                            : null);
                    values.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                keys.Add(item["key"]?.ToString());
                values.Add(item["value"]?.ToString());
                errors.Add("");
            }

            da.SetDataList(0, keys);
            da.SetDataList(1, values);
            da.SetDataList(2, errors);
        }

        // The project data has no icon of its own yet; it shares the project
        // info one, as the project info field components do among themselves.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ProjectInfo;

        public override Guid ComponentGuid =>
            new Guid("2e8b6d41-57c3-4f0a-9e62-b1a4d8c05f73");
    }
}
