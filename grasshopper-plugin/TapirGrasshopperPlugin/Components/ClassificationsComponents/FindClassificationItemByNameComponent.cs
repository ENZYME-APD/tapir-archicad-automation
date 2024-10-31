using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;
using System.Collections.Generic;
using ClassificationIdObj = TapirGrasshopperPlugin.Data.IdObj;
using Newtonsoft.Json;
using System.Linq;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class FindClassificationItemByName : ArchicadAccessorComponent
    {
        public FindClassificationItemByName ()
          : base (
                "Classification Item by Name",
                "CItemByName",
                "Finds a Classification Item by name in the given Classification System.",
                "Classifications"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("ClassificationSystemId", "CSystemId", "The id of a classification system.", GH_ParamAccess.item);
            pManager.AddTextParameter ("Classification Item name", "ItemName", "Classification Item name to find.", GH_ParamAccess.item);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("ClassificationItemId", "CItemId", "Found ClassificationItem id.", GH_ParamAccess.item);
        }

        private ClassificationItemDetailsObj FindClassificationItemInTree (List<ClassificationItemObj> branch, string ClassificationItemName)
        {
            foreach (ClassificationItemObj item in branch) {
                if (item.ClassificationItem.Id.ToLower () == ClassificationItemName) {
                    return item.ClassificationItem;
                }
                if (item.ClassificationItem.Children != null) {
                    ClassificationItemDetailsObj foundInChildren = FindClassificationItemInTree (item.ClassificationItem.Children, ClassificationItemName);
                    if (foundInChildren != null) {
                        return foundInChildren;
                    }
                }
            }

            return null;
        }

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            ClassificationIdObj classificationSystemId = new ClassificationIdObj ();
            if (!DA.GetData (0, ref classificationSystemId)) {
                return;
            }

            string ClassificationItemName = "";
            if (!DA.GetData (1, ref ClassificationItemName)) {
                return;
            }

            ClassificationSystemObj classificationSystem = new ClassificationSystemObj () {
                ClassificationSystemId = classificationSystemId
            };

            JObject classificationSystemObj = JObject.FromObject (classificationSystem);
            CommandResponse response = SendArchicadCommand ("GetAllClassificationsInSystem", classificationSystemObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }

            AllClassificationItemsInSystem classificationItemsInSystem = response.Result.ToObject<AllClassificationItemsInSystem> ();
            ClassificationItemName = ClassificationItemName.ToLower ();
            ClassificationItemDetailsObj found = FindClassificationItemInTree (classificationItemsInSystem.ClassificationItems, ClassificationItemName);

            if (found == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "ClassificationItem is not found.");
            } else {
                DA.SetData (0, found.ClassificationItemId);
            }
        }

        // TODO
        // protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.ClassificationItemByName;

        public override Guid ComponentGuid => new Guid ("46026689-054d-4164-9d35-ac56150cd733");
    }
}
