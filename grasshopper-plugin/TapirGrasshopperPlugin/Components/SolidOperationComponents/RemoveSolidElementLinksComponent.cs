using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Commands;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.SolidOperationComponents
{
    public class RemoveSolidElementLinksComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "RemoveSolidElementLinks";

        public RemoveSolidElementLinksComponent()
            : base(
                "RemoveSolidElementLinks",
                "Remove solid element operation links between target and operator elements.",
                GroupNames.ElementSolidOperations)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "TargetGuids",
                "Identifiers of the target elements.");

            InGenerics(
                "OperatorGuids",
                "Identifiers of the operator elements.");

            // Appended after the original inputs, so saved definitions keep their wiring.
            InBoolean(
                "DryRun",
                "If true, nothing is removed: ErrorMessages says what the removal would do for each " +
                "operation, from the check made before removing (the operator is linked to the target). " +
                "Default is false.",
                false);

            SetOptionality(2);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each operation (empty when it succeeded, or with DryRun when the link " +
                "would be removed).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> targets))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<GH_ObjectWrapper> operators))
            {
                return;
            }

            if (targets.Count != operators.Count)
            {
                this.AddError(
                    "The size of the inputs TargetGuids and OperatorGuids must be equal.");
                return;
            }

            var input = new SolidLinksParameters { SolidLinks = new List<SolidLink>() };

            for (var i = 0; i < targets.Count; i++)
            {
                var targetId = GuidObject<ElementGuid>.CreateFromWrapper(targets[i]);
                var operatorId = GuidObject<ElementGuid>.CreateFromWrapper(operators[i]);
                if (targetId == null || operatorId == null)
                {
                    this.AddError(
                        "Invalid element identifier in the TargetGuids or OperatorGuids input.");
                    return;
                }

                input.SolidLinks.Add(
                    new SolidLink
                    {
                        TargetId = targetId,
                        OperatorId = operatorId
                    });
            }

            var parameters = JObject.FromObject(input);

            // Only sent when it differs from the command's default.
            if (da.GetOptional(2, false))
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
            Properties.Resources.RemoveSolidElementLinks;

        public override Guid ComponentGuid =>
            new Guid("19601cfe-6c32-46a6-ae18-775933c3f673");
    }
}
