using Grasshopper.Kernel;
using Grasshopper.Kernel.Special;
using System;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;
using TapirGrasshopperPlugin.Components;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class ClassificationSystemValueList : ArchicadAccessorValueList
    {
        public ClassificationSystemValueList () :
            base ("Classification Systems", "", "Value List for Classification Systems.", "Classifications")
        {
        }

        public override void RefreshItems ()
        {
            CommandResponse response = SendArchicadCommand ("GetAllClassificationSystems", null);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }

            ListItems.Clear ();

            AllClassificationSystems classificationSystems = response.Result.ToObject<AllClassificationSystems> ();
            foreach (ClassificationSystemDetailsObj system in classificationSystems.ClassificationSystems) {
                ListItems.Add (new GH_ValueListItem (system.ToString (), '"' + system.ClassificationSystemId.Guid + '"'));
            }
            ListItems[0].Selected = true;
        }

        public override Guid ComponentGuid => new Guid ("a4206a77-3e1e-42e1-b220-dbd7aafdf8f5");
    }
}