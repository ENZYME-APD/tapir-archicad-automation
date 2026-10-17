using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class ShowAllHiddenElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ShowAllHiddenElements";

        public ShowAllHiddenElementsComponent()
            : base(
                "ShowAllHiddenElements",
                "Show every element hidden in the current window, by HideElements or in Archicad.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetConvertedCadValues(
                    CommandName,
                    null,
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
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.UnlockElements;

        public override Guid ComponentGuid =>
            new Guid("df1ce1dd-db6f-4e28-bfc0-a034943a0a4a");
    }
}
