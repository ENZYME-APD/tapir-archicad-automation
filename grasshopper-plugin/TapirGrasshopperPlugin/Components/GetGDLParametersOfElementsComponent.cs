using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Text.Json;
using Tapir.Data;
using Tapir.Utilities;

namespace Tapir.Components
{
    public class ElementGDLParametersObj
    {
        [JsonProperty ("gdlParametersOfElements")]
        public List<GDLParameterListObj> GdlParametersOfElements;
    }

    public class GetGDLParametersOfElementsComponent : ArchicadAccessorComponent
    {
        public GetGDLParametersOfElementsComponent ()
          : base (
                "Get GDL parameters of elements",
                "ElemGDLParameters",
                "Get GDL parameters of elements.",
                "Elements"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("ElementIds", "ElementIds", "Element ids to get parameters for.", GH_ParamAccess.list);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("ElementIds", "ElementIds", "List of element ids with valid GDL parameters.", GH_ParamAccess.list);
            pManager.AddGenericParameter ("GDLParamLists", "GDLParamLists", "List of GDL parameters lists for elements.", GH_ParamAccess.list);
        }

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            List<ElementIdItemObj> elements = new List<ElementIdItemObj> ();
            if (!DA.GetDataList (0, elements)) {
                return;
            }

            ElementsObj inputElements = new ElementsObj () {
                Elements = elements
            };

            JObject inputElementsObj = JObject.FromObject (inputElements);
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "GetGDLParametersOfElements", inputElementsObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }

            List<ElementIdItemObj> validElementIds = new List<ElementIdItemObj> ();
            List<GDLParameterListObj> validGdlParameters = new List<GDLParameterListObj> ();
            ElementGDLParametersObj elemGdlParameters = response.Result.ToObject<ElementGDLParametersObj> ();

            for (int i = 0; i < elemGdlParameters.GdlParametersOfElements.Count; i++) {
                GDLParameterListObj paramList = elemGdlParameters.GdlParametersOfElements[i];
                if (paramList == null || paramList.Parameters == null || paramList.Parameters.Count == 0) {
                    continue;
                }
                validElementIds.Add (inputElements.Elements[i]);
                validGdlParameters.Add (paramList);
            }

            DA.SetDataList (0, validElementIds);
            DA.SetDataList (1, validGdlParameters);
        }

        protected override System.Drawing.Bitmap Icon => Tapir.Properties.Resources.TapirLogo;

        public override Guid ComponentGuid => new Guid ("c963f9b4-5e98-4c8a-8ed9-f2d67000a6e8");
    }
}
