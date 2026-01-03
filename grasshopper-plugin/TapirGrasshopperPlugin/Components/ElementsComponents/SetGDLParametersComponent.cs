using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class SetGDLParametersComponent : ArchicadExecutorComponent
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
                    out ElementsObject elements))
            {
                return;
            }

            if (!da.TryGet(
                    1,
                    out string parameterName))
            {
                return;
            }

            bool isString = false;
            bool isInt = false;
            bool isDouble = false;
            bool isBool = false;
            string stringValue = null;
            int intValue = 0;
            double doubleValue = 0;
            bool boolValue = false;
            if (da.TryGet(
                    2,
                    out stringValue))
            {
                isString = true;
            }
            else if (da.TryGet(
                    2,
                    out intValue))
            {
                isInt = true;
            }
            else if (da.TryGet(
                    2,
                    out doubleValue))
            {
                isDouble = true;
            }
            else if (da.TryGet(
                    2,
                    out boolValue))
            {
                isBool = true;
            }
            else
            {
                this.AddError(
                    $"Value input must be string, integer or a real number");
            }
            this.ClearRuntimeMessages();

            SetGdlParameterDetails setParametersInput = null;
            if (isString)
            {
                setParametersInput = new SetGdlParameterDetailsString
                {
                    Name = parameterName,
                    Value = stringValue
                };
            }
            else if (isInt)
            {
                setParametersInput = new SetGdlParameterDetailsInteger
                {
                    Name = parameterName,
                    Value = intValue
                };
            }
            else if (isDouble)
            {
                setParametersInput = new SetGdlParameterDetailsDouble
                {
                    Name = parameterName,
                    Value = doubleValue
                };
            }
            else if (isBool)
            {
                setParametersInput = new SetGdlParameterDetailsBoolean
                {
                    Name = parameterName,
                    Value = boolValue
                };
            }
            else
            {
                this.AddError(
                    $"Value input must be string, integer, real number or a boolean");
                return;
            }

            var input = new ElementsWithGDLParametersInput
            {
                ElementsWithGDLParameters =
                    new List<ElementWithGDLParameters>()
            };

            foreach (var element in elements.Elements)
            {
                input.ElementsWithGDLParameters.Add(
                    new ElementWithGDLParameters()
                    {
                        ElementId = element.ElementId,
                        GdlParameterList = new SetGdlParameterArray()
                    });
                input.ElementsWithGDLParameters.Last().
                    GdlParameterList.Add(setParametersInput);
            }

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