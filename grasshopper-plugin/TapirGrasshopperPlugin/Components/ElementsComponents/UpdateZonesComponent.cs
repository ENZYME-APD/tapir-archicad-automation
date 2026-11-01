using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class UpdateZonesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "UpdateZones";

        public UpdateZonesComponent()
            : base(
                "UpdateZones",
                "Update Zones: recalculate their geometry, Zone Stamps and connected elements. Updates the given Zones; without them the selected Zones, or every Zone when nothing is selected.",
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

            InGenerics(
                "Zones",
                "Zones to update, on any story. They stay selected until Archicad has updated them, after the command has returned. If one of them cannot be updated, none is. An empty list updates nothing. Without this input the selected Zones are updated, or every Zone when nothing is selected. Optional.");

            SetOptionality(new[] { 0, 1, 2, 3 });
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

            // A connected input never falls back to the selection or every Zone, also when its list
            // is empty.
            var zonesInput = Params.Input[3];
            if (zonesInput.SourceCount > 0 || zonesInput.VolatileDataCount > 0)
            {
                if (!da.TryCreateFromList(
                        3,
                        out ElementsObject zones))
                {
                    return;
                }

                parameters["zones"] = JArray.FromObject(zones.Elements);
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
