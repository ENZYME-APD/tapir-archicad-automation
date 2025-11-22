using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
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
        public static string Doc =>
            "Sets the classifications of elements. " +
            "In order to set the classification of an element to unclassified, " +
            "omit the classificationItemId field. " +
            "It works for subelements of hierarchal elements also.";

        public override string CommandName => "SetClassificationsOfElements";

        public SetClassificationsOfElementsComponent()
            : base(
                "Set Elements Classifications",
                "...",
                Doc,
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
                "");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.GetItems(
                    0,
                    out List<string> jsonElements)) { return; }

            var jObject = JObject.FromObject(jsonElements);

            if (!GetConvertedResponse(
                    CommandName,
                    jObject,
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