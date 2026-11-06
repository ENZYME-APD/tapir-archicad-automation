using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class SearchLibraryPartsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "SearchLibraryParts";

        public SearchLibraryPartsComponent()
            : base(
                "SearchLibraryParts",
                "Finds the loaded library parts derived from an ancestor library part (by its unique ID) whose name " +
                "matches a pattern with * wildcards. Archicad returns at most 50 of them.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "AncestorUnId",
                "The unique ID of the ancestor library part, '{main GUID}-{revision GUID}', as " +
                "GetLibraryPartIdentities returns it. Only the library parts derived from it are found.");

            InTextWithDefault(
                "Pattern",
                "The name pattern to match, with * as wildcard, for example 'Chair*'. '*' finds every library part " +
                "derived from the ancestor.",
                "*");
        }

        protected override void AddOutputs()
        {
            OutIntegers(
                "Indices",
                "The index of each library part found, as GetAvailableLibraryParts returns it.");

            OutTexts(
                "Names",
                "The name of each library part found.");

            OutTexts(
                "FileNames",
                "The file name of each library part found.");

            OutTexts(
                "UnIds",
                "The unique ID of each library part found, '{main GUID}-{revision GUID}'.");

            OutTexts(
                "ParentUnIds",
                "The unique ID of the parent of each library part found.");

            OutTexts(
                "TypeIds",
                "The type of each library part found, e.g. Object, Door, Window or Lamp.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            string ancestorUnId = null;
            string pattern = null;
            if (!da.GetData(0, ref ancestorUnId) ||
                string.IsNullOrEmpty(ancestorUnId) ||
                !da.GetData(1, ref pattern) ||
                string.IsNullOrEmpty(pattern))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject
                    {
                        ["ancestorUnId"] = ancestorUnId,
                        ["pattern"] = pattern
                    },
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

            foreach (var item in JsonOutputHelp.Items(response, "libraryParts"))
            {
                indices.Add(JsonOutputHelp.Scalar(item, "index"));
                names.Add(JsonOutputHelp.Scalar(item, "name"));
                fileNames.Add(JsonOutputHelp.Scalar(item, "fileName"));
                unIds.Add(JsonOutputHelp.Scalar(item, "unId"));
                parentUnIds.Add(JsonOutputHelp.Scalar(item, "parentUnId"));
                typeIds.Add(JsonOutputHelp.Scalar(item, "typeId"));
            }

            da.SetDataList(0, indices);
            da.SetDataList(1, names);
            da.SetDataList(2, fileNames);
            da.SetDataList(3, unIds);
            da.SetDataList(4, parentUnIds);
            da.SetDataList(5, typeIds);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.FindLibraryParts;

        public override Guid ComponentGuid =>
            new Guid("85405c88-bcb1-457e-a518-c0efdd17b8bf");
    }
}
