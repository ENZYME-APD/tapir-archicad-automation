using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.SolidOperationComponents
{
    public class MergeElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "MergeElements";

        public MergeElementsComponent()
            : base(
                "MergeElements",
                "Merges the given construction elements (the Merge command of Archicad, which cleans up the " +
                "junctions of slabs, beams, walls and similar elements). Undoable. The element types are not " +
                "filtered: Archicad reports the ones it cannot merge.",
                GroupNames.ElementSolidOperations)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the construction elements to merge together. At least two.");
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult),
                ExecutionResult.Doc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return;
            }

            // The command answers with a single execution result, not wrapped in an object.
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["elements"] = JArray.FromObject(elements.Elements) },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var result = ExecutionResult.Deserialize(response);
            if (!result.Success)
            {
                this.AddError(
                    $"Failed execution: {result.Message()}");
            }

            da.SetData(
                0,
                result.Message());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MergeElements;

        public override Guid ComponentGuid =>
            new Guid("69e3de32-c5ac-444c-83a0-de1d7ef754ad");
    }
}
