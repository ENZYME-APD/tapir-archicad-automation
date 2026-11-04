using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Components.NavigatorComponents;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class GetLibraryPartDetailsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetLibraryPartDetails";

        public GetLibraryPartDetailsComponent()
            : base(
                "GetLibraryPartDetails",
                "Gets the details of loaded library parts, by name or index: path, library tree path, containing " +
                "library, and for GDL objects the GUID, subtype, license, UI default flag, which scripts and pictures " +
                "they have, and optionally the interface and migration scripts. Can also tell whether each one is in " +
                "the subtype of another library part. The outputs list the parts given by name first, then the ones " +
                "given by index. Available from Archicad 28.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Name of each loaded library part file with its extension, as the fileName of GetAvailableLibraryParts " +
                "or the name of FindLibraryParts (e.g. 'Door 26.gsm'). Optional.");

            InIntegers(
                "Indices",
                "Index of each library part, as returned by FindLibraryParts. Optional.");

            InTexts(
                "Passwords",
                "Password of each password protected library part, used to read its scripts (input only 1 to use the " +
                "same password for all; one per library part, names first, then indices). Optional.");

            NavigatorComponentHelp.AddOptionalText(
                this,
                "SubTypeOf",
                "The name of another loaded library part, e.g. a subtype template. When given, IsInSubTypeOf tells " +
                "whether each library part is in that subtype. Optional.");

            NavigatorComponentHelp.AddOptionalBoolean(
                this,
                "IncludeScripts",
                "True to also return the interface script and the forward and backward migration scripts (in " +
                "Details_JSON). False by default. Optional.");

            SetOptionality(new[] { 0, 1, 2 });
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Names",
                "The name of each library part.");

            OutTexts(
                "Paths",
                "The location of each library part file.");

            OutIntegers(
                "Indices",
                "The index of each library part in the loaded libraries.");

            OutTexts(
                "LibraryNames",
                "The name of the library containing each library part.");

            OutTexts(
                "Guids",
                "The main GUID of each GDL object (empty for a library part that is not a GDL object).");

            OutTexts(
                "SubTypeGuids",
                "The main GUID of the subtype (parent) of each GDL object.");

            OutBooleans(
                "IsInSubTypeOf",
                "Whether each GDL object is in the subtype given by SubTypeOf (empty without SubTypeOf).");

            OutTexts(
                "Details_JSON",
                "All details of each library part as a JSON object: name, path, index, libraryTreePath, library and " +
                "gsmObject (guid, isSubType, subTypeGuid, isInSubTypeOf, isUIDefault, license, licenseVersion, " +
                "contents and, with IncludeScripts, scripts).");

            OutErrorMessages(
                "Error message of each library part (empty when it was read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var names = da.OptionalList<string>(0);
            var indices = da.OptionalList<int>(1);
            var passwords = da.OptionalList<string>(2);

            var count = names.Count + indices.Count;
            if (count == 0)
            {
                return;
            }

            if (!this.CheckSizes(
                    count,
                    "Names and Indices",
                    ("Passwords", passwords.Count)))
            {
                return;
            }

            var libraryParts = new JArray();
            foreach (var name in names)
            {
                libraryParts.Add(new JObject { ["name"] = name });
            }

            foreach (var index in indices)
            {
                libraryParts.Add(new JObject { ["index"] = index });
            }

            if (passwords.Count > 0)
            {
                for (var i = 0; i < libraryParts.Count; i++)
                {
                    var password = passwords.At(i);
                    if (!string.IsNullOrEmpty(password))
                    {
                        libraryParts[i]["password"] = password;
                    }
                }
            }

            var parameters = new JObject { ["libraryParts"] = libraryParts };
            NavigatorComponentHelp.CopyOptional<string>(da, 3, parameters, "subTypeOf");
            NavigatorComponentHelp.CopyOptional<bool>(da, 4, parameters, "includeScripts");

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var outNames = new List<string>();
            var outPaths = new List<string>();
            var outIndices = new List<int?>();
            var libraryNames = new List<string>();
            var guids = new List<string>();
            var subTypeGuids = new List<string>();
            var isInSubTypeOf = new List<bool?>();
            var details = new List<string>();
            var errors = new List<string>();

            foreach (var item in JsonOutputHelp.Items(response, "libraryParts"))
            {
                var isError = JsonOutputHelp.IsError(item);
                outNames.Add(isError ? null : item["name"]?.ToString());
                outPaths.Add(isError ? null : item["path"]?.ToString());
                outIndices.Add(isError ? null : (int?)item["index"]);
                libraryNames.Add(isError ? null : item["library"]?["name"]?.ToString());
                guids.Add(isError ? null : item["gsmObject"]?["guid"]?.ToString());
                subTypeGuids.Add(isError ? null : item["gsmObject"]?["subTypeGuid"]?.ToString());
                isInSubTypeOf.Add(isError ? null : (bool?)item["gsmObject"]?["isInSubTypeOf"]);
                details.Add(isError ? null : NavigatorComponentHelp.JsonText(item));
                errors.Add(JsonOutputHelp.ErrorMessage(item));
            }

            da.SetDataList(0, outNames);
            da.SetDataList(1, outPaths);
            da.SetDataList(2, outIndices);
            da.SetDataList(3, libraryNames);
            da.SetDataList(4, guids);
            da.SetDataList(5, subTypeGuids);
            da.SetDataList(6, isInSubTypeOf);
            da.SetDataList(7, details);
            da.SetDataList(8, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAvailableLibraryParts;

        public override Guid ComponentGuid =>
            new Guid("c8376f17-11c9-4b3f-8549-fa9be3a39044");
    }
}
