using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Components.NavigatorComponents;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class GetLibraryPartFileInfoComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetLibraryPartFileInfo";

        // The script outputs after SubTypeUnIds, with the key of each in "scripts".
        private static readonly (string Output, string Key, string Description)[] ScriptOutputs =
        {
            ("MasterScripts", "master", "master script"),
            ("ParameterScripts", "parameter", "parameter script"),
            ("Scripts2D", "2d", "2D script"),
            ("Scripts3D", "3d", "3D script"),
            ("PropertiesScripts", "properties", "properties script")
        };

        public GetLibraryPartFileInfoComponent()
            : base(
                "GetLibraryPartFileInfo",
                "Reads library part files (.gsm) without loading them into the libraries of the project: the " +
                "unique ID of the part, the unique ID of its subtype and its master, parameter, 2D, 3D and " +
                "properties scripts. Password protected parts need their password. Available from Archicad 28.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Files",
                "The absolute path of each library part file (.gsm) to read.");

            InTexts(
                "Scripts",
                "The scripts to read: master, parameter, 2d, 3d and/or properties. All five by default. Optional.");

            NavigatorComponentHelp.AddOptionalText(
                this,
                "Password",
                "The password of password protected library parts. Reading a script of a protected part without " +
                "the right password is an error for that file. Optional.");

            SetOptionality(new[] { 1 });
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Paths",
                "The path of each file, as given.");

            OutTexts(
                "UnIds",
                "The unique ID of each library part, its main and revision GUID.");

            OutTexts(
                "SubTypeUnIds",
                "The unique ID of the subtype each library part belongs to.");

            foreach (var script in ScriptOutputs)
            {
                OutTexts(
                    script.Output,
                    $"The {script.Description} of each library part (empty when it was not requested or the part " +
                    "does not have this section).");
            }

            OutErrorMessages(
                "Error message of each file (empty when it was read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> files) ||
                files.Count == 0)
            {
                return;
            }

            var scripts = da.OptionalList<string>(1);
            var unknownScripts = scripts
                .Where(s => ScriptOutputs.All(o => o.Key != s))
                .ToList();
            if (unknownScripts.Count > 0)
            {
                this.AddError(
                    $"Unknown scripts in the input Scripts: {string.Join(", ", unknownScripts)}. " +
                    $"Accepted: {string.Join(", ", ScriptOutputs.Select(o => o.Key))}.");
                return;
            }

            var parameters = new JObject { ["files"] = new JArray(files) };
            if (scripts.Count > 0)
            {
                parameters["scripts"] = new JArray(scripts.Distinct());
            }

            NavigatorComponentHelp.CopyOptional<string>(da, 2, parameters, "password");

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var paths = new List<string>();
            var unIds = new List<string>();
            var subTypeUnIds = new List<string>();
            var scriptTexts = ScriptOutputs.Select(_ => new List<string>()).ToList();
            var errors = new List<string>();

            foreach (var item in JsonOutputHelp.Items(response, "files"))
            {
                var isError = JsonOutputHelp.IsError(item);
                paths.Add(isError ? null : item["path"]?.ToString());
                unIds.Add(isError ? null : item["unID"]?.ToString());
                subTypeUnIds.Add(isError ? null : item["subTypeUnID"]?.ToString());
                for (var i = 0; i < ScriptOutputs.Length; i++)
                {
                    var section = isError ? null : item["scripts"]?[ScriptOutputs[i].Key];
                    scriptTexts[i].Add(
                        (bool?)section?["present"] == true
                            ? section["script"]?.ToString()
                            : null);
                }
                errors.Add(JsonOutputHelp.ErrorMessage(item));
            }

            da.SetDataList(0, paths);
            da.SetDataList(1, unIds);
            da.SetDataList(2, subTypeUnIds);
            for (var i = 0; i < ScriptOutputs.Length; i++)
            {
                da.SetDataList(3 + i, scriptTexts[i]);
            }
            da.SetDataList(3 + ScriptOutputs.Length, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetLibraryPartScripts;

        public override Guid ComponentGuid =>
            new Guid("2d87f42c-5501-40d3-bc48-51aba264ecb4");
    }
}
