using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetOpeningDefaultComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetOpeningDefault";

        public GetOpeningDefaultComponent()
            : base(
                "GetOpeningDefault",
                "Returns the settings of the Opening tool defaults: the extrusion parameters (as " +
                "GetOpeningDetails returns them) and the floor plan parameters (as GetOpeningFloorPlanParameters " +
                "returns them), each as a JSON object. Available from Archicad 29.",
                GroupNames.ElementDetails)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                "ExtrusionParameters",
                "The size and extrusion settings of the Opening tool defaults, as a JSON object.");

            OutText(
                "FloorPlanParameters",
                "The floor plan settings of the Opening tool defaults, as a JSON object.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out var response))
            {
                return;
            }

            da.SetData(
                0,
                OpeningSettingsHelp.ToText(response["extrusionParameters"]));
            da.SetData(
                1,
                OpeningSettingsHelp.ToText(response["floorPlanParameters"]));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetOpeningDefault;

        public override Guid ComponentGuid =>
            new Guid("3f136597-a32a-44c8-9948-223c58bff3af");
    }
}
