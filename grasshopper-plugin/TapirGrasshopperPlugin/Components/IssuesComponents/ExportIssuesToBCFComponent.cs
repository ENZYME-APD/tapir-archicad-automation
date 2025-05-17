using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class ExportIssuesToBCFComponent : ArchicadAccessorComponent
    {
        public class ParametersOfExport
        {
            [JsonProperty ("issues")]
            public List<IssueIdItemObj> Issues;

            [JsonProperty ("exportPath")]
            public string ExportPath;

            [JsonProperty ("useExternalId")]
            public bool UseExternalId;

            [JsonProperty ("alignBySurveyPoint")]
            public bool AlignBySurveyPoint;
        }

        public ExportIssuesToBCFComponent ()
          : base (
                "Export Issues to BCF",
                "ExportToBCF",
                "Export Issues to BCF.",
                "Issues"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("IssueGuids", "IssueGuids", "Issues to export.", GH_ParamAccess.list);
            pManager.AddTextParameter ("ExportedFilePath", "ExportedFilePath", "Path to the output BCF file.", GH_ParamAccess.item);
            pManager.AddBooleanParameter ("UseExternalId", "UseExternalId", "Use external IFC ID or Archicad IFC ID as referenced in BCF topics.", GH_ParamAccess.item, @default: true);
            pManager.AddBooleanParameter ("AlignBySurveyPoint", "AlignBySurveyPoint", "Align BCF views by Archicad Survey Point or Archicad Project Origin.", GH_ParamAccess.item, @default: true);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            IssuesObj issues = IssuesObj.Create (DA, 0);
            if (issues == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input IssueGuids failed to collect data.");
                return;
            }

            string exportedFilePath = "";
            if (!DA.GetData (1, ref exportedFilePath)) {
                return;
            }

            bool useExternalId = true;
            if (!DA.GetData (2, ref useExternalId)) {
                return;
            }

            bool alignBySurveyPoint = true;
            if (!DA.GetData (3, ref alignBySurveyPoint)) {
                return;
            }

            ParametersOfExport parametersOfExport = new ParametersOfExport {
                Issues = issues.Issues,
                ExportPath = exportedFilePath,
                UseExternalId = useExternalId,
                AlignBySurveyPoint = alignBySurveyPoint
            };
            JObject parameters = JObject.FromObject (parametersOfExport);
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "ExportIssuesToBCF", parameters);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.ExportIssuesToBCF;

        public override Guid ComponentGuid => new Guid ("541081dc-ab2d-4bde-8e56-175070da4f21");
    }
}
