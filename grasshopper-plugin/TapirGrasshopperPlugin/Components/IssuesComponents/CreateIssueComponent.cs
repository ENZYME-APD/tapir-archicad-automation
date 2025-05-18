using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class CreateIssueComponent : ArchicadAccessorComponent
    {
        public class ParametersOfNewIssue
        {
            [JsonProperty ("name")]
            public string Name;
        }

        public CreateIssueComponent ()
          : base (
                "Create Issue",
                "CreateIssue",
                "Create an issue.",
                "Issues"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddTextParameter ("Name", "Name", "Name", GH_ParamAccess.item);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("IssueGuid", "IssueGuid", "Issue Guid.", GH_ParamAccess.item);
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            string name = "";
            if (!DA.GetData (0, ref name)) {
                return;
            }

            ParametersOfNewIssue parametersOfNewIssue = new ParametersOfNewIssue {
                Name = name
            };
            JObject parameters = JObject.FromObject (parametersOfNewIssue);
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "CreateIssue", parameters);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }

            IssueIdItemObj newIssue = response.Result.ToObject<IssueIdItemObj> ();
            DA.SetData (0, newIssue.IssueId);
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.CreateIssue;

        public override Guid ComponentGuid => new Guid ("fdd43474-b0de-4354-8856-c1dc0b07195f");
    }
}
