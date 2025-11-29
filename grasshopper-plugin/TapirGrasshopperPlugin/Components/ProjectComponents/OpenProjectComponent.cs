using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Generic;
using TapirGrasshopperPlugin.ResponseTypes.Project;

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
                nameof(ProjectFilePathObject.ProjectFilePath),
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
            if (!da.TryGetItem(
                    0,
                    out string path))
            {
                return;
            }

            if (!TryGetConvertedResponse(
                    CommandName,
                    new ProjectFilePathObject { ProjectFilePath = path },
                    out JObject response))
            {
                return;
            }

            da.SetData(
                0,
                ExecutionResult.Deserialize(response).Message());
        }

        public override Guid ComponentGuid =>
            new Guid("730b6a52-e30e-4863-82a2-9457b79748c0");
    }
}