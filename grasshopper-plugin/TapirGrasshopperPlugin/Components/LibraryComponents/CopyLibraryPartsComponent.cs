using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class CopyLibraryPartsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CopyLibraryParts";

        public CopyLibraryPartsComponent()
            : base(
                "CopyLibraryParts",
                "Copies loaded library parts, given by name or by an element placed from them, under a new file name " +
                "into a folder, by default the embedded library. The copy gets a new unique ID unless the main GUID " +
                "is kept. An existing file is never overwritten. The libraries are reloaded once afterwards, so the " +
                "copies can be found by name. The parts given by name come first, then the ones given by element. " +
                "Available from Archicad 28.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "LibraryPartNames",
                "Name of each loaded library part file to copy with its extension, as the fileName of " +
                "GetAvailableLibraryParts (e.g. 'My Door.gsm'). Optional.");

            InGenerics(
                "ElementGuids",
                "Identifier of each object, lamp, window, door, skylight, zone or curtain wall frame, panel, junction " +
                "or accessory whose library part is copied. Optional.");

            InTexts(
                "NewNames",
                "The file name of each copy with its extension (e.g. 'My Door copy.gsm'), without a folder; one " +
                "for each library part, names first, then elements.");

            InTexts(
                "TargetFolders",
                "The absolute path of an existing folder to put each copy into. The folder of the embedded library " +
                "by default. A copy can only be found by name after the reload when the folder belongs to a loaded " +
                "library. Input only 1 to use the same folder for all. Optional.");

            InBooleans(
                "KeepMainGuid",
                "True to keep the main GUID of the original in the copy. False by default: the copy gets a new " +
                "unique ID. Input only 1 to use the same for all. Optional.");

            SetOptionality(new[] { 0, 1, 3, 4 });
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "LibraryPartNames",
                "The file name of each copy with its extension.");

            OutTexts(
                "UnIds",
                "The unique ID of each copy, its main and revision GUID.");

            OutTexts(
                "Paths",
                "The absolute path of each copy.");

            OutBoolean(
                "LibrariesReloaded",
                "Whether the libraries were reloaded after copying. False when nothing was copied or the reload " +
                "failed; then the copies are not found by name until the libraries are reloaded.");

            OutErrorMessages(
                "Error message of each library part (empty when it was copied).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var names = da.OptionalList<string>(0);
            var elementWrappers = da.OptionalList<GH_ObjectWrapper>(1);
            var newNames = da.OptionalList<string>(2);
            var targetFolders = da.OptionalList<string>(3);
            var keepMainGuids = da.OptionalList<bool>(4);

            if (!this.TryGetElementIds(
                    elementWrappers,
                    "ElementGuids",
                    out var elementIds))
            {
                return;
            }

            var libraryParts = new JArray();
            foreach (var name in names)
            {
                libraryParts.Add(new JObject { ["libraryPartName"] = name });
            }

            foreach (var elementId in elementIds)
            {
                libraryParts.Add(new JObject { ["elementId"] = elementId });
            }

            if (libraryParts.Count == 0)
            {
                return;
            }

            if (newNames.Count != libraryParts.Count)
            {
                this.AddError(
                    "The size of the input NewNames must be equal to the number of library parts given by " +
                    "LibraryPartNames and ElementGuids.");
                return;
            }

            if (!this.CheckSizes(
                    libraryParts.Count,
                    "NewNames",
                    ("TargetFolders", targetFolders.Count),
                    ("KeepMainGuid", keepMainGuids.Count)))
            {
                return;
            }

            for (var i = 0; i < libraryParts.Count; i++)
            {
                libraryParts[i]["newName"] = newNames[i];

                if (targetFolders.Count > 0 &&
                    !string.IsNullOrEmpty(targetFolders.At(i)))
                {
                    libraryParts[i]["targetFolder"] = targetFolders.At(i);
                }

                if (keepMainGuids.Count > 0)
                {
                    libraryParts[i]["keepMainGuid"] = keepMainGuids.At(i);
                }
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["libraryParts"] = libraryParts },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var copyNames = new List<string>();
            var unIds = new List<string>();
            var paths = new List<string>();
            var errors = new List<string>();

            foreach (var item in JsonOutputHelp.Items(response, "libraryParts"))
            {
                var isError = JsonOutputHelp.IsError(item);
                copyNames.Add(isError ? null : item["libraryPartName"]?.ToString());
                unIds.Add(isError ? null : item["unID"]?.ToString());
                paths.Add(isError ? null : item["path"]?.ToString());
                errors.Add(JsonOutputHelp.ErrorMessage(item));
            }

            da.SetDataList(0, copyNames);
            da.SetDataList(1, unIds);
            da.SetDataList(2, paths);
            da.SetData(3, (bool?)response["librariesReloaded"] ?? false);
            da.SetDataList(4, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CopyFilesIntoEmbeddedLibrary;

        public override Guid ComponentGuid =>
            new Guid("296dc126-156a-4679-8b54-8d64a7d71f41");
    }
}
