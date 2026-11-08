using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class PickLibraryFolderComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "PickLibraryFolder";

        public PickLibraryFolderComponent()
            : base(
                "PickLibraryFolder",
                "Opens Archicad's dialog for choosing or creating a library folder, waits for the user and returns " +
                "the location of the folder, or reports that the user cancelled. The folder is not added to the " +
                "libraries of the project. Archicad waits for the user, so it only runs on the Execute button.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "DefaultName",
                "The default name the dialog offers for a new library folder. Optional; default: New Library.");

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "Cancelled",
                "True when the user cancelled the dialog.");

            OutText(
                "Path",
                "The location of the chosen or created folder. It is not added to the libraries of the project; " +
                "use AddLibraries for that. Empty when cancelled.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            string defaultName = null;
            if (da.GetData(0, ref defaultName) &&
                !string.IsNullOrEmpty(defaultName))
            {
                parameters["defaultName"] = defaultName;
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
                (bool?)response["cancelled"]);
            da.SetData(
                1,
                response["path"]?.ToString());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.PickLibraryFolder;

        public override Guid ComponentGuid =>
            new Guid("91ef89ca-d6c2-4ca0-b71f-58aa89476f13");
    }
}
