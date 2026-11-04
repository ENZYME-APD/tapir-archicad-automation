using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Components.NavigatorComponents;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class GetLibraryPartScriptsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetLibraryPartScripts";

        // The script outputs after UnIds, with the key of each in "scripts".
        private static readonly (string Output, string Key, string Description)[] ScriptOutputs =
        {
            ("MasterScripts", "master", "master script"),
            ("ParameterScripts", "parameter", "parameter script"),
            ("Scripts2D", "2d", "2D script"),
            ("Scripts3D", "3d", "3D script"),
            ("PropertiesScripts", "properties", "properties script")
        };

        public GetLibraryPartScriptsComponent()
            : base(
                "GetLibraryPartScripts",
                "Gets the master, parameter, 2D, 3D and properties scripts of loaded library parts, given by name or " +
                "by an element placed from them, with the unique ID of each part. A script section the part does not " +
                "have is empty. Password protected parts need their password. The outputs list the parts given by name " +
                "first, then the ones given by element. Available from Archicad 28.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "LibraryPartNames",
                "Name of each loaded library part file with its extension, as the fileName of " +
                "GetAvailableLibraryParts (e.g. 'My Door.gsm'). Optional.");

            InGenerics(
                "ElementGuids",
                "Identifier of each object, lamp, window, door, skylight, zone or curtain wall frame, panel, junction " +
                "or accessory whose library part is read. Optional.");

            InTexts(
                "Scripts",
                "The scripts to read: master, parameter, 2d, 3d and/or properties. All five by default. Optional.");

            NavigatorComponentHelp.AddOptionalText(
                this,
                "Password",
                "The password of password protected library parts. Optional.");

            SetOptionality(new[] { 0, 1, 2 });
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "LibraryPartNames",
                "The name of each library part file with its extension.");

            OutTexts(
                "UnIds",
                "The unique ID of each library part, its main and revision GUID.");

            foreach (var script in ScriptOutputs)
            {
                OutTexts(
                    script.Output,
                    $"The {script.Description} of each library part (empty when it was not requested or the part " +
                    "does not have this section).");
            }

            OutErrorMessages(
                "Error message of each library part (empty when its scripts were read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var names = da.OptionalList<string>(0);
            var elementWrappers = da.OptionalList<GH_ObjectWrapper>(1);
            var scripts = da.OptionalList<string>(2);

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

            var parameters = new JObject { ["libraryParts"] = libraryParts };
            if (scripts.Count > 0)
            {
                parameters["scripts"] = new JArray(scripts.Distinct());
            }

            NavigatorComponentHelp.CopyOptional<string>(da, 3, parameters, "password");

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var outNames = new List<string>();
            var unIds = new List<string>();
            var scriptTexts = ScriptOutputs.Select(_ => new List<string>()).ToList();
            var errors = new List<string>();

            foreach (var item in JsonOutputHelp.Items(response, "libraryParts"))
            {
                var isError = JsonOutputHelp.IsError(item);
                outNames.Add(isError ? null : item["libraryPartName"]?.ToString());
                unIds.Add(isError ? null : item["unID"]?.ToString());
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

            da.SetDataList(0, outNames);
            da.SetDataList(1, unIds);
            for (var i = 0; i < ScriptOutputs.Length; i++)
            {
                da.SetDataList(2 + i, scriptTexts[i]);
            }
            da.SetDataList(2 + ScriptOutputs.Length, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetLibraryPartScripts;

        public override Guid ComponentGuid =>
            new Guid("d927c5fc-19d1-4e9a-b3f6-bd8e229cd00f");
    }
}
