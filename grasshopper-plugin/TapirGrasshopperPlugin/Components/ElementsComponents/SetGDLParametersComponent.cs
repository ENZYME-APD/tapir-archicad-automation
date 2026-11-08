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
    // Sends one SetGDLParametersOfElements request for every branch of the
    // inputs, so a tree input makes one undo step in Archicad (#203).
    public class SetGDLParametersComponent
        : BatchedExecutorComponent<ElementWithGDLParameters>
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

        protected override int DryRunInputIndex => 3;

        protected override int OldValuesOutputIndex => 1;

        protected override int NewValuesOutputIndex => 2;

        protected override bool TryGetBatchItems(
            IGH_DataAccess da,
            out List<ElementWithGDLParameters> items)
        {
            items = null;

            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return false;
            }

            if (!da.TryGet(
                    1,
                    out string parameterName))
            {
                return false;
            }

            // The Value is tried as each type in turn and every failed
            // conversion adds an error, so the messages are cleared after the
            // tries; the messages of the earlier iterations are put back.
            var earlierMessages =
                new List<KeyValuePair<GH_RuntimeMessageLevel, string>>();
            foreach (var level in new[]
                     {
                         GH_RuntimeMessageLevel.Error,
                         GH_RuntimeMessageLevel.Warning,
                         GH_RuntimeMessageLevel.Remark
                     })
            {
                foreach (var message in RuntimeMessages(level))
                {
                    earlierMessages.Add(
                        new KeyValuePair<GH_RuntimeMessageLevel, string>(
                            level,
                            message));
                }
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
            this.ClearRuntimeMessages();
            foreach (var message in earlierMessages)
            {
                AddRuntimeMessage(
                    message.Key,
                    message.Value);
            }

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
                return false;
            }

            items = new List<ElementWithGDLParameters>();

            foreach (var element in elements.Elements)
            {
                items.Add(
                    new ElementWithGDLParameters()
                    {
                        ElementId = element.ElementId,
                        GdlParameterList = new SetGdlParameterArray
                        {
                            setParametersInput
                        }
                    });
            }

            return true;
        }

        // The branches used to be sent one after the other, so of a
        // parameter given twice for an element the last value won. GDL
        // parameter names are not case sensitive.
        protected override string GetBatchItemKey(
            ElementWithGDLParameters item)
        {
            return $"{item.ElementId?.Guid?.ToLowerInvariant()}|" +
                   $"{item.GdlParameterList?.FirstOrDefault()?.Name?.ToLowerInvariant()}";
        }

        protected override object CreateBatchParameters(
            List<ElementWithGDLParameters> items)
        {
            return new ElementsWithGDLParametersInput
            {
                ElementsWithGDLParameters = items
            };
        }

        // The ExecutionResults output says "Success." or the failure of each
        // element, and a real run in which every element failed is an error.
        protected override List<string> GetBatchErrorMessages(
            JObject response,
            bool dryRun)
        {
            var results = ExecutionResultsResponse.Deserialize(response).
                ExecutionResults;

            if (!dryRun &&
                results.Count > 0 &&
                results.All(x => !x.Success))
            {
                this.AddError(
                    $"Failed executions, the first error message: {results.First().Message()}");
            }

            return results.Select(x => x.Message()).ToList();
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetElementGDLs;

        public override Guid ComponentGuid =>
            new Guid("c6e20c16-6edc-446d-88c2-c83f2e30c0b3");
    }
}