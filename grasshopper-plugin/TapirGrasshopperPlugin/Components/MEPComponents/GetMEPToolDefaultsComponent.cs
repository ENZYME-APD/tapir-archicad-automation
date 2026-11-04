using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class GetMEPToolDefaultsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetMEPToolDefaults";

        public GetMEPToolDefaultsComponent()
            : base(
                "GetMEPToolDefaults",
                "Retrieves the current settings of the MEP tools per domain, without changing them: the routing " +
                "tool (MEP system, branch preference table, floor plan display settings from Archicad 29, and its " +
                "segment, node, elbow (from Archicad 29) and transition defaults), the flexible segment tool, and " +
                "the library part and port settings of the terminal, accessory, fitting and equipment tools. " +
                "Available from Archicad 28.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Domains",
                "The MEP domains to get the tool defaults of: Ventilation, Piping or CableCarrier. " +
                "Optional; all three when it is omitted.");

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Domains",
                "The domain of each item of ToolDefaults.");

            OutTexts(
                "ToolDefaults",
                "The tool defaults of each requested domain as JSON (routingElement, flexibleSegment, terminal, " +
                "accessory, fitting, ... where Archicad gives them).");

            OutText(
                "EquipmentDefault",
                "The default of the Equipment tool, which has no domain, as JSON (null when Archicad gives none).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();
            da.TryGetList(0, out List<string> domains);
            if (domains != null &&
                domains.Count > 0)
            {
                parameters["domains"] = new JArray(domains);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var domainNames = new List<string>();
            var toolDefaults = new List<string>();
            foreach (var item in JsonOutputHelp.Items(response, "toolDefaults"))
            {
                domainNames.Add(item["domain"]?.ToString());
                toolDefaults.Add(item.ToString(Formatting.Indented));
            }

            var equipment = response["equipment"];
            da.SetDataList(0, domainNames);
            da.SetDataList(1, toolDefaults);
            da.SetData(
                2,
                equipment == null || equipment.Type == JTokenType.Null
                    ? null
                    : equipment.ToString(Formatting.Indented));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetMEPElementDetails;

        public override Guid ComponentGuid =>
            new Guid("a9f782d0-35a8-418d-88c5-cb274d79cb71");
    }
}
