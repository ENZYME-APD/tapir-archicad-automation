using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class GetLibraryPartIdentitiesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetLibraryPartIdentities";

        public GetLibraryPartIdentitiesComponent()
            : base(
                "GetLibraryPartIdentities",
                "Gets the unique ID (main and revision GUID), the reference GUID and the Teamwork lock status of " +
                "library parts given by index or file path, and whether they are in a server library. The outputs " +
                "list the parts given by index first, then the ones given by path.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InIntegers(
                "Indices",
                "Index of each loaded library part, as GetAvailableLibraryParts returns it. Optional.");

            InTexts(
                "Paths",
                "Path of each library part file, for example a .gsm file. Optional.");

            SetOptionality(new[] { 0, 1 });
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Paths",
                "The path of each library part file.");

            OutTexts(
                "UnIds",
                "The unique ID of each library part, '{main GUID}-{revision GUID}'.");

            OutTexts(
                "MainGuids",
                "The main GUID of each unique ID. It stays the same across the revisions of a library part.");

            OutTexts(
                "RevisionGuids",
                "The revision GUID of each unique ID. It changes whenever the library part is saved.");

            OutTexts(
                "RefGuids",
                "The reference GUID of each library part file (empty when Archicad could not tell it).");

            OutTexts(
                "LockStatuses",
                "The Teamwork lock status of each library part file: Free, Editable, Locked, NotAvailable or " +
                "NotExist (empty when Archicad could not tell it).");

            OutBooleans(
                "IsServerLibraryParts",
                "True when the library part is in a BIMcloud or Teamwork server library.");

            OutErrorMessages(
                "Error message of each library part (empty when it was read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var indices = da.OptionalList<int>(0);
            var paths = da.OptionalList<string>(1);

            var libraryParts = new JArray();
            foreach (var index in indices)
            {
                libraryParts.Add(new JObject { ["index"] = index });
            }

            foreach (var path in paths)
            {
                libraryParts.Add(new JObject { ["path"] = path });
            }

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

            var outPaths = new List<string>();
            var unIds = new List<string>();
            var mainGuids = new List<string>();
            var revisionGuids = new List<string>();
            var refGuids = new List<string>();
            var lockStatuses = new List<string>();
            var isServer = new List<bool?>();
            var errors = new List<string>();

            foreach (var item in JsonOutputHelp.Items(response, "identities"))
            {
                var isError = JsonOutputHelp.IsError(item);
                outPaths.Add(isError ? null : item["path"]?.ToString());
                unIds.Add(isError ? null : item["unId"]?.ToString());
                mainGuids.Add(isError ? null : item["mainGuid"]?.ToString());
                revisionGuids.Add(isError ? null : item["revisionGuid"]?.ToString());
                refGuids.Add(isError ? null : item["refGuid"]?.ToString());
                lockStatuses.Add(isError ? null : item["lockStatus"]?.ToString());
                isServer.Add(isError ? null : (bool?)item["isServerLibraryPart"]);
                errors.Add(JsonOutputHelp.ErrorMessage(item));
            }

            da.SetDataList(0, outPaths);
            da.SetDataList(1, unIds);
            da.SetDataList(2, mainGuids);
            da.SetDataList(3, revisionGuids);
            da.SetDataList(4, refGuids);
            da.SetDataList(5, lockStatuses);
            da.SetDataList(6, isServer);
            da.SetDataList(7, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAvailableLibraryParts;

        public override Guid ComponentGuid =>
            new Guid("56e06d61-2340-4d6c-bb69-4f8b2960e0c2");
    }
}
