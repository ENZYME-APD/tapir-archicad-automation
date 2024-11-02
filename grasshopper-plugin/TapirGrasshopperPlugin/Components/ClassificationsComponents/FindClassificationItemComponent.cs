using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;
using System.Collections.Generic;
using ClassificationIdObj = TapirGrasshopperPlugin.Data.IdObj;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class FindClassificationItem : ArchicadAccessorComponent
    {
        public FindClassificationItem ()
          : base (
                "Find Classification Item",
                "FindClassification",
                "Finds a Classification Item by name or id in the given Classification System.",
                "Classifications"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("ClassificationSystemId", "SystemId", "The id of a classification system.", GH_ParamAccess.item);
            pManager.AddTextParameter ("Classification Item id", "ItemId", "Classification Item id to find.", GH_ParamAccess.item);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("ClassificationItemId", "ItemGuid", "Found ClassificationItem id.", GH_ParamAccess.item);
        }

        public override void AddedToDocument (GH_Document document)
        {
            base.AddedToDocument (document);

            new ClassificationSystemValueList ().AddAsSource (this, 0);
        }

        private ClassificationItemDetailsObj FindClassificationItemInTree (List<ClassificationItemObj> branch, string classificationItemId)
        {
            foreach (ClassificationItemObj item in branch) {
                if (item.ClassificationItem.Id.ToLower () == classificationItemId) {
                    return item.ClassificationItem;
                }
                if (item.ClassificationItem.Children != null) {
                    ClassificationItemDetailsObj foundInChildren = FindClassificationItemInTree (item.ClassificationItem.Children, classificationItemId);
                    if (foundInChildren != null) {
                        return foundInChildren;
                    }
                }
            }

            return null;
        }

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            ClassificationIdObj classificationSystemId = ClassificationIdObj.Create (DA, 0);
            if (classificationSystemId == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input ClassificationSystemId failed to collect data.");
                return;
            }

            string ClassificationItemId = "";
            if (!DA.GetData (1, ref ClassificationItemId)) {
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
            ClassificationItemId = ClassificationItemId.ToLower ();
            ClassificationItemDetailsObj found = FindClassificationItemInTree (classificationItemsInSystem.ClassificationItems, ClassificationItemId);

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
