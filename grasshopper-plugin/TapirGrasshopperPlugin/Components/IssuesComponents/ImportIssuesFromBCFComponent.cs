using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class ImportIssuesFromBCFComponent : ArchicadExecutorComponent
    {
        public static string Doc => "Import Issues from BCF.";
        public override string CommandName => "ImportIssuesFromBCF";

        public class ParametersOfImport
        {
            [JsonProperty("importPath")]
            public string ImportPath;

            [JsonProperty("alignBySurveyPoint")]
            public bool AlignBySurveyPoint;
        }

        public ImportIssuesFromBCFComponent()
            : base(
                "Import Issues from BCF",
                "ImportFromBCF",
                Doc,
                GroupNames.Issues)
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
            pManager.AddTextParameter(
                "FilePath",
                "FilePath",
                "Path to the input BCF file.",
                GH_ParamAccess.item);
            pManager.AddBooleanParameter(
                "AlignBySurveyPoint",
                "AlignBySurveyPoint",
                "Align BCF views by Archicad Survey Point or Archicad Project Origin.",
                GH_ParamAccess.item,
                @default: true);
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var filePath = "";
            if (!da.GetData(
                    0,
                    ref filePath))
            {
                return;
            }

            var alignBySurveyPoint = true;
            if (!da.GetData(
                    1,
                    ref alignBySurveyPoint))
            {
                return;
            }

            var parameters = new ParametersOfImport
            {
                ImportPath = filePath,
                AlignBySurveyPoint = alignBySurveyPoint
            };

            GetResponse(
                CommandName,
                parameters);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ImportIssuesFromBCF;

        public override Guid ComponentGuid =>
            new Guid("4cf3e832-dca0-4a6b-997b-aa593be9d293");
    }
}