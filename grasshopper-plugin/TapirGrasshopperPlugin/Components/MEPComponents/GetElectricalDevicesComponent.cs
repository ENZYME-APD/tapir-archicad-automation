using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class GetElectricalDevicesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetElectricalDevices";

        public GetElectricalDevicesComponent()
            : base(
                "GetElectricalDevices",
                "Get the electrical devices (lights, sockets and other consumers) of the project or the given ones: " +
                "parent circuit, placement, apparent power, load classification, demand load and directly connected " +
                "MEP elements. Tree outputs have one branch per device. " +
                "Available from Archicad 30.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the electrical devices. Optional: every electrical device of the project when not given.");

            SetOptionality(new[] { 0 });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifier of each electrical device.");

            OutGenerics(
                "ParentCircuitGuids",
                "Identifier of the circuit each device is connected to (null when it is not connected).");

            ElectricalPlacementOutputs.AddOutputs(this);

            OutNumberList(
                "ApparentPowers",
                "Apparent power of each device in VA.");

            OutBooleans(
                "IsApparentPowerEnabled",
                "True if the apparent power of the device is enabled.");

            OutTexts(
                "LoadClassificationNames",
                "Name of the load classification of each device, as defined in the electrical project settings.");

            OutNumberList(
                "DemandLoads",
                "Demand load of each device.");

            OutGenericTree(
                "ConnectedElementGuids",
                "Identifiers of the MEP elements directly connected to each device (one branch per device).");

            OutErrorMessages(
                "Error message of each device (empty when successful).");
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

            var parameters = new JObject();
            if (ids.Count > 0)
            {
                parameters["elements"] = ElectricalComponentHelp.ElementsArray(ids);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response) ||
                ElectricalComponentHelp.ReportResponseError(this, response))
            {
                return;
            }

            var elementGuids = new List<object>();
            var parentCircuitGuids = new List<object>();
            var placements = new ElectricalPlacementOutputs();
            var apparentPowers = new List<object>();
            var isApparentPowerEnabled = new List<object>();
            var loadClassificationNames = new List<object>();
            var demandLoads = new List<object>();
            var connectedElementGuids = new DataTree<object>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(response, "electricalDevices");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                connectedElementGuids.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    item = null;
                    errors.Add(JsonOutputHelp.ErrorMessage(items[i]));
                }
                else
                {
                    errors.Add("");
                }

                elementGuids.Add(ElectricalComponentHelp.ElementIdOf(item?["elementId"]));
                parentCircuitGuids.Add(ElectricalComponentHelp.ElementIdOf(item?["parentCircuitId"]));
                placements.Add(item);
                apparentPowers.Add(JsonOutputHelp.Scalar(item, "apparentPower"));
                isApparentPowerEnabled.Add(JsonOutputHelp.Scalar(item, "isApparentPowerEnabled"));
                loadClassificationNames.Add(JsonOutputHelp.Scalar(item, "loadClassificationName"));
                demandLoads.Add(JsonOutputHelp.Scalar(item, "demandLoad"));
                ElectricalComponentHelp.AddElementIds(item?["connectedElements"], connectedElementGuids, path);
            }

            da.SetDataList(0, elementGuids);
            da.SetDataList(1, parentCircuitGuids);
            placements.SetData(da, 2);
            var next = 2 + ElectricalPlacementOutputs.Count;
            da.SetDataList(next, apparentPowers);
            da.SetDataList(next + 1, isApparentPowerEnabled);
            da.SetDataList(next + 2, loadClassificationNames);
            da.SetDataList(next + 3, demandLoads);
            da.SetDataTree(next + 4, connectedElementGuids);
            da.SetDataList(next + 5, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetElectricalDevices;

        public override Guid ComponentGuid =>
            new Guid("7191e626-7924-48eb-9273-c5119b9052c2");
    }
}
