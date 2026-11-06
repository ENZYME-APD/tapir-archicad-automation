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
    public class UnmergeElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "UnmergeElements";

        public UnmergeElementsComponent()
            : base(
                "UnmergeElements",
                "Removes the merge between the two elements of each given pair. Undoable; one result per pair.",
                GroupNames.ElementSolidOperations)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids1",
                "First element of each merged pair to separate.");

            InGenerics(
                "ElementGuids2",
                "Second element of each merged pair to separate. The order within a pair does not matter.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each pair (empty when it was unmerged).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> firstElements))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<GH_ObjectWrapper> secondElements))
            {
                return;
            }

            if (firstElements.Count != secondElements.Count)
            {
                this.AddError(
                    "The size of the inputs ElementGuids1 and ElementGuids2 must be equal.");
                return;
            }

            var pairs = new JArray();
            for (var i = 0; i < firstElements.Count; i++)
            {
                var elementId1 = GuidObject<ElementGuid>.CreateFromWrapper(firstElements[i]);
                var elementId2 = GuidObject<ElementGuid>.CreateFromWrapper(secondElements[i]);
                if (elementId1 == null || elementId2 == null)
                {
                    this.AddError(
                        "Invalid element identifier in the ElementGuids1 or ElementGuids2 input.");
                    return;
                }

                pairs.Add(
                    new JObject
                    {
                        ["elementId1"] = new JObject { ["guid"] = elementId1.Guid },
                        ["elementId2"] = new JObject { ["guid"] = elementId2.Guid }
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["elementPairs"] = pairs },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.UnmergeElements;

        public override Guid ComponentGuid =>
            new Guid("341fccaf-8222-4b3c-b7fc-668ca9541771");
    }
}
