using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class CheckMEPPortConnectabilityComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "CheckMEPPortConnectability";

        public CheckMEPPortConnectabilityComponent()
            : base(
                "CheckMEPPortConnectability",
                "Checks whether MEP ports can be connected to other ports, for the given pairs of ports. " +
                "Archicad 28 and later check the physical connection. Available from Archicad 27.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "PortIds",
                "The ports to check, as returned by GetMEPPorts.");

            InTexts(
                "OtherPortIds",
                "The port to connect each of them to, as returned by GetMEPPorts " +
                "(input only 1 to check all against the same port).");
        }

        protected override void AddOutputs()
        {
            OutBooleans(
                "IsConnectable",
                "True if the port can be connected to the other port (empty when the check failed).");

            OutErrorMessages(
                "Error message of each pair of ports (empty when it was checked).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!MEPComponentHelp.TryGetGuidTexts(this, da, 0, out var portIds) ||
                !MEPComponentHelp.TryGetGuidTexts(this, da, 1, out var otherPortIds) ||
                portIds.Count == 0 ||
                otherPortIds.Count == 0)
            {
                return;
            }

            if (otherPortIds.Count != 1 &&
                otherPortIds.Count != portIds.Count)
            {
                this.AddError(
                    "The size of the input OtherPortIds must be 1 or equal to the size of the input PortIds.");
                return;
            }

            var pairs = new JArray();
            for (var i = 0; i < portIds.Count; i++)
            {
                pairs.Add(
                    new JObject
                    {
                        ["portId"] = portIds[i],
                        ["otherPortId"] = otherPortIds[otherPortIds.Count == 1 ? 0 : i]
                    });
            }

            var parameters = new JObject { ["portPairs"] = pairs };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var isConnectable = new List<object>();
            var errors = new List<string>();
            foreach (var result in JsonOutputHelp.Items(response, "results"))
            {
                if (JsonOutputHelp.IsError(result))
                {
                    isConnectable.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(result));
                    continue;
                }
                isConnectable.Add(JsonOutputHelp.Scalar(result, "isConnectable"));
                errors.Add("");
            }

            da.SetDataList(0, isConnectable);
            da.SetDataList(1, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CheckMEPPortConnectability;

        public override Guid ComponentGuid =>
            new Guid("ddc5d94a-3a14-46bd-b377-6871f7aacc6a");
    }
}
