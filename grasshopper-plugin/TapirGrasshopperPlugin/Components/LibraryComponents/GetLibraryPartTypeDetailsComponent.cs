using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class GetLibraryPartTypeDetailsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetLibraryPartTypeDetails";

        public GetLibraryPartTypeDetailsComponent()
            : base(
                "GetLibraryPartTypeDetails",
                "Gets the type details of library parts: full unique ID and its direct parent's, type, the detail " +
                "flags of objects, lamps, labels and zones, the frame and orientation settings of doors and windows, " +
                "and the element type and variation of the tool that places the library part. For their path, " +
                "library, GUID, license and scripts see GetLibraryPartDetails. The outputs list the parts given by " +
                "index first, then by name, then by unique ID.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InIntegers(
                "Indices",
                LibraryPartIdentifierHelp.IndicesDescription);

            InTexts(
                "Names",
                LibraryPartIdentifierHelp.NamesDescription);

            InTexts(
                "UnIds",
                LibraryPartIdentifierHelp.UnIdsDescription);

            SetOptionality(new[] { 0, 1, 2 });
        }

        protected override void AddOutputs()
        {
            OutIntegers(
                "Indices",
                "The current index of each library part. It may change when the libraries are reloaded.");

            OutTexts(
                "Names",
                "The name of each library part.");

            OutTexts(
                "FileNames",
                "The file name of each library part.");

            OutTexts(
                "UnIds",
                "The full unique ID (main and revision GUID) of each library part.");

            OutTexts(
                "ParentUnIds",
                "The full unique ID of the direct parent subtype of each library part.");

            OutTexts(
                "TypeIds",
                "The type of each library part, e.g. Object, Door, Window, Lamp, Label or Macro.");

            OutBooleans(
                "IsPlaceable",
                "Whether each library part can be placed.");

            OutBooleans(
                "IsTemplate",
                "Whether each library part is a template.");

            OutTexts(
                "CreatorElementTypes",
                "The type of the element each library part is placed as, its creator tool (empty when Archicad " +
                "does not report one).");

            OutTexts(
                "CreatorVariations",
                "The four-character variation ID of the creator tool, when it is not the generic variation.");

            OutTexts(
                "Details_JSON",
                "All type details of each library part as a JSON object, with objectDetails (fixSize, " +
                "autoHotspot) of an Object, Lamp, Label or Zone, and doorWindowDetails (mirThick, wallInset, " +
                "leftFram, righFram, topFram, botFram, orientation, contours3D) of a Door or Window.");

            OutErrorMessages(
                "Error message of each library part (empty when it was found).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var libraryParts = LibraryPartIdentifierHelp.Identifiers(da, 0);
            if (libraryParts.Count == 0)
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["libraryParts"] = libraryParts },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var indices = new List<object>();
            var names = new List<object>();
            var fileNames = new List<object>();
            var unIds = new List<object>();
            var parentUnIds = new List<object>();
            var typeIds = new List<object>();
            var isPlaceable = new List<object>();
            var isTemplate = new List<object>();
            var creatorElementTypes = new List<object>();
            var creatorVariations = new List<object>();
            var details = new List<string>();
            var errors = new List<string>();

            foreach (var item in JsonOutputHelp.Items(response, "libraryParts"))
            {
                var part = JsonOutputHelp.IsError(item) ? null : item;
                indices.Add(JsonOutputHelp.Scalar(part, "index"));
                names.Add(JsonOutputHelp.Scalar(part, "name"));
                fileNames.Add(JsonOutputHelp.Scalar(part, "fileName"));
                unIds.Add(JsonOutputHelp.Scalar(part, "unId"));
                parentUnIds.Add(JsonOutputHelp.Scalar(part, "parentUnId"));
                typeIds.Add(JsonOutputHelp.Scalar(part, "typeId"));
                isPlaceable.Add(JsonOutputHelp.Scalar(part, "isPlaceable"));
                isTemplate.Add(JsonOutputHelp.Scalar(part, "isTemplate"));
                creatorElementTypes.Add(JsonOutputHelp.Scalar(part, "creatorElementType"));
                creatorVariations.Add(JsonOutputHelp.Scalar(part, "creatorVariation"));
                details.Add(part?.ToString(Formatting.None));
                errors.Add(JsonOutputHelp.ErrorMessage(item));
            }

            da.SetDataList(0, indices);
            da.SetDataList(1, names);
            da.SetDataList(2, fileNames);
            da.SetDataList(3, unIds);
            da.SetDataList(4, parentUnIds);
            da.SetDataList(5, typeIds);
            da.SetDataList(6, isPlaceable);
            da.SetDataList(7, isTemplate);
            da.SetDataList(8, creatorElementTypes);
            da.SetDataList(9, creatorVariations);
            da.SetDataList(10, details);
            da.SetDataList(11, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetLibraryPartDetails;

        public override Guid ComponentGuid =>
            new Guid("461d0981-6d53-43e3-a9dc-6e0f10a94ad2");
    }
}
