using Grasshopper.Kernel;
using Newtonsoft.Json;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetGDLParametersOfElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetGDLParametersOfElements";

        public GetGDLParametersOfElementsComponent()
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
                "Elements Guids to get detailList for.");

            InText(
                "ParamName",
                "Parameter name to find.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "List of element Guids with valid GDL detailList.");

            OutTexts(
                "ParamValues",
                "Values of the found GDL detailList.");

            OutTexts(
                "JsonValues",
                "...");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var inputElements = ElementsObj.Create(
                da,
                0);

            if (inputElements == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input ElementGuids failed to collect data.");
                return;
            }

            if (!da.GetItem(
                    1,
                    out string parameterName))
            {
                return;
            }

            if (!GetConvertedResponse(
                    CommandName,
                    inputElements,
                    out GDLParametersResponse response))
            {
                return;
            }

            var gdlHolders = response.ToGdlHolders(
                inputElements.Elements.Select(x => x.ElementId.Guid).ToList(),
                parameterName.ToLower());

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