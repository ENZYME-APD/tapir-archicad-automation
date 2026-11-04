using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class GetLibraryPartMetadataComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetLibraryPartMetadata";

        public GetLibraryPartMetadataComponent()
            : base(
                "GetLibraryPartMetadata",
                "Returns the description, keywords, author and placeable, password protected and hierarchical pages " +
                "flags of loaded library parts, given by file name. Use it to search library parts by description or " +
                "keywords and to tell placeable parts from macros. Available from Archicad 28.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Name of each loaded library part file with its extension, as the fileName of GetAvailableLibraryParts " +
                "(e.g. 'Door 26.gsm').");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Descriptions",
                "The description of each library part.");

            OutTexts(
                "Keywords",
                "The keywords of each library part as stored, usually a comma separated list.");

            OutTexts(
                "Authors",
                "The author of each library part.");

            OutBooleans(
                "IsPlaceable",
                "True if the library part can be placed in the plan; false e.g. for macros.");

            OutBooleans(
                "IsPasswordProtected",
                "True if the library part is password protected and encrypted.");

            OutBooleans(
                "IsHierarchicalPagesUsed",
                "True if the user interface of the library part uses hierarchical pages.");

            OutErrorMessages(
                "Error message of each library part (empty when its metadata was read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(0, out List<string> names))
            {
                return;
            }

            var libraryParts = new JArray();
            foreach (var name in names)
            {
                libraryParts.Add(new JObject { ["name"] = name });
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["libraryParts"] = libraryParts },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var descriptions = new List<string>();
            var keywords = new List<string>();
            var authors = new List<string>();
            var isPlaceable = new List<bool?>();
            var isPasswordProtected = new List<bool?>();
            var isHierarchicalPagesUsed = new List<bool?>();
            var errors = new List<string>();

            foreach (var item in JsonOutputHelp.Items(response, "libraryParts"))
            {
                var isError = JsonOutputHelp.IsError(item);
                descriptions.Add(isError ? null : item["description"]?.ToString());
                keywords.Add(isError ? null : item["keywords"]?.ToString());
                authors.Add(isError ? null : item["author"]?.ToString());
                isPlaceable.Add(isError ? null : (bool?)item["isPlaceable"]);
                isPasswordProtected.Add(isError ? null : (bool?)item["isPasswordProtected"]);
                isHierarchicalPagesUsed.Add(isError ? null : (bool?)item["isHierarchicalPagesUsed"]);
                errors.Add(JsonOutputHelp.ErrorMessage(item));
            }

            da.SetDataList(0, descriptions);
            da.SetDataList(1, keywords);
            da.SetDataList(2, authors);
            da.SetDataList(3, isPlaceable);
            da.SetDataList(4, isPasswordProtected);
            da.SetDataList(5, isHierarchicalPagesUsed);
            da.SetDataList(6, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetLibraryPartMetadata;

        public override Guid ComponentGuid =>
            new Guid("be40ba00-215e-4577-8329-64cc1171ee61");
    }
}
