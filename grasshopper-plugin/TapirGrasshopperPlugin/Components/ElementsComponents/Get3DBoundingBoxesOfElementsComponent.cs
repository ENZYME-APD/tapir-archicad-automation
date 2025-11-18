using Grasshopper.Kernel;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class Get3DBoundingBoxesOfElementsComponent
        : ArchicadAccessorComponent
    {
        public static string Doc => "Get 3D bounding boxes of elements.";

        public Get3DBoundingBoxesOfElementsComponent()
            : base(
                "Elem 3D Bounding Boxes",
                "Elem3DBoxes",
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
                "Elements Guids to get the bounding box of.",
                GH_ParamAccess.list);
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddBoxParameter(
                "BoundingBoxes",
                "BoundingBoxes",
                "Bounding boxes.",
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

            if (!GetConvertedResponse(
                    CommandName,
                    inputElements,
                    out BoundingBoxes3DObj boxObject))
            {
                return;
            }

            da.SetDataList(
                0,
                boxObject.BoundingBoxes3D.Select(x => x.ToRhino()));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.Elems3DBoxes;

        public override Guid ComponentGuid =>
            new Guid("2044841d-a1af-40c5-ab9d-291187261d69");

        public override string CommandName => "Get3DBoundingBoxes";
    }
}