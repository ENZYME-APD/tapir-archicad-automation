using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class GetPrinterAndPlotterInfoComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetPrinterAndPlotterInfo";

        public GetPrinterAndPlotterInfoComponent()
            : base(
                "GetPrinterAndPlotterInfo",
                "Retrieves the printer selected in Archicad, with its name and orientation, and the selected plotter, with its name and model.",
                GroupNames.General)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                "PrinterName",
                "The name of the selected printer.");

            OutBoolean(
                "Portrait",
                "The orientation of the printer: true for portrait, false for landscape.");

            OutText(
                "PlotterName",
                "The name of the selected plotter.");

            OutText(
                "PlotterModel",
                "The model of the selected plotter.");

            OutText(
                "PrinterError",
                "The error message when the printer cannot be read (empty otherwise).");

            OutText(
                "PlotterError",
                "The error message when the plotter cannot be read (empty otherwise).");
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

            var printer = response["printer"];
            if (!JsonOutputHelp.IsError(printer))
            {
                da.SetData(
                    0,
                    printer?["name"]?.ToString());

                da.SetData(
                    1,
                    (bool?)printer?["portrait"]);
            }

            var plotter = response["plotter"];
            if (!JsonOutputHelp.IsError(plotter))
            {
                da.SetData(
                    2,
                    plotter?["name"]?.ToString());

                da.SetData(
                    3,
                    plotter?["model"]?.ToString());
            }

            da.SetData(
                4,
                JsonOutputHelp.ErrorMessage(printer));

            da.SetData(
                5,
                JsonOutputHelp.ErrorMessage(plotter));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetPrinterAndPlotterInfo;

        public override Guid ComponentGuid =>
            new Guid("78c8a440-1830-4408-9f9c-4c502efc5317");
    }
}
