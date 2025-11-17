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
        public static string CommandName => "SetClassificationsOfElements";

        public SetClassificationsOfElementsComponent()
            : base(
                "Set Element Classifications",
                "...",
                "...",
                GroupNames.Element)
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

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddTextParameter(
                nameof(ExecutionResultsResponse.ExecutionResults),
                "",
                "",
                GH_ParamAccess.list);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.GetItems(
                    0,
                    out List<string> jsonElements)) { return; }

            var jObject = JObject.FromObject(jsonElements);

            if (!GetResponse(
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