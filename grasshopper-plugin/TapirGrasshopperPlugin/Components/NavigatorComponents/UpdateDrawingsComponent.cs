using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.ResponseTypes.Element;
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
                "DrawingIds",
                "ElementsGuids of the Drawings to update.");
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
            if (!ElementsObj.TryCreate(
                    da,
                    0,
                    out ElementsObj input))
            {
                return;
            }

            if (!TryGetConvertedResponse(
                    CommandName,
                    input,
                    ExecutionResult.Deserialize,
                    out ExecutionResult response))
            {
                return;
            }

            da.SetData(
                0,
                response.Message());
        }

        public override Guid ComponentGuid =>
            new Guid("bfba1890-06d6-4902-b817-5e06e99398b2");
    }
}