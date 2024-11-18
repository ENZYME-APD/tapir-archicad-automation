using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class GetAllPropertiesComponent : ArchicadAccessorComponent
    {
        public GetAllPropertiesComponent ()
          : base (
                "All Properties",
                "AllProperties",
                "Get all properties.",
                "Properties"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {

        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("PropertyId", "PropertyId", "Property id.", GH_ParamAccess.list);
            pManager.AddTextParameter ("GroupName", "GroupName", "Property group name.", GH_ParamAccess.list);
            pManager.AddTextParameter ("PropertyName", "PropertyName", "Property name.", GH_ParamAccess.list);
            pManager.AddTextParameter ("FullName", "FullName", "Full name containing the group and the property name.", GH_ParamAccess.list);
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "GetAllProperties", null);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }

            AllProperties properties = response.Result.ToObject<AllProperties> ();
            List<PropertyIdObj> propertyIds = new List<PropertyIdObj> ();
            List<string> propertyGroupNames = new List<string> ();
            List<string> propertyNames = new List<string> ();
            List<string> fullNames = new List<string> ();
            foreach (PropertyDetailsObj detail in properties.Properties) {
                propertyIds.Add (detail.PropertyId);
                propertyGroupNames.Add (detail.PropertyGroupName);
                propertyNames.Add (detail.PropertyName);
                fullNames.Add (ArchicadUtils.JoinNames (detail.PropertyGroupName, detail.PropertyName));
            }

            DA.SetDataList (0, propertyIds);
            DA.SetDataList (1, propertyGroupNames);
            DA.SetDataList (2, propertyNames);
            DA.SetDataList (3, fullNames);
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.AllProperties;

        public override Guid ComponentGuid => new Guid ("79f924e4-5b26-4efe-bfaf-0e82f9bb3821");
    }
}
