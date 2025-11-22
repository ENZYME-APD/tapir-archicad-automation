using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.ResponseTypes.Issues;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class ExportIssuesToBCFComponent : ArchicadAccessorComponent
    {
        public static string Doc => "Export Issues to BCF.";
        public override string CommandName => "ExportIssuesToBCF";

        public ExportIssuesToBCFComponent()
            : base(
                "Export Issues to BCF",
                "ExportToBCF",
                Doc,
                GroupNames.Issues)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "IssueGuid",
                "Issues to export.");

            InText(
                "ExportedFilePath",
                "Path to the output BCF file.");

            InBoolean(
                "UseExternalId",
                "Use external IFC ID or Archicad IFC ID as referenced in BCF topics.",
                true);

            InBoolean(
                "AlignBySurveyPoint",
                "Align BCF views by Archicad Survey Point or Archicad Project Origin.",
                true);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var issues = IssuesObj.Create(
                da,
                0);
            if (issues == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input IssueGuids failed to collect data.");
                return;
            }

            var exportedFilePath = "";
            if (!da.GetData(
                    1,
                    ref exportedFilePath))
            {
                return;
            }

            var useExternalId = true;
            if (!da.GetData(
                    2,
                    ref useExternalId))
            {
                return;
            }

            var alignBySurveyPoint = true;
            if (!da.GetData(
                    3,
                    ref alignBySurveyPoint))
            {
                return;
            }

            var parameters = new ParametersOfExport
            {
                Issues = issues.Issues,
                ExportPath = exportedFilePath,
                UseExternalId = useExternalId,
                AlignBySurveyPoint = alignBySurveyPoint
            };

            GetResponse(
                CommandName,
                parameters);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ExportIssuesToBCF;

        public override Guid ComponentGuid =>
            new Guid("541081dc-ab2d-4bde-8e56-175070da4f21");
    }
}