using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ChangeLibraryPartOfElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ChangeLibraryPartOfElements";

        public ChangeLibraryPartOfElementsComponent()
            : base(
                "ChangeLibraryPartOfElements",
                "Changes the library part of the given Object, Lamp, Window, Door, Skylight, Zone and symbol Label " +
                "elements to another library part of the same type, given by name, index or guid, e.g. to replace " +
                "missing library parts with ones from the loaded libraries. The elements themselves (guid, " +
                "properties, classifications) are kept. Components of stairs and railings are not supported, even " +
                "when they are reported as Objects. Optionally keeps the values of the GDL parameters with the " +
                "same name and type, and the size (A and B). Give the new library part in exactly one of " +
                "LibraryPartNames, LibraryPartIndices and LibraryPartGuids.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to change the library part of.");

            InTexts(
                "LibraryPartNames",
                "Name of the new library part, as GetAvailableLibraryParts returns it in documentName " +
                "(input only 1 to use the same value for all). Optional.");

            InIntegers(
                "LibraryPartIndices",
                "Index of the new library part, as GetAvailableLibraryParts returns it " +
                "(input only 1 to use the same value for all). Optional.");

            InTexts(
                "LibraryPartGuids",
                "Main guid of the new library part, as GetAvailableLibraryParts returns it in guid " +
                "(input only 1 to use the same value for all). Optional.");

            InBooleans(
                "KeepParameters",
                "Copy the values of the GDL parameters with the same name, type and array dimensions to the new " +
                "library part. Defaults to true (input only 1 to use the same value for all). Optional.");

            InBooleans(
                "KeepSize",
                "Keep the size (A and B) of the element; false gives the new part's default size. Defaults to true " +
                "(input only 1 to use the same value for all). Optional.");

            SetOptionality(new[] { 1, 2, 3, 4, 5 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when its library part was changed).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> elementWrappers) ||
                !this.TryGetElementIds(
                    elementWrappers,
                    "ElementGuids",
                    out var elementIds))
            {
                return;
            }

            var names = da.OptionalList<string>(1);
            var indices = da.OptionalList<int>(2);
            var guids = da.OptionalList<string>(3);
            var keepParameters = da.OptionalList<bool>(4);
            var keepSize = da.OptionalList<bool>(5);

            var givenLibraryParts = (names.Count > 0 ? 1 : 0) +
                                    (indices.Count > 0 ? 1 : 0) +
                                    (guids.Count > 0 ? 1 : 0);
            if (givenLibraryParts != 1)
            {
                this.AddError(
                    "Give the new library part in exactly one of LibraryPartNames, LibraryPartIndices and LibraryPartGuids.");
                return;
            }

            if (!this.CheckSizes(
                    elementIds.Count,
                    "ElementGuids",
                    ("LibraryPartNames", names.Count),
                    ("LibraryPartIndices", indices.Count),
                    ("LibraryPartGuids", guids.Count),
                    ("KeepParameters", keepParameters.Count),
                    ("KeepSize", keepSize.Count)))
            {
                return;
            }

            var items = new JArray();
            for (var i = 0; i < elementIds.Count; i++)
            {
                var libraryPart = names.Count > 0
                    ? new JObject { ["name"] = names.At(i) }
                    : indices.Count > 0
                        ? new JObject { ["index"] = indices.At(i) }
                        : new JObject { ["guid"] = guids.At(i) };

                var item = new JObject
                {
                    ["elementId"] = elementIds[i],
                    ["libraryPart"] = libraryPart
                };

                if (keepParameters.Count > 0)
                {
                    item["keepParameters"] = keepParameters.At(i);
                }

                if (keepSize.Count > 0)
                {
                    item["keepSize"] = keepSize.At(i);
                }

                items.Add(item);
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["elementsWithLibraryPart"] = items },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ChangeLibraryPartOfElements;

        public override Guid ComponentGuid =>
            new Guid("c378276e-8df1-40db-b5cc-c1201fffab29");
    }
}
