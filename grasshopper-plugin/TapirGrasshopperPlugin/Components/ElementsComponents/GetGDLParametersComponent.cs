using Grasshopper.Kernel;
using Newtonsoft.Json;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetGDLParametersComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetGDLParametersOfElements";

        public GetGDLParametersComponent()
            : base(
                "ElementGDLParameters",
                "Get GDL parameter values of elements.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Elements Guids to get detail list for.");

            InText("ParameterName");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "List of element Guids with valid GDL detail list.");

            OutTexts(
                "ParameterValues",
                "Values of the found GDL detail list.");

            OutTexts(
                nameof(GdlParameterDetails),
                "JSON dictionary of the retrieved parameters.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject inputs))
            {
                return;
            }

            if (!da.TryGet(
                    1,
                    out string parameterName))
            {
                return;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    inputs,
                    ToAddOn,
                    JHelp.Deserialize<GDLParametersResponse>,
                    out GDLParametersResponse response))
            {
                return;
            }

            var gdlHolders = response.ToGdlHolders(
                inputs.Elements.Select(x => x.ElementId).ToList(),
                parameterName);

            da.SetDataList(
                0,
                gdlHolders.Select(x => x.ElementId));

            da.SetDataList(
                1,
                gdlHolders.Select(x => x.GdlParameterDetails.Value));

            da.SetDataList(
                2,
                gdlHolders.Select(x => JsonConvert.SerializeObject(
                    x.GdlParameterDetails,
                    Formatting.Indented)));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ElemGDLParameters;

        public override Guid ComponentGuid =>
            new Guid("c963f9b4-5e98-4c8a-8ed9-f2d67000a6e8");
    }
}