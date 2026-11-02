using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetProjectDataComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetProjectData";

        public GetProjectDataComponent()
            : base(
                "GetProjectData",
                "Gets the content of the project data with the given names.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Names of the project data to get.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Contents",
                "Content of each project data, aligned with Names (null where there is no project data with the name).");

            OutErrorMessages(
                "Error message of each name (empty when its project data was found).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!ProjectDataHelp.TryGetNames(
                    da,
                    out var parameters))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out var response))
            {
                return;
            }

            var contents = new List<string>();
            var errors = new List<string>();
            foreach (var value in JsonOutputHelp.Items(
                         response,
                         "values"))
            {
                if (JsonOutputHelp.IsError(value))
                {
                    contents.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(value));
                    continue;
                }

                contents.Add(value["content"]?.ToString());
                errors.Add("");
            }

            da.SetDataList(0, contents);
            da.SetDataList(1, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetProjectData;

        public override Guid ComponentGuid =>
            new Guid("2c1ca766-8e93-4532-bddf-208d993d5044");
    }
}
