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
        public static string Doc => "Get GDL parameter values of elements.";
        public override string CommandName => "GetGDLParametersOfElements";

        public GetGDLParametersOfElementsComponent()
            : base(
                "Elem GDL Parameters",
                "Elem GDL Parameters",
                Doc,
                GroupNames.Elements)
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter(
                "ElementGuids",
                "ElementGuids",
                "Elements Guids to get detailList for.",
                GH_ParamAccess.list);
            pManager.AddTextParameter(
                "ParamName",
                "ParamName",
                "Parameter name to find.",
                GH_ParamAccess.item);
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddTextParameter(
                "ElementGuids",
                "ElementGuids",
                "List of element Guids with valid GDL detailList.",
                GH_ParamAccess.list);
            pManager.AddTextParameter(
                "ParamValues",
                "ParamValues",
                "Values of the found GDL detailList.",
                GH_ParamAccess.list);
            pManager.AddTextParameter(
                "JsonValues",
                "JsonValues",
                "...",
                GH_ParamAccess.list);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            ElementsObj inputElements = ElementsObj.Create(
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

            // new from here

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

            // new to here

            //List<ElementIdItemObj> validElementIds =
            //    new List<ElementIdItemObj>();
            //List<string> paramValues = new List<string>();

            //for (int i = 0; i < response.GdlLists.Count; i++)
            //{
            //    GdlParameterList paramList = response.GdlLists[i];

            //    if (paramList == null || paramList.GdlParameterArray == null ||
            //        paramList.GdlParameterArray.Count == 0)
            //    {
            //        continue;
            //    }

            //    string paramValue = null;

            //    foreach (GdlParameterDetails details in paramList
            //                 .GdlParameterArray)
            //    {
            //        if (details.Name.ToLower() == paramName)
            //        {
            //            paramValue = details.Value.ToString();
            //            break;
            //        }
            //    }

            //    if (paramValue == null)
            //    {
            //        continue;
            //    }

            //    validElementIds.Add(inputElements.Elements[i]);
            //    paramValues.Add(paramValue);
            //}

            //da.SetDataList(
            //    0,
            //    validElementIds);
            //da.SetDataList(
            //    1,
            //    paramValues);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ElemGDLParameters;

        public override Guid ComponentGuid =>
            new Guid("c963f9b4-5e98-4c8a-8ed9-f2d67000a6e8");
    }
}