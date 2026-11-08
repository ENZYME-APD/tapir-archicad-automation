using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.IFCComponents
{
    // Sends one RemoveIFCClassificationReferencesOfElements request for every
    // branch of the inputs, so a tree input makes one undo step in Archicad
    // (#203). The branches with DryRun on go in a second request with dryRun
    // set.
    public class RemoveIFCClassificationReferencesOfElementsComponent
        : BatchedExecutorComponent<JObject>
    {
        public override string CommandName => "RemoveIFCClassificationReferencesOfElements";

        public RemoveIFCClassificationReferencesOfElementsComponent()
            : base(
                "RemoveIFCClassificationReferencesOfElements",
                "Remove IFC classification references stored on elements, given by the name of their " +
                "classification and optionally their identification; without identification every stored " +
                "reference of that classification is removed. Only references stored on the elements (the ones " +
                "GetIFCClassificationReferencesOfElements returns with StoredOnly) can be removed. " +
                "Available only in Archicad 25, 26 and 27 for now.",
                GroupNames.IFC)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to remove the classification reference from.");

            InTexts(
                "ClassificationNames",
                "Name of the classification (IfcClassification) the reference belongs to (input only 1 to use the same value for all).");

            InTexts(
                "Identifications",
                "Identification of the reference to remove; when nothing is connected every stored reference of the classification is removed. Optional (input only 1 to use the same value for all).");

            SetOptionality(2);

            // Appended after the original inputs, so saved definitions keep their wiring.
            InBoolean(
                "DryRun",
                "If true, nothing is removed: ErrorMessages says what the removal would do for each element, " +
                "from the check made before removing (the element has a stored reference of the " +
                "classification, and of the identification when given). Default is false.",
                false);

            SetOptionality(3);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when it succeeded, or with DryRun when the reference " +
                "would be removed).");
        }

        protected override int DryRunInputIndex => 3;

        protected override bool TryGetBatchItems(
            IGH_DataAccess da,
            out List<JObject> items)
        {
            items = null;

            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> elementWrappers))
            {
                return false;
            }

            if (!TryGetTexts(
                    da,
                    1,
                    "ClassificationNames",
                    elementWrappers.Count,
                    out List<string> classificationNames))
            {
                return false;
            }

            if (!TryGetTexts(
                    da,
                    2,
                    "Identifications",
                    elementWrappers.Count,
                    out List<string> identifications))
            {
                return false;
            }

            var references = new List<JObject>();
            for (var i = 0; i < elementWrappers.Count; i++)
            {
                var elementId = GuidObject<ElementGuid>.CreateFromWrapper(elementWrappers[i]);
                if (elementId == null)
                {
                    this.AddError("Invalid element identifier in the ElementGuids input.");
                    return false;
                }

                var item = new JObject
                {
                    ["elementId"] = new JObject { ["guid"] = elementId.Guid },
                    ["referencedSource"] = new JObject
                    {
                        ["name"] = Pick(classificationNames, i)
                    }
                };
                AddIfGiven(item, "identification", Pick(identifications, i));
                references.Add(item);
            }

            items = references;
            return true;
        }

        // The command checks every item before removing any, so a reference
        // given in two branches would pass the check twice and fail the
        // second removal; the branches used to be sent one after the other.
        // It is sent once and both branches get its result.
        protected override string GetBatchItemKey(
            JObject item)
        {
            var identification = item["identification"];
            return $"{((string)item["elementId"]?["guid"])?.ToLowerInvariant()}|" +
                   $"{(string)item["referencedSource"]?["name"]}|" +
                   (identification == null ? "*" : "=" + (string)identification);
        }

        protected override object CreateBatchParameters(
            List<JObject> items)
        {
            return new JObject { ["elementIFCClassificationReferences"] = new JArray(items) };
        }

        // Reads a text list input that holds 0 (only when optional), 1 (the
        // same value for every element) or as many items as ElementGuids.
        private bool TryGetTexts(
            IGH_DataAccess da,
            int index,
            string inputName,
            int elementCount,
            out List<string> texts)
        {
            if (!da.TryGetList(
                    index,
                    out texts))
            {
                texts = new List<string>();
                return Params.Input[index].Optional;
            }

            if (texts.Count > 1 &&
                texts.Count != elementCount)
            {
                this.AddError(
                    $"The size of the input {inputName} must be 1 or equal to the size of the input ElementGuids.");
                return false;
            }

            return true;
        }

        // The value of a text list input for the element at index; null when
        // the input is empty.
        private static string Pick(
            List<string> texts,
            int index)
        {
            if (texts.Count == 0)
            {
                return null;
            }
            return texts[texts.Count == 1 ? 0 : index];
        }

        // Sets the key only when the value was given, so that the add-on's
        // default applies otherwise.
        private static void AddIfGiven(
            JObject target,
            string key,
            string value)
        {
            if (value != null)
            {
                target[key] = value;
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RemoveIFCClassificationReferencesOfElements;

        public override Guid ComponentGuid =>
            new Guid("86c40e19-c5ff-425f-8297-c2974168c4de");
    }
}
