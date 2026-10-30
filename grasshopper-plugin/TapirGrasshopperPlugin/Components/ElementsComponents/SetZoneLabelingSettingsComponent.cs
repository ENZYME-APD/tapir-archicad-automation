using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class SetZoneLabelingSettingsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetZoneLabelingSettings";

        public SetZoneLabelingSettingsComponent()
            : base(
                "SetZoneLabelingSettings",
                "Turns the automatic labeling of Zones on or off and selects the Label Favorite it places. " +
                "Only the given settings are changed. Available from Archicad 28.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            inManager.AddBooleanParameter(
                "AutomaticLabeling",
                "AutomaticLabeling",
                "Turns the automatic labeling of Zones (placing a Label on each new Zone) on or off. Optional."
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            InText(
                "FavoriteName",
                "Name of the Label Favorite placed on each new Zone by the automatic labeling. Optional.");

            SetOptionality(new[] { 0, 1 });
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            var automaticLabeling = false;
            if (da.GetData(0, ref automaticLabeling))
            {
                parameters["automaticLabeling"] = automaticLabeling;
            }

            string favoriteName = null;
            if (da.GetData(1, ref favoriteName) &&
                !string.IsNullOrEmpty(favoriteName))
            {
                parameters["favoriteName"] = favoriteName;
            }

            if (parameters.Count == 0)
            {
                this.AddError("Give AutomaticLabeling, FavoriteName or both.");
                return;
            }

            if (TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out var response))
            {
                this.CheckExecutionResult(response);
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetZoneLabelingSettings;

        public override Guid ComponentGuid =>
            new Guid("23ff0471-58bd-4e21-9483-9e91c2be2a58");
    }
}
