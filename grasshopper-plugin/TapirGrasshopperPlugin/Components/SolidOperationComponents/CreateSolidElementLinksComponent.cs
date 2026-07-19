using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Commands;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.SolidOperationComponents
{
    public class CreateSolidElementLinksComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateSolidElementLinks";

        public CreateSolidElementLinksComponent()
            : base(
                "CreateSolidElementLinks",
                "Create solid element operation links between target and operator elements.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "TargetGuids",
                "Identifiers of the target elements (to be cut or modified).");

            InGenerics(
                "OperatorGuids",
                "Identifiers of the operator elements (performing the operation).");

            InText(
                "Operation",
                "The solid operation type: Subtraction, SubtractionUpwards, SubtractionDownwards, Intersection or Addition.");
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

            if (!da.TryGet(
                    2,
                    out string operation) ||
                string.IsNullOrEmpty(operation))
            {
                this.AddError("Operation is required.");
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
                        OperatorId = operatorId,
                        Operation = operation
                    });
            }

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateSolidElementLinks;

        public override Guid ComponentGuid =>
            new Guid("046b5aba-13b2-45cb-b8cd-b7b4f5eb946b");
    }
}
