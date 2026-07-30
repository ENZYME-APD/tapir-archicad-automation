using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetCalculationUnitsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetCalculationUnits";

        public GetCalculationUnitsComponent()
            : base(
                "GetCalculationUnits",
                "Get the project calculation units (length, area, volume, angle).",
                GroupNames.Project)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                "LengthUnit",
                "Unit of length values.");

            OutText(
                "LengthAccuracy",
                "Accuracy of length values.");

            OutInteger(
                "LengthDecimals",
                "Number of decimals of length values.");

            OutInteger(
                "LengthRoundInch",
                "Round inch setting of length values.");

            OutText(
                "AreaUnit",
                "Unit of area values.");

            OutText(
                "AreaAccuracy",
                "Accuracy of area values.");

            OutInteger(
                "AreaDecimals",
                "Number of decimals of area values.");

            OutText(
                "VolumeUnit",
                "Unit of volume values.");

            OutText(
                "VolumeAccuracy",
                "Accuracy of volume values.");

            OutInteger(
                "VolumeDecimals",
                "Number of decimals of volume values.");

            OutText(
                "AngleUnit",
                "Unit of angle values.");

            OutInteger(
                "AngleAccuracy",
                "Accuracy of angle values.");

            OutInteger(
                "AngleDecimals",
                "Number of decimals of angle values.");
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

            var length = response["length"];
            var area = response["area"];
            var volume = response["volume"];
            var angle = response["angle"];

            da.SetData(0, JsonOutputHelp.Scalar(length, "unit"));
            da.SetData(1, JsonOutputHelp.Scalar(length, "accuracy"));
            da.SetData(2, JsonOutputHelp.Scalar(length, "decimals"));
            da.SetData(3, JsonOutputHelp.Scalar(length, "roundInch"));
            da.SetData(4, JsonOutputHelp.Scalar(area, "unit"));
            da.SetData(5, JsonOutputHelp.Scalar(area, "accuracy"));
            da.SetData(6, JsonOutputHelp.Scalar(area, "decimals"));
            da.SetData(7, JsonOutputHelp.Scalar(volume, "unit"));
            da.SetData(8, JsonOutputHelp.Scalar(volume, "accuracy"));
            da.SetData(9, JsonOutputHelp.Scalar(volume, "decimals"));
            da.SetData(10, JsonOutputHelp.Scalar(angle, "unit"));
            da.SetData(11, JsonOutputHelp.Scalar(angle, "accuracy"));
            da.SetData(12, JsonOutputHelp.Scalar(angle, "decimals"));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetCalculationUnits;

        public override Guid ComponentGuid =>
            new Guid("42b28eda-a438-4d2b-94de-0ff00882251f");
    }
}
