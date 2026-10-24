using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class UpdateZonesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "UpdateZones";

        private const int ZonesInputIndex = 3;

        public UpdateZonesComponent()
            : base(
                "UpdateZones",
                "Update Zones: recalculate their geometry, Zone Stamps and connected elements. Without Zones the selected Zones are updated, and with an empty selection every Zone of the project.",
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
                "The Zones to update. When connected, exactly these Zones are updated (an empty list updates nothing); when a Zone cannot be selected, nothing is updated. When not connected, the selected Zones are updated, and with an empty selection every Zone of the project. Optional.");

            SetOptionality(new[] { 0, 1, 2, ZonesInputIndex });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each given Zone (empty when it was updated). Empty when Zones is not connected.");

            OutBooleans(
                "AreaChanged",
                "Whether the update changed the area of each given Zone. Empty when Zones is not connected.");
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

            // A connected but empty Zones input updates nothing, which is not the same as
            // leaving zones out (that updates the selection, or every Zone), so the input
            // is sent whenever something is wired to it.
            var zonesGiven = Params.Input[ZonesInputIndex].SourceCount > 0;
            if (zonesGiven)
            {
                if (!da.TryCreateFromList(
                        ZonesInputIndex,
                        out ElementsObject zones))
                {
                    this.AddError("Invalid element identifier in the Zones input.");
                    return;
                }

                parameters["zones"] = JArray.FromObject(zones.Elements);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out var response))
            {
                return;
            }

            if (response["success"] is JValue success &&
                success.Type == JTokenType.Boolean &&
                !(bool)success &&
                response["error"] is JObject error)
            {
                this.AddError(error["message"]?.ToString() ?? $"The {CommandName} command failed.");
            }

            var errorMessages = new List<string>();
            var areaChanged = new List<bool>();
            if (zonesGiven && response["zones"] is JArray zoneResults)
            {
                foreach (var token in zoneResults)
                {
                    var zoneResult = token as JObject;
                    var zoneError = zoneResult?["error"] as JObject;
                    errorMessages.Add(zoneError?["message"]?.ToString() ?? string.Empty);
                    areaChanged.Add(zoneResult?["areaChanged"]?.Type == JTokenType.Boolean && (bool)zoneResult["areaChanged"]);
                }
            }

            da.SetDataList(0, errorMessages);
            da.SetDataList(1, areaChanged);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.UpdateZones;

        public override Guid ComponentGuid =>
            new Guid("c8bf4d26-4c8f-4153-9474-390ebd11c005");
    }
}
