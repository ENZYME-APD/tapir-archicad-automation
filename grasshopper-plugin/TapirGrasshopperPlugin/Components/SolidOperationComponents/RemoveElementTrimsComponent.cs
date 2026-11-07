using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.SolidOperationComponents
{
    public class RemoveElementTrimsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "RemoveElementTrims";

        public RemoveElementTrimsComponent()
            : base(
                "RemoveElementTrims",
                "Remove the trim between an element and the roof or shell trimming it, per pair.",
                GroupNames.ElementSolidOperations)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the trimmed elements.");

            InGenerics(
                "TrimmingElementGuids",
                "Identifiers of the roofs or shells trimming them, one per element.");

            // Appended after the original inputs, so saved definitions keep their wiring.
            InBoolean(
                "DryRun",
                "If true, nothing is removed: ErrorMessages says what the removal would do for each pair, " +
                "from the check made before removing (the element is trimmed by the trimming element). " +
                "Default is false.",
                false);

            SetOptionality(2);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each pair (empty when it succeeded, or with DryRun when the trim would be " +
                "removed).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(0, out List<GH_ObjectWrapper> elements))
            {
                return;
            }

            if (!da.TryGetList(1, out List<GH_ObjectWrapper> trimmingElements))
            {
                return;
            }

            if (elements.Count != trimmingElements.Count)
            {
                this.AddError(
                    "The size of the inputs ElementGuids and TrimmingElementGuids must be equal.");
                return;
            }

            var pairs = new JArray();
            for (var i = 0; i < elements.Count; i++)
            {
                var elementId = GuidObject<ElementGuid>.CreateFromWrapper(elements[i]);
                var trimmingId = GuidObject<ElementGuid>.CreateFromWrapper(trimmingElements[i]);
                if (elementId == null || trimmingId == null)
                {
                    this.AddError(
                        "Invalid element identifier in the ElementGuids or TrimmingElementGuids input.");
                    return;
                }

                pairs.Add(new JObject
                {
                    ["elementId"] = new JObject { ["guid"] = elementId.Guid },
                    ["trimmingElementId"] = new JObject { ["guid"] = trimmingId.Guid }
                });
            }

            var parameters = new JObject { ["elementPairs"] = pairs };

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
            Properties.Resources.RemoveElementTrims;

        public override Guid ComponentGuid =>
            new Guid("1d04c3f2-86b4-b27e-bc1b-e46331d9bdac");
    }
}
