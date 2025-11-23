using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class DeleteElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteElements";

        public DeleteElementsComponent()
            : base(
                "DeleteElements",
                "Delete elements",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Element Ids to delete.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!ElementsObj.TryCreate(
                    da,
                    0,
                    out ElementsObj elements))
            {
                return;
            }

            if (!TryGetConvertedResponse(
                    CommandName,
                    elements,
                    out ExecutionResultObj executionResult)) { return; }

            if (!executionResult.Success)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    executionResult.Error.Message);
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteElements;

        public override Guid ComponentGuid =>
            new Guid("9880138a-731a-40f6-a820-aa47923a872f");
    }
}