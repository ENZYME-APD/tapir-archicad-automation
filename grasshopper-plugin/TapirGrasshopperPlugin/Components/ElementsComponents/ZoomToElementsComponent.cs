using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ZoomToElementsComponent : ArchicadExecutorComponent
    {
        // ZoomToElements zooms to the elements in the 3D window too, which FitInWindow's
        // element list does not; it needs elements, so with none given FitInWindow fits
        // the whole drawing as before.
        public override string CommandName => "ZoomToElements";

        private const string FitInWindowCommandName = "FitInWindow";

        public ZoomToElementsComponent()
            : base(
                "ZoomToElements",
                "Zoom the current window, 2D or 3D, to the given elements; the selection stays as it was. If no element is given, it works as 'fit in window', zooms to all elements in the project.",
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

            string commandName =
                input.Elements != null && input.Elements.Count > 0
                    ? CommandName
                    : FitInWindowCommandName;

            if (!TryGetConvertedCadValues(
                    commandName,
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
            Properties.Resources.ZoomToElements;

        public override Guid ComponentGuid =>
            new Guid("30bf1eeb-6542-4d37-81ea-20d5cff55d72");
    }
}