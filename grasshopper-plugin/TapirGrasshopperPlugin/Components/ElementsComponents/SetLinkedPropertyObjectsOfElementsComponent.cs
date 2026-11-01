using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class SetLinkedPropertyObjectsOfElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetLinkedPropertyObjectsOfElements";

        public SetLinkedPropertyObjectsOfElementsComponent()
            : base(
                "SetLinkedPropertyObjectsOfElements",
                "Sets how property objects (the library parts of the old listing system) are linked to the given " +
                "elements, or to the element defaults of the given element types: whether the property objects " +
                "matching their criteria are linked, and which one is assigned individually (0 removes the " +
                "individual assignment). A link that is not given stays unchanged. All changes are made in one " +
                "undoable step. The other inputs apply to the elements first, then to the element types.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to set the links of. Optional.");

            InTexts(
                "ElementTypes",
                "Element types to set the links of the element defaults of. Optional.");

            InBooleans(
                "LinkByCriteria",
                "Link the property objects matching their criteria (input only 1 to use the same value for all). Optional.");

            InIntegers(
                "IndividualLibraryPartIndices",
                "Index of the library part of the property object to assign individually, as GetAvailableLibraryParts " +
                "returns it; 0 removes the individual assignment (input only 1 to use the same value for all). Optional.");

            SetOptionality(new[] { 0, 1, 2, 3 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element or element type (empty when it succeeded).");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            AddAsSource<ElementTypeValueList>(
                document,
                1);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var elementWrappers = da.OptionalList<GH_ObjectWrapper>(0);
            var elementTypes = da.OptionalList<string>(1);
            var linkByCriteria = da.OptionalList<bool>(2);
            var individualIndices = da.OptionalList<int>(3);

            if (!this.TryGetElementIds(
                    elementWrappers,
                    "ElementGuids",
                    out var elementIds))
            {
                return;
            }

            var count = elementIds.Count + elementTypes.Count;
            if (count == 0 ||
                !this.CheckSizes(
                    count,
                    "ElementGuids and ElementTypes together",
                    ("LinkByCriteria", linkByCriteria.Count),
                    ("IndividualLibraryPartIndices", individualIndices.Count)))
            {
                return;
            }

            var items = new JArray();
            for (var i = 0; i < count; i++)
            {
                var item = i < elementIds.Count
                    ? new JObject { ["elementId"] = elementIds[i] }
                    : new JObject { ["elementType"] = elementTypes[i - elementIds.Count] };

                if (linkByCriteria.Count > 0)
                {
                    item["linkByCriteria"] = linkByCriteria.At(i);
                }

                if (individualIndices.Count > 0)
                {
                    item["individualLibraryPartIndex"] = individualIndices.At(i);
                }

                items.Add(item);
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["elementsWithLinkedPropertyObjects"] = items },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetLinkedPropertyObjectsOfElements;

        public override Guid ComponentGuid =>
            new Guid("e66e64af-1c01-40ae-bcb2-73a6a10b2d83");
    }
}
