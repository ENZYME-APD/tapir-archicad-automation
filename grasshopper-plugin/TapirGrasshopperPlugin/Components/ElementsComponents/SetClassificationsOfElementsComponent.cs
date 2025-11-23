using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class SetClassificationsOfElementsComponent
        : ArchicadAccessorComponent
    {
        public override string CommandName => "SetClassificationsOfElements";

        public SetClassificationsOfElementsComponent()
            : base(
                "SetElementsClassifications",
                "Sets the classifications of elements. " +
                "In order to set the classification of an element to unclassified, " +
                "omit the classificationItemId field. " +
                "It works for subelements of hierarchal elements also.",
                GroupNames.Elements)
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
            //pManager.AddTextParameter(
            //    nameof(ElementGDLParametersObj),
            //    nameof(ElementGDLParametersObj),
            //    nameof(ElementGDLParametersObj),
            //    GH_ParamAccess.list);
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
            if (!da.TryGetItems(
                    0,
                    out List<string> jsonElements)) { return; }

            if (!TryGetConvertedResponse(
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
            new Guid("a51682dd-0225-4664-8d8b-d28872e38526");
    }
}