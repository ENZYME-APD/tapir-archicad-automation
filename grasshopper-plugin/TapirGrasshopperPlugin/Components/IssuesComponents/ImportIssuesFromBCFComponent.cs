using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class ImportIssuesFromBCFComponent : ArchicadAccessorComponent
    {
        public class ParametersOfImport
        {
            [JsonProperty ("importPath")]
            public string ImportPath;

            [JsonProperty ("alignBySurveyPoint")]
            public bool AlignBySurveyPoint;
        }

        public ImportIssuesFromBCFComponent ()
          : base (
                "Import Issues from BCF",
                "ImportFromBCF",
                "Import Issues from BCF.",
                "Issues"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddTextParameter ("FilePath", "FilePath", "Path to the input BCF file.", GH_ParamAccess.item);
            pManager.AddBooleanParameter ("AlignBySurveyPoint", "AlignBySurveyPoint", "Align BCF views by Archicad Survey Point or Archicad Project Origin.", GH_ParamAccess.item, @default: true);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            string filePath = "";
            if (!DA.GetData (0, ref filePath)) {
                return;
            }

            bool alignBySurveyPoint = true;
            if (!DA.GetData (1, ref alignBySurveyPoint)) {
                return;
            }

            ParametersOfImport parametersOfImport = new ParametersOfImport {
                ImportPath = filePath,
                AlignBySurveyPoint = alignBySurveyPoint
            };
            JObject parameters = JObject.FromObject (parametersOfImport);
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "ImportIssuesFromBCF", parameters);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.ImportIssuesFromBCF;

        public override Guid ComponentGuid => new Guid ("4cf3e832-dca0-4a6b-997b-aa593be9d293");
    }
}
