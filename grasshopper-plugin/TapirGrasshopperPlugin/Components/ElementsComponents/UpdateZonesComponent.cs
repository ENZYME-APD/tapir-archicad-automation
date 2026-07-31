using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class UpdateZonesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "UpdateZones";

        public UpdateZonesComponent()
            : base(
                "UpdateZones",
                "Update all Zones: recalculate their geometry, Zone Stamps and connected elements.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InBoolean(
                "KeepStampPosition",
                "Keep the position of the Zone Stamps. Defaults to true. Optional.");

            InBoolean(
                "UndoTopTrim",
                "Undo the trimming of the top of the Zones. Defaults to false. Optional.");

            InBoolean(
                "UndoBottomTrim",
                "Undo the trimming of the bottom of the Zones. Defaults to false. Optional.");

            SetOptionality(new[] { 0, 1, 2 });
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            var keepStampPosition = true;
            if (da.GetData(0, ref keepStampPosition))
            {
                parameters["keepStampPosition"] = keepStampPosition;
            }

            var undoTopTrim = false;
            if (da.GetData(1, ref undoTopTrim))
            {
                parameters["undoTopTrim"] = undoTopTrim;
            }

            var undoBottomTrim = false;
            if (da.GetData(2, ref undoBottomTrim))
            {
                parameters["undoBottomTrim"] = undoBottomTrim;
            }

            TryGetCadResponse(
                CommandName,
                parameters,
                ToAddOn,
                out _);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.UpdateZones;

        public override Guid ComponentGuid =>
            new Guid("c8bf4d26-4c8f-4153-9474-390ebd11c005");
    }
}
