using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetElementChangesSinceComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetElementChangesSince";

        public GetElementChangesSinceComponent()
            : base(
                "GetElementChangesSince",
                "Get the elements created, modified and deleted since a state captured by CaptureElementState, " +
                "or between two captured states. Reading every element again to find what changed is not needed.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "StateId",
                "The state to compare from, as captured by CaptureElementState.");

            InGeneric(
                "ToStateId",
                "The state to compare to, captured with the same type as StateId. Optional: without it the " +
                "project as it is now is compared, so the outputs list what changed since StateId was captured.");

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "NewElements",
                "The elements that exist in the later state but not in the earlier one.");

            OutGenerics(
                "ModifiedElements",
                "The elements that exist in both states and changed between them.");

            OutGenerics(
                "DeletedElements",
                "The elements that exist in the earlier state but not in the later one. They cannot be read any " +
                "more, only their identifiers are known.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreate(
                    0,
                    out ElementStateGuid stateId))
            {
                return;
            }

            ElementStateGuid toStateId = null;

            if (da.TryGet(
                    1,
                    out GH_ObjectWrapper toStateWrapper) &&
                toStateWrapper != null)
            {
                toStateId = GuidObject<ElementStateGuid>.CreateFromWrapper(toStateWrapper);
                if (toStateId == null)
                {
                    this.AddError("Invalid ToStateId.");
                    return;
                }
            }

            var parameters = new GetElementChangesSinceParameters
            {
                StateId = stateId,
                ToStateId = toStateId
            };

            if (!TryGetConvertedCadValues(
                    CommandName,
                    parameters,
                    ToAddOn,
                    JHelp.Deserialize<ElementChangesObject>,
                    out ElementChangesObject response))
            {
                return;
            }

            da.SetDataList(
                0,
                response.NewElements);

            da.SetDataList(
                1,
                response.ModifiedElements);

            da.SetDataList(
                2,
                response.DeletedElements);
        }

        // No icon of its own yet: the one of the revision changes of elements.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetRevisionChangesOfElements;

        public override Guid ComponentGuid =>
            new Guid("3a9f6d21-8e47-4c0b-b5d2-6f1a84c97e0d");
    }
}
