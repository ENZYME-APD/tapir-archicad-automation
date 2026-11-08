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

            // Appended after the original inputs, so saved definitions keep their wiring.
            InBoolean(
                "DryRun",
                "If true, nothing is deleted: ErrorMessages says what the deletion would do for each folder, " +
                "from the checks made before deleting (the folder exists and is not the root folder). " +
                "Archicad can still refuse to delete a folder that passes them. Default is false.",
                false);

            SetOptionality(2);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each folder (empty when it was deleted, or with DryRun when it would be " +
                "deleted).");
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

            var parameters = new JObject { ["attributeFolders"] = new JArray(folders) };

            // Only sent when it differs from the command's default.
            if (da.GetOptional(2, false))
            {
                parameters["dryRun"] = true;
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                parameters,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteAttributeFolders;

        public override Guid ComponentGuid =>
            new Guid("972d421c-3728-40b5-adb1-dcc6bb22477d");
    }
}
