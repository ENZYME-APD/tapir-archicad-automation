using Grasshopper.Kernel;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class FindPropertyByName : ArchicadAccessorComponent
    {
        public FindPropertyByName ()
          : base (
                "Property by Name",
                "PropertyByName",
                "Finds a property by group name and name.",
                "Properties"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddTextParameter ("Group name", "GroupName", "Property group name to find.", GH_ParamAccess.item);
            pManager.AddTextParameter ("Property name", "PropertyName", "Property name to find.", GH_ParamAccess.item);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("PropertyGuid", "PropertyGuid", "Found property Guid.", GH_ParamAccess.item);
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            string propertyGroupName = "";
            string propertyName = "";
            if (!DA.GetData (0, ref propertyGroupName) || !DA.GetData (1, ref propertyName)) {
                return;
            }

            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "GetAllProperties", null);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }

            AllProperties properties = response.Result.ToObject<AllProperties> ();
            PropertyDetailsObj found = null;
            propertyGroupName = propertyGroupName.ToLower ();
            propertyName = propertyName.ToLower ();
            foreach (PropertyDetailsObj detail in properties.Properties) {
                if (detail.PropertyGroupName.ToLower () == propertyGroupName && detail.PropertyName.ToLower () == propertyName) {
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

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.PropertyByName;

        public override Guid ComponentGuid => new Guid ("9bb30fb5-9a68-4672-b807-4e35b2d27761");
    }
}
