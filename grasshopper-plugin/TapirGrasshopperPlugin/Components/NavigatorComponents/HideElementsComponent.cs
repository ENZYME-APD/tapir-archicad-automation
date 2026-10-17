using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class HideElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "HideElements";

        public HideElementsComponent()
            : base(
                "HideElements",
                "Hide the given elements in the current window. ShowAllHiddenElements shows them again.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Elements to hide.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject input))
            {
                return;
            }

            if (input.Elements.Count == 0)
            {
                return;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    input,
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
            Properties.Resources.LockElements;

        public override Guid ComponentGuid =>
            new Guid("12a4e70c-c3fa-4be2-91d1-9cf16c4868f1");
    }
}
