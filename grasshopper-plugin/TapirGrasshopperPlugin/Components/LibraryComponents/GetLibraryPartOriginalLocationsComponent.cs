using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class GetLibraryPartOriginalLocationsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetLibraryPartOriginalLocations";

        public GetLibraryPartOriginalLocationsComponent()
            : base(
                "GetLibraryPartOriginalLocations",
                "Gets the original location of the given library parts, the file they came from, e.g. before they " +
                "were embedded into the project. The library parts are given by the index GetAvailableLibraryParts " +
                "returns. Available from Archicad 27.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InIntegers(
                "Indices",
                "Index of each library part, as returned by GetAvailableLibraryParts.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Paths",
                "The path of the file each library part originally came from (empty for a library part that failed).");

            OutErrorMessages(
                "Error message of each library part (empty when its original location was read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(0, out List<int> indices))
            {
                return;
            }

            var libraryParts = new JArray();
            foreach (var index in indices)
            {
                libraryParts.Add(new JObject { ["index"] = index });
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["libraryParts"] = libraryParts },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var paths = new List<string>();
            var errors = new List<string>();
            foreach (var item in JsonOutputHelp.Items(response, "locations"))
            {
                paths.Add(JsonOutputHelp.IsError(item) ? null : item["path"]?.ToString());
                errors.Add(JsonOutputHelp.ErrorMessage(item));
            }

            da.SetDataList(0, paths);
            da.SetDataList(1, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAvailableLibraryParts;

        public override Guid ComponentGuid =>
            new Guid("4f388a75-6dc4-45ee-83b8-4cccb97d7b2a");
    }
}
