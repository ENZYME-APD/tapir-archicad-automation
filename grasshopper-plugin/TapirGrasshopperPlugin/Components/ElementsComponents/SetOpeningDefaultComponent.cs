using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class SetOpeningDefaultComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetOpeningDefault";

        public SetOpeningDefaultComponent()
            : base(
                "SetOpeningDefault",
                "Changes the settings of the Opening tool defaults, with the names GetOpeningDefault returns. " +
                "Only the given settings change. Available from Archicad 29.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "ExtrusionParameters",
                OpeningSettingsHelp.ExtrusionParametersDescription + " Optional.");

            InText(
                "FloorPlanParameters",
                OpeningSettingsHelp.FloorPlanParametersDescription + " Optional.");

            SetOptionality(new[] { 0, 1 });
        }

        protected override void AddOutputs()
        {
            OutText(
                "ElementType",
                "The type of the elements the changed settings are the defaults of.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!OpeningSettingsHelp.TryParse(
                    this,
                    da.GetOptional<string>(0, null),
                    "ExtrusionParameters",
                    OpeningSettingsHelp.ExtrusionKeys,
                    out var extrusionParameters) ||
                !OpeningSettingsHelp.TryParse(
                    this,
                    da.GetOptional<string>(1, null),
                    "FloorPlanParameters",
                    OpeningSettingsHelp.FloorPlanKeys,
                    out var floorPlanParameters))
            {
                return;
            }

            var parameters = new JObject();
            if (extrusionParameters != null)
            {
                parameters["extrusionParameters"] = extrusionParameters;
            }

            if (floorPlanParameters != null)
            {
                parameters["floorPlanParameters"] = floorPlanParameters;
            }

            if (parameters.Count == 0)
            {
                this.AddError("Give ExtrusionParameters, FloorPlanParameters or both.");
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out var response))
            {
                return;
            }

            da.SetData(
                0,
                response["elementType"]?.ToString());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetOpeningDefault;

        public override Guid ComponentGuid =>
            new Guid("7e930ff6-f001-4b31-b695-cf11d914d23e");
    }
}
