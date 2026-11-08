using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class DeleteClassificationSystemsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteClassificationSystems";

        public DeleteClassificationSystemsComponent()
            : base(
                "DeleteClassificationSystems",
                "Delete the given Classification Systems.",
                GroupNames.Classifications)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ClassificationSystemGuids",
                "Identifiers of the classification systems to delete.");

            // Appended after the original inputs, so saved definitions keep their wiring.
            InBoolean(
                "DryRun",
                "If true, nothing is deleted: ErrorMessages says what the deletion would do for each " +
                "classification system, from the check made before deleting (the classification system " +
                "exists). Archicad can still refuse to delete one that exists. Default is false.",
                false);

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each classification system (empty when it was deleted, or with DryRun when " +
                "it would be deleted).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> wrappers))
            {
                return;
            }

            var input = new DeleteClassificationSystemsParameters
            {
                ClassificationSystemIds = new List<ClassificationSystemGuidWrapper>()
            };

            foreach (var wrapper in wrappers)
            {
                var id = GuidObject<ClassificationGuid>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError(
                        "Invalid classification system identifier in the ClassificationSystemGuids input.");
                    return;
                }
                input.ClassificationSystemIds.Add(
                    new ClassificationSystemGuidWrapper { ClassificationSystemId = id });
            }

            var parameters = JObject.FromObject(input);

            // Only sent when it differs from the command's default.
            if (da.GetOptional(1, false))
            {
                parameters["dryRun"] = true;
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                parameters,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteClassificationSystems;

        public override Guid ComponentGuid =>
            new Guid("92807694-4a15-4384-9819-8c50830f0c0b");
    }
}
