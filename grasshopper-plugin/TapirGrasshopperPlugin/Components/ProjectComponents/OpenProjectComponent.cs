using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class OpenProjectComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "OpenProject";

        public OpenProjectComponent()
            : base(
                "OpenProject",
                "Opens a given project.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "FilePath",
                "Path of the ArchiCad file that gets opened.");
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
            if (!da.TryGet(
                    0,
                    out string path))
            {
                return;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    new { projectFilePath = path },
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

        public override Guid ComponentGuid =>
            new Guid("730b6a52-e30e-4863-82a2-9457b79748c0");
    }
}