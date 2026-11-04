using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class FindDimensionStandardComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "FindDimensionStandard";

        public FindDimensionStandardComponent()
            : base(
                "FindDimensionStandard",
                "Finds a dimension standard (Options > Project Preferences > Dimensions) by its name and " +
                "returns its index, name and guid. Available from Archicad 27.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Name",
                "The name of the dimension standard, as listed in Options > Project Preferences > Dimensions.");
        }

        protected override void AddOutputs()
        {
            OutInteger(
                "Index",
                "The index of the dimension standard.");

            OutText(
                "Name",
                "The name of the dimension standard.");

            OutText(
                "DimensionStandardGuid",
                "The guid of the dimension standard.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string name) ||
                string.IsNullOrEmpty(name))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["name"] = name },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(
                0,
                response.Value<int?>("index"));

            da.SetData(
                1,
                response["name"]?.ToString());

            da.SetData(
                2,
                JsonOutputHelp.GuidOf(response["dimensionStandardId"]));
        }

        // Shares the icon of GetDimensionStandards (GetCalculationUnits) until it gets its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetCalculationUnits;

        public override Guid ComponentGuid =>
            new Guid("2584ea3f-f039-44dc-92c9-ab375554eb8f");
    }
}
