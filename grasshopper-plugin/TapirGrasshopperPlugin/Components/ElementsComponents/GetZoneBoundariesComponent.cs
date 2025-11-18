using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using Rhino.Geometry;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ZoneBoundaryParameters
    {
        [JsonProperty("zoneElementId")]
        public ElementIdObj ZoneElementId;
    }

    public class ZoneBoundary
    {
        [JsonProperty("connectedElementId")]
        public ElementIdObj ConnectedElementId;

        [JsonProperty("isExternal")]
        public bool IsExternal;

        [JsonProperty("neighbouringZoneElementId")]
        public ElementIdObj NeighbouringZoneElementId;

        [JsonProperty("area")]
        public double Area;

        [JsonProperty("polygonOutline")]
        public List<Point3D> PolygonCoordinates;
    }

    public class ZoneBoundariesOutput
    {
        [JsonProperty("zoneBoundaries")]
        public List<ZoneBoundary> ZoneBoundaries;
    }

    public class GetZoneBoundariesComponent : ArchicadAccessorComponent
    {
        public static string Doc =>
            "Gets the boundaries of the given Zone (connected elements, neighbour zones, etc.).";

        public override string CommandName => "GetZoneBoundaries";

        public GetZoneBoundariesComponent()
            : base(
                "ZoneBoundaries",
                "ZoneBoundaries",
                Doc,
                GroupNames.Elements)
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter(
                "ZoneElementGuid",
                "ZoneElementGuid",
                "The identifier of a zone.",
                GH_ParamAccess.item);
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter(
                "ConnectedElement",
                "ConnectedElement",
                "The identifier of the connected element on the boundary.",
                GH_ParamAccess.list);
            pManager.AddBooleanParameter(
                "IsExternal",
                "IsExternal",
                "True if the boundary is an external one.",
                GH_ParamAccess.list);
            pManager.AddGenericParameter(
                "NeighbouringZone",
                "NeighbouringZone",
                "Returns the unique identifer of the other Zone the element connects to if the boundary is internal. Please note that this boundary does not represent the boundary of the element with the other Zone.",
                GH_ParamAccess.list);
            pManager.AddNumberParameter(
                "Area",
                "Area",
                "The area of the polygon of the boundary.",
                GH_ParamAccess.list);
            pManager.AddCurveParameter(
                "Polygon outline",
                "Polygon",
                "The outline polygon of the boundary.",
                GH_ParamAccess.list);
        }

        protected override void Solve(
            IGH_DataAccess DA)
        {
            var inputZone = ElementIdItemObj.Create(
                DA,
                0);
            if (inputZone == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input ZoneElementGuid failed to collect data.");
                return;
            }

            var parameters = new ZoneBoundaryParameters()
            {
                ZoneElementId = inputZone.ElementId
            };

            if (!GetConvertedResponse(
                    CommandName,
                    parameters,
                    out ZoneBoundariesOutput zoneBoundaries))
            {
                return;
            }

            var connectedElements = new List<ElementIdItemObj>();
            var isExternals = new List<bool>();
            var neighbouringZones = new List<ElementIdItemObj>();
            var areas = new List<double>();
            var polygons = new List<PolyCurve>();

            foreach (var zb in zoneBoundaries.ZoneBoundaries)
            {
                connectedElements.Add(
                    new ElementIdItemObj()
                    {
                        ElementId = zb.ConnectedElementId
                    });
                isExternals.Add(zb.IsExternal);
                neighbouringZones.Add(
                    new ElementIdItemObj()
                    {
                        ElementId = zb.NeighbouringZoneElementId
                    });
                areas.Add(zb.Area);
                polygons.Add(
                    Utilities.Convert.ToPolyCurve(zb.PolygonCoordinates));
            }

            DA.SetDataList(
                0,
                connectedElements);
            DA.SetDataList(
                1,
                isExternals);
            DA.SetDataList(
                2,
                neighbouringZones);
            DA.SetDataList(
                3,
                areas);
            DA.SetDataList(
                4,
                polygons);
        }

        // protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.ZoneBoundaries;

        public override Guid ComponentGuid =>
            new Guid("50a6793f-e9ba-4a6f-a20d-bcaccaa3e6d9");
    }
}