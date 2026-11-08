using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ReleaseElementStateComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ReleaseElementState";

        public ReleaseElementStateComponent()
            : base(
                "ReleaseElementState",
                "Free a state captured by CaptureElementState. Every state is also dropped when the project is " +
                "closed or Archicad quits.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "StateId",
                "The state to free, as captured by CaptureElementState.");
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

            // An unknown state is a failed execution result, which
            // TryGetConvertedCadValues reports as an error.
            TryGetConvertedCadValues(
                CommandName,
                new ReleaseElementStateParameters { StateId = stateId },
                ToAddOn,
                ExecutionResultResponse.Deserialize,
                out ExecutionResultResponse _);
        }

        // No icon of its own yet: the one of releasing elements.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ReleaseElements;

        public override Guid ComponentGuid =>
            new Guid("e52b7c08-1d94-4a6f-8c3e-b7d05a2f91c6");
    }
}
