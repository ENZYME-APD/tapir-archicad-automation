using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetGridSettingsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetGridSettings";

        public GetGridSettingsComponent()
            : base(
                "GetGridSettings",
                "Gets the grid and snap grid settings of the database of the current window, and which database that " +
                "is. Each database has its own grid settings. Available from Archicad 27.",
                GroupNames.Navigator)
        {
        }

        protected override void AddOutputs()
        {
            AddGridOutputs(this);
        }

        // The outputs of the grid settings, shared with SetGridSettings.
        internal static void AddGridOutputs(
            Component component)
        {
            component.OutGeneric(
                "DatabaseGuid",
                "The database whose grid settings these are: the database of the current window.");

            component.OutText(
                "WindowType",
                "The type of the window of that database.");

            component.OutText(
                "Settings_JSON",
                "The grid settings as a JSON object, with the field names of API_GridType: " +
                SetGridSettingsComponent.SettingsKeysText + " (lengths in meters, colors with red, green, blue and " +
                "alpha from 0 to 1).");
        }

        internal static void SetGridOutputs(
            IGH_DataAccess da,
            JObject response)
        {
            var databaseId = response["database"]?["databaseId"]?.ToObject<DatabaseGuidObject>();
            if (databaseId != null)
            {
                da.SetData(0, new DatabaseGuidWrapper { DatabaseId = databaseId });
            }

            da.SetData(1, response["database"]?["windowType"]?.ToString());
            da.SetData(2, NavigatorComponentHelp.JsonText(response["settings"]));
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

            SetGridOutputs(
                da,
                response);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetViewSettings;

        public override Guid ComponentGuid =>
            new Guid("2cb41bf7-1980-407d-83de-39bd22904b08");
    }
}
