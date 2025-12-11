using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class SetGDLParametersComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "SetGDLParametersOfElements";

        public SetGDLParametersComponent()
            : base(
                "SetElementGDLs",
                "Sets the given GDL parameters of the given elements.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Elements Guids to set the parameters of.");

            InTexts(
                "ParameterNames",
                "Parameter name to find and set.");

            InGenerics(
                "Values",
                "Value to set the parameter to.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                nameof(ExecutionResultsResponse.ExecutionResults),
                ExecutionResult.Doc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elementsObj))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<string> parameterNames))
            {
                return;
            }

            if (!da.TryGetList(
                    2,
                    out List<object> values))
            {
                return;
            }

            if (parameterNames.Count != values.Count)
            {
                this.AddError("Parameter name to value count mismatch.");
                return;
            }

            var input = elementsObj.ToElementsWithGDLParameters(
                parameterNames,
                values);

            if (!TryGetConvertedCadValues(
                    CommandName,
                    input,
                    ToAddOn,
                    ExecutionResultsResponse.Deserialize,
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