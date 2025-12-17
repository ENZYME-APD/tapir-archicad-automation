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
        public string GetCommandName => "GetGDLParametersOfElements";

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

            InText(
                "ParameterName",
                "Parameter name to find and set.");

            InGeneric(
                "Value",
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
                    out ElementsObject elementsObject))
            {
                return;
            }

            if (!da.TryGet(
                    1,
                    out string parameterName))
            {
                return;
            }

            if (!da.TryGet(
                    2,
                    out object value))
            {
                return;
            }

            if (!TryGetConvertedCadValues(
                    GetCommandName,
                    elementsObject,
                    ToAddOn,
                    JHelp.Deserialize<GDLParametersResponse>,
                    out GDLParametersResponse getResponse))
            {
                return;
            }

            var gdlHolders = getResponse.ToGdlHolders(
                elementsObject.Elements.Select(x => x.ElementId).ToList(),
                parameterName);

            var input = gdlHolders.ToSetInput(value);

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