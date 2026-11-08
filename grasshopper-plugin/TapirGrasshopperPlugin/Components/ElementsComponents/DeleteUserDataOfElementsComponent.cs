using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class DeleteUserDataOfElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteUserDataOfElements";

        public DeleteUserDataOfElementsComponent()
            : base(
                "DeleteUserDataOfElements",
                "Deletes the user data the Tapir add-on stored on the given elements: every key " +
                "SetElementUserData stored on them. " +
                "Archicad keeps the user data of an element per add-on, so the data of other add-ons is left " +
                "as it is. All deletions are one undo step.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to delete the user data of.");

            // Appended after the original inputs, so saved definitions keep their wiring.
            InBoolean(
                "DryRun",
                "If true, nothing is deleted: ErrorMessages says what the deletion would do for each " +
                "element, from the check made before deleting (the element exists). Archicad can still " +
                "refuse to delete the user data of an element that passes it. Default is false.",
                false);

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when its user data was deleted, or with DryRun when it " +
                "would be deleted).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return;
            }

            var parameters = JObject.FromObject(elements);

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
            Properties.Resources.DeleteUserDataOfElements;

        public override Guid ComponentGuid =>
            new Guid("254c7963-9dcb-484c-b290-44147e0bcefc");
    }
}
