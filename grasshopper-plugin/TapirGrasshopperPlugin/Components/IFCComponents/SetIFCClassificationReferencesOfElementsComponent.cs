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
    public class SetIFCClassificationReferencesOfElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetIFCClassificationReferencesOfElements";

        public SetIFCClassificationReferencesOfElementsComponent()
            : base(
                "SetIFCClassificationReferencesOfElements",
                "Store IFC classification references (identification, name, location and the referenced " +
                "classification with its name, edition, edition date, description and location) on elements. " +
                "Every text input takes 1 item for all elements or one per element; the optional ones are left " +
                "empty when nothing is connected. Available only in Archicad 25, 26 and 27 for now.",
                GroupNames.IFC)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to store the classification reference on.");

            InTexts(
                "Identifications",
                "Identification of the classification reference, for example a classification code. Optional (input only 1 to use the same value for all).");

            InTexts(
                "Names",
                "Name of the classification reference. Optional (input only 1 to use the same value for all).");

            InTexts(
                "Locations",
                "Location of the classification reference, for example a URL. Optional (input only 1 to use the same value for all).");

            InTexts(
                "ClassificationNames",
                "Name of the classification (IfcClassification) the reference belongs to (input only 1 to use the same value for all).");

            InTexts(
                "ClassificationEditions",
                "Edition of the classification. Optional (input only 1 to use the same value for all).");

            InTexts(
                "ClassificationEditionDates",
                "Edition date of the classification. Optional (input only 1 to use the same value for all).");

            InTexts(
                "ClassificationDescriptions",
                "Description of the classification. Optional (input only 1 to use the same value for all).");

            InTexts(
                "ClassificationLocations",
                "Location of the classification, for example a URL. Optional (input only 1 to use the same value for all).");

            SetOptionality(
                new[]
                {
                    1,
                    2,
                    3,
                    5,
                    6,
                    7,
                    8
                });
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
                    "Identifications",
                    elementWrappers.Count,
                    out List<string> identifications))
            {
                return;
            }

            if (!TryGetTexts(
                    da,
                    2,
                    "Names",
                    elementWrappers.Count,
                    out List<string> names))
            {
                return;
            }

            if (!TryGetTexts(
                    da,
                    3,
                    "Locations",
                    elementWrappers.Count,
                    out List<string> locations))
            {
                return;
            }

            if (!TryGetTexts(
                    da,
                    4,
                    "ClassificationNames",
                    elementWrappers.Count,
                    out List<string> classificationNames))
            {
                return;
            }

            if (!TryGetTexts(
                    da,
                    5,
                    "ClassificationEditions",
                    elementWrappers.Count,
                    out List<string> editions))
            {
                return;
            }

            if (!TryGetTexts(
                    da,
                    6,
                    "ClassificationEditionDates",
                    elementWrappers.Count,
                    out List<string> editionDates))
            {
                return;
            }

            if (!TryGetTexts(
                    da,
                    7,
                    "ClassificationDescriptions",
                    elementWrappers.Count,
                    out List<string> descriptions))
            {
                return;
            }

            if (!TryGetTexts(
                    da,
                    8,
                    "ClassificationLocations",
                    elementWrappers.Count,
                    out List<string> classificationLocations))
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

                var referencedSource = new JObject
                {
                    ["name"] = Pick(classificationNames, i)
                };
                AddIfGiven(referencedSource, "edition", Pick(editions, i));
                AddIfGiven(referencedSource, "editionDate", Pick(editionDates, i));
                AddIfGiven(referencedSource, "description", Pick(descriptions, i));
                AddIfGiven(referencedSource, "location", Pick(classificationLocations, i));

                var item = new JObject
                {
                    ["elementId"] = new JObject { ["guid"] = elementId.Guid }
                };
                AddIfGiven(item, "identification", Pick(identifications, i));
                AddIfGiven(item, "name", Pick(names, i));
                AddIfGiven(item, "location", Pick(locations, i));
                item["referencedSource"] = referencedSource;
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
            Properties.Resources.SetIFCClassificationReferencesOfElements;

        public override Guid ComponentGuid =>
            new Guid("20fc9d45-f2c7-4aad-8258-7485df03a8c2");
    }
}
