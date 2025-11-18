using Grasshopper.Kernel;
using Newtonsoft.Json;
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

        protected override void AddInputs()
        {
            AddText(
                "FilePath",
                "Path to the input BCF file.");

            AddBoolean(
                "AlignBySurveyPoint",
                "Align BCF views by Archicad Survey Point or Archicad Project Origin.",
                true);
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