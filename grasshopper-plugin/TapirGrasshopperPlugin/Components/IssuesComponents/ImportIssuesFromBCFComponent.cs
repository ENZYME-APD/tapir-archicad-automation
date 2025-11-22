using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Issues;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class ImportIssuesFromBCFComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ImportIssuesFromBCF";

        public ImportIssuesFromBCFComponent()
            : base(
                "ImportIssuesFromBCF",
                "Import Issues from BCF.",
                GroupNames.Issues)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "FilePath",
                "Path to the input BCF file.");

            InBoolean(
                "AlignBySurveyPoint",
                "Align BCF views by Archicad Survey Point or Archicad Project Origin.",
                true);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetItem(
                    0,
                    out string filePath))
            {
                return;
            }

            if (!da.TryGetItem(
                    1,
                    out bool alignBySurveyPoint))
            {
                return;
            }

            var parameters = new ParametersOfImport
            {
                ImportPath = filePath,
                AlignBySurveyPoint = alignBySurveyPoint
            };

            SetArchiCadValues(
                CommandName,
                parameters);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ImportIssuesFromBCF;

        public override Guid ComponentGuid =>
            new Guid("4cf3e832-dca0-4a6b-997b-aa593be9d293");
    }
}