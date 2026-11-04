using Grasshopper.Kernel;
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
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when its user data was deleted).");
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

            SetCadValuesWithErrorMessages(
                CommandName,
                elements,
                ToAddOn,
                da);
        }

        // Shares the icon of DeleteUserDataOfAttributes until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteUserDataOfAttributes;

        public override Guid ComponentGuid =>
            new Guid("254c7963-9dcb-484c-b290-44147e0bcefc");
    }
}
