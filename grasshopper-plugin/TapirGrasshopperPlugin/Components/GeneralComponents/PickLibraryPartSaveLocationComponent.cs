using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class PickLibraryPartSaveLocationComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "PickLibraryPartSaveLocation";

        public PickLibraryPartSaveLocationComponent()
            : base(
                "PickLibraryPartSaveLocation",
                "Opens Archicad's Save Library Part dialog (object, door or window), waits for the user and returns " +
                "the location chosen to save the library part to, or reports that the user cancelled. Nothing is " +
                "saved. Archicad waits for the user, so it only runs on the Execute button.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "DefaultPath",
                "The location (library folder and file name) the dialog starts from. Optional; default: " +
                "<LibraryPartType>.gsm in the embedded library of the project.");

            InText(
                "LibraryPartType",
                "Which Save Library Part dialog to open: Object, Door or Window. Optional; default: Object.");

            InBoolean(
                "BrowseForFolder",
                "Lets the user save outside the embedded library. Default is false.",
                false);

            InText(
                "Title",
                "The title of the dialog. Optional; default: Archicad's own title.");

            SetOptionality(new[] { 0, 1, 2, 3 });
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "Cancelled",
                "True when the user cancelled the dialog.");

            OutText(
                "Path",
                "The location the user chose to save the library part to. Nothing is saved there. Empty when cancelled.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            string defaultPath = null;
            if (da.GetData(0, ref defaultPath) &&
                !string.IsNullOrEmpty(defaultPath))
            {
                parameters["defaultPath"] = defaultPath;
            }

            string libraryPartType = null;
            if (da.GetData(1, ref libraryPartType) &&
                !string.IsNullOrEmpty(libraryPartType))
            {
                parameters["libraryPartType"] = libraryPartType;
            }

            if (da.GetOptional(2, false))
            {
                parameters["browseForFolder"] = true;
            }

            string title = null;
            if (da.GetData(3, ref title) &&
                !string.IsNullOrEmpty(title))
            {
                parameters["title"] = title;
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
            Properties.Resources.PickLibraryPartSaveLocation;

        public override Guid ComponentGuid =>
            new Guid("2f01663a-35b3-4f4f-aee9-ce80e2733461");
    }
}
