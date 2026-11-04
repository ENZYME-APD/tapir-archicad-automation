using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class CreateAnalyticalSupportsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateAnalyticalSupports";

        public CreateAnalyticalSupportsComponent()
            : base(
                "CreateAnalyticalSupports",
                "Creates analytical supports from their settings and supported member part, as " +
                "GetAnalyticalSupports returns them, in one undo step. Available from Archicad 26.",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Supports",
                "Each support to create as JSON, as the Supports output of GetAnalyticalSupports gives it: " +
                "{\"supportSettings\": ..., \"supportedMemberPartRef\": ...}. Each is created from the defaults " +
                "of the Analytical Support tool.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifier of each created support.");

            OutErrorMessages(
                "Error message of each support (empty when it was created).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(0, out List<string> texts) ||
                !AnalyticalJsonHelp.TryParseObjects(this, texts, "Supports", out List<JObject> items))
            {
                return;
            }

            var supports = new JArray();
            foreach (var item in items)
            {
                supports.Add(AnalyticalJsonHelp.Unwrap(item, "support"));
            }

            SetCadValuesWithCreatedIds<ElementGuid>(
                CommandName,
                new JObject { ["supports"] = supports },
                ToAddOn,
                da,
                "elements",
                "elementId");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateAnalyticalLoadCases;

        public override Guid ComponentGuid =>
            new Guid("8c070014-920e-4649-9ff3-62160755afd6");
    }
}
