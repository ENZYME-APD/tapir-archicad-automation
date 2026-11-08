using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Generic;

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
                this.AddError(
                    "ClassificationGuid to ElementGuid count mismatch!");
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

            var parameters = JObject.FromObject(new { elementClassifications });

            // Only sent when it differs from the command's default.
            var dryRun = da.GetOptional(
                3,
                false);
            if (dryRun)
            {
                parameters["dryRun"] = true;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var messages = ExecutionResultsResponse.ErrorMessages(response);
            if (dryRun)
            {
                AddDryRunWarnings(messages);
            }

            da.SetDataList(
                0,
                messages);
            da.SetDataList(
                1,
                ExecutionResultsResponse.WouldChangeValues(
                    response,
                    "oldValue"));
            da.SetDataList(
                2,
                ExecutionResultsResponse.WouldChangeValues(
                    response,
                    "newValue"));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetClassifications;

        public override Guid ComponentGuid =>
            new Guid("d5f807eb-ba90-4616-bd31-325c1701506a");
    }
}