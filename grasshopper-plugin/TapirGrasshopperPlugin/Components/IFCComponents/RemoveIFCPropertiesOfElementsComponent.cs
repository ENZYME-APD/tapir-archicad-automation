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
    public class RemoveIFCPropertiesOfElementsComponent : ArchicadExecutorComponent
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
                    "PropertySetNames",
                    elementWrappers.Count,
                    out List<string> propertySetNames))
            {
                return;
            }

            if (!TryGetTexts(
                    da,
                    2,
                    "PropertyNames",
                    elementWrappers.Count,
                    out List<string> propertyNames))
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

                items.Add(
                    new JObject
                    {
                        ["elementId"] = new JObject { ["guid"] = elementId.Guid },
                        ["propertySetName"] = Pick(propertySetNames, i),
                        ["propertyName"] = Pick(propertyNames, i)
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["elementIFCProperties"] = items },
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

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RemoveIFCPropertiesOfElements;

        public override Guid ComponentGuid =>
            new Guid("1fa8f5b9-8942-45e4-b297-4008cdf80c05");
    }
}
