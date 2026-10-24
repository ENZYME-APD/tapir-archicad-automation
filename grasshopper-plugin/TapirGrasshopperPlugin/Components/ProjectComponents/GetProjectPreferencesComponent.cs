using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetProjectPreferencesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetProjectPreferences";

        public GetProjectPreferencesComponent()
            : base(
                "GetProjectPreferences",
                "Get the pages of the project preferences (Options > Project Preferences): the working " +
                "units, the calculation units, the dimensions and the zones. The units, decimals and " +
                "accuracies of each page come as one list per column, the zone settings as values, and " +
                "the whole structure as a JSON text that SetProjectPreferences takes back as it is. " +
                "A page the add-on does not know yet gives empty outputs.",
                GroupNames.Project)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                "JsonPreferences",
                ProjectPreferencesHelp.JsonOutputDescription);

            foreach (var table in ProjectPreferencesHelp.Tables)
            {
                table.AddOutputs(this);
            }

            OutInteger(
                ProjectPreferencesHelp.ZoneActiveHeightLimitsName,
                ProjectPreferencesHelp.ZoneActiveHeightLimitsDescription);

            OutNumberList(
                ProjectPreferencesHelp.ZoneHeightLimitsName,
                ProjectPreferencesHelp.ZoneHeightLimitsDescription);

            OutNumberList(
                ProjectPreferencesHelp.ZoneReductionPercentsName,
                ProjectPreferencesHelp.ZoneReductionPercentsDescription);

            OutNumberItem(
                ProjectPreferencesHelp.ZoneReductionPercentAboveLimitsName,
                ProjectPreferencesHelp.ZoneReductionPercentAboveLimitsDescription);
        }

        private void OutNumberItem(
            string name,
            string description)
        {
            outManager.AddNumberParameter(
                name,
                name,
                description.WithTypeName("numberItem"),
                GH_ParamAccess.item);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            // Without preferenceTypes the command answers every page it knows,
            // so an add-on without a page leaves that page's outputs empty
            // instead of failing the call.
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(
                0,
                response.ToString(Formatting.Indented));

            var index = 1;
            foreach (var table in ProjectPreferencesHelp.Tables)
            {
                table.SetOutputs(
                    da,
                    response[table.PageKey],
                    index);
                index += table.ParamCount;
            }

            ProjectPreferencesHelp.SetZoneOutputs(
                da,
                response["zones"],
                index);
        }

        // The project settings glyph until IconSource/GetProjectPreferences.svg
        // is rasterized into Resources.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetElectricalProjectSettings;

        public override Guid ComponentGuid =>
            new Guid("786113dc-c500-4409-8885-2a2510ee5057");
    }
}
