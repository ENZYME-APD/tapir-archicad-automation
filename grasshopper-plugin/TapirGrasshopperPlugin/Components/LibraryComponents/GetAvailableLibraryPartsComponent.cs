using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class GetAvailableLibraryPartsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAvailableLibraryParts";

        public GetAvailableLibraryPartsComponent()
            : base(
                "GetAvailableLibraryParts",
                "List library parts currently available to the project. Optionally filter by type (e.g. Door, Window, Object, Lamp).",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "FilterByTypeId",
                "Optional. Filter by library part type (e.g. Door, Window, Object, Lamp).");

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Guids",
                "Main guid of each library part.");

            OutIntegers(
                "Indices",
                "Index of each library part.");

            OutTexts(
                "DocumentNames",
                "Document name of each library part.");

            OutTexts(
                "FileNames",
                "File name of each library part.");

            OutTexts(
                "TypeIds",
                "Type of each library part (e.g. Door, Window, Object, Lamp).");

            OutInteger(
                "SkippedCount",
                "Number of library parts skipped while listing.");

            OutIntegers(
                "SkippedIndices",
                "Index of each skipped library part (sample).");

            OutIntegers(
                "SkippedCodes",
                "Error code of each skipped library part (sample).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();
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

            foreach (var item in JsonOutputHelp.Items(response, "libraryParts"))
            {
                guids.Add(JsonOutputHelp.Scalar(item, "guid"));
                indices.Add(JsonOutputHelp.Scalar(item, "index"));
                documentNames.Add(JsonOutputHelp.Scalar(item, "documentName"));
                fileNames.Add(JsonOutputHelp.Scalar(item, "fileName"));
                typeIds.Add(JsonOutputHelp.Scalar(item, "typeId"));
            }

            var skippedIndices = new List<object>();
            var skippedCodes = new List<object>();
            foreach (var skipped in JsonOutputHelp.Items(response, "skippedSample"))
            {
                skippedIndices.Add(JsonOutputHelp.Scalar(skipped, "index"));
                skippedCodes.Add(JsonOutputHelp.Scalar(skipped, "code"));
            }

            da.SetDataList(0, guids);
            da.SetDataList(1, indices);
            da.SetDataList(2, documentNames);
            da.SetDataList(3, fileNames);
            da.SetDataList(4, typeIds);
            da.SetData(5, JsonOutputHelp.Scalar(response, "skippedCount"));
            da.SetDataList(6, skippedIndices);
            da.SetDataList(7, skippedCodes);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAvailableLibraryParts;

        public override Guid ComponentGuid =>
            new Guid("3f1ff684-73e0-445f-962b-791efd70a776");
    }
}
