using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class SetGridSettingsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetGridSettings";

        // The keys of the settings object (the fields of API_GridType).
        private static readonly string[] SettingsKeys =
        {
            "spaceMainX",
            "spaceMainY",
            "nLineMainX",
            "nLineMainY",
            "gridSwitch",
            "spaceAuxX",
            "spaceAuxY",
            "nLineAuxX",
            "nLineAuxY",
            "gridMode",
            "snapOffsX",
            "snapOffsY",
            "snapStepX",
            "snapStepY",
            "status",
            "gridAngle",
            "gridOffsX",
            "gridOffsY",
            "gridOffsZ",
            "backColor",
            "gridColor"
        };

        // The keys of backColor and gridColor; a color may give only some of them.
        private static readonly string[] ColorKeys =
        {
            "red",
            "green",
            "blue",
            "alpha"
        };

        public static string SettingsKeysText => string.Join(", ", SettingsKeys);

        public SetGridSettingsComponent()
            : base(
                "SetGridSettings",
                "Changes the grid and snap grid settings of the database of the current window; only the given fields " +
                "change. Returns the settings read back after the change. Available from Archicad 27.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Settings_JSON",
                "The grid settings to change as a JSON object, in the form of the Settings_JSON output of " +
                "GetGridSettings: " + SettingsKeysText + ". Only the given keys are changed; a color (backColor, " +
                "gridColor) may give only some of red, green, blue and alpha.");
        }

        protected override void AddOutputs()
        {
            GetGridSettingsComponent.AddGridOutputs(this);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var text = "";
            if (!da.GetData(0, ref text) ||
                !NavigatorComponentHelp.TryParseObject(
                    this,
                    "Settings_JSON",
                    text,
                    SettingsKeys,
                    out JObject settings))
            {
                return;
            }

            foreach (var colorKey in new[] { "backColor", "gridColor" })
            {
                var color = settings[colorKey];
                if (color == null)
                {
                    continue;
                }

                if (!(color is JObject))
                {
                    this.AddError($"The {colorKey} of the input Settings_JSON must be a JSON object.");
                    return;
                }

                if (!NavigatorComponentHelp.TryParseObject(
                        this,
                        $"Settings_JSON ({colorKey})",
                        color.ToString(),
                        ColorKeys,
                        out JObject colorObject))
                {
                    return;
                }
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["settings"] = settings },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            GetGridSettingsComponent.SetGridOutputs(
                da,
                response);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetViewSettings;

        public override Guid ComponentGuid =>
            new Guid("2bb60a08-19e9-476f-a99f-c414a697bede");
    }
}
