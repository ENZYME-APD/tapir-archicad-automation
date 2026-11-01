using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class DeleteSelectedElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteSelectedElements";

        public DeleteSelectedElementsComponent()
            : base(
                "DeleteSelectedElements",
                "Deletes the currently selected elements in one undo step, the same as pressing the Delete key.",
                GroupNames.Elements)
        {
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out var response))
            {
                this.CheckExecutionResult(response);
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteSelectedElements;

        public override Guid ComponentGuid =>
            new Guid("47ece79e-ee27-4fb3-9199-a7211fb94c3b");
    }
}
