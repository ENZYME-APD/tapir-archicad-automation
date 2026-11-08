using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class ResetZoomComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ResetZoom";

        public ResetZoomComponent()
            : base(
                "ResetZoom",
                "Steps back in the zoom history of the current window, like Archicad's Zoom Back.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            NavigatorComponentHelp.AddOptionalInteger(
                this,
                "NumberOfStepsBack",
                "The number of steps to go back in the zoom history of the current window (1 to 32767). " +
                "Optional, the default is 1.");
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
            var parameters = new JObject();

            NavigatorComponentHelp.CopyOptional<int>(
                da,
                0,
                parameters,
                "numberOfStepsBack");

            if (!TryGetConvertedCadValues(
                    CommandName,
                    parameters,
                    ToAddOn,
                    ExecutionResult.Deserialize,
                    out ExecutionResult response))
            {
                return;
            }

            if (!response.Success)
            {
                this.AddError(response.Message());
            }

            da.SetData(
                0,
                response.Message());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ResetZoom;

        public override Guid ComponentGuid =>
            new Guid("ae1594cb-7150-45f6-870f-823d039bcef5");
    }
}
