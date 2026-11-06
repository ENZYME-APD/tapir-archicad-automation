using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Issues;

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

            // Appended after the original inputs, so saved definitions keep their wiring.
            InBoolean(
                "Overwrite",
                "false refuses to export when a file already exists at ExportedFilePath, and writes " +
                "nothing. Default is true.",
                true);

            SetOptionality(
                new[]
                {
                    2,
                    3,
                    4
                });
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

            bool useExternalId = da.GetOptional(
                2,
                true);

            bool alignBySurveyPoint = da.GetOptional(
                3,
                true);

            var parameters = JObject.FromObject(
                new
                {
                    issues = issues.Issues,
                    exportPath,
                    useExternalId,
                    alignBySurveyPoint
                });

            // Only sent when it differs from the command's default.
            bool overwrite = da.GetOptional(
                4,
                true);
            if (!overwrite)
            {
                parameters["overwrite"] = false;
            }

            SetCadValues(
                CommandName,
                parameters,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ExportIssuesToBCF;

        public override Guid ComponentGuid =>
            new Guid("541081dc-ab2d-4bde-8e56-175070da4f21");
    }
}