using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class ModifyElectricalDevicesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ModifyElectricalDevices";

        private const int PlacementInputsIndex = 5;

        public ModifyElectricalDevicesComponent()
            : base(
                "ModifyElectricalDevices",
                "Modify the apparent power, load classification and placement (anchor point, orientation, rotation, " +
                "mirroring, library part, size) of electrical devices. Only the given fields are changed. " +
                "Use ConnectToElectricalCircuits to change the circuit of a device. " +
                "Available from Archicad 30.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the electrical devices to modify.");

            InIntegers(
                "ApparentPowers",
                "New apparent power of each device in VA, not negative (input only 1 to use the same power for all). Optional.");

            InBooleans(
                "IsApparentPowerEnabled",
                "Enable the apparent power of each device (input only 1 to use the same value for all). Optional.");

            InTexts(
                "LoadClassificationNames",
                "New load classification of each device: one of the load classifications of the electrical project " +
                "settings, or an empty text for none (input only 1 to use the same classification for all). Optional.");

            InPoints(
                "AnchorPoints",
                "New anchor point of each device (input only 1 to use the same point for all). Optional.");

            SetOptionality(new[] { 1, 2, 3, 4 });

            ElectricalPlacementInputs.Add(this, PlacementInputsIndex);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each electrical device (empty when it was modified successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!ElectricalComponentHelp.TryGetElementIds(
                    this,
                    da,
                    0,
                    out List<ElementGuid> ids))
            {
                return;
            }
            if (ids.Count == 0)
            {
                this.AddError("The input ElementGuids must contain at least one item.");
                return;
            }

            var fields = new ElectricalItemFields(this, 0, ids.Count);
            if (!fields.AddNonNegativeIntegers(da, 1, "apparentPower") ||
                !fields.AddBooleans(da, 2, "isApparentPowerEnabled") ||
                !fields.AddTexts(da, 3, "loadClassificationName") ||
                !fields.AddPoints(da, 4, "anchorPoint") ||
                !ElectricalPlacementInputs.TryRead(fields, da, PlacementInputsIndex))
            {
                return;
            }

            var items = new JArray();
            for (var i = 0; i < ids.Count; i++)
            {
                var item = new JObject
                {
                    ["elementId"] = new JObject { ["guid"] = ids[i].Guid }
                };
                fields.ApplyTo(item, i);
                items.Add(item);
            }

            var parameters = new JObject { ["electricalDevicesData"] = items };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response) ||
                ElectricalComponentHelp.ReportResponseError(this, response))
            {
                return;
            }

            da.SetDataList(0, ExecutionResultsResponse.ErrorMessages(response));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyElectricalDevices;

        public override Guid ComponentGuid =>
            new Guid("6054374c-00cb-4579-b35a-5382fb1c5f08");
    }
}
