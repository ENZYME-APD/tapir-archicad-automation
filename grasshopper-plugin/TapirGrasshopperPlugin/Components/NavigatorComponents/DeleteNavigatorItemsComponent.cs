using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class DeleteNavigatorItem : ArchicadExecutorComponent
    {
        public DeleteNavigatorItem ()
          : base (
                "DeleteNavigatorItems",
                "DeleteNavigatorItems",
                "Deletes items from navigator tree.",
                "Navigator"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("NavigatorItemIds", "NavigatorItemIds", "Identifier of navigator items to delete.", GH_ParamAccess.list);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            NavigatorItemIdsObj navigatorItemIds = NavigatorItemIdsObj.Create (DA, 0);
            if (navigatorItemIds == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input NavigatorItemIds failed to collect data.");
                return;
            }

            JObject inputObj = JObject.FromObject (navigatorItemIds);
            CommandResponse response = SendArchicadCommand ("DeleteNavigatorItems", inputObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.DeleteNavigatorItems;

        public override Guid ComponentGuid => new Guid ("b4ff32b4-91ac-405d-96ed-1938aec11eb3");
    }
}