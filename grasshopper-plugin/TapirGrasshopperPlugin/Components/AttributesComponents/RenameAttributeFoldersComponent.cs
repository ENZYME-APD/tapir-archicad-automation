using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class RenameAttributeFoldersComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "RenameAttributeFolders";

        public RenameAttributeFoldersComponent()
            : base(
                "RenameAttributeFolders",
                "Rename attribute folders, each given by its path from the root folder of the attribute type. " +
                "A folder keeps its place and contents; only the last name of its path changes. " +
                "Available from Archicad 26.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "AttributeType",
                "Type of the attributes the folders hold (e.g. Layer, Line, Fill, Composite, Surface, ZoneCategory, Profile, PenTable, MEPSystem, BuildingMaterial).");

            InTexts(
                "FolderPaths",
                "Path of each folder to rename: the folder names from the root folder down, separated by '/' (e.g. Structure/Walls).");

            InTexts(
                "NewNames",
                "New name of each folder, one per item of FolderPaths.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each folder (empty when it was renamed).");
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

            if (!da.TryGetList(
                    1,
                    out List<string> folderPaths))
            {
                return;
            }

            if (!da.TryGetList(
                    2,
                    out List<string> newNames))
            {
                return;
            }

            if (newNames.Count != folderPaths.Count)
            {
                this.AddError(
                    "The size of the input NewNames must be equal to the size of the input FolderPaths.");
                return;
            }

            var input = new RenameAttributeFoldersParameters
            {
                AttributeFolders = new List<AttributeFolderToRename>()
            };

            for (var i = 0; i < folderPaths.Count; i++)
            {
                input.AttributeFolders.Add(
                    new AttributeFolderToRename(
                        attributeType,
                        folderPaths[i],
                        newNames[i]));
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        // TODO: render IconSource/RenameAttributeFolders.svg to a 24 px
        // Resources/RenameAttributeFolders.png and register it in
        // Properties/Resources.resx; the binary asset could not be added in
        // this automated change, so the keynote folder icon stands in.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyKeynoteFolders;

        public override Guid ComponentGuid =>
            new Guid("dd822956-459c-4e0b-a43d-ca78ca674c1f");
    }
}
