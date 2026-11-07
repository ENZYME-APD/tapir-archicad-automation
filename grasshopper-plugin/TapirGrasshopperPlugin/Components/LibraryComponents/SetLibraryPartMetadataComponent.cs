using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Components.MEPComponents;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class SetLibraryPartMetadataComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetLibraryPartMetadata";

        public SetLibraryPartMetadataComponent()
            : base(
                "SetLibraryPartMetadata",
                "Sets the author, keywords, license, license version, password and the placeable, UI default, subtype and " +
                "hierarchical pages flags of loaded library parts. Only the given fields are changed. A library part that " +
                "is not editable (e.g. a part of a library package or a read-only library) is refused and left unchanged. " +
                "Available from Archicad 28.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Name of each loaded library part file with its extension, as the fileName of GetAvailableLibraryParts.");

            InTexts(
                "Authors",
                "New author of each library part (input only 1 to use the same value for all). Optional.");

            InTexts(
                "Keywords",
                "New keywords of each library part (input only 1 to use the same value for all). Optional.");

            InTexts(
                "Licenses",
                "New license of each library part (input only 1 to use the same value for all). Optional.");

            InTexts(
                "LicenseVersions",
                "New license version of each library part (input only 1 to use the same value for all). Optional.");

            InTexts(
                "Passwords",
                "New password of each library part (input only 1 to use the same value for all). Optional.");

            InBooleans(
                "Placeable",
                "Whether each library part can be placed in the project (input only 1 to use the same value for all). Optional.");

            InBooleans(
                "UIDefault",
                "The UI Default flag of each library part (input only 1 to use the same value for all). Optional.");

            InBooleans(
                "IsSubtype",
                "Whether each library part is a subtype template (input only 1 to use the same value for all). Optional.");

            InBooleans(
                "HierarchicalPagesUsed",
                "Whether each library part uses hierarchical pages in its user interface " +
                "(input only 1 to use the same value for all). Optional.");

            SetOptionality(new[] { 1, 2, 3, 4, 5, 6, 7, 8, 9 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each library part (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> names))
            {
                return;
            }

            var fields = new ElectricalItemFields(this, 0, names.Count);
            if (!fields.AddTexts(da, 1, "author") ||
                !fields.AddTexts(da, 2, "keywords") ||
                !fields.AddTexts(da, 3, "license") ||
                !fields.AddTexts(da, 4, "licenseVersion") ||
                !fields.AddTexts(da, 5, "password") ||
                !fields.AddBooleans(da, 6, "placeable") ||
                !fields.AddBooleans(da, 7, "uiDefault") ||
                !fields.AddBooleans(da, 8, "isSubtype") ||
                !fields.AddBooleans(da, 9, "hierarchicalPagesUsed"))
            {
                return;
            }

            var items = new JArray();
            for (var i = 0; i < names.Count; i++)
            {
                var metadata = new JObject();
                fields.ApplyTo(metadata, i);
                if (metadata.Count == 0)
                {
                    this.AddError("Give at least one metadata field to set.");
                    return;
                }

                items.Add(
                    new JObject
                    {
                        ["name"] = names[i],
                        ["metadata"] = metadata
                    });
            }

            var parameters = new JObject { ["libraryParts"] = items };

            SetCadValuesWithErrorMessages(
                CommandName,
                parameters,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetLibraryPartMetadata;

        public override Guid ComponentGuid =>
            new Guid("12228c46-ce03-4f61-b829-da2f07efdbcf");
    }
}
