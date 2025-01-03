using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class AttachElementsToIssueComponent : ArchicadAccessorComponent
    {
        public class ParametersOfAttachElements
        {
            [JsonProperty ("issueId")]
            public IssueIdObj IssueId;

            [JsonProperty ("elements")]
            public List<ElementIdItemObj> Elements;

            [JsonProperty ("type")]
            public string Type;
        }

        public AttachElementsToIssueComponent ()
          : base (
                "Attach Elements to an Issue",
                "AttachElements",
                "Attach Elements to an Issue.",
                "Issues"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("IssueId", "IssueId", "Issue id.", GH_ParamAccess.item);
            pManager.AddGenericParameter ("ElementIds", "ElementIds", "Elements to attach.", GH_ParamAccess.list);
            pManager.AddGenericParameter ("Type", "Type", "Type of elements.", GH_ParamAccess.item);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
        }

        public override void AddedToDocument (GH_Document document)
        {
            base.AddedToDocument (document);

            new IssueElementTypeValueList ().AddAsSource (this, 2);
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            IssueIdObj issueId = IssueIdObj.Create (DA, 0);
            if (issueId == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input IssueId failed to collect data.");
                return;
            }

            ElementsObj elements = ElementsObj.Create (DA, 1);
            if (elements == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input ElementIds failed to collect data.");
                return;
            }

            string type = "";
            if (!DA.GetData (2, ref type)) {
                return;
            }

            ParametersOfAttachElements parametersOfAttachElements = new ParametersOfAttachElements {
                IssueId = issueId,
                Elements = elements.Elements,
                Type = type
            };
            JObject parameters = JObject.FromObject (parametersOfAttachElements);
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "AttachElementsToIssue", parameters);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
        }

        // protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.AttachElements;

        public override Guid ComponentGuid => new Guid ("15e08be9-913c-4766-93ce-edadc1a261c5");
    }
}
