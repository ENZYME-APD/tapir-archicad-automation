using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class CreateAttributeFoldersComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateAttributeFolders";

        public CreateAttributeFoldersComponent()
            : base(
                "CreateAttributeFolders",
                "Create attribute folders, each given by its path from the root folder of the attribute type. " +
                "Missing parent folders are created too; a folder that already exists is kept. " +
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
                "Path of each folder to create: the folder names from the root folder down, separated by '/' (e.g. Structure/Walls).");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each folder (empty when it was created or already existed).");
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

            var input = new AttributeFoldersParameters
            {
                AttributeFolders = folderPaths.ConvertAll(
                    pathText => new AttributeFolder(
                        attributeType,
                        pathText))
            };

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        // TODO: render IconSource/CreateAttributeFolders.svg to a 24 px
        // Resources/CreateAttributeFolders.png and register it in
        // Properties/Resources.resx; the binary asset could not be added in
        // this automated change, so the keynote folder icon stands in.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateKeynoteFolders;

        public override Guid ComponentGuid =>
            new Guid("82c05777-9aae-4e5b-a4d2-10221e2e9567");
    }
}
