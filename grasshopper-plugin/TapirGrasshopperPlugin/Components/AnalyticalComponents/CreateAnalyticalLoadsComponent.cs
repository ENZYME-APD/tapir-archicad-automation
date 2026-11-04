using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class CreateAnalyticalLoadsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateAnalyticalLoads";

        public CreateAnalyticalLoadsComponent()
            : base(
                "CreateAnalyticalLoads",
                "Creates analytical point, edge and surface loads from their settings, loaded member part and " +
                "custom data, as GetAnalyticalLoads returns them, in one undo step. Available from Archicad 26.",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Loads",
                "Each load to create as JSON, as the Loads output of GetAnalyticalLoads gives it: " +
                "{\"pointLoad\": ...}, {\"edgeLoad\": ..., \"customData\": {\"curve\": ...}} or " +
                "{\"surfaceLoad\": ..., \"customData\": {\"contour\": ...}}. Each is created from the defaults " +
                "of its tool; an edge load needs the curve and a surface load the contour it is applied on.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifier of each created load.");

            OutErrorMessages(
                "Error message of each load (empty when it was created).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(0, out List<string> texts) ||
                !AnalyticalJsonHelp.TryParseObjects(this, texts, "Loads", out List<JObject> items))
            {
                return;
            }

            var loads = new JArray();
            for (var i = 0; i < items.Count; i++)
            {
                JObject load = null;
                foreach (var pair in AnalyticalJsonHelp.LoadKinds)
                {
                    if (items[i][pair.Key] != null)
                    {
                        // A point load has no custom data; the geometry that
                        // GetAnalyticalLoads also returns is left out.
                        load = pair.Key == "pointLoad"
                            ? AnalyticalJsonHelp.Pick(items[i], pair.Key)
                            : AnalyticalJsonHelp.Pick(items[i], pair.Key, "customData");
                        break;
                    }
                }
                if (load == null)
                {
                    this.AddError(
                        $"Item {i} of the input Loads has no pointLoad, edgeLoad or surfaceLoad.");
                    return;
                }
                loads.Add(load);
            }

            SetCadValuesWithCreatedIds<ElementGuid>(
                CommandName,
                new JObject { ["loads"] = loads },
                ToAddOn,
                da,
                "elements",
                "elementId");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateAnalyticalLoadCases;

        public override Guid ComponentGuid =>
            new Guid("390d2787-4b9a-40ac-a013-791d6244d0c9");
    }
}
