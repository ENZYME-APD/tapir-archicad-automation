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
                "Read the project data SetProjectData stored under names in the project. " +
                "With no Names input every stored project data is listed with its content.",
                GroupNames.CustomData)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Names to read, one output item per name in this order. " +
                "Leave empty to list every project data stored in the project.");

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Names",
                "Name of each project data.");

            OutTexts(
                "Contents",
                "Stored content of each name (null when nothing is stored under it).");

            OutErrorMessages(
                "Error message of each name (empty when it was read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            da.TryGetList(
                0,
                out List<string> names);
            names = names ?? new List<string>();

            if (names.Exists(string.IsNullOrEmpty))
            {
                this.AddError("Every name must be a non-empty text.");
                return;
            }

            // An unconnected or empty Names input lists every stored project
            // data: GetProjectData needs the names, GetProjectDataKeys lists them.
            if (names.Count == 0)
            {
                if (!TryGetCadResponse(
                        "GetProjectDataKeys",
                        null,
                        ToAddOn,
                        out JObject keysResponse))
                {
                    return;
                }

                foreach (var key in JsonOutputHelp.Items(
                             keysResponse,
                             "keys"))
                {
                    names.Add(key["name"]?.ToString());
                }

                if (names.Count == 0)
                {
                    da.SetDataList(0, new List<string>());
                    da.SetDataList(1, new List<string>());
                    da.SetDataList(2, new List<string>());
                    return;
                }
            }

            var input = new ProjectDataNamesParameters
            {
                Names = names
            };

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(input),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var contents = new List<string>();
            var errors = new List<string>();

            // One item per name, in the order of the names. An error item
            // carries no name, so the requested one is handed out for it.
            var items = JsonOutputHelp.Items(
                response,
                "values");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                if (JsonOutputHelp.IsError(item))
                {
                    contents.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                contents.Add(item["content"]?.ToString());
                errors.Add("");
            }

            da.SetDataList(0, names);
            da.SetDataList(1, contents);
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
