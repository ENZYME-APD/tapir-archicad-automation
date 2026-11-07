using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class GetUnusedLibraryPartsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetUnusedLibraryParts";

        public GetUnusedLibraryPartsComponent()
            : base(
                "GetUnusedLibraryParts",
                "Lists the loaded library parts that no element of the project is placed from. Read only. Every " +
                "element of the floor plan and of every view and layout is checked; a library part used only in " +
                "another way (called as a macro, as a property object, as an interior elevation marker, in " +
                "stairs and railings, in the tool defaults or in Favorites) is listed as unused.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "FilterByTypeId",
                "Optional. Only list the unused library parts of this type (e.g. Door, Window, Object, Lamp).");

            InBoolean(
                "IncludeNotPlaceable",
                "Also list the library parts that cannot be placed (macros, pictures, property objects, list " +
                "schemes and the like). Default is false.",
                false);

            SetOptionality(new[] { 0, 1 });
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Guids",
                "Main guid of each unused library part.");

            OutIntegers(
                "Indices",
                "Index of each unused library part.");

            OutTexts(
                "DocumentNames",
                "Document name of each unused library part.");

            OutTexts(
                "FileNames",
                "File name of each unused library part.");

            OutTexts(
                "TypeIds",
                "Type of each unused library part (e.g. Door, Window, Object, Lamp).");

            OutBooleans(
                "IsPlaceable",
                "Whether each unused library part can be placed.");

            OutInteger(
                "UsedCount",
                "The number of different loaded library parts that are placed in the project.");

            OutInteger(
                "ScannedElementCount",
                "The number of elements checked, in the floor plan and every view and layout.");

            OutBoolean(
                "Complete",
                "False when a view, layout or library part could not be read (see Warnings): then a listed " +
                "library part may still be used.");

            OutTexts(
                "Warnings",
                "What could not be read.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject
            {
                ["includeNotPlaceable"] = da.GetOptional(1, false)
            };

            if (da.TryGet(
                    0,
                    out string filter) &&
                !string.IsNullOrEmpty(filter))
            {
                parameters["filterByTypeId"] = filter;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var guids = new List<object>();
            var indices = new List<object>();
            var documentNames = new List<object>();
            var fileNames = new List<object>();
            var typeIds = new List<object>();
            var isPlaceable = new List<object>();

            foreach (var item in JsonOutputHelp.Items(response, "unusedLibraryParts"))
            {
                guids.Add(JsonOutputHelp.Scalar(item, "guid"));
                indices.Add(JsonOutputHelp.Scalar(item, "index"));
                documentNames.Add(JsonOutputHelp.Scalar(item, "documentName"));
                fileNames.Add(JsonOutputHelp.Scalar(item, "fileName"));
                typeIds.Add(JsonOutputHelp.Scalar(item, "typeId"));
                isPlaceable.Add(JsonOutputHelp.Scalar(item, "isPlaceable"));
            }

            var warnings = new List<string>();
            foreach (var warning in JsonOutputHelp.Items(response, "warnings"))
            {
                warnings.Add(warning?.ToString());
            }

            da.SetDataList(0, guids);
            da.SetDataList(1, indices);
            da.SetDataList(2, documentNames);
            da.SetDataList(3, fileNames);
            da.SetDataList(4, typeIds);
            da.SetDataList(5, isPlaceable);
            da.SetData(6, JsonOutputHelp.Scalar(response, "usedLibraryPartCount"));
            da.SetData(7, JsonOutputHelp.Scalar(response, "scannedElementCount"));
            da.SetData(8, JsonOutputHelp.Scalar(response, "complete"));
            da.SetDataList(9, warnings);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetUnusedLibraryParts;

        public override Guid ComponentGuid =>
            new Guid("4a91d7e3-2b6c-48f0-b5d4-8e3f17a92c60");
    }
}
