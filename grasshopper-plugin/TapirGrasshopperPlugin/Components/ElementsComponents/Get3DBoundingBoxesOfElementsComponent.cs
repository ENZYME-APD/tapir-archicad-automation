﻿using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class Get3DBoundingBoxesOfElementsComponent : ArchicadAccessorComponent
    {
        public Get3DBoundingBoxesOfElementsComponent ()
          : base (
                "Elem 3D Bounding Boxes",
                "Elem3DBoxes",
                "Get 3D bounding boxes of elements.",
                "Elements"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("ElementIds", "ElementIds", "Element ids to get the bounding box of.", GH_ParamAccess.list);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddBoxParameter ("BoundingBoxes", "BoundingBoxes", "Bounding boxes.", GH_ParamAccess.list);
        }

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            ElementsObj inputElements = ElementsObj.Create (DA, 0);
            if (inputElements == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input ElementIds failed to collect data.");
                return;
            }

            JObject inputElementsObj = JObject.FromObject (inputElements);
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "Get3DBoundingBoxes", inputElementsObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }

            BoundingBoxes3DObj boundingBoxes = response.Result.ToObject<BoundingBoxes3DObj> ();
            List<BoundingBox> boundingBoxesValue = new List<BoundingBox> ();
            foreach (BoundingBox3DObj boundingBox in boundingBoxes.BoundingBoxes3D) {
                BoundingBox box = new BoundingBox () {
                    Min = new Point3d (boundingBox.BoundingBox3D.XMin, boundingBox.BoundingBox3D.YMin, boundingBox.BoundingBox3D.ZMin),
                    Max = new Point3d (boundingBox.BoundingBox3D.XMax, boundingBox.BoundingBox3D.YMax, boundingBox.BoundingBox3D.ZMax)
                };
                boundingBoxesValue.Add (box);
            }
            DA.SetDataList (0, boundingBoxesValue);
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.Elems3DBoxes;

        public override Guid ComponentGuid => new Guid ("2044841d-a1af-40c5-ab9d-291187261d69");
    }
}