using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class GetLibraryPartSectionsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetLibraryPartSections";

        public GetLibraryPartSectionsComponent()
            : base(
                "GetLibraryPartSections",
                "Lists the sections of library parts given by index: their four-character type code, " +
                "sub-identifier, size and whether they are text (scripts, comments) or binary (previews).",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InIntegers(
                "Indices",
                "Index of each loaded library part, as GetAvailableLibraryParts returns it.");

            InTexts(
                "Passwords",
                "Password of each protected library part, needed to read the size of its protected sections " +
                "(input only 1 to use the same password for all). Optional.");

            SetOptionality(new[] { 1 });
        }

        protected override void AddOutputs()
        {
            OutIntegers(
                "Indices",
                "The index of each library part.");

            OutTextTree(
                "SectionTypes",
                "The four-character code of the type of each section, as GetLibraryPartSection takes it (one " +
                "branch per library part).");

            OutIntegerTree(
                "SubIdents",
                "The sub-identifier of each section, which tells apart sections of the same type (one branch per " +
                "library part).");

            OutIntegerTree(
                "Sizes",
                "The size of each section in bytes (null when Archicad could not read the section, for example a " +
                "protected one without its password; one branch per library part).");

            OutBooleanTree(
                "IsTexts",
                "True when the section is text (scripts, comments), which GetLibraryPartSection reads; binary " +
                "sections, such as previews, are only listed (one branch per library part).");

            OutErrorMessages(
                "Error message of each library part (empty when its sections were listed).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<int> indices) ||
                indices.Count == 0)
            {
                return;
            }

            var passwords = da.OptionalList<string>(1);
            if (!this.CheckSizes(
                    indices.Count,
                    "Indices",
                    ("Passwords", passwords.Count)))
            {
                return;
            }

            var libraryParts = new JArray();
            for (var i = 0; i < indices.Count; i++)
            {
                var libraryPart = new JObject { ["index"] = indices[i] };
                if (passwords.Count > 0 &&
                    !string.IsNullOrEmpty(passwords.At(i)))
                {
                    libraryPart["password"] = passwords.At(i);
                }
                libraryParts.Add(libraryPart);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["libraryParts"] = libraryParts },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var outIndices = new List<object>();
            var sectionTypes = new DataTree<string>();
            var subIdents = new DataTree<object>();
            var sizes = new DataTree<object>();
            var isTexts = new DataTree<object>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "libraryParts");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                sectionTypes.EnsurePath(path);
                subIdents.EnsurePath(path);
                sizes.EnsurePath(path);
                isTexts.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    outIndices.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                outIndices.Add(JsonOutputHelp.Scalar(item, "index"));
                if (item["sections"] is JArray sections)
                {
                    foreach (var section in sections)
                    {
                        sectionTypes.Add(section["sectionType"]?.ToString(), path);
                        subIdents.Add(JsonOutputHelp.Scalar(section, "subIdent"), path);
                        sizes.Add(JsonOutputHelp.Scalar(section, "size"), path);
                        isTexts.Add(JsonOutputHelp.Scalar(section, "isText"), path);
                    }
                }
                errors.Add("");
            }

            da.SetDataList(0, outIndices);
            da.SetDataTree(1, sectionTypes);
            da.SetDataTree(2, subIdents);
            da.SetDataTree(3, sizes);
            da.SetDataTree(4, isTexts);
            da.SetDataList(5, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetLibraryPartCustomSection;

        public override Guid ComponentGuid =>
            new Guid("58c0ef3a-c2c5-43e3-a1c4-cca7ada8875a");
    }
}
