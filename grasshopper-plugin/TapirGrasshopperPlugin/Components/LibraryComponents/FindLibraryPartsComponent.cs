using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Components.NavigatorComponents;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class FindLibraryPartsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "FindLibraryParts";

        public FindLibraryPartsComponent()
            : base(
                "FindLibraryParts",
                "Finds loaded library parts with the Library Manager: all of them, GDL objects, images or text " +
                "files, optionally only those whose name contains a text. Returns the name, path and index of each. " +
                "Available from Archicad 28.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            NavigatorComponentHelp.AddOptionalText(
                this,
                "Kind",
                "Which library parts to find: All (default), Object (GDL objects), Image or Text. Optional.");

            NavigatorComponentHelp.AddOptionalText(
                this,
                "NameContains",
                "Only the library parts whose name contains this text, ignoring case. Optional.");

            NavigatorComponentHelp.AddOptionalBoolean(
                this,
                "FirstOnly",
                "True to return only the first matching library part. False by default. Optional.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Names",
                "The name of each matching library part.");

            OutTexts(
                "Paths",
                "The location of each matching library part file.");

            OutIntegers(
                "Indices",
                "The index of each matching library part in the loaded libraries.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();
            NavigatorComponentHelp.CopyOptional<string>(da, 0, parameters, "kind");
            NavigatorComponentHelp.CopyOptional<string>(da, 1, parameters, "nameContains");
            NavigatorComponentHelp.CopyOptional<bool>(da, 2, parameters, "firstOnly");

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var names = new List<string>();
            var paths = new List<string>();
            var indices = new List<int?>();
            foreach (var item in JsonOutputHelp.Items(response, "libraryParts"))
            {
                names.Add(item["name"]?.ToString());
                paths.Add(item["path"]?.ToString());
                indices.Add((int?)item["index"]);
            }

            da.SetDataList(0, names);
            da.SetDataList(1, paths);
            da.SetDataList(2, indices);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAvailableLibraryParts;

        public override Guid ComponentGuid =>
            new Guid("076cd01a-e8bb-41c5-9b5a-d7986b529941");
    }
}
