using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetProjectPreferencesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetProjectPreferences";

        public GetProjectPreferencesComponent()
            : base(
                "GetProjectPreferences",
                "Gets pages of the project preferences (Options > Project Preferences): the working units, " +
                "the dimensions, the calculation units and the zones. Each page is a JSON text that " +
                "SetProjectPreferences takes back.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "PreferenceTypes",
                "The pages to get: " + string.Join(", ", ProjectPreferencesPages.TypeNames) +
                ". Every page when empty.");

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            for (var i = 0; i < ProjectPreferencesPages.TypeNames.Length; i++)
            {
                OutText(
                    ProjectPreferencesPages.TypeNames[i],
                    $"The {ProjectPreferencesPages.TypeNames[i]} page as a JSON text (null when it was not asked for).");
            }
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            da.TryGetList(
                0,
                out List<string> preferenceTypes);

            var parameters = new JObject();
            if (preferenceTypes != null &&
                preferenceTypes.Count > 0)
            {
                parameters["preferenceTypes"] = JArray.FromObject(preferenceTypes);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out var response))
            {
                return;
            }

            for (var i = 0; i < ProjectPreferencesPages.Keys.Length; i++)
            {
                da.SetData(
                    i,
                    response[ProjectPreferencesPages.Keys[i]]?.ToString(Formatting.Indented));
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetProjectPreferences;

        public override Guid ComponentGuid =>
            new Guid("b98ae142-48f1-43dd-98e0-c09d184ea04f");
    }
}
