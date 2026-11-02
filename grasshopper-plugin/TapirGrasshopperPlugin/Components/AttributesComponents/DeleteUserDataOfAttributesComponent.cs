using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class DeleteUserDataOfAttributesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteUserDataOfAttributes";

        public DeleteUserDataOfAttributesComponent()
            : base(
                "DeleteUserDataOfAttributes",
                "Removes the user data of attributes, every key at once. Archicad does not record this change as " +
                "an undo step, so Undo does not revert it.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "AttributeGuids",
                "Identifiers of the attributes to remove the user data of.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each attribute (empty when its user data was removed).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out AttributeGuidItemsObject input))
            {
                return;
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteUserDataOfAttributes;

        public override Guid ComponentGuid =>
            new Guid("d0c1a3f3-a05a-4209-83e5-dc56b624d8b4");
    }
}
