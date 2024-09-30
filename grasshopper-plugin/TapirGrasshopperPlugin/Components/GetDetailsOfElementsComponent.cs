using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Text.Json;
using Tapir.Data;
using Tapir.Utilities;

namespace Tapir.Components
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

    public class GetDetailsOfWallsComponent : ArchicadAccessorComponent
    {
        public GetDetailsOfWallsComponent ()
          : base (
                "Get wall details",
                "WallDetails",
                "Get details of wall elements.",
                "Elements"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("ElementIds", "ElementIds", "Element ids to get details of.", GH_ParamAccess.list);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("WallIds", "WallIds", "Element ids of the found walls.", GH_ParamAccess.list);
            pManager.AddPointParameter ("Begin coordinates", "BegCoords", "Begin coordinates of walls.", GH_ParamAccess.list);
            pManager.AddPointParameter ("End coordinates", "EndCoords", "End coordinates of walls.", GH_ParamAccess.list);
        }

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            List<ElementIdItemObj> elements = new List<ElementIdItemObj> ();
            if (!DA.GetDataList (0, elements)) {
                return;
            }

            ElementsObj inputElements = new ElementsObj () {
                Elements = elements
            };

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
                    ElementId = elements[i].ElementId
                });
                begCoords.Add (new Point2d (wallDetails.BegCoordinate.X, wallDetails.BegCoordinate.Y));
                endCoords.Add (new Point2d (wallDetails.EndCoordinate.X, wallDetails.EndCoordinate.Y));
            }

            DA.SetDataList (0, walls);
            DA.SetDataList (1, begCoords);
            DA.SetDataList (2, endCoords);
        }

        protected override System.Drawing.Bitmap Icon => Tapir.Properties.Resources.TapirLogo;

        public override Guid ComponentGuid => new Guid ("2b7b8e37-b293-475f-a333-d6afe4c5ffff");
    }
}
