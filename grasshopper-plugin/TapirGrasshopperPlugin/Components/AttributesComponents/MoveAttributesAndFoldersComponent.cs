using Grasshopper.Kernel;
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
                "Move attributes and attribute folders (with their contents) into the target folder, " +
                "all of the same attribute type. Available from Archicad 26.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "AttributeType",
                "Type of the attributes and folders to move and of the target folder (e.g. Layer, Line, Fill, Composite, Surface, ZoneCategory, Profile, PenTable, MEPSystem, BuildingMaterial).");

            InTextWithDefault(
                "TargetFolderPath",
                "Path of the folder to move into: the folder names from the root folder down, separated by '/' (e.g. Structure/Walls). " +
                "Empty for the root folder.",
                "");

            InGenerics(
                "AttributeGuids",
                "Identifiers of the attributes to move. Optional.");

            InTexts(
                "FolderPaths",
                "Path of each folder to move, separated by '/' like TargetFolderPath. Optional.");

            SetOptionality(new[] { 2, 3 });
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "AttributeErrorMessages",
                "Error message of each attribute of AttributeGuids (empty when it was moved).");

            OutTexts(
                "FolderErrorMessages",
                "Error message of each folder of FolderPaths (empty when it was moved).");
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

            var targetFolderPath = da.GetOptional(
                1,
                "");

            da.TryGetList(
                2,
                out List<GH_ObjectWrapper> attributeWrappers);
            attributeWrappers = attributeWrappers ?? new List<GH_ObjectWrapper>();

            da.TryGetList(
                3,
                out List<string> folderPaths);
            folderPaths = folderPaths ?? new List<string>();

            if (attributeWrappers.Count == 0 &&
                folderPaths.Count == 0)
            {
                this.AddError(
                    "Nothing to move: give at least one item in AttributeGuids or FolderPaths.");
                return;
            }

            var input = new MoveAttributesAndFoldersParameters
            {
                TargetFolder = new AttributeFolder(
                    attributeType,
                    targetFolderPath)
            };

            if (attributeWrappers.Count > 0)
            {
                input.AttributeIds = new List<AttributeGuidWrapper>();

                foreach (var wrapper in attributeWrappers)
                {
                    var attributeId = GuidObject<AttributeGuidObject>.CreateFromWrapper(wrapper);
                    if (attributeId == null)
                    {
                        this.AddError(
                            "Invalid attribute identifier in the AttributeGuids input.");
                        return;
                    }

                    input.AttributeIds.Add(
                        new AttributeGuidWrapper { AttributeId = attributeId });
                }
            }

            if (folderPaths.Count > 0)
            {
                input.AttributeFolders = folderPaths.ConvertAll(
                    pathText => new AttributeFolder(
                        attributeType,
                        pathText));
            }

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(input),
                    ToAddOn,
                    out var response))
            {
                return;
            }

            // The command answers with one result per attribute and one per
            // folder, in input order, so each list gets its own output.
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

        // TODO: render IconSource/MoveAttributesAndFolders.svg to a 24 px
        // Resources/MoveAttributesAndFolders.png and register it in
        // Properties/Resources.resx; the binary asset could not be added in
        // this automated change, so the navigator item move icon stands in.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MoveNavigatorItem;

        public override Guid ComponentGuid =>
            new Guid("c99fa630-6035-4975-b540-6ce608013e13");
    }
}
