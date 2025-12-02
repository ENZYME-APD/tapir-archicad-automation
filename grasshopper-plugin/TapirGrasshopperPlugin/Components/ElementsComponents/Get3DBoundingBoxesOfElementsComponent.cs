using Grasshopper.Kernel;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class Get3DBoundingBoxesOfElementsComponent
        : ArchicadAccessorComponent
    {
        public override string CommandName => "Get3DBoundingBoxes";

        public Get3DBoundingBoxesOfElementsComponent()
            : base(
                "Element3DBoundingBoxes",
                "Gets the 3D BoundingBoxes of Elements.",
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
            OutBoxes("BoundingBoxes");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return;
            }

            if (!TryGetConvertedValues(
                    CommandName,
                    elements,
                    ToAddOn,
                    JHelp.Deserialize<BoundingBoxes3DObj>,
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
    }
}