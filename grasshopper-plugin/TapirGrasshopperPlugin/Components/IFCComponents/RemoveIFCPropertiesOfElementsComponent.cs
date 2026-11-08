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
    // Sends one RemoveIFCPropertiesOfElements request for every branch of the
    // inputs, so a tree input makes one undo step in Archicad (#203). The
    // branches with DryRun on go in a second request with dryRun set.
    public class RemoveIFCPropertiesOfElementsComponent
        : BatchedExecutorComponent<JObject>
    {
        public override string CommandName => "RemoveIFCPropertiesOfElements";

        public RemoveIFCPropertiesOfElementsComponent()
            : base(
                "RemoveIFCPropertiesOfElements",
                "Remove local IFC properties, given by property set and property name, from elements. " +
                "Available only in Archicad 25, 26 and 27 for now.",
                GroupNames.IFC)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to remove the IFC property from.");

            InTexts(
                "PropertySetNames",
                "Name of the property set, for example Pset_DoorCommon (input only 1 to use the same value for all).");

            InTexts(
                "PropertyNames",
                "Name of the property, for example FireRating (input only 1 to use the same value for all).");

            // Appended after the original inputs, so saved definitions keep their wiring.
            InBoolean(
                "DryRun",
                "If true, nothing is removed: ErrorMessages says what the removal would do for each element, " +
                "from the check made before removing (the element has the local IFC property). Default is " +
                "false.",
                false);

            SetOptionality(3);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when it succeeded, or with DryRun when the property " +
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
                    "PropertySetNames",
                    elementWrappers.Count,
                    out List<string> propertySetNames))
            {
                return false;
            }

            if (!TryGetTexts(
                    da,
                    2,
                    "PropertyNames",
                    elementWrappers.Count,
                    out List<string> propertyNames))
            {
                return false;
            }

            var properties = new List<JObject>();
            for (var i = 0; i < elementWrappers.Count; i++)
            {
                var elementId = GuidObject<ElementGuid>.CreateFromWrapper(elementWrappers[i]);
                if (elementId == null)
                {
                    this.AddError("Invalid element identifier in the ElementGuids input.");
                    return false;
                }

                properties.Add(
                    new JObject
                    {
                        ["elementId"] = new JObject { ["guid"] = elementId.Guid },
                        ["propertySetName"] = Pick(propertySetNames, i),
                        ["propertyName"] = Pick(propertyNames, i)
                    });
            }

            items = properties;
            return true;
        }

        // The command checks every item before removing any, so a property
        // given in two branches would pass the check twice and fail the
        // second removal; the branches used to be sent one after the other.
        // It is sent once and both branches get its result.
        protected override string GetBatchItemKey(
            JObject item)
        {
            return $"{((string)item["elementId"]?["guid"])?.ToLowerInvariant()}|" +
                   $"{(string)item["propertySetName"]}|" +
                   $"{(string)item["propertyName"]}";
        }

        protected override object CreateBatchParameters(
            List<JObject> items)
        {
            return new JObject { ["elementIFCProperties"] = new JArray(items) };
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

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RemoveIFCPropertiesOfElements;

        public override Guid ComponentGuid =>
            new Guid("1fa8f5b9-8942-45e4-b297-4008cdf80c05");
    }
}
