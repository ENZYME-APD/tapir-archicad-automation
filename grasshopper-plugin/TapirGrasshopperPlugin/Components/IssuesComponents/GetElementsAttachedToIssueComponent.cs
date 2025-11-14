using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;
using static TapirGrasshopperPlugin.Components.IssuesComponents.AttachElementsToIssueComponent;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class GetElementsAttachedToIssueComponent : ArchicadAccessorComponent
    {
        public class ParametersOfGetAttachedElements
        {
            [JsonProperty ("issueId")]
            public IssueIdObj IssueId;

            [JsonProperty ("type")]
            public string Type;
        }

        public GetElementsAttachedToIssueComponent ()
          : base (
                "Get Elements Attached to an Issue",
                "GetAttachedElements",
                "Get Elements Attached to an Issue.",
                "Issues"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("IssueGuid", "IssueGuid", "Issue Guid.", GH_ParamAccess.item);
            pManager.AddGenericParameter ("Type", "Type", "Type of elements.", GH_ParamAccess.item);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("ElementGuids", "ElementGuids", "Attached elements of the given issue.", GH_ParamAccess.list);
        }

        public override void AddedToDocument (GH_Document document)
        {
            base.AddedToDocument (document);

            new IssueElementTypeValueList ().AddAsSource (this, 1);
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            IssueIdObj issueId = IssueIdObj.Create (DA, 0);
            if (issueId == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input IssueGuid failed to collect data.");
                return;
            }

            string type = "";
            if (!DA.GetData (1, ref type)) {
                return;
            }

            ParametersOfGetAttachedElements parametersOfGetAttachedElements = new ParametersOfGetAttachedElements {
                IssueId = issueId,
                Type = type
            };
            JObject parameters = JObject.FromObject (parametersOfGetAttachedElements);
            CommandResponse response = SendArchicadAddOnCommand ("GetElementsAttachedToIssue", parameters);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }

            ElementsObj elements = response.Result.ToObject<ElementsObj> ();
            DA.SetDataList (0, elements.Elements);
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.GetElementsAttachedToAnIssue;

        public override Guid ComponentGuid => new Guid ("097b773b-3cd4-43a5-a41d-1fb57bc3e2e2");
    }
}
