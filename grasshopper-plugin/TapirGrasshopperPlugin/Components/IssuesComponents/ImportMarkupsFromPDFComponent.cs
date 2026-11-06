using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;
using TapirGrasshopperPlugin.Types.Issues;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public class ImportMarkupsFromPDFComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ImportMarkupsFromPDF";

        public ImportMarkupsFromPDFComponent()
            : base(
                "ImportMarkupsFromPDF",
                "Import the markups of a PDF published from Archicad as Issues, like Issue Manager > " +
                "Import PDF markups, without any dialog, and report the Issues it created. " +
                "Available from Archicad 29.",
                GroupNames.Issues)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "FilePath",
                "Path to the marked-up PDF file.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "IssueGuids",
                "Identifiers of the Issues the import created.");

            OutText(
                nameof(ExecutionResult.Message),
                ExecutionResult.Doc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string pdfFilePath) ||
                string.IsNullOrWhiteSpace(pdfFilePath))
            {
                this.AddError("FilePath is required.");
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(new { pdfFilePath }),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetDataList(
                0,
                (response["issues"]?.ToObject<List<IssueGuidWrapper>>() ??
                    new List<IssueGuidWrapper>())
                .Select(x => x.IssueId));

            if (response["executionResult"] is JObject resultObject)
            {
                var result = ExecutionResult.Deserialize(resultObject);
                if (!result.Success)
                {
                    this.AddError($"Failed execution: {result.Message()}");
                }
                da.SetData(1, result.Message());
            }
        }

        // Shares the icon of the BCF import until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ImportIssuesFromBCF;

        public override Guid ComponentGuid =>
            new Guid("b7d2c4f1-9a3e-4c58-8e61-2f0a7d9c5b13");
    }
}
