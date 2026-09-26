using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;
using TapirGrasshopperPlugin.Types.Properties;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class ImportPropertiesXmlComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ImportPropertiesXml";

        public ImportPropertiesXmlComponent()
            : base(
                "ImportPropertiesXml",
                "Import Custom Property Definitions from a Property Manager export (XML) and report " +
                "the property definitions the import created and removed.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Xml",
                "The content of a Property Manager export (XML), e.g. read with the Read File component.");

            InTextWithDefault(
                "ConflictPolicy",
                "What to do with a property whose name already exists in its group: " +
                "append (import it under a new unused name), replace (replace the existing definition) " +
                "or skip (keep the existing one). Default append.",
                "append");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "CreatedPropertyGuids",
                "Identifiers of the property definitions the import created.");

            OutGenerics(
                "RemovedPropertyGuids",
                "Identifiers of the property definitions the import removed.");

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

            var input = new ImportPropertiesXmlParameters
            {
                Xml = xml,
                ConflictPolicy = da.GetOptional(1, "append")
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
                response["created"]?.ToObject<List<PropertyGuidObject>>() ??
                new List<PropertyGuidObject>());

            da.SetDataList(
                1,
                response["removed"]?.ToObject<List<PropertyGuidObject>>() ??
                new List<PropertyGuidObject>());

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
            Properties.Resources.ImportPropertiesXml;

        public override Guid ComponentGuid =>
            new Guid("92b9cb40-2013-4f1d-ac0b-e68728bf4e2d");
    }
}
