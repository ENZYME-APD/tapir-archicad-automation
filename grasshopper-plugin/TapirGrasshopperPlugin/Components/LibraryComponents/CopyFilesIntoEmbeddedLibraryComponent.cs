using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class CopyFilesIntoEmbeddedLibraryComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CopyFilesIntoEmbeddedLibrary";

        public CopyFilesIntoEmbeddedLibraryComponent()
            : base(
                "CopyFilesIntoEmbeddedLibrary",
                "Copies the given files, keeping their names, into a folder of the embedded library with one library operation. " +
                "The target folder is created when it is missing. " +
                "With OverwriteIfExists an item of the same name in that folder is replaced.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Paths",
                "Absolute paths of the files to copy. Each keeps its file name inside the target folder.");

            InText(
                "TargetFolder",
                "Folder relative to the embedded library to copy the files into, for example 'Office/Doors'; " +
                "missing folders are created. Optional, by default the root of the embedded library.");

            InBoolean(
                "OverwriteIfExists",
                "Replace an item with the same name in the target folder.",
                false);

            SetOptionality(new[] { 1, 2 });
        }

        protected override void AddOutputs()
        {
            OutText(
                "ErrorMessage",
                "Error message of the command (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> paths))
            {
                return;
            }

            var files = new JArray();
            foreach (var path in paths)
            {
                files.Add(new JObject { ["path"] = path });
            }

            var parameters = new JObject { ["files"] = files };

            string targetFolder = null;
            if (da.GetData(1, ref targetFolder) &&
                !string.IsNullOrEmpty(targetFolder))
            {
                parameters["targetFolder"] = targetFolder;
            }

            if (da.GetOptional(2, false))
            {
                parameters["overwriteIfExists"] = true;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(
                0,
                LibraryComponentHelp.ExecutionResultMessage(response));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CopyFilesIntoEmbeddedLibrary;

        public override Guid ComponentGuid =>
            new Guid("720db40c-7751-469b-9b22-cf9610a3db92");
    }
}
