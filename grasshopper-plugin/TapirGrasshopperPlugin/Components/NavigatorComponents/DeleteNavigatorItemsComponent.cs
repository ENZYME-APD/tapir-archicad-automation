using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class DeleteNavigatorItem : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteNavigatorItems";

        public DeleteNavigatorItem()
            : base(
                "DeleteNavigatorItems",
                "Deletes navigator items from the navigator tree: views and folders of the View Map, layouts and " +
                "master layouts of the Layout Book, and independent worksheets and details of the Project Map. " +
                "Layout subsets and marker-linked worksheets and details cannot be deleted. An item that Archicad " +
                "refuses to delete (in a Teamwork project: it is not reserved or the user has no right to delete " +
                "it) is reported as a failed execution result.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "NavigatorItemIds",
                "Identifier of navigator items to delete.");

            // Appended after the original input, so saved definitions keep their wiring.
            InBoolean(
                "DryRun",
                "If true, nothing is deleted: ErrorMessages says what the deletion would do for each " +
                "item, from the checks made before deleting (corrupt id, item not found, layout subset, " +
                "View Map folder that is not empty). Archicad can still refuse an item that passes them. " +
                "Default is false.",
                false);

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each navigator item (empty when it was deleted, or with DryRun when it " +
                "would be deleted).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out NavigatorItemsObject input))
            {
                return;
            }

            var parameters = JObject.FromObject(input);

            // Only sent when it differs from the command's default.
            if (da.GetOptional(1, false))
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
            Properties.Resources.DeleteNavigatorItems;

        public override Guid ComponentGuid =>
            new Guid("b4ff32b4-91ac-405d-96ed-1938aec11eb3");
    }
}