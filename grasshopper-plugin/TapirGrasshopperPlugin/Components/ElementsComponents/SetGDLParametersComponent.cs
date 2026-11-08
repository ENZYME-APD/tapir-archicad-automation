using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
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
                GroupNames.ElementModification)
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

            // Appended after the original inputs, so saved definitions keep their wiring.
            InBoolean(
                "DryRun",
                "If true, no element is changed and no undo step is created: the parameter is written to " +
                "each element's parameter list and its Parameter Script runs as in a real run, " +
                "ExecutionResults says what the change would do for each element, from the checks made " +
                "before changing it, and OldValues and NewValues list the parameters a real run would " +
                "change. Default is false.",
                false);

            SetOptionality(3);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                nameof(ExecutionResultsResponse.ExecutionResults),
                ExecutionResult.Doc);

            // Appended after the original output, so saved definitions keep their wiring.
            OutTexts(
                "OldValues",
                "With DryRun, for each element that passed the checks, the parameters a real run would " +
                "change (the given one and the ones the Parameter Script changes with it) with their values " +
                "now, as a JSON array in the format of GetGDLParametersOfElements; null for the other " +
                "elements and without DryRun.");

            OutTexts(
                "NewValues",
                "With DryRun, for each element that passed the checks, the same parameters with the values " +
                "a real run would give them, as a JSON array in the format of GetGDLParametersOfElements; " +
                "null for the other elements and without DryRun.");
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

            var dryRun = da.GetOptional(
                3,
                false);
            if (!dryRun)
            {
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
                return;
            }

            // Only sent when it differs from the command's default.
            var parameters = JObject.FromObject(input);
            parameters["dryRun"] = true;

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject dryRunResponse))
            {
                return;
            }

            // A dry run changes nothing, so its refusals are reported, not
            // failed.
            AddDryRunWarnings(
                ExecutionResultsResponse.ErrorMessages(dryRunResponse));

            da.SetDataList(
                0,
                ExecutionResultsResponse.Deserialize(dryRunResponse).
                    ExecutionResults.Select(x => x.Message()));
            da.SetDataList(
                1,
                ExecutionResultsResponse.WouldChangeValues(
                    dryRunResponse,
                    "oldValue"));
            da.SetDataList(
                2,
                ExecutionResultsResponse.WouldChangeValues(
                    dryRunResponse,
                    "newValue"));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetElementGDLs;

        public override Guid ComponentGuid =>
            new Guid("c6e20c16-6edc-446d-88c2-c83f2e30c0b3");
    }
}