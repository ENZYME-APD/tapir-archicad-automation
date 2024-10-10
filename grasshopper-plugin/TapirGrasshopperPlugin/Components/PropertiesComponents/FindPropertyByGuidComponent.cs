using Grasshopper.Kernel;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class FindPropertyByGuidComponent : ArchicadAccessorComponent
    {
        public FindPropertyByGuidComponent ()
          : base (
                "Property by Guid",
                "PropertyByGuid",
                "Finds a property by guid.",
                "Properties"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddTextParameter ("PropertyGuid", "PropertyGuid", "Property guid name to find.", GH_ParamAccess.item);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("PropertyId", "PropertyId", "Found property id.", GH_ParamAccess.item);
        }

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            string propertyGuid = "";
            if (!DA.GetData (0, ref propertyGuid)) {
                return;
            }

            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "GetAllProperties", null);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }

            AllProperties properties = response.Result.ToObject<AllProperties> ();
            PropertyDetailsObj found = null;
            propertyGuid = propertyGuid.ToLower ();
            foreach (PropertyDetailsObj detail in properties.Properties) {
                if (detail.PropertyId.Guid.ToLower () == propertyGuid) {
                    found = detail;
                    break;
                }
            }

            if (found == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Property is not found.");
            } else {
                DA.SetData (0, found.PropertyId);
            }
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.PropertyByGuid;

        public override Guid ComponentGuid => new Guid ("d7f26316-9d62-48b4-854f-ab3d79db1cbf");
    }
}
