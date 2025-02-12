using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class Point2D
    {
        [JsonProperty ("x")]
        public double X;

        [JsonProperty ("y")]
        public double Y;
    }

    public class WallDetails
    {
        [JsonProperty ("geometryType")]
        public string GeometryType;

        [JsonProperty ("begCoordinate")]
        public Point2D BegCoordinate;

        [JsonProperty ("endCoordinate")]
        public Point2D EndCoordinate;
    }

    public class GetDetailsOfElementsComponent : ArchicadAccessorComponent
    {
        public GetDetailsOfElementsComponent ()
          : base (
                "Elem Details",
                "ElemDetails",
                "Get details of elements.",
                "Elements"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("ElementGuids", "ElementGuids", "Element Guids to get details of.", GH_ParamAccess.list);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("ElementGuids", "ElementGuids", "Element Guids of the found elements.", GH_ParamAccess.list);
            pManager.AddTextParameter ("ElemType", "ElemType", "Element type.", GH_ParamAccess.list);
            pManager.AddTextParameter ("ElementID", "ElementID", "ElementID property values.", GH_ParamAccess.list);
            pManager.AddIntegerParameter ("StoryIndex", "StoryIndex", "Story index.", GH_ParamAccess.list);
            pManager.AddIntegerParameter ("LayerIndex", "LayerIndex", "Layer index.", GH_ParamAccess.list);
            pManager.AddIntegerParameter ("DrawOrder", "DrawOrder", "Drawing order.", GH_ParamAccess.list);
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            ElementsObj inputElements = ElementsObj.Create (DA, 0);
            if (inputElements == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input ElementGuids failed to collect data.");
                return;
            }

            JObject inputElementsObj = JObject.FromObject (inputElements);
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "GetDetailsOfElements", inputElementsObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }

            List<ElementIdItemObj> validElements = new List<ElementIdItemObj> ();
            List<String> elemTypes = new List<String> ();
            List<String> elementIDs = new List<String> ();
            List<int> storyIndices = new List<int> ();
            List<int> layerIndices = new List<int> ();
            List<int> drawIndices = new List<int> ();

            DetailsOfElementsObj detailsOfElements = response.Result.ToObject<DetailsOfElementsObj> ();
            for (int i = 0; i < detailsOfElements.DetailsOfElements.Count; i++) {
                DetailsOfElementObj detailsOfElement = detailsOfElements.DetailsOfElements[i];
                if (detailsOfElement == null) {
                    continue;
                }
                validElements.Add (new ElementIdItemObj () {
                    ElementId = inputElements.Elements[i].ElementId
                });
                elemTypes.Add (detailsOfElement.Type);
                elementIDs.Add (detailsOfElement.ElementID);
                storyIndices.Add (detailsOfElement.FloorIndex);
                layerIndices.Add (detailsOfElement.LayerIndex);
                drawIndices.Add (detailsOfElement.DrawIndex);
            }

            DA.SetDataList (0, validElements);
            DA.SetDataList (1, elemTypes);
            DA.SetDataList (2, elementIDs);
            DA.SetDataList (3, storyIndices);
            DA.SetDataList (4, layerIndices);
            DA.SetDataList (5, drawIndices);
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.ElemDetails;

        public override Guid ComponentGuid => new Guid ("d1509981-6510-4c09-8727-dba5981109f8");
    }

    public class GetDetailsOfWallsComponent : ArchicadAccessorComponent
    {
        public GetDetailsOfWallsComponent ()
          : base (
                "Wall Details",
                "WallDetails",
                "Get details of wall elements.",
                "Elements"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("ElementGuids", "ElementGuids", "Element Guids to get details of.", GH_ParamAccess.list);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("WallGuids", "WallGuids", "Element Guids of the found walls.", GH_ParamAccess.list);
            pManager.AddPointParameter ("Begin coordinate", "BegCoord", "Begin coordinate.", GH_ParamAccess.list);
            pManager.AddPointParameter ("End coordinate", "EndCoord", "End coordinate.", GH_ParamAccess.list);
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            ElementsObj inputElements = ElementsObj.Create (DA, 0);
            if (inputElements == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input ElementGuids failed to collect data.");
                return;
            }

            JObject inputElementsObj = JObject.FromObject (inputElements);
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "GetDetailsOfElements", inputElementsObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }

            List<ElementIdItemObj> walls = new List<ElementIdItemObj> ();
            List<Point2d> begCoords = new List<Point2d> ();
            List<Point2d> endCoords = new List<Point2d> ();
            DetailsOfElementsObj detailsOfElements = response.Result.ToObject<DetailsOfElementsObj> ();
            for (int i = 0; i < detailsOfElements.DetailsOfElements.Count; i++) {
                DetailsOfElementObj detailsOfElement = detailsOfElements.DetailsOfElements[i];
                if (detailsOfElement.Type != "Wall") {
                    continue;
                }
                WallDetails wallDetails = detailsOfElement.Details.ToObject<WallDetails> ();
                if (wallDetails == null) {
                    continue;
                }
                if (wallDetails.GeometryType != "Straight" && wallDetails.GeometryType != "Trapezoid") {
                    continue;
                }
                walls.Add (new ElementIdItemObj () {
                    ElementId = inputElements.Elements[i].ElementId
                });
                begCoords.Add (new Point2d (wallDetails.BegCoordinate.X, wallDetails.BegCoordinate.Y));
                endCoords.Add (new Point2d (wallDetails.EndCoordinate.X, wallDetails.EndCoordinate.Y));
            }

            DA.SetDataList (0, walls);
            DA.SetDataList (1, begCoords);
            DA.SetDataList (2, endCoords);
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.WallDetails;

        public override Guid ComponentGuid => new Guid ("2b7b8e37-b293-475f-a333-d6afe4c5ffff");
    }
}
