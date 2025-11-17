using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetGDLParametersOfElementsComponent : ArchicadAccessorComponent
    {
        public static string CommandName => "GetGDLParametersOfElements";

        public GetGDLParametersOfElementsComponent()
            : base(
                "Elem GDL Parameters",
                "ElemGDLParameters",
                "Get GDL parameter values of elements.",
                "Elements")
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter(
                "ElementGuids",
                "ElementGuids",
                "Element Guids to get parameters for.",
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
            pManager.AddGenericParameter(
                "ElementGuids",
                "ElementGuids",
                "List of element Guids with valid GDL parameters.",
                GH_ParamAccess.list);
            pManager.AddTextParameter(
                "ParamValues",
                "ParamValues",
                "Values of the found GDL parameters.",
                GH_ParamAccess.list);
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
                    out string paramName))
            {
                return;
            }

            paramName = paramName.ToLower();

            var inputElementsObj = JObject.FromObject(inputElements);

            if (!GetResponse(
                    CommandName,
                    inputElementsObj,
                    out GDLParametersResponse elemGdlParameters))
            {
                return;
            }

            var validElementIds = new List<ElementIdItemObj>();
            var paramValues = new List<string>();

            for (var i = 0;
                 i < elemGdlParameters.GdlParametersOfElements.Count;
                 i++)
            {
                GDLParameterList paramList =
                    elemGdlParameters.GdlParametersOfElements[i];

                if (paramList == null || paramList.Parameters == null ||
                    paramList.Parameters.Count == 0)
                {
                    continue;
                }

                string paramValue = null;

                foreach (var details in paramList.Parameters)
                {
                    if (details.Name.ToLower() == paramName)
                    {
                        paramValue = details.Value.ToString();
                        break;
                    }
                }

                if (paramValue == null)
                {
                    continue;
                }

                validElementIds.Add(inputElements.Elements[i]);
                paramValues.Add(paramValue);
            }

            da.SetDataList(
                0,
                validElementIds);
            da.SetDataList(
                1,
                paramValues);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ElemGDLParameters;

        public override Guid ComponentGuid =>
            new Guid("c963f9b4-5e98-4c8a-8ed9-f2d67000a6e8");
    }
}