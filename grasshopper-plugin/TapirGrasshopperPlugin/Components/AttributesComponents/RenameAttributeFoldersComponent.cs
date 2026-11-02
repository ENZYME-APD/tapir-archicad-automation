using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class RenameAttributeFoldersComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "RenameAttributeFolders";

        public RenameAttributeFoldersComponent()
            : base(
                "RenameAttributeFolders",
                "Renames attribute folders, each given by attribute type and path, to the given new names. " +
                "The root folder cannot be renamed. Available from Archicad 26.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "AttributeTypes",
                AttributeFolderInput.AttributeTypesDescription);

            inManager.AddTextParameter(
                "Paths",
                "Paths",
                ("Path of each folder to rename, one branch per folder: the folder names from the root " +
                "folder down to the folder to rename, the root folder itself not included.").WithTypeName("textTree"),
                GH_ParamAccess.tree);

            InTexts(
                "NewNames",
                "New name of each folder (one per branch of Paths).");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each folder (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!AttributeFolderInput.TryGetFolders(
                    this,
                    da,
                    0,
                    1,
                    out var folders))
            {
                return;
            }

            if (!da.TryGetList(
                    2,
                    out List<string> newNames))
            {
                return;
            }

            if (newNames.Count != folders.Count)
            {
                this.AddError(
                    "The size of the input NewNames must be equal to the number of branches in the input Paths.");
                return;
            }

            for (var i = 0; i < folders.Count; i++)
            {
                folders[i]["newName"] = newNames[i];
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["attributeFolders"] = new JArray(folders) },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RenameAttributeFolders;

        public override Guid ComponentGuid =>
            new Guid("df2c7204-7ade-4704-8bf9-0301b1722881");
    }
}
