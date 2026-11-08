using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class PickPictureFileComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "PickPictureFile";

        public PickPictureFileComponent()
            : base(
                "PickPictureFile",
                "Opens Archicad's Open Picture dialog (the one of the texture settings), waits for the user and " +
                "returns the location of the chosen picture, or reports that the user cancelled. Archicad adds the " +
                "chosen picture to the embedded library when it is not in it yet. Archicad waits for the user, so " +
                "it only runs on the Execute button.",
                GroupNames.General)
        {
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "Cancelled",
                "True when the user cancelled the dialog or chose no valid picture.");

            OutText(
                "Path",
                "The location of the chosen picture. Empty when cancelled.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
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
            Properties.Resources.PickPictureFile;

        public override Guid ComponentGuid =>
            new Guid("930aed49-2589-4522-8190-8e04fd8cd24c");
    }
}
