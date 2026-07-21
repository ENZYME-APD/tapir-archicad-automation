using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class SetLayoutSettingsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetLayoutSettings";

        public SetLayoutSettingsComponent()
            : base(
                "SetLayoutSettings",
                "Set settings of layouts, including Layout Info Panel custom data fields. " +
                "Each input item is a JSON object for one layout, e.g. " +
                "{\"layoutNavigatorItemId\":{\"guid\":\"...\"},\"layoutName\":\"A-01\",\"customLayoutNumber\":\"01\"}. " +
                "The layout is identified by layoutDatabaseId or layoutNavigatorItemId.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "LayoutsData",
                "One JSON object per layout with the settings to change (see component description).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> layoutsData))
            {
                return;
            }

            var items = new JArray();
            foreach (var json in layoutsData)
            {
                try
                {
                    items.Add(JObject.Parse(json));
                }
                catch (Exception ex)
                {
                    this.AddError(
                        $"Invalid JSON in the LayoutsData input: {ex.Message}");
                    return;
                }
            }

            var parameters = new JObject { ["layoutsData"] = items };

            TryGetCadResponse(
                CommandName,
                parameters,
                ToAddOn,
                out _);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetLayoutSettings;

        public override Guid ComponentGuid =>
            new Guid("d831c9d3-8f2f-421f-bd7a-d532551dccf7");
    }
}
