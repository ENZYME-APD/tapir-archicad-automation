using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using Rhino.Geometry;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetZoneBoundariesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetZoneBoundaries";

        public GetZoneBoundariesComponent()
            : base(
                "ZoneBoundaries",
                "Gets the boundaries of the given Zone (connected elements, neighbour zones, etc.).",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "ZoneElementGuid",
                "The identifier of a zone.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ConnectedElement",
                "The identifier of the connected element on the boundary.");

            OutBooleans(
                "IsExternal",
                "True if the boundary is an external one.");

            OutGenerics(
                "NeighbouringZone",
                "Returns the unique identifer of the other Zone the element connects to if the boundary is internal. " +
                "Please note that this boundary does not represent the boundary of the element with the other Zone.");

            OutNumbers(
                "Area",
                "The area of the polygon of the boundary.");

            OutCurves(
                "Polygon outline",
                "The outline polygon of the boundary.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!ElementIdItemObj.TryCreate(
                    this,
                    da,
                    0,
                    out ElementIdItemObj inputZone))
            {
                return;
            }

            if (!TryGetConvertedResponse(
                    CommandName,
                    new ZoneBoundaryParameters
                    {
                        ZoneElementId = inputZone.ElementId
                    },
                    out ZoneBoundariesOutput response))
            {
                return;
            }

            var connectedElements = new List<ElementIdItemObj>();
            var isExternals = new List<bool>();
            var neighbouringZones = new List<ElementIdItemObj>();
            var areas = new List<double>();
            var polygons = new List<PolyCurve>();

            foreach (var zb in response.ZoneBoundaries)
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

            da.SetDataList(
                0,
                connectedElements);
            da.SetDataList(
                1,
                isExternals);
            da.SetDataList(
                2,
                neighbouringZones);
            da.SetDataList(
                3,
                areas);
            da.SetDataList(
                4,
                polygons);
        }

        // protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.ZoneBoundaries;

        public override Guid ComponentGuid =>
            new Guid("50a6793f-e9ba-4a6f-a20d-bcaccaa3e6d9");
    }
}