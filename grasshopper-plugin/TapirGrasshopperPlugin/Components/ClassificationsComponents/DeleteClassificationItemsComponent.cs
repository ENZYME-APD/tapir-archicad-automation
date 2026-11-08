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
    public class DeleteClassificationItemsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteClassificationItems";

        public DeleteClassificationItemsComponent()
            : base(
                "DeleteClassificationItems",
                "Delete the given Classification Items.",
                GroupNames.Classifications)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ClassificationItemGuids",
                "Identifiers of the classification items to delete.");

            // Appended after the original inputs, so saved definitions keep their wiring.
            InBoolean(
                "DryRun",
                "If true, nothing is deleted: ErrorMessages says what the deletion would do for each " +
                "classification item, from the check made before deleting (the classification item exists). " +
                "Archicad can still refuse to delete one that exists. Default is false.",
                false);

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each classification item (empty when it was deleted, or with DryRun when " +
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

            var input = new DeleteClassificationItemsParameters
            {
                ClassificationItemIds = new List<ClassificationItemGuidWrapper>()
            };

            foreach (var wrapper in wrappers)
            {
                var id = GuidObject<ClassificationGuid>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError(
                        "Invalid classification item identifier in the ClassificationItemGuids input.");
                    return;
                }
                input.ClassificationItemIds.Add(
                    new ClassificationItemGuidWrapper { ClassificationItemId = id });
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
            Properties.Resources.DeleteClassificationItems;

        public override Guid ComponentGuid =>
            new Guid("e0ad2b8f-3bde-4684-a303-8daf7dd6bac3");
    }
}
