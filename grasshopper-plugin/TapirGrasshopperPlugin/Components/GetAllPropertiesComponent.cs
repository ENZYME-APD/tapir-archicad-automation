using Grasshopper;
using Grasshopper.Kernel;
using Newtonsoft.Json;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.Json;
using TapirGrasshopperPlugin.Components;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace Tapir.Components
{
    public class AllProperties
    {
        [JsonProperty ("properties")]
        public List<PropertyDetailsObj> Properties { get; set; }
    }

    public class GetAllPropertiesComponent : ArchicadAccessorComponent
    {
        public GetAllPropertiesComponent ()
          : base (
                "Get all properties",
                "GetProperties",
                "Get all of the properties in the current plan.",
                "Properties"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {

        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("PropertyDetails", "PropertyDetails", "The list of property details.", GH_ParamAccess.list);
        }

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "GetAllProperties", null);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
            AllProperties allProperties = response.Result.ToObject<AllProperties> ();
            DA.SetDataList (0, allProperties.Properties);
        }

        protected override System.Drawing.Bitmap Icon => Properties.Resources.TapirLogo;

        public override Guid ComponentGuid => new Guid ("8a431baa-acf4-47a7-8787-2aa5834b2ae6");
    }
}
