using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class GetAllClassificationsComponent : ArchicadAccessorComponent
    {
        public GetAllClassificationsComponent ()
          : base ("All Classifications in the given Classification System.",
                  "AllClassifications",
                  "Get all Classifications in the given Classification System.",
                  "Classifications")
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddTextParameter ("ClassificationSystemId", "SystemId", "Found Classification System Id.", GH_ParamAccess.list);
            pManager.AddTextParameter ("ClassificationSystemDisplayId", "SystemDisplayId", "Found Classification System display id.", GH_ParamAccess.list);
            pManager.AddTextParameter ("ClassificationItemId", "ItemId", "Found ClassificationItem id.", GH_ParamAccess.list);
            pManager.AddTextParameter ("ClassificationItemDisplayId", "ItemDisplayId", "Found ClassificationItem display id.", GH_ParamAccess.list);
        }

        private List<ClassificationItemDetailsObj> GetAllClassificationItemFromTree (List<ClassificationItemObj> tree)
        {
            List<ClassificationItemDetailsObj> list = new List<ClassificationItemDetailsObj> ();
            foreach (ClassificationItemObj item in tree) {
                list.Add (item.ClassificationItem);
                if (item.ClassificationItem.Children != null) {
                    list.AddRange (GetAllClassificationItemFromTree (item.ClassificationItem.Children));
                }
            }

            return list;
        }

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            CommandResponse response = SendArchicadCommand ("GetAllClassificationSystems", null);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }

            Dictionary<ClassificationSystemDetailsObj, List<ClassificationItemDetailsObj>> itemsPerSystems = new Dictionary<ClassificationSystemDetailsObj, List<ClassificationItemDetailsObj>> ();
            AllClassificationSystems classificationSystems = response.Result.ToObject<AllClassificationSystems> ();
            foreach (ClassificationSystemDetailsObj system in classificationSystems.ClassificationSystems) {
                ClassificationSystemObj classificationSystem = new ClassificationSystemObj () {
                    ClassificationSystemId = system.ClassificationSystemId
                };

                JObject classificationSystemObj = JObject.FromObject (classificationSystem);
                response = SendArchicadCommand ("GetAllClassificationsInSystem", classificationSystemObj);
                if (!response.Succeeded) {
                    AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                    return;
                }

                AllClassificationItemsInSystem classificationItemsInSystem = response.Result.ToObject<AllClassificationItemsInSystem> ();
                List<ClassificationItemDetailsObj> itemsInSystem = GetAllClassificationItemFromTree (classificationItemsInSystem.ClassificationItems);
                itemsPerSystems.Add (system, itemsInSystem);
            }

            List<string> systemIds = new List<string> ();
            List<string> systemDisplayIds = new List<string> ();
            List<string> itemIds = new List<string> ();
            List<string> itemDisplayIds = new List<string> ();
            foreach (KeyValuePair<ClassificationSystemDetailsObj, List<ClassificationItemDetailsObj>> itemsInSystem in itemsPerSystems) {
                foreach (ClassificationItemDetailsObj itemDetail in itemsInSystem.Value) {
                    systemIds.Add (itemsInSystem.Key.ClassificationSystemId.Guid);
                    systemDisplayIds.Add (itemsInSystem.Key.ToString ());
                    itemIds.Add (itemDetail.ClassificationItemId.Guid);
                    itemDisplayIds.Add (itemDetail.ToString ());
                }
            }

            DA.SetDataList (0, systemIds);
            DA.SetDataList (1, systemDisplayIds);
            DA.SetDataList (2, itemIds);
            DA.SetDataList (3, itemDisplayIds);
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.AllClassifications;

        public override Guid ComponentGuid => new Guid ("46a81cf1-e043-4cb7-b587-c2a6d3349bd8");
    }
}