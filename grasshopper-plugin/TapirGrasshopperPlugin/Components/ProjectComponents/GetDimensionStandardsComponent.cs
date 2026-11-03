using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetDimensionStandardsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetDimensionStandards";

        public GetDimensionStandardsComponent()
            : base(
                "GetDimensionStandards",
                "Get the dimension standards of the project (Options > Project Preferences > Dimensions). " +
                "Available from Archicad 27.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Name",
                "Only the dimension standard with this name is returned (case-sensitive). Optional.");

            SetOptionality(new[] { 0 });
        }

        protected override void AddOutputs()
        {
            OutIntegers(
                "Indices",
                "The 1-based index of each dimension standard.");

            OutTexts(
                "Names",
                "Name of each dimension standard.");

            OutTexts(
                "LinearUnits",
                "Unit of the linear dimensions of each standard.");

            OutIntegers(
                "LinearDecimals",
                "Number of decimals of the linear dimensions of each standard.");

            OutTexts(
                "AngleUnits",
                "Unit of the angle dimensions of each standard.");

            OutIntegers(
                "AngleDecimals",
                "Number of decimals of the angle dimensions of each standard.");

            OutTexts(
                "AreaUnits",
                "Unit of the area calculations of each standard.");

            OutIntegers(
                "AreaDecimals",
                "Number of decimals of the area calculations of each standard.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            string name = null;
            if (da.GetData(0, ref name) && !string.IsNullOrEmpty(name))
            {
                parameters["name"] = name;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var indices = new List<int>();
            var names = new List<string>();
            var linearUnits = new List<string>();
            var linearDecimals = new List<int>();
            var angleUnits = new List<string>();
            var angleDecimals = new List<int>();
            var areaUnits = new List<string>();
            var areaDecimals = new List<int>();

            foreach (var item in JsonOutputHelp.Items(response, "dimensionStandards"))
            {
                var settings = item["settings"];
                indices.Add((int?)item["index"] ?? 0);
                names.Add(item["name"]?.ToString() ?? "");
                linearUnits.Add(settings?["linear"]?["unit"]?.ToString() ?? "");
                linearDecimals.Add((int?)settings?["linear"]?["decimals"] ?? 0);
                angleUnits.Add(settings?["angle"]?["unit"]?.ToString() ?? "");
                angleDecimals.Add((int?)settings?["angle"]?["decimals"] ?? 0);
                areaUnits.Add(settings?["area"]?["unit"]?.ToString() ?? "");
                areaDecimals.Add((int?)settings?["area"]?["decimals"] ?? 0);
            }

            da.SetDataList(0, indices);
            da.SetDataList(1, names);
            da.SetDataList(2, linearUnits);
            da.SetDataList(3, linearDecimals);
            da.SetDataList(4, angleUnits);
            da.SetDataList(5, angleDecimals);
            da.SetDataList(6, areaUnits);
            da.SetDataList(7, areaDecimals);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetCalculationUnits;

        public override Guid ComponentGuid =>
            new Guid("b7e3d1a4-5c2f-4e8b-9a61-3f0d2c7e94b5");
    }
}
