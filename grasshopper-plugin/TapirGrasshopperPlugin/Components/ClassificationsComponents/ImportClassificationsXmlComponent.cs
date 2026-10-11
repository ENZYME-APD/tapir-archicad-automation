using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class ImportClassificationsXmlComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ImportClassificationsXml";

        public ImportClassificationsXmlComponent()
            : base(
                "ImportClassificationsXml",
                "Import Classification Systems from a Classification Manager export (XML) and report " +
                "the classification systems and items the import created and removed.",
                GroupNames.Classifications)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Xml",
                "The content of a Classification Manager export (XML), e.g. read with the Read File component.");

            InTextWithDefault(
                "SystemConflictPolicy",
                "What to do with a system whose name already exists: merge, replace or skip " +
                "(keep the existing one). Default merge.",
                "merge");

            InTextWithDefault(
                "ItemConflictPolicy",
                "What to do with an item whose id already exists in a merged system: replace or skip " +
                "(keep the existing one). Default skip.",
                "skip");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "CreatedGuids",
                "Identifiers of the classification systems and items the import created.");

            OutGenerics(
                "RemovedGuids",
                "Identifiers of the classification systems and items the import removed.");

            OutText(
                nameof(ExecutionResult.Message),
                ExecutionResult.Doc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string xml) ||
                string.IsNullOrWhiteSpace(xml))
            {
                this.AddError("Xml is required.");
                return;
            }

            var input = new ImportClassificationsXmlParameters
            {
                Xml = xml,
                SystemConflictPolicy = da.GetOptional(1, "merge"),
                ItemConflictPolicy = da.GetOptional(2, "skip")
            };

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(input),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetDataList(
                0,
                response["created"]?.ToObject<List<ClassificationGuid>>() ??
                new List<ClassificationGuid>());

            da.SetDataList(
                1,
                response["removed"]?.ToObject<List<ClassificationGuid>>() ??
                new List<ClassificationGuid>());

            if (response["executionResult"] is JObject resultObject)
            {
                var result = ExecutionResult.Deserialize(resultObject);
                if (!result.Success)
                {
                    this.AddError($"Failed execution: {result.Message()}");
                }
                da.SetData(2, result.Message());
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ImportClassificationsXml;

        public override Guid ComponentGuid =>
            new Guid("e75a1d65-5a33-47cb-af21-240bde0782af");
    }
}
