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
    public class FindProperty : Component
    {
        public FindProperty ()
          : base (
                "Find property",
                "FindProperty",
                "Finds a property by group name and name in a list of properties.",
                "Properties"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("PropertyDetails", "PropertyDetails", "The list of property details to find in.", GH_ParamAccess.list);
            pManager.AddTextParameter ("Group name", "GroupName", "Property group name to find.", GH_ParamAccess.item);
            pManager.AddTextParameter ("Property name", "PropertyName", "Property name to find.", GH_ParamAccess.item);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("PropertyId", "PropertyId", "Found property id.", GH_ParamAccess.item);
        }

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            List<PropertyDetailsObj> details = new List<PropertyDetailsObj> ();
            if (!DA.GetDataList<PropertyDetailsObj> (0, details)) {
                return;
            }

            string propertyGroupName = "";
            string propertyName = "";
            if (!DA.GetData<string> (1, ref propertyGroupName) || !DA.GetData<string> (2, ref propertyName)) {
                return;
            }
            PropertyDetailsObj found = null;
            foreach (PropertyDetailsObj detail in details) {
                if (detail.PropertyGroupName == propertyGroupName && detail.PropertyName == propertyName) {
                    found = detail;
                    break;
                }
            }
            if (found != null) {
                DA.SetData (0, found.PropertyId);
            }
        }

        protected override System.Drawing.Bitmap Icon => Properties.Resources.TapirLogo;

        public override Guid ComponentGuid => new Guid ("9bb30fb5-9a68-4672-b807-4e35b2d27761");
    }
}
