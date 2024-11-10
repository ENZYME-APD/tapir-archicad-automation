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
            pManager.AddTextParameter ("ClassificationSystemNameAndVersion", "SystemNameAndVersion", "Found Classification System name and version.", GH_ParamAccess.list);
            pManager.AddTextParameter ("ClassificationItemId", "ItemId", "Found ClassificationItem id.", GH_ParamAccess.list);
            pManager.AddTextParameter ("ClassificationItemDisplayId", "ItemDisplayId", "Found ClassificationItem display id.", GH_ParamAccess.list);
            pManager.AddTextParameter ("ClassificationItemName", "ItemName", "Found ClassificationItem name.", GH_ParamAccess.list);
            pManager.AddTextParameter ("ClassificationItemPath", "ItemPath", "Path to ClassificationItem.", GH_ParamAccess.list);
        }

        private List<Tuple<ClassificationItemDetailsObj, string>> GetAllClassificationItemFromTree (List<ClassificationItemObj> tree, string pathToRoot = "")
        {
            List<Tuple<ClassificationItemDetailsObj, string>> list = new List<Tuple<ClassificationItemDetailsObj, string>> ();
            foreach (ClassificationItemObj item in tree) {
                string path = pathToRoot + '/' + item.ClassificationItem.Id;
                list.Add (new Tuple<ClassificationItemDetailsObj, string> (item.ClassificationItem, path));
                if (item.ClassificationItem.Children != null) {
                    list.AddRange (GetAllClassificationItemFromTree (item.ClassificationItem.Children, path));
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

            Dictionary<ClassificationSystemDetailsObj, List<Tuple<ClassificationItemDetailsObj, string>>> itemsPerSystems = new Dictionary<ClassificationSystemDetailsObj, List<Tuple<ClassificationItemDetailsObj, string>>> ();
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
                List<Tuple<ClassificationItemDetailsObj, string>> itemsInSystem = GetAllClassificationItemFromTree (classificationItemsInSystem.ClassificationItems);
                itemsPerSystems.Add (system, itemsInSystem);
            }

            List<string> systemIds = new List<string> ();
            List<string> systemNamesAndVersions = new List<string> ();
            List<string> itemIds = new List<string> ();
            List<string> itemDisplayIds = new List<string> ();
            List<string> itemNames = new List<string> ();
            List<string> itemPaths = new List<string> ();
            foreach (KeyValuePair<ClassificationSystemDetailsObj, List<Tuple<ClassificationItemDetailsObj, string>>> itemsInSystem in itemsPerSystems) {
                ClassificationSystemDetailsObj system = itemsInSystem.Key;
                foreach (Tuple<ClassificationItemDetailsObj, string> itemDetailAndPath in itemsInSystem.Value) {
                    ClassificationItemDetailsObj itemDetail = itemDetailAndPath.Item1;
                    string itemPath = itemDetailAndPath.Item2;

                    systemIds.Add (system.ClassificationSystemId.Guid);
                    systemNamesAndVersions.Add (system.ToString ());
                    itemIds.Add (itemDetail.ClassificationItemId.Guid);
                    itemDisplayIds.Add (itemDetail.Id);
                    itemNames.Add (itemDetail.Name);
                    itemPaths.Add (itemPath);
                }
            }

            DA.SetDataList (0, systemIds);
            DA.SetDataList (1, systemNamesAndVersions);
            DA.SetDataList (2, itemIds);
            DA.SetDataList (3, itemDisplayIds);
            DA.SetDataList (4, itemPaths);
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.AllClassifications;

        public override Guid ComponentGuid => new Guid ("46a81cf1-e043-4cb7-b587-c2a6d3349bd8");
    }
}