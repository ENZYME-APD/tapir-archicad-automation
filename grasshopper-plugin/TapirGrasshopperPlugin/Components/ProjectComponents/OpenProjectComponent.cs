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
        public static string CommandName => "OpenProject";

        public OpenProjectComponent ()
            : base (CommandName, "Open Project", "Opens the given project.", GroupNames.Project )
        { }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddTextParameter (nameof(ProjectFilePathObject.ProjectFilePath), nameof (ProjectFilePathObject.ProjectFilePath), nameof (ProjectFilePathObject.ProjectFilePath), GH_ParamAccess.item);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddTextParameter (nameof (ExecutionResultBase.ResultMessage), "", "", GH_ParamAccess.item);
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            if (!DA.GetItem (0, out string path)) return;

            var jObject = JObject.FromObject (new ProjectFilePathObject (path));

            if (!GetResponse(CommandName, jObject, out JObject jResponse)) return;

            var result = ExecutionResultBase.Deserialize (jResponse);

            DA.SetData (0, result.ResultMessage ());
        }

        public override Guid ComponentGuid => new Guid ("730b6a52-e30e-4863-82a2-9457b79748c0");
    }
}
