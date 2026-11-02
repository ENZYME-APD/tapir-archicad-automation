using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Components.MEPComponents;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class SetLibraryPartScriptsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetLibraryPartScripts";

        // The script inputs after Names, with the key of each in "scripts".
        private static readonly (string Input, string Key, string Description)[] Scripts =
        {
            ("MasterScripts", "master", "master script"),
            ("ParameterScripts", "parameter", "parameter script"),
            ("Scripts2D", "2d", "2D script"),
            ("Scripts3D", "3d", "3D script"),
            ("PropertiesScripts", "properties", "properties script"),
            ("InterfaceScripts", "interface", "interface (user interface) script"),
            ("ForwardMigrationScripts", "forwardMigration", "forward migration script"),
            ("BackwardMigrationScripts", "backwardMigration", "backward migration script")
        };

        public SetLibraryPartScriptsComponent()
            : base(
                "SetLibraryPartScripts",
                "Sets and deletes the master, parameter, 2D, 3D, properties, interface and forward and backward migration " +
                "scripts of loaded library parts. Only the given scripts are changed; an empty script makes the section empty, " +
                "DeleteScripts removes it. A library part that is not editable (e.g. a part of a library package or a " +
                "read-only library) is refused and left unchanged. Available from Archicad 28.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Name of each loaded library part file with its extension, as the fileName of GetAvailableLibraryParts.");

            foreach (var script in Scripts)
            {
                InTexts(
                    script.Input,
                    $"New {script.Description} of each library part " +
                    "(input only 1 to use the same script for all; an empty text empties the script). Optional.");
            }

            InTextTree(
                "DeleteScripts",
                "Script sections to delete from each library part (one branch per library part, or one branch for all): " +
                "master, parameter, 2d, 3d, properties, interface, forwardMigration or backwardMigration. " +
                "A script cannot be both set and deleted. Optional.");

            var optional = new int[Scripts.Length + 1];
            for (var i = 0; i < optional.Length; i++)
            {
                optional[i] = i + 1;
            }
            SetOptionality(optional);
        }

        private void InTextTree(
            string name,
            string description)
        {
            inManager.AddTextParameter(
                name,
                name,
                description,
                GH_ParamAccess.tree);
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

            var scriptFields = new ElectricalItemFields(this, 0, names.Count);
            for (var i = 0; i < Scripts.Length; i++)
            {
                if (!scriptFields.AddTexts(da, i + 1, Scripts[i].Key))
                {
                    return;
                }
            }

            da.TryGetTree(Scripts.Length + 1, out GH_Structure<GH_String> deleteScripts);
            var deleteBranches = deleteScripts?.Branches ?? new List<List<GH_String>>();
            if (deleteBranches.Count > 1 &&
                deleteBranches.Count != names.Count)
            {
                this.AddError(
                    "The DeleteScripts input must have 0, 1 or as many branches as the size of the input Names.");
                return;
            }

            var items = new JArray();
            for (var i = 0; i < names.Count; i++)
            {
                var item = new JObject { ["name"] = names[i] };

                var scripts = new JObject();
                scriptFields.ApplyTo(scripts, i);
                if (scripts.Count > 0)
                {
                    item["scripts"] = scripts;
                }

                if (deleteBranches.Count > 0)
                {
                    var toDelete = new JArray();
                    foreach (var script in deleteBranches[deleteBranches.Count == 1 ? 0 : i])
                    {
                        if (!string.IsNullOrEmpty(script?.Value))
                        {
                            toDelete.Add(script.Value);
                        }
                    }
                    if (toDelete.Count > 0)
                    {
                        item["deleteScripts"] = toDelete;
                    }
                }

                items.Add(item);
            }

            var parameters = new JObject { ["libraryParts"] = items };

            SetCadValuesWithErrorMessages(
                CommandName,
                parameters,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetLibraryPartScripts;

        public override Guid ComponentGuid =>
            new Guid("c74822ae-d00f-41f7-9876-f89509bfc1e1");
    }
}
