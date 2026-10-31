using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class DeleteAttributeFoldersComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteAttributeFolders";

        public DeleteAttributeFoldersComponent()
            : base(
                "DeleteAttributeFolders",
                "Deletes attribute folders, each given by attribute type and path, together with all the subfolders and " +
                "deletable attributes they contain (a folder holding an attribute that cannot be deleted is kept). " +
                "The root folder cannot be deleted. Available from Archicad 26.",
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
            Properties.Resources.DeleteAttributeFolders;

        public override Guid ComponentGuid =>
            new Guid("972d421c-3728-40b5-adb1-dcc6bb22477d");
    }
}
