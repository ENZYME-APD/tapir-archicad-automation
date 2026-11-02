using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Analytical;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class SetAnalyticalVisibilitySettingsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetAnalyticalVisibilitySettings";

        private static (string Key, string Name, string Description)[] Flags =>
            AnalyticalVisibilitySettings.Flags;

        public SetAnalyticalVisibilitySettingsComponent()
            : base(
                "SetAnalyticalVisibilitySettings",
                "Changes the given fields of the Structural Analytical Model visibility settings; " +
                "the others keep their value. Available from Archicad 26.",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "VisibilityMode",
                AnalyticalVisibilitySettings.VisibilityModeDoc +
                " Optional; unchanged when it is omitted.");

            // Booleans without a default value, so that a setting left
            // unconnected keeps its current value instead of being turned off.
            foreach (var flag in Flags)
            {
                inManager.AddBooleanParameter(
                    flag.Name,
                    flag.Name,
                    (flag.Description + " Optional; unchanged when it is omitted.")
                    .WithTypeName("booleanItem"),
                    GH_ParamAccess.item);
            }

            InText(
                "LoadVisibilityMode",
                AnalyticalVisibilitySettings.LoadVisibilityModeDoc +
                " Optional; unchanged when it is omitted.");

            for (var i = 0; i < Flags.Length + 2; i++)
            {
                SetOptionality(i);
            }
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var settings = new JObject();

            if (da.TryGet(
                    0,
                    out string visibilityMode) &&
                !string.IsNullOrEmpty(visibilityMode))
            {
                settings["visibilityMode"] = visibilityMode;
            }

            for (var i = 0; i < Flags.Length; i++)
            {
                if (da.TryGet(
                        i + 1,
                        out bool value))
                {
                    settings[Flags[i].Key] = value;
                }
            }

            if (da.TryGet(
                    Flags.Length + 1,
                    out string loadVisibilityMode) &&
                !string.IsNullOrEmpty(loadVisibilityMode))
            {
                settings["loadVisibilityMode"] = loadVisibilityMode;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["visibilitySettings"] = settings },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            AnalyticalHelp.ReportExecutionResult(
                this,
                response,
                "Failed to set the Structural Analytical Model visibility settings.");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetAnalyticalVisibilitySettings;

        public override Guid ComponentGuid =>
            new Guid("52a616cc-8c48-4c52-964e-5df27c3fe12b");
    }
}
