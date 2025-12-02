using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Issues;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class ExportIssuesToBCFComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "ExportIssuesToBCF";

        public ExportIssuesToBCFComponent()
            : base(
                "ExportIssuesToBCF",
                "Export Issues to BCF.",
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
            if (!da.TryCreateFromList(
                    0,
                    out IssuesObj issues))
            {
                return;
            }

            if (!da.TryGet(
                    1,
                    out string exportPath))
            {
                return;
            }

            if (!da.TryGet(
                    2,
                    out bool useExternalId))
            {
                return;
            }

            if (!da.TryGet(
                    3,
                    out bool alignBySurveyPoint))
            {
                return;
            }

            SetValues(
                CommandName,
                new
                {
                    issues = issues.Issues,
                    exportPath,
                    useExternalId,
                    alignBySurveyPoint
                },
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ExportIssuesToBCF;

        public override Guid ComponentGuid =>
            new Guid("541081dc-ab2d-4bde-8e56-175070da4f21");
    }
}