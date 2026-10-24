using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class SetProjectPreferencesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetProjectPreferences";

        public SetProjectPreferencesComponent()
            : base(
                "SetProjectPreferences",
                "Set pages of the project preferences (Options > Project Preferences): the working units, " +
                "the calculation units, the dimensions and the zones. Only the given fields change, the " +
                "rest of each page keeps its value. Every input is optional: the lists of a page give its " +
                "rows and their new values, JsonPreferences takes the whole structure GetProjectPreferences " +
                "outputs (or a part of it) for the other fields and for restoring saved preferences. " +
                "In Teamwork the PreferencesDialog lockable object set has to be reserved first " +
                "(ReserveLockableObjectSet). Available from Archicad 27.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "JsonPreferences",
                ProjectPreferencesHelp.JsonInputDescription);

            foreach (var table in ProjectPreferencesHelp.Tables)
            {
                table.AddInputs(this);
            }

            // The item inputs get no default value: a default would be sent
            // even when nothing is connected.
            inManager.AddIntegerParameter(
                ProjectPreferencesHelp.ZoneActiveHeightLimitsName,
                ProjectPreferencesHelp.ZoneActiveHeightLimitsName,
                ProjectPreferencesHelp.ZoneActiveHeightLimitsDescription.WithTypeName("integerItem"),
                GH_ParamAccess.item);

            InNumbers(
                ProjectPreferencesHelp.ZoneHeightLimitsName,
                ProjectPreferencesHelp.ZoneHeightLimitsDescription +
                " One to three items; the given items change the limits of the same index.");

            InNumbers(
                ProjectPreferencesHelp.ZoneReductionPercentsName,
                ProjectPreferencesHelp.ZoneReductionPercentsDescription +
                " One to three items; the given items change the limits of the same index.");

            InNumberItem(
                ProjectPreferencesHelp.ZoneReductionPercentAboveLimitsName,
                ProjectPreferencesHelp.ZoneReductionPercentAboveLimitsDescription);

            SetOptionality(
                Enumerable.Range(0, Params.Input.Count).ToArray());
        }

        private void InNumberItem(
            string name,
            string description)
        {
            inManager.AddNumberParameter(
                name,
                name,
                description.WithTypeName("numberItem"),
                GH_ParamAccess.item);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Pages",
                "Name of each page that was sent, as the command answers them: workingUnits, " +
                "calculationUnits, dimensions or zones.");

            OutErrorMessages(
                "Error message of each page (empty when the page was set).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            var json = da.GetOptional<string>(
                0,
                null);
            if (!string.IsNullOrWhiteSpace(json))
            {
                try
                {
                    parameters = JObject.Parse(json);
                }
                catch (Exception ex)
                {
                    this.AddError(
                        $"Invalid JSON in the JsonPreferences input: {ex.Message}");
                    return;
                }
            }

            var index = 1;
            foreach (var table in ProjectPreferencesHelp.Tables)
            {
                if (!table.TryApplyInputs(
                        this,
                        da,
                        index,
                        parameters))
                {
                    return;
                }
                index += table.ParamCount;
            }

            if (!ProjectPreferencesHelp.TryApplyZoneInputs(
                    this,
                    da,
                    index,
                    parameters))
            {
                return;
            }

            if (parameters.Count == 0)
            {
                this.AddError(
                    "No preference given: connect JsonPreferences or the inputs of at least one page.");
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

            // The command answers one execution result per page, by name: a
            // page that could not be set is reported there instead of failing
            // the call.
            var pages = new List<string>();
            var errorMessages = new List<string>();
            foreach (var page in response.Properties())
            {
                var succeeded = (bool?)page.Value["success"] == true;
                var errorMessage = succeeded
                    ? ""
                    : page.Value["error"]?["message"]?.ToString() ?? "Unknown error.";
                pages.Add(page.Name);
                errorMessages.Add(errorMessage);
                if (!succeeded)
                {
                    this.AddError(
                        $"{page.Name}: {errorMessage}");
                }
            }

            da.SetDataList(
                0,
                pages);

            da.SetDataList(
                1,
                errorMessages);
        }

        // The project settings glyph with the pencil until
        // IconSource/SetProjectPreferences.svg is rasterized into Resources.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyElectricalProjectSettings;

        public override Guid ComponentGuid =>
            new Guid("52e55e02-0793-41c5-a644-39224a151b8f");
    }
}
