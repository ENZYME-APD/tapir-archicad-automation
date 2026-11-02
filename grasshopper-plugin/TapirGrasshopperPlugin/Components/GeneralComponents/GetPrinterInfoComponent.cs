using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class GetPrinterInfoComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetPrinterInfo";

        public GetPrinterInfoComponent()
            : base(
                "GetPrinterInfo",
                "Retrieves Archicad's current print settings: the print area, grid, fixed text size and scale (the settings PrintView takes).",
                GroupNames.General)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                "PrintArea",
                "The area Archicad prints: currentView, entireDrawing, marquee or unknown.");

            OutBoolean(
                "Grid",
                "Whether the grid is printed.");

            OutBoolean(
                "FixText",
                "Whether text is printed in fixed size.");

            OutInteger(
                "Scale",
                "The print scale.");
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

            da.SetData(
                0,
                response["printArea"]?.ToString());

            da.SetData(
                1,
                (bool?)response["grid"]);

            da.SetData(
                2,
                (bool?)response["fixText"]);

            da.SetData(
                3,
                (int?)response["scale"]);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetPrinterInfo;

        public override Guid ComponentGuid =>
            new Guid("597d7370-93bc-4010-a23f-9cc9156833d3");
    }
}
