using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Linq;
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

        protected override void AddInputs()
        {
            InBoolean(
                "DryRun",
                "If true, nothing is deleted: ElementGuids lists the selected elements the deletion would " +
                "delete (for a marquee, the ones it touches). A real run also deletes the other members of " +
                "their groups when Suspend Groups is off, and the elements linked to them (labels, " +
                "dimensions, ...). Default is false.",
                false);

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Only with DryRun: the selected elements the deletion would delete.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            // Only sent when it differs from the command's default.
            var dryRun = da.GetOptional(
                0,
                false);
            if (dryRun)
            {
                parameters["dryRun"] = true;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out var response) ||
                !this.CheckExecutionResult(response))
            {
                return;
            }

            if (dryRun)
            {
                da.SetDataList(
                    0,
                    JsonOutputHelp.Items(
                            response,
                            "elements")
                        .Select(item => ElementsStructuredGetterComponent.ElementIdOf(item))
                        .ToList());
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteSelectedElements;

        public override Guid ComponentGuid =>
            new Guid("47ece79e-ee27-4fb3-9199-a7211fb94c3b");
    }
}
