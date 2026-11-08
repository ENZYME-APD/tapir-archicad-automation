using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class DeselectAllComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeselectAll";

        public DeselectAllComponent()
            : base(
                "DeselectAll",
                "Clears the selection in the current window of Archicad. " +
                "Unlike ChangeSelection with ClearSelection it does not need to read the selected elements first.",
                GroupNames.Elements)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult.Message),
                ExecutionResult.Doc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetConvertedCadValues(
                    CommandName,
                    null,
                    ToAddOn,
                    ExecutionResultResponse.Deserialize,
                    out ExecutionResultResponse response))
            {
                return;
            }

            da.SetData(
                0,
                response.Message());
        }

        // Shares the icon of ChangeSelection until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ChangeSelection;

        public override Guid ComponentGuid =>
            new Guid("30eaaffb-fc2d-48ac-ab8f-475caac894b5");
    }
}
