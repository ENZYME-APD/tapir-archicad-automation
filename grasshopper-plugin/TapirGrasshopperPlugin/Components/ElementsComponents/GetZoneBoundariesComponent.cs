using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using Rhino.Geometry;
using System.Security.Cryptography.X509Certificates;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using System.Linq;

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
                "Areas",
                "The areas of the boundary polygons.");

            OutCurves(
                "PolygonOutlines",
                "The outlines of the boundary polygons.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreate(
                    0,
                    out ElementGuid zoneElementId))
            {
                return;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    new { zoneElementId },
                    ToAddOn,
                    JHelp.Deserialize<ZoneBoundariesOutput>,
                    out ZoneBoundariesOutput response))
            {
                return;
            }

            da.SetDataList(
                0,
                response.ZoneBoundaries.Select(x => new ElementGuidWrapper
                {
                    ElementId = x.ConnectedElementId
                }));

            da.SetDataList(
                1,
                response.ZoneBoundaries.Select(x => x.IsExternal));

            da.SetDataList(
                2,
                response.ZoneBoundaries.Select(x => new ElementGuidWrapper
                {
                    ElementId = x.NeighbouringZoneElementId
                }));

            da.SetDataList(
                3,
                response.ZoneBoundaries.Select(x => x.Area));

            da.SetDataList(
                4,
                response.ZoneBoundaries.Select(x =>
                    Helps.Convert.ToPolyCurve(x.PolygonCoordinates)));
        }

        public override Guid ComponentGuid =>
            new Guid("50a6793f-e9ba-4a6f-a20d-bcaccaa3e6d9");
    }
}