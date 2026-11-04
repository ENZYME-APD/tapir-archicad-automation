using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class Set3DDocumentSettingsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "Set3DDocumentSettings";

        // The top level keys of the settings object; the nested ones are
        // checked by the command.
        private static readonly string[] SettingsKeys =
        {
            "filterAndCutSettings",
            "window3DFilter",
            "cutSetting",
            "cutaway3D",
            "projectionSetting",
            "floorPlanRotationOrigo",
            "window3DInfo",
            "hiddenLineType",
            "hiddenLineLen",
            "effects3D",
            "drawRect",
            "attributeOptions",
            "currPlanConn",
            "intPlanConn",
            "materialFrom3D",
            "docSrc",
            "skeletonOptions",
            "boundaryDisplay",
            "boundaryPen",
            "boundaryLineType",
            "transparency"
        };

        public static string SettingsKeysText => string.Join(", ", SettingsKeys);

        public Set3DDocumentSettingsComponent()
            : base(
                "Set3DDocumentSettings",
                "Changes the given settings of a 3D Document, given by its navigator item, in the form " +
                "Get3DDocumentSettings returns; the settings not given are kept. Returns the settings read back " +
                "after the change.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "NavigatorItemId",
                "The navigator item of the 3D Document, from the Project Map or the View Map.");

            InText(
                "Settings_JSON",
                "The settings to change as a JSON object, in the form of the Settings_JSON output of " +
                "Get3DDocumentSettings: " + SettingsKeysText + ". Only the given keys are changed; arrays other " +
                "than elemTypeFilter are replaced.");
        }

        protected override void AddOutputs()
        {
            OutText(
                "Settings_JSON",
                "The settings of the 3D Document read back after the change, as a JSON object.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreate(
                    0,
                    out NavigatorGuid navigatorItemId) ||
                !da.TryGet(1, out string settingsText))
            {
                return;
            }

            JObject settings;
            try
            {
                settings = JObject.Parse(settingsText);
            }
            catch (JsonReaderException ex)
            {
                this.AddError($"The input Settings_JSON is not a valid JSON object: {ex.Message}");
                return;
            }

            var unknownKeys = settings
                .Properties()
                .Select(p => p.Name)
                .Where(name => !SettingsKeys.Contains(name))
                .ToList();
            if (unknownKeys.Count > 0)
            {
                this.AddError(
                    $"Unknown keys in the input Settings_JSON: {string.Join(", ", unknownKeys)}. " +
                    $"Accepted keys: {SettingsKeysText}.");
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject
                    {
                        ["navigatorItemId"] = JObject.FromObject(navigatorItemId),
                        ["settings"] = settings
                    },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(0, NavigatorComponentHelp.JsonText(response["settings"]));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetDocumentFrom3DDefaults;

        public override Guid ComponentGuid =>
            new Guid("84ab9690-d8b8-4ac2-a73b-1adbf9c5762b");
    }
}
