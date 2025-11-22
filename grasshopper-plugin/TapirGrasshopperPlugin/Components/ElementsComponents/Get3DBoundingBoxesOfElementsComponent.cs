using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class Get3DBoundingBoxesOfElementsComponent
        : ArchicadAccessorComponent
    {
        public Get3DBoundingBoxesOfElementsComponent()
            : base(
                "Elem 3D Bounding Boxes",
                "Get 3D bounding boxes of elements.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Elements Guids to get the bounding box of.");
        }

        protected override void AddOutputs()
        {
            OutBoxes(
                "BoundingBoxes",
                "Bounding boxes.");
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