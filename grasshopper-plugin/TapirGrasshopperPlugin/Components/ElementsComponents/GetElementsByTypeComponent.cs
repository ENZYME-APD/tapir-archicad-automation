using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Text.Json;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ElementsByTypeObj
    {
        [JsonProperty ("elementType")]
        public string ElementType;
    }


    public class GetElementsByTypeComponent : ArchicadAccessorComponent
    {
        public GetElementsByTypeComponent ()
          : base (
                "Elems By Type",
                "ElemsByType",
                "Get all elements by type.",
                "Elements"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddTextParameter ("Type", "Type", "Element type.", GH_ParamAccess.item);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("ElementIds", "ElementIds", "List of element ids matching the type.", GH_ParamAccess.list);
        }

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            string elemType = "";
            if (!DA.GetData (0, ref elemType)) {
                return;
            }

            ElementsByTypeObj elementsByType = new ElementsByTypeObj () {
                ElementType = elemType
            };
            JObject elementyByTypeObj = JObject.FromObject (elementsByType);
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "GetElementsByType", elementyByTypeObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
            ElementsObj elements = response.Result.ToObject<ElementsObj> ();
            DA.SetDataList (0, elements.Elements);
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.TapirLogo;

        public override Guid ComponentGuid => new Guid ("8075031e-b38d-4f3b-8e5c-8e740d13a091");
    }
}