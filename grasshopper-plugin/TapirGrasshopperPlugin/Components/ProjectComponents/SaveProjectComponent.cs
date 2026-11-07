using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class SaveProjectComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "SaveProject";

        public SaveProjectComponent()
            : base(
                "SaveProject",
                "Saves the current project, in place or to ProjectPath.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "ProjectPath",
                "Full path of a .pln file to save the project to, as File > Save As does: the open " +
                "project then refers to that file. This saves an untitled project, or one opened from " +
                "an older Archicad version. An existing file at the path is overwritten unless " +
                "Overwrite is false. When it is not given, the project is saved in place. Optional.");

            InBoolean(
                "Overwrite",
                "Only together with ProjectPath: false refuses to save when a file already exists at " +
                "ProjectPath, and writes nothing. Default is true.",
                true);

            SetOptionality(new[] { 0, 1 });
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult.Message),
                ExecutionResult.Doc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            var projectPath = da.GetOptional<string>(0, null);
            if (!string.IsNullOrWhiteSpace(projectPath))
            {
                parameters["projectPath"] = projectPath;
            }

            // Only sent when it differs from the command's default.
            if (!da.GetOptional(1, true))
            {
                parameters["overwrite"] = false;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    parameters,
                    ToAddOn,
                    ExecutionResult.Deserialize,
                    out ExecutionResult response))
            {
                return;
            }

            da.SetData(
                0,
                response.Message());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SaveProject;

        public override Guid ComponentGuid =>
            new Guid("8a6d3473-d937-433f-80b2-13b0f07ca669");
    }
}