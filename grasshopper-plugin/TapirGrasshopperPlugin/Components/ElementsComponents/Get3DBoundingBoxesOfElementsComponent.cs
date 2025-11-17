using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class Get3DBoundingBoxesOfElementsComponent
        : ArchicadAccessorComponent
    {
        public Get3DBoundingBoxesOfElementsComponent()
            : base(
                "Elem 3D Bounding Boxes",
                "Elem3DBoxes",
                "Get 3D bounding boxes of elements.",
                "Elements")
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter(
                "ElementGuids",
                "ElementGuids",
                "Element Guids to get the bounding box of.",
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
            IGH_DataAccess DA)
        {
            var inputElements = ElementsObj.Create(
                DA,
                0);
            if (inputElements == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input ElementGuids failed to collect data.");
                return;
            }

            var inputElementsObj = JObject.FromObject(inputElements);
            var response = SendArchicadAddOnCommand(
                "Get3DBoundingBoxes",
                inputElementsObj);
            if (!response.Succeeded)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    response.GetErrorMessage());
                return;
            }

            var boundingBoxes = response.Result.ToObject<BoundingBoxes3DObj>();
            var boundingBoxesValue = new List<BoundingBox>();
            foreach (var boundingBox in boundingBoxes.BoundingBoxes3D)
            {
                var box = new BoundingBox()
                {
                    Min = new Point3d(
                        boundingBox.BoundingBox3D.XMin,
                        boundingBox.BoundingBox3D.YMin,
                        boundingBox.BoundingBox3D.ZMin),
                    Max = new Point3d(
                        boundingBox.BoundingBox3D.XMax,
                        boundingBox.BoundingBox3D.YMax,
                        boundingBox.BoundingBox3D.ZMax)
                };
                boundingBoxesValue.Add(box);
            }

            DA.SetDataList(
                0,
                boundingBoxesValue);
        }

        protected override System.Drawing.Bitmap Icon =>
            TapirGrasshopperPlugin.Properties.Resources.Elems3DBoxes;

        public override Guid ComponentGuid =>
            new Guid("2044841d-a1af-40c5-ab9d-291187261d69");
    }
}