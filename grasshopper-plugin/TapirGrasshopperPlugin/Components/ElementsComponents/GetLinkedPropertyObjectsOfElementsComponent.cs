using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetLinkedPropertyObjectsOfElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetLinkedPropertyObjectsOfElements";

        public GetLinkedPropertyObjectsOfElementsComponent()
            : base(
                "GetLinkedPropertyObjectsOfElements",
                "Gets how property objects (the library parts of the old listing system) are linked to the " +
                "given elements, or to the element defaults of the given element types: whether the property " +
                "objects matching their criteria are linked, which of them match, and which one is assigned " +
                "individually, each with the library part's index (the same as the index returned by " +
                "GetAvailableLibraryParts), name and guid. The outputs list the elements first, then the " +
                "element types.",
                GroupNames.ElementDetails)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to read the links of. Optional.");

            InTexts(
                "ElementTypes",
                "Element types to read the links of the element defaults of. Optional.");

            SetOptionality(new[] { 0, 1 });
        }

        protected override void AddOutputs()
        {
            OutBooleans(
                "LinkByCriteria",
                "True when the property objects matching their criteria are linked.");

            OutIntegers(
                "IndividualLibraryPartIndices",
                "Index of the library part of the individually assigned property object; 0 when there is none.");

            OutTexts(
                "IndividualLibraryPartNames",
                "Name of the individually assigned library part.");

            OutTexts(
                "IndividualLibraryPartGuids",
                "Guid of the individually assigned library part.");

            OutIntegerTree(
                "CriteriaLibraryPartIndices",
                "Library part index of each linked property object whose criteria match (one branch per item).");

            OutTextTree(
                "CriteriaLibraryPartNames",
                "Library part name of each linked property object whose criteria match (one branch per item).");

            OutTextTree(
                "CriteriaLibraryPartGuids",
                "Library part guid of each linked property object whose criteria match (one branch per item).");

            OutErrorMessages(
                "Error message of each element or element type (empty when it was read successfully).");
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

            if (!this.TryGetElementIds(
                    elementWrappers,
                    "ElementGuids",
                    out var elementIds))
            {
                return;
            }

            var elements = new JArray();
            foreach (var elementId in elementIds)
            {
                elements.Add(new JObject { ["elementId"] = elementId });
            }

            foreach (var elementType in elementTypes)
            {
                elements.Add(new JObject { ["elementType"] = elementType });
            }

            if (elements.Count == 0)
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["elements"] = elements },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var linkByCriteria = new List<bool?>();
            var individualIndices = new List<int?>();
            var individualNames = new List<string>();
            var individualGuids = new List<string>();
            var criteriaIndices = new DataTree<int>();
            var criteriaNames = new DataTree<string>();
            var criteriaGuids = new DataTree<string>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "linkedPropertyObjectsOfElements");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                criteriaIndices.EnsurePath(path);
                criteriaNames.EnsurePath(path);
                criteriaGuids.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    linkByCriteria.Add(null);
                    individualIndices.Add(null);
                    individualNames.Add(null);
                    individualGuids.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                linkByCriteria.Add(item.Value<bool?>("linkByCriteria"));
                individualIndices.Add(item.Value<int?>("individualLibraryPartIndex"));
                individualNames.Add(item["individualLibraryPartName"]?.ToString());
                individualGuids.Add(item["individualLibraryPartGuid"]?.ToString());
                errors.Add("");

                if (item["criteriaPropertyObjects"] is JArray criteriaObjects)
                {
                    foreach (var criteriaObject in criteriaObjects)
                    {
                        criteriaIndices.Add(criteriaObject.Value<int>("libraryPartIndex"), path);
                        criteriaNames.Add(criteriaObject["name"]?.ToString(), path);
                        criteriaGuids.Add(criteriaObject["guid"]?.ToString(), path);
                    }
                }
            }

            da.SetDataList(0, linkByCriteria);
            da.SetDataList(1, individualIndices);
            da.SetDataList(2, individualNames);
            da.SetDataList(3, individualGuids);
            da.SetDataTree(4, criteriaIndices);
            da.SetDataTree(5, criteriaNames);
            da.SetDataTree(6, criteriaGuids);
            da.SetDataList(7, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetLinkedPropertyObjectsOfElements;

        public override Guid ComponentGuid =>
            new Guid("73d3dcec-f81f-44aa-9ab7-e2646a6069be");
    }
}
