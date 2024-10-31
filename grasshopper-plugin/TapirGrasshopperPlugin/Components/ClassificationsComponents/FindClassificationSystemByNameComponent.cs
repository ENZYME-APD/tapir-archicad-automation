using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class FindClassificationSystemByName : ArchicadAccessorComponent
    {
        public FindClassificationSystemByName ()
          : base (
                "Classification System by Name",
                "CSystemByName",
                "Finds a Classification System by name.",
                "Classifications"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddTextParameter ("Classification System name", "SystemName", "Classification System name to find.", GH_ParamAccess.item);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("ClassificationSystemId", "CSystemId", "Found ClassificationSystem id.", GH_ParamAccess.item);
        }

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            string ClassificationSystemName = "";
            if (!DA.GetData (0, ref ClassificationSystemName)) {
                return;
            }

            CommandResponse response = SendArchicadCommand ("GetAllClassificationSystems", null);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }

            AllClassificationSystems classificationSystems = response.Result.ToObject<AllClassificationSystems> ();
            ClassificationSystemDetailsObj found = null;
            ClassificationSystemName = ClassificationSystemName.ToLower ();
            foreach (ClassificationSystemDetailsObj system in classificationSystems.ClassificationSystems) {
                if (system.Name.ToLower () == ClassificationSystemName) {
                    found = system;
                    break;
                }
            }

            if (found == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "ClassificationSystem is not found.");
            } else {
                DA.SetData (0, found.ClassificationSystemId);
            }
        }

        // TODO
        // protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.ClassificationSystemByName;

        public override Guid ComponentGuid => new Guid ("a4206a77-3e1e-42e1-b220-dbd7aafdf8f5");
    }
}
