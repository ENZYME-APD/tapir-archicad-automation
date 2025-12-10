using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;

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

            SetOptionality(1);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string importPath))
            {
                return;
            }

            bool alignBySurveyPoint = da.GetOptional(
                1,
                true);

            SetValues(
                CommandName,
                new { importPath, alignBySurveyPoint },
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ImportIssuesFromBCF;

        public override Guid ComponentGuid =>
            new Guid("4cf3e832-dca0-4a6b-997b-aa593be9d293");
    }
}