using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class SetClassificationsOfElementsComponent
        : ArchicadExecutorComponent
    {
        public override string CommandName => "SetClassificationsOfElements";

        public SetClassificationsOfElementsComponent()
            : base(
                "SetClassifications",
                "Set classifications of elements.",
                GroupNames.Classifications)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "Guid",
                "The Guid of a Classification system.");

            InGenerics(
                "ItemGuids",
                "The Guids of Classification items to assign for the given elements.");

            InGenerics(
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
            if (!ClassificationIdObj.TryCreate(
                    da,
                    0,
                    out ClassificationIdObj classificationSystemId))
            {
                return;
            }

            if (!da.TryGetItems(
                    1,
                    out List<GH_ObjectWrapper> inputItemIds))
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

            if (!ElementsObj.TryCreate(
                    da,
                    2,
                    out ElementsObj elements))
            {
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

            SetArchiCadValues(
                CommandName,
                elementClassifications);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetClassifications;

        public override Guid ComponentGuid =>
            new Guid("d5f807eb-ba90-4616-bd31-325c1701506a");
    }
}