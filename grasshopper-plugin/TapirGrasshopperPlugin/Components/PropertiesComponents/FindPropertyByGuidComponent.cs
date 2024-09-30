using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class FindPropertyByGuidComponent : Component
    {
        public FindPropertyByGuidComponent ()
          : base (
                "Find property by guid",
                "FindPropertyByGuid",
                "Finds a property by goud in a list of properties.",
                "Properties"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("PropertyDetails", "PropertyDetails", "The list of property details to find in.", GH_ParamAccess.list);
            pManager.AddTextParameter ("PropertyGuid", "PropertyGuid", "Property guid name to find.", GH_ParamAccess.item);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("PropertyId", "PropertyId", "Found property id.", GH_ParamAccess.item);
        }

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            List<PropertyDetailsObj> details = new List<PropertyDetailsObj> ();
            if (!DA.GetDataList (0, details)) {
                return;
            }

            string propertyGuid = "";
            if (!DA.GetData (1, ref propertyGuid)) {
                return;
            }

            PropertyDetailsObj found = null;
            propertyGuid = propertyGuid.ToLower ();
            foreach (PropertyDetailsObj detail in details) {
                if (detail.PropertyId.Guid.ToLower () == propertyGuid) {
                    found = detail;
                    break;
                }
            }
            if (found != null) {
                DA.SetData (0, found.PropertyId);
            }
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.TapirLogo;

        public override Guid ComponentGuid => new Guid ("d7f26316-9d62-48b4-854f-ab3d79db1cbf");
    }
}
