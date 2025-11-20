using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetSelectedElementsComponent : ArchicadAccessorComponent
    {
        public static string Doc => "Get currently selected elements.";
        public override string CommandName => "GetSelectedElements";

        public GetSelectedElementsComponent()
            : base(
                "Selected Elems",
                "SelectedElems",
                Doc,
                GroupNames.Elements)
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Currently selected element Guids.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!GetConvertedResponse(
                    CommandName,
                    out ElementsObj elements))
            {
                return;
            }

            da.SetDataList(
                0,
                elements.Elements);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SelectedElems;

        public override Guid ComponentGuid =>
            new Guid("1949E4B5-4E37-4F35-8C5C-BEA7575AC1C6");
    }
}