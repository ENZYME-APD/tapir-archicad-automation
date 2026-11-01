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
    public class RemoveIFCClassificationReferencesOfElementsComponent : ArchicadExecutorComponent
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
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> elementWrappers))
            {
                return;
            }

            if (!TryGetTexts(
                    da,
                    1,
                    "ClassificationNames",
                    elementWrappers.Count,
                    out List<string> classificationNames))
            {
                return;
            }

            if (!TryGetTexts(
                    da,
                    2,
                    "Identifications",
                    elementWrappers.Count,
                    out List<string> identifications))
            {
                return;
            }

            var items = new JArray();
            for (var i = 0; i < elementWrappers.Count; i++)
            {
                var elementId = GuidObject<ElementGuid>.CreateFromWrapper(elementWrappers[i]);
                if (elementId == null)
                {
                    this.AddError("Invalid element identifier in the ElementGuids input.");
                    return;
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
                items.Add(item);
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["elementIFCClassificationReferences"] = items },
                ToAddOn,
                da);
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
