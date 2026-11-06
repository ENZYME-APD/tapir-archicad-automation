using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Generic;

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
                "Element ids to delete.");

            // Appended after the original input, so saved definitions keep their wiring.
            InBoolean(
                "DryRun",
                "If true, nothing is deleted and no undo step is created: ErrorMessages says what the " +
                "deletion would do for each element, from the checks made before deleting (the element " +
                "does not exist, its layer is locked, it is not editable). Archicad can still refuse an " +
                "element that passes them. Default is false.",
                false);

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when it was deleted, or with DryRun when it " +
                "would be deleted).");
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

            var parameters = JObject.FromObject(input);

            // Only sent when it differs from the command's default.
            var dryRun = da.GetOptional(
                1,
                false);
            if (dryRun)
            {
                parameters["dryRun"] = true;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var messages = ExecutionResultsResponse.ErrorMessages(response);
            foreach (var message in messages)
            {
                if (string.IsNullOrEmpty(message))
                {
                    continue;
                }

                // A dry run deletes nothing, so its refusals are reported,
                // not failed.
                if (dryRun)
                {
                    AddRuntimeMessage(
                        GH_RuntimeMessageLevel.Warning,
                        message);
                }
                else
                {
                    this.AddError(message);
                }
            }

            da.SetDataList(
                0,
                messages);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteElements;

        public override Guid ComponentGuid =>
            new Guid("9880138a-731a-40f6-a820-aa47923a872f");
    }
}