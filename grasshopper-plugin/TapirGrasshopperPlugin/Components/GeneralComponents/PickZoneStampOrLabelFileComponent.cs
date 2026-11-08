using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class PickZoneStampOrLabelFileComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "PickZoneStampOrLabelFile";

        public PickZoneStampOrLabelFileComponent()
            : base(
                "PickZoneStampOrLabelFile",
                "Opens Archicad's dialog for opening a zone stamp or label library part, waits for the user and " +
                "returns the location of the chosen file, or reports that the user cancelled. Archicad waits for " +
                "the user, so it only runs on the Execute button.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "ObjectType",
                "The type of library part the dialog lets the user open: Label or ZoneStamp. Optional; default: Label.");

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "Cancelled",
                "True when the user cancelled the dialog or chose no valid zone stamp or label.");

            OutText(
                "Path",
                "The location of the chosen zone stamp or label. Empty when cancelled.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            string objectType = null;
            if (da.GetData(0, ref objectType) &&
                !string.IsNullOrEmpty(objectType))
            {
                parameters["objectType"] = objectType;
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
            Properties.Resources.PickZoneStampOrLabelFile;

        public override Guid ComponentGuid =>
            new Guid("daa07866-1c7f-4972-8d66-76fcb4a4096a");
    }
}
