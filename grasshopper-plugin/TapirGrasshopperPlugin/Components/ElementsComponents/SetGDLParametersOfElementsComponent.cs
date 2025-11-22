using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class SetGDLParametersOfElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "SetGDLParametersOfElements";

        public SetGDLParametersOfElementsComponent()
            : base(
                "SetElementGDLs",
                "Sets the given GDL parameters of the given elements.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "...",
                "...");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                nameof(ExecutionResultsResponse.ExecutionResults),
                "");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.GetItems(
                    0,
                    out List<string> jsonElements)) { return; }

            if (!GetConvertedResponse(
                    CommandName,
                    jsonElements,
                    out ExecutionResultsResponse response))
            {
                return;
            }

            da.SetDataList(
                0,
                response.ExecutionResults.Select(x => x.Message()));
        }

        public override Guid ComponentGuid =>
            new Guid("c6e20c16-6edc-446d-88c2-c83f2e30c0b3");
    }
}