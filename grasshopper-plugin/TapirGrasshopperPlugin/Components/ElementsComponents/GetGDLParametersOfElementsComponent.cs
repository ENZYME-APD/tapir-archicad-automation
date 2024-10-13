using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
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
                "Elem GDL Parameters",
                "ElemGDLParameters",
                "Get GDL parameter values of elements.",
                "Elements"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("ElementIds", "ElementIds", "Element ids to get parameters for.", GH_ParamAccess.list);
            pManager.AddTextParameter ("ParamName", "ParamName", "Parameter name to find.", GH_ParamAccess.item);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("ElementIds", "ElementIds", "List of element ids with valid GDL parameters.", GH_ParamAccess.list);
            pManager.AddTextParameter ("ParamValues", "ParamValues", "Values of the found GDL parameters.", GH_ParamAccess.list);
        }

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            List<ElementIdItemObj> elements = new List<ElementIdItemObj> ();
            if (!DA.GetDataList (0, elements)) {
                return;
            }

            string paramName = "";
            if (!DA.GetData (1, ref paramName)) {
                return;
            }
            paramName = paramName.ToLower ();

            ElementsObj inputElements = new ElementsObj () {
                Elements = elements
            };
            JObject inputElementsObj = JObject.FromObject (inputElements);

            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "GetGDLParametersOfElements", inputElementsObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }

            ElementGDLParametersObj elemGdlParameters = response.Result.ToObject<ElementGDLParametersObj> ();
            List<ElementIdItemObj> validElementIds = new List<ElementIdItemObj> ();
            List<string> paramValues = new List<string> ();

            for (int i = 0; i < elemGdlParameters.GdlParametersOfElements.Count; i++) {
                GDLParameterListObj paramList = elemGdlParameters.GdlParametersOfElements[i];
                if (paramList == null || paramList.Parameters == null || paramList.Parameters.Count == 0) {
                    continue;
                }
                string paramValue = null;
                foreach (GDLParameterDetailsObj details in paramList.Parameters) {
                    if (details.Name.ToLower () == paramName) {
                        paramValue = details.Value.ToString ();
                        break;
                    }
                }

                if (paramValue == null) {
                    continue;
                }

                validElementIds.Add (inputElements.Elements[i]);
                paramValues.Add (paramValue);
            }

            DA.SetDataList (0, validElementIds);
            DA.SetDataList (1, paramValues);
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.ElemGDLParameters;

        public override Guid ComponentGuid => new Guid ("c963f9b4-5e98-4c8a-8ed9-f2d67000a6e8");
    }
}
