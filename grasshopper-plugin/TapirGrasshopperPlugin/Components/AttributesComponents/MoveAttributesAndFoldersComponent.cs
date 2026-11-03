using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;
using TapirGrasshopperPlugin.Types.Generic;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class MoveAttributesAndFoldersComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "MoveAttributesAndFolders";

        public MoveAttributesAndFoldersComponent()
            : base(
                "MoveAttributesAndFolders",
                "Moves attributes and attribute folders (with their contents) into the target folder, which must be " +
                "of the same attribute type; an empty target path is the root folder. Returns one execution result " +
                "per attribute and per folder. Available from Archicad 26.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "AttributeType",
                "Attribute type of the target folder, and so of the attributes and folders to move: Layer, Line, " +
                "Fill, Composite, Surface, LayerCombination, ZoneCategory, Profile, PenTable, MEPSystem, " +
                "OperationProfile or BuildingMaterial.");

            InTexts(
                "TargetPath",
                "Path of the folder to move into: the folder names from the root folder down, the root folder " +
                "itself not included. Empty = the root folder.");

            InGenerics(
                "AttributeGuids",
                "Identifiers of the attributes to move. Optional.");

            inManager.AddTextParameter(
                "FolderPaths",
                "FolderPaths",
                ("Path of each folder to move with its contents, one branch per folder: the folder names from " +
                "the root folder down, the root folder itself not included. Optional.").WithTypeName("textTree"),
                GH_ParamAccess.tree);

            SetOptionality(new[] { 1, 2, 3 });
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "AttributeErrorMessages",
                "Error message of each attribute (empty when it was moved).");

            OutTexts(
                "FolderErrorMessages",
                "Error message of each folder (empty when it was moved).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string attributeType))
            {
                return;
            }

            da.TryGetList(
                1,
                out List<string> targetPath);
            targetPath.RemoveAll(string.IsNullOrEmpty);

            da.TryGetList(
                2,
                out List<GH_ObjectWrapper> attributeWrappers);

            var attributeIds = new JArray();
            foreach (var wrapper in attributeWrappers)
            {
                var attributeId = GuidObject<AttributeGuidObject>.CreateFromWrapper(wrapper);
                if (attributeId == null)
                {
                    this.AddError(
                        "Invalid attribute identifier in the AttributeGuids input.");
                    return;
                }

                attributeIds.Add(
                    JObject.FromObject(
                        new AttributeGuidWrapper { AttributeId = attributeId }));
            }

            da.TryGetTree(
                3,
                out GH_Structure<GH_String> folderPathTree);

            var attributeFolders = new JArray();
            foreach (var path in AttributeFolderInput.Paths(folderPathTree))
            {
                if (path.Count == 0)
                {
                    continue;
                }

                attributeFolders.Add(
                    AttributeFolderInput.Folder(
                        attributeType,
                        path));
            }

            var parameters = new JObject
            {
                ["targetFolder"] = AttributeFolderInput.Folder(
                    attributeType,
                    targetPath)
            };

            if (attributeIds.Count > 0)
            {
                parameters["attributeIds"] = attributeIds;
            }

            if (attributeFolders.Count > 0)
            {
                parameters["attributeFolders"] = attributeFolders;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetDataList(
                0,
                ExecutionResultsResponse.ErrorMessages(
                    response,
                    "attributeExecutionResults"));

            da.SetDataList(
                1,
                ExecutionResultsResponse.ErrorMessages(
                    response,
                    "folderExecutionResults"));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MoveAttributesAndFolders;

        public override Guid ComponentGuid =>
            new Guid("48e2f3ea-efe4-4354-bf37-45565c242585");
    }
}
