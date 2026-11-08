using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    // Sends one SetClassificationsOfElements request for every branch of the
    // inputs, so a tree input makes one undo step in Archicad (#203).
    public class SetClassificationsOfElementsComponent
        : BatchedExecutorComponent<ElementClassificationObj>
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
                "ClassificationSystemGuid",
                "The Guid of a Classification system.");

            InGenerics(
                "ClassificationItemGuids",
                "The Guids of Classification items to assign for the given elements.");

            InGenerics(
                "ElementGuids",
                "Elements Guids to set the classification for.");

            // Appended after the original inputs, so saved definitions keep their wiring.
            InBoolean(
                "DryRun",
                "If true, nothing is changed and no undo step is created: ErrorMessages says what setting " +
                "the classification would do for each element, from the checks made before writing (the " +
                "element or the classification item does not exist), and OldValues and NewValues show the " +
                "change. Default is false.",
                false);

            SetOptionality(3);
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            AddAsSource<ClassificationSystemValueList>(
                document,
                0);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when it succeeded, or with DryRun when it would " +
                "succeed).");

            // Appended after the original output, so saved definitions keep their wiring.
            OutTexts(
                "OldValues",
                "With DryRun, the element's classification in the system now, for each element that passed " +
                "the checks, as JSON (classificationSystemId, and classificationItemId when it is " +
                "classified); null for the other elements and without DryRun.");

            OutTexts(
                "NewValues",
                "With DryRun, the classification a real run would set, for each element that passed the " +
                "checks, as JSON (classificationSystemId, and classificationItemId unless it would be " +
                "unclassified); null for the other elements and without DryRun.");
        }

        protected override int DryRunInputIndex => 3;

        protected override int OldValuesOutputIndex => 1;

        protected override int NewValuesOutputIndex => 2;

        protected override bool TryGetBatchItems(
            IGH_DataAccess da,
            out List<ElementClassificationObj> items)
        {
            items = null;

            if (!da.TryCreate(
                    0,
                    out ClassificationGuid systemId))
            {
                return false;
            }

            if (!da.TryGetList(
                    1,
                    out List<GH_ObjectWrapper> wrappers))
            {
                return false;
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
                return false;
            }

            if (guids.Count != 1 && elements.Elements.Count != guids.Count)
            {
                this.AddError(
                    "ClassificationGuid to ElementGuid count mismatch!");
                return false;
            }

            items = new List<ElementClassificationObj>();

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

                items.Add(elementClassification);
            }

            return true;
        }

        // An element has one item per classification system; the branches
        // used to be sent one after the other, so the last one won.
        protected override string GetBatchItemKey(
            ElementClassificationObj item)
        {
            return $"{item.ElementId?.Guid?.ToLowerInvariant()}|" +
                   $"{item.Classification?.ClassificationSystemId?.Guid?.ToLowerInvariant()}";
        }

        protected override object CreateBatchParameters(
            List<ElementClassificationObj> items)
        {
            return new ElementClassificationsObject
            {
                ElementClassifications = items
            };
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetClassifications;

        public override Guid ComponentGuid =>
            new Guid("d5f807eb-ba90-4616-bd31-325c1701506a");
    }
}