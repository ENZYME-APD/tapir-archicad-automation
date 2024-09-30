using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class FindGDLParameterByNameComponent : Component
    {
        public FindGDLParameterByNameComponent ()
          : base (
                "Find GDL parameter value by name",
                "FindGDLParamByName",
                "Find GDL parameter value by name.",
                "Elements"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("ElementIds", "ElementIds", "Element ids for the GDL parameter lists.", GH_ParamAccess.list);
            pManager.AddGenericParameter ("GDLParamLists", "GDLParamLists", "GDL parameters lists to search in.", GH_ParamAccess.list);
            pManager.AddTextParameter ("Parameter name", "ParamName", "Parameter name to find.", GH_ParamAccess.item);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("ElementIds", "ElementIds", "Element ids the parameter is available for.", GH_ParamAccess.list);
            pManager.AddTextParameter ("ParamValues", "ParamValues", "Values of the found GDL parameters.", GH_ParamAccess.list);
        }

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            List<ElementIdItemObj> elementIds = new List<ElementIdItemObj> ();
            if (!DA.GetDataList (0, elementIds)) {
                return;
            }

            List<GDLParameterListObj> paramLists = new List<GDLParameterListObj> ();
            if (!DA.GetDataList (1, paramLists)) {
                return;
            }

            string paramName = "";
            if (!DA.GetData (2, ref paramName)) {
                return;
            }

            if (elementIds.Count != paramLists.Count) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "The count of ElementIds and GDLParamLists must be the same.");
                return;
            }

            List<ElementIdItemObj> validElementIds = new List<ElementIdItemObj> ();
            List<string> paramValues = new List<string> ();
            paramName = paramName.ToLower ();
            for (int i = 0; i < paramLists.Count; i++) {
                GDLParameterListObj paramList = paramLists[i];
                foreach (GDLParameterDetailsObj details in paramList.Parameters) {
                    if (details.Name.ToLower () == paramName) {
                        validElementIds.Add (elementIds[i]);
                        paramValues.Add (details.Value.ToString ());
                        break;
                    }
                }
            }

            DA.SetDataList (0, validElementIds);
            DA.SetDataList (1, paramValues);
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.TapirLogo;

        public override Guid ComponentGuid => new Guid ("c4c0f24c-21c2-4d21-9fa8-a5b32ae55e92");
    }
}
