using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class Set3DWindowSettingsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "Set3DWindowSettings";

        public Set3DWindowSettingsComponent()
            : base(
                "Set3DWindowSettings",
                "Changes the pixel size of the 3D window and/or the zoom (scale and displacement) of its " +
                "projection; the values not given are kept. Returns the settings read back after the change. " +
                "Archicad may need a rebuild of the 3D window (RebuildView) before it shows the change, e.g. " +
                "before SaveCurrentWindowAsImage.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            NavigatorComponentHelp.AddOptionalInteger(
                this,
                "HSize",
                "The horizontal size of the 3D window in pixels (1 to 32767). Optional.");

            NavigatorComponentHelp.AddOptionalInteger(
                this,
                "VSize",
                "The vertical size of the 3D window in pixels (1 to 32767). Optional.");

            NavigatorComponentHelp.AddOptionalNumber(
                this,
                "ZoomScaleX",
                "The horizontal zoom scale of the 3D projection (greater than 0). Optional.");

            NavigatorComponentHelp.AddOptionalNumber(
                this,
                "ZoomScaleY",
                "The vertical zoom scale of the 3D projection (greater than 0). Optional.");

            NavigatorComponentHelp.AddOptionalNumber(
                this,
                "ZoomDispX",
                "The horizontal displacement (pan) of the 3D projection. Optional.");

            NavigatorComponentHelp.AddOptionalNumber(
                this,
                "ZoomDispY",
                "The vertical displacement (pan) of the 3D projection. Optional.");
        }

        protected override void AddOutputs()
        {
            Get3DWindowSettingsComponent.Add3DWindowSettingsOutputs(this);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            NavigatorComponentHelp.CopyOptional<int>(da, 0, parameters, "hSize");
            NavigatorComponentHelp.CopyOptional<int>(da, 1, parameters, "vSize");
            NavigatorComponentHelp.CopyOptional<double>(da, 2, parameters, "zoomScaleX");
            NavigatorComponentHelp.CopyOptional<double>(da, 3, parameters, "zoomScaleY");
            NavigatorComponentHelp.CopyOptional<double>(da, 4, parameters, "zoomDispX");
            NavigatorComponentHelp.CopyOptional<double>(da, 5, parameters, "zoomDispY");

            if (parameters.Count == 0)
            {
                this.AddError(
                    "Give at least one of the inputs.");
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            Get3DWindowSettingsComponent.Set3DWindowSettingsOutputs(
                da,
                response);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.Set3DWindowSettings;

        public override Guid ComponentGuid =>
            new Guid("06a19e3c-80ce-4d6c-8f39-15ff67cc5b14");
    }
}
