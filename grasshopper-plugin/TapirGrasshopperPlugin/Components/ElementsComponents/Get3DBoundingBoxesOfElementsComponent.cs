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
            if (!ElementsObj.TryCreate(
                    this,
                    da,
                    0,
                    out ElementsObj elements))
            {
                return;
            }

            if (!TryGetConvertedResponse(
                    CommandName,
                    elements,
                    out BoundingBoxes3DObj response))
            {
                return;
            }

            da.SetDataList(
                0,
                response.BoundingBoxes3D.Select(x => x.ToRhino()));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.Elems3DBoxes;

        public override Guid ComponentGuid =>
            new Guid("2044841d-a1af-40c5-ab9d-291187261d69");

        public override string CommandName => "Get3DBoundingBoxes";
    }
}