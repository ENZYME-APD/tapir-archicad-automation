﻿using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class SetClassificationsOfElementsComponent : ArchicadExecutorComponent
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
            pManager.AddGenericParameter ("ClassificationSystemGuid", "ClsSystemGuid", "The Guid of a classification system.", GH_ParamAccess.item);
            pManager.AddGenericParameter ("ClassificationItemGuids", "ClsItemGuids", "The Guids of classification items to assign for the given elements.", GH_ParamAccess.list);
            pManager.AddGenericParameter ("ElementGuids", "ElementGuids", "Element Guids to set the classification for.", GH_ParamAccess.list);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
        }

        public override void AddedToDocument (GH_Document document)
        {
            base.AddedToDocument (document);

            new ClassificationSystemValueList ().AddAsSource (this, 0);
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            ClassificationIdObj classificationSystemId = ClassificationIdObj.Create (DA, 0);
            if (classificationSystemId == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input ClsSystemGuid failed to collect data.");
                return;
            }

            List<GH_ObjectWrapper> inputItemIds = new List<GH_ObjectWrapper> ();
            if (!DA.GetDataList (1, inputItemIds)) {
                return;
            }

            List<ClassificationIdObj> classificationItemIds = new List<ClassificationIdObj> ();
            foreach (GH_ObjectWrapper obj in inputItemIds) {
                ClassificationIdObj itemId = ClassificationIdObj.Create (obj);
                if (itemId != null) {
                    classificationItemIds.Add (itemId);
                }
            }

            ElementsObj elements = ElementsObj.Create (DA, 2);
            if (elements == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input ElementGuids failed to collect data.");
                return;
            }

            if (classificationItemIds.Count != 1 && elements.Elements.Count != classificationItemIds.Count) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "The count of ClsItemGuids must be 1 or the same as the count of ElementGuids.");
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
                        ClassificationSystemId = classificationSystemId
                    }
                };
                ClassificationIdObj classificationItemId = classificationItemIds.Count == 1 ? classificationItemIds[0] : classificationItemIds[i];
                elementClassification.Classification.ClassificationItemId = classificationItemId.IsNullGuid () ? null : classificationItemId;
                elementClassifications.ElementClassifications.Add (elementClassification);
            }

            JObject elementClassificationsObj = JObject.FromObject (elementClassifications);
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "SetClassificationsOfElements", elementClassificationsObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
        }

        protected override System.Drawing.Bitmap Icon => Properties.Resources.SetClassifications;

        public override Guid ComponentGuid => new Guid ("d5f807eb-ba90-4616-bd31-325c1701506a");
    }
}
