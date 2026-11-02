using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.TeamworkComponents
{
    // The BIMcloud library commands take a list of libraries, the server and
    // the folder on the server, and answer with one result per library.
    public abstract class AbsBIMcloudLibrariesComponent : ArchicadExecutorComponent
    {
        protected AbsBIMcloudLibrariesComponent(
            string name,
            string description)
            : base(
                name,
                description,
                GroupNames.Teamwork)
        {
        }

        protected abstract string LibrariesKey { get; }
        protected abstract string LibrariesInputName { get; }
        protected abstract string LibrariesDescription { get; }

        protected override void AddInputs()
        {
            InTexts(
                LibrariesInputName,
                LibrariesDescription);

            InText(
                "ServerUrl",
                "URL of the BIMcloud or Teamwork server, e.g. 'https://bimcloud.example.com:22000'.");

            InText(
                "ParentPath",
                "Path of the folder on the server that holds the libraries, as shown in BIMcloud Manager.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each library (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> libraries))
            {
                return;
            }

            if (!da.TryGet(
                    1,
                    out string serverUrl) ||
                string.IsNullOrEmpty(serverUrl))
            {
                this.AddError("ServerUrl is required.");
                return;
            }

            if (!da.TryGet(
                    2,
                    out string parentPath))
            {
                return;
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject
                {
                    [LibrariesKey] = new JArray(libraries),
                    ["serverUrl"] = serverUrl,
                    ["parentPath"] = parentPath ?? ""
                },
                ToAddOn,
                da);
        }
    }

    public class UploadLibrariesToBIMcloudComponent : AbsBIMcloudLibrariesComponent
    {
        public override string CommandName => "UploadLibrariesToBIMcloud";

        public UploadLibrariesToBIMcloudComponent()
            : base(
                "UploadLibrariesToBIMcloud",
                "Uploads local library folders or container files (.lcf) to a folder of a BIMcloud or Teamwork " +
                "server, as BIMcloud libraries. Returns a result per library. Available from Archicad 29.")
        {
        }

        protected override string LibrariesKey => "libraryPaths";
        protected override string LibrariesInputName => "LibraryPaths";

        protected override string LibrariesDescription =>
            "Paths of the local library folders or container files (.lcf) to upload.";

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.UploadLibrariesToBIMcloud;

        public override Guid ComponentGuid =>
            new Guid("85d131d0-08ea-46a7-9bb9-aa10f3e42617");
    }

    public class RemoveLibrariesFromBIMcloudComponent : AbsBIMcloudLibrariesComponent
    {
        public override string CommandName => "RemoveLibrariesFromBIMcloud";

        public RemoveLibrariesFromBIMcloudComponent()
            : base(
                "RemoveLibrariesFromBIMcloud",
                "Removes libraries, given by name, from a folder of a BIMcloud or Teamwork server. " +
                "Returns a result per library. Available from Archicad 29.")
        {
        }

        protected override string LibrariesKey => "libraryNames";
        protected override string LibrariesInputName => "LibraryNames";

        protected override string LibrariesDescription =>
            "Names of the libraries on the server to remove.";

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RemoveLibrariesFromBIMcloud;

        public override Guid ComponentGuid =>
            new Guid("44daad19-833a-41c6-9dba-5d43c4f38211");
    }
}
