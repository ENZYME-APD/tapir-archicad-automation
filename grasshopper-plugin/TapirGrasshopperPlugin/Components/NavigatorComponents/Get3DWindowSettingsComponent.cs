using Grasshopper.Kernel;
using Grasshopper.Kernel.Parameters;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class Get3DWindowSettingsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "Get3DWindowSettings";

        public Get3DWindowSettingsComponent()
            : base(
                "Get3DWindowSettings",
                "Gets the 3D window settings: the pixel size of the 3D window and the zoom (scale and " +
                "displacement) of its projection.",
                GroupNames.Navigator)
        {
        }

        protected override void AddOutputs()
        {
            Add3DWindowSettingsOutputs(this);
        }

        // Shared with Set3DWindowSettings, which returns the settings read back
        // after the change.
        public static void Add3DWindowSettingsOutputs(
            GH_Component component)
        {
            NavigatorComponentHelp.AddOutput(
                component,
                new Param_Integer(),
                "HSize",
                "The horizontal size of the 3D window in pixels.".WithTypeName("integerItem"));

            NavigatorComponentHelp.AddOutput(
                component,
                new Param_Integer(),
                "VSize",
                "The vertical size of the 3D window in pixels.".WithTypeName("integerItem"));

            NavigatorComponentHelp.AddOutput(
                component,
                new Param_Number(),
                "ZoomScaleX",
                "The horizontal zoom scale of the 3D projection.".WithTypeName("numberItem"));

            NavigatorComponentHelp.AddOutput(
                component,
                new Param_Number(),
                "ZoomScaleY",
                "The vertical zoom scale of the 3D projection.".WithTypeName("numberItem"));

            NavigatorComponentHelp.AddOutput(
                component,
                new Param_Number(),
                "ZoomDispX",
                "The horizontal displacement (pan) of the 3D projection.".WithTypeName("numberItem"));

            NavigatorComponentHelp.AddOutput(
                component,
                new Param_Number(),
                "ZoomDispY",
                "The vertical displacement (pan) of the 3D projection.".WithTypeName("numberItem"));
        }

        public static void Set3DWindowSettingsOutputs(
            IGH_DataAccess da,
            JObject response,
            int firstIndex = 0)
        {
            da.SetData(firstIndex, response.Value<int?>("hSize"));
            da.SetData(firstIndex + 1, response.Value<int?>("vSize"));
            da.SetData(firstIndex + 2, response.Value<double?>("zoomScaleX"));
            da.SetData(firstIndex + 3, response.Value<double?>("zoomScaleY"));
            da.SetData(firstIndex + 4, response.Value<double?>("zoomDispX"));
            da.SetData(firstIndex + 5, response.Value<double?>("zoomDispY"));
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

            Set3DWindowSettingsOutputs(
                da,
                response);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.Get3DWindowSettings;

        public override Guid ComponentGuid =>
            new Guid("f16fec6b-002f-4bf3-998f-aa34467c32f9");
    }
}
