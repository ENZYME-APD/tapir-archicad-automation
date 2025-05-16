using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class FindClassificationItemByIdComponent : ArchicadAccessorComponent
    {
        public FindClassificationItemByIdComponent ()
          : base (
                "Find Classification Item By Id",
                "ClassificationById",
                "Finds a Classification Item by id in the given Classification System.",
                "Classifications"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("ClassificationSystemGuid", "SystemGuid", "The Guid of a classification system.", GH_ParamAccess.item);
            pManager.AddTextParameter ("Classification Item id", "ItemId", "Classification Item id to find.", GH_ParamAccess.item);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("ClassificationItemGuid", "ItemGuid", "Found ClassificationItem Guid.", GH_ParamAccess.item);
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

        protected override void Solve (IGH_DataAccess DA)
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

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.ClassificationById;

        public override Guid ComponentGuid => new Guid ("46026689-054d-4164-9d35-ac56150cd733");
    }
}
