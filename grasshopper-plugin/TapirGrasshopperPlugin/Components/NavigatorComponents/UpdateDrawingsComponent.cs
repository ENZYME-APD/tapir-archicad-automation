using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.ResponseTypes.Generic;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class UpdateDrawingsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "UpdateDrawings";

        public UpdateDrawingsComponent()
            : base(
                "UpdateDrawings",
                "Performs a drawing update on the given elements.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Elements Guids of the Drawings to update.");
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult),
                ExecutionResult.Doc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            SolveByElementsInputResponse(da);
        }

        public override Guid ComponentGuid =>
            new Guid("bfba1890-06d6-4902-b817-5e06e99398b2");
    }
}