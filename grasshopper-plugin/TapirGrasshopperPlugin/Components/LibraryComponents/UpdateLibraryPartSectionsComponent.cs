using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class UpdateLibraryPartSectionsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "UpdateLibraryPartSections";

        // The text sections the command can replace.
        private static readonly string[] SectionTypes =
        {
            "MasterScript",
            "2DScript",
            "3DScript",
            "ParameterScript",
            "UIScript",
            "Comment",
            "Keywords",
            "Copyright"
        };

        public UpdateLibraryPartSectionsComponent()
            : base(
                "UpdateLibraryPartSections",
                "Replaces the text of existing sections (scripts, comment, keywords, copyright) of a loaded library " +
                "part, found by its name. Each section must already exist; no section is added. Each section is " +
                "read back after the update.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Name",
                "Name of the loaded library part, without the file extension (a trailing '.gsm' is accepted too).");

            InTexts(
                "SectionTypes",
                "The type of each section to replace: MasterScript, 2DScript, 3DScript, ParameterScript, UIScript, " +
                "Comment, Keywords or Copyright.");

            InTexts(
                "Texts",
                "The new content of each section (input only 1 to use the same text for all).");

            InIntegers(
                "SubIdents",
                "The sub-identifier of each section, for a library part with more sections of the same type. 0 by " +
                "default (input only 1 to use the same for all). Optional.");

            SetOptionality(new[] { 3 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each section (empty when it was replaced).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            string name = null;
            if (!da.GetData(0, ref name) ||
                string.IsNullOrEmpty(name) ||
                !da.TryGetList(
                    1,
                    out List<string> types) ||
                !da.TryGetList(
                    2,
                    out List<string> texts))
            {
                return;
            }

            var subIdents = da.OptionalList<int>(3);
            if (!this.CheckSizes(
                    types.Count,
                    "SectionTypes",
                    ("Texts", texts.Count),
                    ("SubIdents", subIdents.Count)))
            {
                return;
            }

            var unknownTypes = types
                .Where(t => !SectionTypes.Contains(t))
                .Distinct()
                .ToList();
            if (unknownTypes.Count > 0)
            {
                this.AddError(
                    $"Unknown section types in the input SectionTypes: {string.Join(", ", unknownTypes)}. " +
                    $"Accepted: {string.Join(", ", SectionTypes)}.");
                return;
            }

            var sections = new JArray();
            for (var i = 0; i < types.Count; i++)
            {
                var section = new JObject
                {
                    ["type"] = types[i],
                    ["text"] = texts.At(i) ?? ""
                };
                if (subIdents.Count > 0)
                {
                    section["subIdent"] = subIdents.At(i);
                }
                sections.Add(section);
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject
                {
                    ["name"] = name,
                    ["sections"] = sections
                },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetLibraryPartScripts;

        public override Guid ComponentGuid =>
            new Guid("def845c4-f014-45e9-8c04-be1165159c97");
    }
}
