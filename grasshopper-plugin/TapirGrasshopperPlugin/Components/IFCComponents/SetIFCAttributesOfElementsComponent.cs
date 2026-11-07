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
    public class SetIFCAttributesOfElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetIFCAttributesOfElements";

        public SetIFCAttributesOfElementsComponent()
            : base(
                "SetIFCAttributesOfElements",
                "Set the values of IFC attributes (Name, Description, ObjectType, Tag, ...) of elements. The " +
                "attribute must be one GetIFCAttributesOfElements returns for the element; its IFC value type is " +
                "kept. Available only in Archicad 25, 26 and 27 for now.",
                GroupNames.IFC)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to set the IFC attribute of.");

            InTexts(
                "AttributeNames",
                "Name of the IFC attribute, for example Name, Description, ObjectType or Tag (input only 1 to use the same value for all).");

            InTexts(
                "Values",
                "New value of the IFC attribute (input only 1 to use the same value for all).");
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
                    "AttributeNames",
                    elementWrappers.Count,
                    out List<string> attributeNames))
            {
                return;
            }

            if (!TryGetTexts(
                    da,
                    2,
                    "Values",
                    elementWrappers.Count,
                    out List<string> values))
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
                        ["name"] = Pick(attributeNames, i),
                        ["value"] = Pick(values, i) ?? ""
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["elementIFCAttributeValues"] = items },
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
            Properties.Resources.SetIFCAttributesOfElements;

        public override Guid ComponentGuid =>
            new Guid("05b3b1ea-6a38-4b41-99e7-f0eecfc3a80a");
    }
}
