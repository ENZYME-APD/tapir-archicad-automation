using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class ModifyLibraryListComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ModifyLibraryList";

        public ModifyLibraryListComponent()
            : base(
                "ModifyLibraryList",
                "Adds libraries by path and removes loaded libraries by name or path, in one change of the library " +
                "list. Returns the result of each addition and removal. Available from Archicad 28.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "AddPaths",
                "Absolute path of each library folder or container file to add. Optional.");

            InTexts(
                "RemoveNames",
                "Name of each loaded library to remove, as returned by GetLibraries. Optional.");

            InTexts(
                "RemovePaths",
                "Absolute path of each loaded library to remove, as returned by GetLibraries. Optional.");

            SetOptionality(new[] { 0, 1, 2 });
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "AddErrorMessages",
                "Error message of each library to add (empty when it was added).");

            OutTexts(
                "RemoveErrorMessages",
                "Error message of each library to remove, names first, then paths (empty when it was removed).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var addPaths = da.OptionalList<string>(0);
            var removeNames = da.OptionalList<string>(1);
            var removePaths = da.OptionalList<string>(2);

            var add = new JArray();
            foreach (var path in addPaths)
            {
                add.Add(new JObject { ["path"] = path });
            }

            var remove = new JArray();
            foreach (var name in removeNames)
            {
                remove.Add(new JObject { ["name"] = name });
            }

            foreach (var path in removePaths)
            {
                remove.Add(new JObject { ["path"] = path });
            }

            if (add.Count == 0 && remove.Count == 0)
            {
                this.AddError("Give a library to add or to remove.");
                return;
            }

            var parameters = new JObject();
            if (add.Count > 0)
            {
                parameters["add"] = add;
            }

            if (remove.Count > 0)
            {
                parameters["remove"] = remove;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetDataList(0, ExecutionResultsResponse.ErrorMessages(response, "addResults"));
            da.SetDataList(1, ExecutionResultsResponse.ErrorMessages(response, "removeResults"));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetLibraries;

        public override Guid ComponentGuid =>
            new Guid("e232a398-d1f5-4e0c-ac20-7bef82032c7f");
    }
}
