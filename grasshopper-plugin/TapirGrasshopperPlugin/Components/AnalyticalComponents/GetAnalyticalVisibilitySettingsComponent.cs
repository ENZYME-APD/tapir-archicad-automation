using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Analytical;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class GetAnalyticalVisibilitySettingsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAnalyticalVisibilitySettings";

        public GetAnalyticalVisibilitySettingsComponent()
            : base(
                "GetAnalyticalVisibilitySettings",
                "Retrieves the visibility settings of the Structural Analytical Model. " +
                "Available from Archicad 26.",
                GroupNames.Analytical)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                "VisibilityMode",
                AnalyticalVisibilitySettings.VisibilityModeDoc);

            foreach (var flag in AnalyticalVisibilitySettings.Flags)
            {
                OutBoolean(
                    flag.Name,
                    flag.Description);
            }

            OutText(
                "LoadVisibilityMode",
                AnalyticalVisibilitySettings.LoadVisibilityModeDoc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var settings = response["visibilitySettings"];

            da.SetData(
                0,
                JsonOutputHelp.Scalar(
                    settings,
                    "visibilityMode"));

            for (var i = 0; i < AnalyticalVisibilitySettings.Flags.Length; i++)
            {
                da.SetData(
                    i + 1,
                    JsonOutputHelp.Scalar(
                        settings,
                        AnalyticalVisibilitySettings.Flags[i].Key));
            }

            da.SetData(
                AnalyticalVisibilitySettings.Flags.Length + 1,
                JsonOutputHelp.Scalar(
                    settings,
                    "loadVisibilityMode"));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAnalyticalVisibilitySettings;

        public override Guid ComponentGuid =>
            new Guid("dc34dbdd-3e24-4e34-a601-8f256bcb9ff2");
    }
}
