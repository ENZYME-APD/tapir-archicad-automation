using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class SetClassificationsOfElementsComponent
        : ArchicadExecutorComponent
    {
        public static string Doc => "Set classifications of elements.";
        public override string CommandName => "SetClassificationsOfElements";

        public SetClassificationsOfElementsComponent()
            : base(
                "Set Classifications",
                "SetClassifications",
                Doc,
                GroupNames.Classifications)
        {
        }

        protected override void AddInputs()
        {
            AddGeneric(
                "ClassificationSystemGuid",
                "The Guid of a classification system.");

            AddGenerics(
                "ClassificationItemGuids",
                "The Guids of classification items to assign for the given elements.");

            AddGenerics(
                "ElementGuids",
                "Elements Guids to set the classification for.");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            base.AddedToDocument(document);

            new ClassificationSystemValueList().AddAsSource(
                this,
                0);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var classificationSystemId = ClassificationIdObj.Create(
                da,
                0);
            if (classificationSystemId == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input ClsSystemGuid failed to collect data.");
                return;
            }

            var inputItemIds = new List<GH_ObjectWrapper>();
            if (!da.GetDataList(
                    1,
                    inputItemIds))
            {
                return;
            }

            var classificationItemIds = new List<ClassificationIdObj>();
            foreach (var obj in inputItemIds)
            {
                var itemId = ClassificationIdObj.Create(obj);
                if (itemId != null)
                {
                    classificationItemIds.Add(itemId);
                }
            }

            var elements = ElementsObj.Create(
                da,
                2);
            if (elements == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input ElementGuids failed to collect data.");
                return;
            }

            if (classificationItemIds.Count != 1 && elements.Elements.Count !=
                classificationItemIds.Count)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "The count of ClsItemGuids must be 1 or the same as the count of ElementGuids.");
                return;
            }

            var elementClassifications = new ElementClassificationsObj()
            {
                ElementClassifications =
                    new List<ElementClassificationObj>()
            };

            for (var i = 0; i < elements.Elements.Count; i++)
            {
                var elementId = elements.Elements[i];
                var elementClassification = new ElementClassificationObj()
                {
                    ElementId = elementId.ElementId,
                    Classification = new ClassificationObj()
                    {
                        ClassificationSystemId = classificationSystemId
                    }
                };
                var classificationItemId = classificationItemIds.Count == 1
                    ? classificationItemIds[0]
                    : classificationItemIds[i];
                elementClassification.Classification.ClassificationItemId =
                    classificationItemId.IsNullGuid()
                        ? null
                        : classificationItemId;
                elementClassifications.ElementClassifications.Add(
                    elementClassification);
            }

            GetResponse(
                CommandName,
                elementClassifications);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetClassifications;

        public override Guid ComponentGuid =>
            new Guid("d5f807eb-ba90-4616-bd31-325c1701506a");
    }
}