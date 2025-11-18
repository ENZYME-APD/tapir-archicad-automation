using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Element;
using TapirGrasshopperPlugin.ResponseTypes.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class SetGDLParametersOfElementsComponent : ArchicadAccessorComponent
    {
        public static string Doc => "Sets the given GDL parameters of the given elements.";
        public override string CommandName => "SetGDLParametersOfElements";

        public SetGDLParametersOfElementsComponent()
            : base(
                "Set Elements GDL GdlArray",
                "...",
                Doc,
                GroupNames.Elements)
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
            pManager.AddTextParameter(
                nameof(GDLParametersResponse),
                nameof(GDLParametersResponse),
                nameof(GDLParametersResponse),
                GH_ParamAccess.list);
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