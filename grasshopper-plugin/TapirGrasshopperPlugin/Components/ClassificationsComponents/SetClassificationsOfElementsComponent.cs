using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using System.Linq;
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
                "Sets the classifications of elements. " +
                "In order to set the classification of an element to unclassified, " +
                "omit the classificationItemId field. " +
                "It works for subelements of hierarchal elements also.",
                GroupNames.Classifications)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "SystemGuid",
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
            if (!da.TryCreate(
                    0,
                    out ClassificationGuid systemId))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<GH_ObjectWrapper> wrappers))
            {
                return;
            }

            var guids = new List<ClassificationGuid>();

            foreach (var wrapper in wrappers)
            {
                var itemId = ClassificationGuid.CreateFromWrapper(wrapper);
                if (itemId != null)
                {
                    guids.Add(itemId);
                }
            }

            if (!da.TryCreateFromList(
                    2,
                    out ElementsObject elements))
            {
                return;
            }

            if (guids.Count != 1 && elements.Elements.Count != guids.Count)
            {
                this.AddError("Classification- to ElementGuid count mismatch!");
                return;
            }

            var elementClassifications = new List<ElementClassificationObj>();

            for (var i = 0; i < elements.Elements.Count; i++)
            {
                var elementId = elements.Elements[i];

                var elementClassification = new ElementClassificationObj
                {
                    ElementId = elementId.ElementId,
                    Classification = new ClassificationObj
                    {
                        ClassificationSystemId = systemId
                    }
                };

                var classificationItemId = guids.Count == 1
                    ? guids[0]
                    : guids[i];

                elementClassification.Classification.ClassificationItemId =
                    classificationItemId.IsNullGuid()
                        ? null
                        : classificationItemId;

                elementClassifications.Add(elementClassification);
            }

            SetValues(
                CommandName,
                new { elementClassifications },
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetClassifications;

        public override Guid ComponentGuid =>
            new Guid("d5f807eb-ba90-4616-bd31-325c1701506a");
    }
}