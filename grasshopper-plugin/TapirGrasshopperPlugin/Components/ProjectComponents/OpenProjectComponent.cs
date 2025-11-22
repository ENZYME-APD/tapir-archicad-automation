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
        public static string Doc => "Opens the given project.";
        public override string CommandName => "OpenProject";

        public OpenProjectComponent()
            : base(
                "Open Project",
                "Open Project",
                Doc,
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InText(
                nameof(ProjectFilePathObject.ProjectFilePath),
                nameof(ProjectFilePathObject.ProjectFilePath));
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResultBase.Message),
                "");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.GetItem(
                    0,
                    out string path))
            {
                return;
            }

            if (!GetConvertedResponse(
                    CommandName,
                    new ProjectFilePathObject(path),
                    out JObject jResponse))
            {
                return;
            }

            var result = ExecutionResultBase.Deserialize(jResponse);

            da.SetData(
                0,
                result.Message());
        }

        public override Guid ComponentGuid =>
            new Guid("730b6a52-e30e-4863-82a2-9457b79748c0");
    }
}