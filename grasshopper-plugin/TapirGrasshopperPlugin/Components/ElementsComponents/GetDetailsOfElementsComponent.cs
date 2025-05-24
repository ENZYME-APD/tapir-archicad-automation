using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Rhino.Collections;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class Point2D
    {
        public static Point2D Create (object obj)
        {
            if (obj is Point2D) {
                return obj as Point2D;
            } else if (obj is Point2d) {
                Point2d point2D = (Point2d) obj;
                return new Point2D () {
                    X = point2D.X,
                    Y = point2D.Y
                };
            } else if (obj is Point3d) {
                Point3d point3D = (Point3d) obj;
                return new Point2D () {
                    X = point3D.X,
                    Y = point3D.Y
                };
            } else {
                return null;
            }
        }

        [JsonProperty ("x")]
        public double X;

        [JsonProperty ("y")]
        public double Y;
    }

    public class WallDetails
    {
        [JsonProperty ("geometryType")]
        public string GeometryType;

        [JsonProperty ("zCoordinate")]
        public double ZCoordinate;

        [JsonProperty ("begCoordinate")]
        public Point2D BegCoordinate;

        [JsonProperty ("endCoordinate")]
        public Point2D EndCoordinate;

        [JsonProperty ("height")]
        public double Height;
    }

    public class ColumnDetails
    {
        [JsonProperty ("origin")]
        public Point2D OrigoCoordinate;
    }

    public class HoleDetails
    {
        [JsonProperty ("polygonOutline")]
        public List<Point2D> PolygonCoordinates;
    }

    public class SlabDetails
    {
        [JsonProperty ("zCoordinate")]
        public double ZCoordinate;

        [JsonProperty ("polygonOutline")]
        public List<Point2D> PolygonCoordinates;

        [JsonProperty ("holes")]
        public List<HoleDetails> Holes;
    }

    public class PolylineDetails
    {
        [JsonProperty ("coordinates")]
        public List<Point2D> Coordinates;
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
            pManager.AddNumberParameter ("Height", "Height", "Height.", GH_ParamAccess.list);
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
            List<Point3d> begCoords = new List<Point3d> ();
            List<Point3d> endCoords = new List<Point3d> ();
            List<double> heights = new List<double> ();
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
                begCoords.Add (new Point3d (wallDetails.BegCoordinate.X, wallDetails.BegCoordinate.Y, wallDetails.ZCoordinate));
                endCoords.Add (new Point3d (wallDetails.EndCoordinate.X, wallDetails.EndCoordinate.Y, wallDetails.ZCoordinate));
                heights.Add (wallDetails.Height);
            }

            DA.SetDataList (0, walls);
            DA.SetDataList (1, begCoords);
            DA.SetDataList (2, endCoords);
            DA.SetDataList (3, heights);
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.WallDetails;

        public override Guid ComponentGuid => new Guid ("2b7b8e37-b293-475f-a333-d6afe4c5ffff");
    }

    public class GetDetailsOfColumnsComponent : ArchicadAccessorComponent
    {
        public GetDetailsOfColumnsComponent ()
          : base (
                "Column Details",
                "ColumnDetails",
                "Get details of column elements.",
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
            pManager.AddGenericParameter ("ColumnGuids", "ColumnGuids", "Element Guids of the found columns.", GH_ParamAccess.list);
            pManager.AddPointParameter ("Origo coordinate", "OrigoCoord", "Origo coordinate.", GH_ParamAccess.list);
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

            List<ElementIdItemObj> columns = new List<ElementIdItemObj> ();
            List<Point2d> origoCoords = new List<Point2d> ();
            DetailsOfElementsObj detailsOfElements = response.Result.ToObject<DetailsOfElementsObj> ();
            for (int i = 0; i < detailsOfElements.DetailsOfElements.Count; i++) {
                DetailsOfElementObj detailsOfElement = detailsOfElements.DetailsOfElements[i];
                if (detailsOfElement.Type != "Column") {
                    continue;
                }
                ColumnDetails columnDetails = detailsOfElement.Details.ToObject<ColumnDetails> ();
                if (columnDetails == null) {
                    continue;
                }
                columns.Add (new ElementIdItemObj () {
                    ElementId = inputElements.Elements[i].ElementId
                });
                origoCoords.Add (new Point2d (columnDetails.OrigoCoordinate.X, columnDetails.OrigoCoordinate.Y));
            }

            DA.SetDataList (0, columns);
            DA.SetDataList (1, origoCoords);
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.ColumnDetails;

        public override Guid ComponentGuid => new Guid ("ded49694-9869-4670-af85-645535a7be6a");
    }

    public class GetDetailsOfSlabsComponent : ArchicadAccessorComponent
    {
        public GetDetailsOfSlabsComponent ()
          : base (
                "Slab Details",
                "SlabDetails",
                "Get details of slab elements.",
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
            pManager.AddGenericParameter ("SlabGuids", "SlabGuids", "Element Guids of the found slabs.", GH_ParamAccess.list);
            pManager.AddCurveParameter ("Polygon outlines", "Polygons", "The outline polygons of the slabs.", GH_ParamAccess.list);
            pManager.AddCurveParameter ("Holes", "HolePolygons", "The outline polygons of the holes in the slabs.", GH_ParamAccess.tree);
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

            List<ElementIdItemObj> slabs = new List<ElementIdItemObj> ();
            List<Polyline> polygons = new List<Polyline> ();
            DataTree<Polyline> holePolygonsTree = new DataTree<Polyline> ();
            DetailsOfElementsObj detailsOfElements = response.Result.ToObject<DetailsOfElementsObj> ();
            for (int i = 0; i < detailsOfElements.DetailsOfElements.Count; i++) {
                DetailsOfElementObj detailsOfElement = detailsOfElements.DetailsOfElements[i];
                if (detailsOfElement.Type != "Slab") {
                    continue;
                }
                SlabDetails slabDetails = detailsOfElement.Details.ToObject<SlabDetails> ();
                if (slabDetails == null) {
                    continue;
                }
                slabs.Add (new ElementIdItemObj () {
                    ElementId = inputElements.Elements[i].ElementId
                });
                polygons.Add (Utilities.Convert.ToPolygon (slabDetails.PolygonCoordinates, slabDetails.ZCoordinate));

                List<Polyline> holePolygons = new List<Polyline> ();
                foreach (HoleDetails holeDetail in slabDetails.Holes) {
                    holePolygons.Add (Utilities.Convert.ToPolygon (holeDetail.PolygonCoordinates, slabDetails.ZCoordinate));
                }
                holePolygonsTree.AddRange (holePolygons, new GH_Path (slabs.Count));
            }

            DA.SetDataList (0, slabs);
            DA.SetDataList (1, polygons);
            DA.SetDataTree (2, holePolygonsTree);
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.SlabDetails;

        public override Guid ComponentGuid => new Guid ("f942eece-cc80-4945-a911-fe548dae4ae8");
    }

    public class GetDetailsOfPolylinesComponent : ArchicadAccessorComponent
    {
        public GetDetailsOfPolylinesComponent ()
          : base (
                "Polyline Details",
                "PolylineDetails",
                "Get details of polyline elements.",
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
            pManager.AddGenericParameter ("PolylineGuids", "PolylineGuids", "Element Guids of the found polylines.", GH_ParamAccess.list);
            pManager.AddCurveParameter ("Coordinates", "Coordinates", "The coordinates of the polylines.", GH_ParamAccess.list);
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

            List<ElementIdItemObj> polylines = new List<ElementIdItemObj> ();
            List<Polyline> rhinoPolylines = new List<Polyline> ();
            DetailsOfElementsObj detailsOfElements = response.Result.ToObject<DetailsOfElementsObj> ();
            for (int i = 0; i < detailsOfElements.DetailsOfElements.Count; i++) {
                DetailsOfElementObj detailsOfElement = detailsOfElements.DetailsOfElements[i];
                if (detailsOfElement.Type != "PolyLine") {
                    continue;
                }
                PolylineDetails polylineDetails = detailsOfElement.Details.ToObject<PolylineDetails> ();
                if (polylineDetails == null) {
                    continue;
                }
                polylines.Add (new ElementIdItemObj () {
                    ElementId = inputElements.Elements[i].ElementId
                });
                rhinoPolylines.Add (Utilities.Convert.ToPolyline (polylineDetails.Coordinates));
            }

            DA.SetDataList (0, polylines);
            DA.SetDataList (1, rhinoPolylines);
        }

        // protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.PolylineDetails;

        public override Guid ComponentGuid => new Guid ("b96c3b7e-303d-44f2-af22-6fd07ade11fc");
    }
}
