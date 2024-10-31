using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;
using ClassificationIdObj = TapirGrasshopperPlugin.Data.IdObj;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class SetClassificationsOfElementsComponent : ArchicadAccessorComponent
    {
        public SetClassificationsOfElementsComponent ()
          : base (
                "Set Classifications",
                "SetClassifications",
                "Set classifications of elements.",
                "Classifications"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("ClassificationSystemId", "CSystemId", "The id of a classification system.", GH_ParamAccess.item);
            pManager.AddGenericParameter ("ElementIds", "ElementIds", "Element ids to set the classification for.", GH_ParamAccess.list);
            pManager.AddGenericParameter ("ClassificationItemIds", "CItemIds", "The ids of classification items to assign for the given elements.", GH_ParamAccess.list);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
        }

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            ClassificationIdObj classificationSystemId = new ClassificationIdObj ();
            if (!DA.GetData (0, ref classificationSystemId)) {
                return;
            }
            ElementsObj elements = ElementsObj.Create (DA, 1);
            if (elements == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input ElementIds failed to collect data.");
                return;
            }
            List<ClassificationIdObj> classificationItemIds = new List<ClassificationIdObj> ();
            if (!DA.GetDataList (2, classificationItemIds)) {
                return;
            }

            if (classificationItemIds.Count != 1 && elements.Elements.Count != classificationItemIds.Count) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "The count of CItemIds must be 1 or the same as the count of ElementIds.");
                return;
            }

            ElementClassificationsObj elementClassifications = new ElementClassificationsObj () {
                ElementClassifications = new List<ElementClassificationObj> ()
            };

            for (int i = 0; i < elements.Elements.Count; i++) {
                ElementIdItemObj elementId = elements.Elements[i];
                ElementClassificationObj elementClassification = new ElementClassificationObj () {
                    ElementId = elementId.ElementId,
                    Classification = new ClassificationObj () {
                        ClassificationSystemId = classificationSystemId,
                        ClassificationItemId = classificationItemIds.Count == 1 ? classificationItemIds[0] : classificationItemIds[i]
                    }
                };
                elementClassifications.ElementClassifications.Add (elementClassification);
            }

            JObject elementClassificationsObj = JObject.FromObject (elementClassifications);
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "SetClassificationsOfElements", elementClassificationsObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
        }

        // TODO
        // protected override System.Drawing.Bitmap Icon => Properties.Resources.SetClassifications;

        public override Guid ComponentGuid => new Guid ("d5f807eb-ba90-4616-bd31-325c1701506a");
    }
}
