using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class ImportAttributesFromProjectComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ImportAttributesFromProject";

        public ImportAttributesFromProjectComponent()
            : base(
                "ImportAttributesFromProject",
                "Imports Line, Fill, ZoneCategory, PenTable and OperationProfile attributes from another project or " +
                "template file into the open project, with their complete definition, like Append or Overwrite by " +
                "name in Archicad's Attribute Manager. A new attribute gets a new guid. An attribute whose name " +
                "already exists is skipped, or overwritten in place with OverwriteExisting. DryRun reports what " +
                "would happen without changing anything. The names are the legacy 8-bit names of the source file. " +
                "Available from Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "ProjectFilePath",
                "The absolute path of the project or template file (for example .pln or .tpl) to import the attributes from.");

            InTexts(
                "AttributeTypes",
                "The attribute types to import: Line, Fill, ZoneCategory, PenTable or OperationProfile. Other " +
                "types get an error.");

            InTexts(
                "AttributeNames",
                "Import only the attributes with these names. Empty = every attribute of the given types.");

            InBoolean(
                "OverwriteExisting",
                "Overwrite an attribute whose name already exists in the open project (it keeps its index and " +
                "guid). Default is false: such an attribute is skipped.");

            InBoolean(
                "DryRun",
                "Change nothing, only report what an import would do. Default is false.");

            SetOptionality(new[] { 2, 3, 4 });
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "AttributeTypes",
                "Attribute type of each branch of the other outputs, in the order of the input AttributeTypes.");

            OutTextTree(
                "Names",
                "Names of the imported (with DryRun: importable) attributes in the source file (one branch per " +
                "attribute type).");

            OutIntegerTree(
                "SourceIndices",
                "Index of each attribute in the source file (one branch per attribute type).");

            OutTextTree(
                "Results",
                "created, overwritten, skipped or failed for each attribute (with DryRun: what an import would " +
                "do; one branch per attribute type).");

            OutGenericTree(
                "AttributeGuids",
                "The attribute in the open project (null when it was not created; one branch per attribute type).");

            OutTextTree(
                "AttributeErrorMessages",
                "Error message of each attribute (empty when it did not fail; one branch per attribute type).");

            OutErrorMessages(
                "Error message of each attribute type (empty when it was read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string projectFilePath) ||
                string.IsNullOrEmpty(projectFilePath))
            {
                this.AddError("ProjectFilePath is required.");
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<string> attributeTypes) ||
                attributeTypes.Count == 0)
            {
                this.AddError("AttributeTypes is required.");
                return;
            }

            var attributeNames = da.OptionalList<string>(2);
            var overwriteExisting = da.GetOptional(
                3,
                false);
            var dryRun = da.GetOptional(
                4,
                false);

            var parameters = new JObject
            {
                ["projectFilePath"] = projectFilePath,
                ["attributeTypes"] = new JArray(attributeTypes),
                ["overwriteExisting"] = overwriteExisting,
                ["dryRun"] = dryRun
            };
            if (attributeNames.Count > 0)
            {
                parameters["attributeNames"] = new JArray(attributeNames);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var items = JsonOutputHelp.Items(
                response,
                "attributesByType");
            var types = new List<object>();
            var names = new DataTree<object>();
            var sourceIndices = new DataTree<object>();
            var results = new DataTree<object>();
            var guids = new DataTree<object>();
            var attributeErrors = new DataTree<string>();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                names.EnsurePath(path);
                sourceIndices.EnsurePath(path);
                results.EnsurePath(path);
                guids.EnsurePath(path);
                attributeErrors.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    types.Add(i < attributeTypes.Count ? attributeTypes[i] : null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                types.Add(JsonOutputHelp.Scalar(item, "attributeType"));
                errors.Add("");

                if (!(item["attributes"] is JArray attributes))
                {
                    continue;
                }

                foreach (var attribute in attributes)
                {
                    names.Add(
                        JsonOutputHelp.Scalar(attribute, "name"),
                        path);
                    sourceIndices.Add(
                        JsonOutputHelp.Scalar(attribute, "sourceIndex"),
                        path);
                    results.Add(
                        JsonOutputHelp.Scalar(attribute, "result"),
                        path);

                    var guid = JsonOutputHelp.WrappedGuidOf(
                        attribute,
                        "attributeId");
                    guids.Add(
                        guid == null
                            ? null
                            : new AttributeGuidWrapper
                            {
                                AttributeId = new AttributeGuidObject { Guid = guid }
                            },
                        path);

                    attributeErrors.Add(
                        attribute["error"]?["message"]?.ToString() ?? "",
                        path);
                }
            }

            da.SetDataList(0, types);
            da.SetDataTree(1, names);
            da.SetDataTree(2, sourceIndices);
            da.SetDataTree(3, results);
            da.SetDataTree(4, guids);
            da.SetDataTree(5, attributeErrors);
            da.SetDataList(6, errors);
        }

        // Shares the icon of GetAttributesFromProject.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAttributesFromProject;

        public override Guid ComponentGuid =>
            new Guid("24846a5e-06e6-48e6-8d6f-4b5961b38e0d");
    }
}
