using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class CreateAttributeFoldersComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateAttributeFolders";

        public CreateAttributeFoldersComponent()
            : base(
                "CreateAttributeFolders",
                "Creates attribute folders, each given by attribute type and path (the folder names from the root folder down). " +
                "Missing parent folders are created too; a folder that already exists is kept and reported as success. " +
                "Available from Archicad 26.",
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
                AttributeFolderInput.PathsDescription.WithTypeName("textTree"),
                GH_ParamAccess.tree);
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

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["attributeFolders"] = new JArray(folders) },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateAttributeFolders;

        public override Guid ComponentGuid =>
            new Guid("da8488b7-3c0e-4f95-8347-9ad384a0370f");
    }
}
