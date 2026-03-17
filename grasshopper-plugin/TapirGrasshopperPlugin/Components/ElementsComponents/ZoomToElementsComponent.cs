using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ZoomToElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "FitInWindow";

        public ZoomToElementsComponent()
            : base(
                "ZoomToElements",
                "Zoom to the given elements. If no element is given, it works as 'fit in window', zooms to all elements in the project.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Element ids to zoom to.");
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

        public override Guid ComponentGuid =>
            new Guid("30bf1eeb-6542-4d37-81ea-20d5cff55d72");
    }
}