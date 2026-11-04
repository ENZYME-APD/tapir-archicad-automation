using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetModulDataComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetModulData";

        public GetModulDataComponent()
            : base(
                "GetModulData",
                "Gets the module data Tapir stored in the project with StoreModulData: the given names, or all " +
                "of them, with their data version and content. Module data are the custom data sections of " +
                "Archicad's ModulData Manager: text saved in the project file under a name, kept apart from " +
                "project data.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Names of the module data to get; the empty name is the default section. Optional: every " +
                "module data Tapir stored when unconnected.");

            InBoolean(
                "IncludeContent",
                "Whether the contents are read too. When false, only the names and data versions are read.",
                true);

            SetOptionality(new[] { 0, 1 });
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Names",
                "Name of each module data.");

            OutIntegers(
                "DataVersions",
                "Data version of each module data, aligned with Names (null where there is no module data).");

            OutTexts(
                "Contents",
                "Content of each module data, aligned with Names (null where there is no module data or " +
                "IncludeContent is false).");

            OutErrorMessages(
                "Error message of each name (empty when its module data was found).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();
            if (da.TryGetList(
                    0,
                    out List<string> names) &&
                names.Count > 0)
            {
                parameters["names"] = JArray.FromObject(names);
            }

            if (da.TryGet(
                    1,
                    out bool includeContent))
            {
                parameters["includeContent"] = includeContent;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out var response))
            {
                return;
            }

            var outNames = new List<string>();
            var dataVersions = new List<int?>();
            var contents = new List<string>();
            var errors = new List<string>();
            var items = JsonOutputHelp.Items(
                response,
                "modulData");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                if (JsonOutputHelp.IsError(item))
                {
                    outNames.Add(names.Count > i ? names[i] : null);
                    dataVersions.Add(null);
                    contents.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                outNames.Add(item["name"]?.ToString());
                dataVersions.Add(item["dataVersion"]?.Value<int>());
                contents.Add(item["content"]?.ToString());
                errors.Add("");
            }

            da.SetDataList(0, outNames);
            da.SetDataList(1, dataVersions);
            da.SetDataList(2, contents);
            da.SetDataList(3, errors);
        }

        // Shares the icon of GetProjectData, the other text data saved in the project.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetProjectData;

        public override Guid ComponentGuid =>
            new Guid("e57dead8-39e0-4ac1-8873-e912cea84d82");
    }
}
