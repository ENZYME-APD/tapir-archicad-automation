using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.SolidOperationComponents
{
    public class CreateSolidOperationsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateSolidOperations";

        public CreateSolidOperationsComponent()
            : base(
                "CreateSolidOperations",
                "Runs a solid operation (subtraction, intersection, addition) on two morphs and returns the " +
                "resulting morphs, one branch per operation. Unlike CreateSolidElementLinks this is not a link: " +
                "the target and the operator morph are replaced by the result. Morphs only; the operations run " +
                "in one undo step.",
                GroupNames.ElementSolidOperations)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "TargetGuids",
                "Identifier of the target morph of each operation.");

            InGenerics(
                "OperatorGuids",
                "Identifier of the operator morph of each operation.");

            InTexts(
                "Operations",
                "The solid operation type of each operation: Subtraction, SubtractionUpwards, SubtractionDownwards, " +
                "Intersection or Addition (input only 1 to use the same for all).");
        }

        protected override void AddOutputs()
        {
            OutGenericTree(
                "ResultGuids",
                "The morphs created by each operation (one branch per operation, empty when it failed).");

            OutErrorMessages(
                "Error message of each operation (empty when it succeeded).");
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

            if (!da.TryGetList(
                    2,
                    out List<string> operations))
            {
                return;
            }

            if (targets.Count != operators.Count)
            {
                this.AddError(
                    "The size of the inputs TargetGuids and OperatorGuids must be equal.");
                return;
            }

            if (operations.Count != 1 &&
                operations.Count != targets.Count)
            {
                this.AddError(
                    "The size of the input Operations must be 1 or equal to the size of the input TargetGuids.");
                return;
            }

            var items = new JArray();
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

                items.Add(
                    new JObject
                    {
                        ["targetId"] = new JObject { ["guid"] = targetId.Guid },
                        ["operatorId"] = new JObject { ["guid"] = operatorId.Guid },
                        ["operation"] = operations[operations.Count == 1 ? 0 : i] ?? ""
                    });
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["operations"] = items },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var resultGuids = new DataTree<ElementGuidWrapper>();
            var errorMessages = new List<string>();

            var results = JsonOutputHelp.Items(
                response,
                "results");
            for (var i = 0; i < results.Count; i++)
            {
                var path = new GH_Path(i);
                resultGuids.EnsurePath(path);

                if (JsonOutputHelp.IsError(results[i]))
                {
                    errorMessages.Add(JsonOutputHelp.ErrorMessage(results[i]));
                    continue;
                }

                foreach (var element in JsonOutputHelp.Items(
                             results[i] as JObject,
                             "elements"))
                {
                    resultGuids.Add(
                        ElementsStructuredGetterComponent.ElementIdOf(element),
                        path);
                }

                errorMessages.Add("");
            }

            da.SetDataTree(0, resultGuids);
            da.SetDataList(1, errorMessages);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateSolidOperations;

        public override Guid ComponentGuid =>
            new Guid("d84da1ad-f745-4c68-a15a-14e11be924d1");
    }
}
