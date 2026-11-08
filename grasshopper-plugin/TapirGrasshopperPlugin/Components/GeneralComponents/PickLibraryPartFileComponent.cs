using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class PickLibraryPartFileComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "PickLibraryPartFile";

        public PickLibraryPartFileComponent()
            : base(
                "PickLibraryPartFile",
                "Opens Archicad's Open Library Part dialog, waits for the user and returns the location of the " +
                "chosen library part, or reports that the user cancelled. Archicad waits for the user, so it only " +
                "runs on the Execute button.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            InBoolean(
                "GeneralGDLObjectsOnly",
                "Sets the dialog's file filter to General GDL Objects. Default is false.",
                false);

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "Cancelled",
                "True when the user cancelled the dialog or chose no valid library part.");

            OutText(
                "Path",
                "The location of the chosen library part. Empty when cancelled.");

            OutBoolean(
                "OpenedInExternalEditor",
                "True when Archicad opened the chosen file in an external editor (Archicad 26 and later; always " +
                "false in Archicad 25). Empty when cancelled.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();
            if (da.GetOptional(0, false))
            {
                parameters["generalGDLObjectsOnly"] = true;
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
            da.SetData(
                2,
                (bool?)response["openedInExternalEditor"]);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.PickLibraryPartFile;

        public override Guid ComponentGuid =>
            new Guid("cdf1fc39-6860-4d04-b9b7-310d67c1da72");
    }
}
