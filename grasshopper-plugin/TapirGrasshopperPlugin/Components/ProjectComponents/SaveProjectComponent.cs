using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class SaveProjectComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "SaveProject";

        public SaveProjectComponent()
            : base(
                "SaveProject",
                "Saves the current project.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
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
            new Guid("8a6d3473-d937-433f-80b2-13b0f07ca669");
    }
}