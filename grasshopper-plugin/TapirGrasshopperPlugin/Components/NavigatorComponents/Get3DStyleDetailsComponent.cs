using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class Get3DStyleDetailsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "Get3DStyleDetails";

        public Get3DStyleDetailsComponent()
            : base(
                "Get3DStyleDetails",
                "Gets the settings of 3D styles by name (an empty name or no names means the current 3D style): " +
                "rendering engine, model type, monochrome and background colors, contours, shadows and hatching. " +
                "Available from Archicad 27.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "The names of the 3D styles. An empty name means the current 3D style. Optional, without names " +
                "the current 3D style is returned.");

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Names",
                "The name of each 3D style (null for a failed item).");

            OutTexts(
                "Settings_JSON",
                "The other settings of each 3D style as a JSON object, in the form the Settings_JSON input of " +
                "Create3DStyles and Change3DStyles accepts (null for a failed item). " + Style3DHelp.SettingsDoc);

            OutErrorMessages(
                "Error message of each 3D style (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            if (da.TryGetList(
                    0,
                    out List<string> names) &&
                names.Count > 0)
            {
                parameters["names"] = new JArray(names);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var styleNames = new List<string>();
            var settings = new List<string>();
            var errorMessages = new List<string>();

            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "styles"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    styleNames.Add(null);
                    settings.Add(null);
                    errorMessages.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                styleNames.Add(item["name"]?.ToString());
                settings.Add(Style3DHelp.SettingsJson(item));
                errorMessages.Add("");
            }

            da.SetDataList(
                0,
                styleNames);

            da.SetDataList(
                1,
                settings);

            da.SetDataList(
                2,
                errorMessages);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.Get3DStyleDetails;

        public override Guid ComponentGuid =>
            new Guid("8f88ddb5-2d62-4091-8145-1dfbc5eeadf7");
    }
}
