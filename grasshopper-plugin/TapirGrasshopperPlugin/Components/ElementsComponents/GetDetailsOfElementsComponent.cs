using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.ResponseTypes.Element;
using Arc = TapirGrasshopperPlugin.ResponseTypes.Element.Arc;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public abstract class AbsGetDetailsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetDetailsOfElements";
        protected abstract string InputFieldName { get; }

        protected ElementsObj inputs;
        protected DetailsOfElementsObj response;

        public AbsGetDetailsComponent(
            string name)
            : base(
                name,
                "Get details of elements.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                InputFieldName,
                "Elements Guids to get the details of.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!ElementsObj.TryCreate(
                    da,
                    0,
                    out inputs))
            {
                return;
            }

            if (!TryGetConvertedResponse(
                    CommandName,
                    inputs,
                    out response))
            {
                return;
            }

            ManageResponse(da);
        }

        protected abstract void ManageResponse(
            IGH_DataAccess da);
    }

    public class GetDetailsOfElementsComponent : AbsGetDetailsComponent
    {
        protected override string InputFieldName => "ElementGuids";

        public GetDetailsOfElementsComponent()
            : base("ElementDetails")
        {
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Elements Guids of the found elements.");

            OutTexts(
                "ElementTypes",
                "Element types.");

            OutTexts(
                "ElementIDs",
                "ElementID property values.");

            OutIntegers(
                "StoryIndexes",
                "Story indexes.");

            OutIntegers(
                "LayerIndexes",
                "Layer indexes.");

            OutIntegers(
                "DrawOrders",
                "Drawing orders.");
        }

        protected override void ManageResponse(
            IGH_DataAccess da)
        {
            var validElements = new List<ElementIdItemObj>();
            var elemTypes = new List<String>();
            var elementIDs = new List<String>();
            var storyIndices = new List<int>();
            var layerIndices = new List<int>();
            var drawIndices = new List<int>();

            for (var i = 0; i < response.DetailsOfElements.Count; i++)
            {
                var detailsOfElement = response.DetailsOfElements[i];
                if (detailsOfElement == null)
                {
                    continue;
                }

                validElements.Add(
                    new ElementIdItemObj
                    {
                        ElementId = inputs.Elements[i].ElementId
                    });
                elemTypes.Add(detailsOfElement.Type);
                elementIDs.Add(detailsOfElement.ElementID);
                storyIndices.Add(detailsOfElement.FloorIndex);
                layerIndices.Add(detailsOfElement.LayerIndex);
                drawIndices.Add(detailsOfElement.DrawIndex);
            }

            da.SetDataList(
                0,
                validElements);
            da.SetDataList(
                1,
                elemTypes);
            da.SetDataList(
                2,
                elementIDs);
            da.SetDataList(
                3,
                storyIndices);
            da.SetDataList(
                4,
                layerIndices);
            da.SetDataList(
                5,
                drawIndices);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ElemDetails;

        public override Guid ComponentGuid =>
            new Guid("d1509981-6510-4c09-8727-dba5981109f8");
    }

    public class GetDetailsOfWallsComponent : AbsGetDetailsComponent
    {
        protected override string InputFieldName => "WallGuids";

        public GetDetailsOfWallsComponent()
            : base("WallDetails")
        {
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "WallGuids",
                "Elements Guids of the found walls.");

            OutPoints(
                "StartCoordinates",
                "Start coordinates.");

            OutPoints(
                "EndCoordinates",
                "End coordinates.");

            OutNumbers(
                "Heights",
                "Heights.");

            OutNumbers(
                "ArcAngles",
                "ArcAngles.");

            OutCurves(
                "Lines",
                "Lines or curves.");
        }

        protected override void ManageResponse(
            IGH_DataAccess da)
        {
            var walls = new List<ElementIdItemObj>();
            var begCoords = new List<Point3d>();
            var endCoords = new List<Point3d>();
            var heights = new List<double>();
            var arcAngles = new List<double>();
            var curves = new List<PolyCurve>();

            for (var i = 0; i < response.DetailsOfElements.Count; i++)
            {
                var detailsOfElement = response.DetailsOfElements[i];
                if (detailsOfElement.Type != "Wall")
                {
                    continue;
                }

                var wallDetails =
                    detailsOfElement.Details.ToObject<WallDetails>();

                if (wallDetails == null)
                {
                    continue;
                }

                if (wallDetails.GeometryType != "Straight" &&
                    wallDetails.GeometryType != "Trapezoid")
                {
                    continue;
                }

                walls.Add(
                    new ElementIdItemObj()
                    {
                        ElementId = inputs.Elements[i].ElementId
                    });
                begCoords.Add(
                    new Point3d(
                        wallDetails.BegCoordinate.X,
                        wallDetails.BegCoordinate.Y,
                        wallDetails.ZCoordinate));
                endCoords.Add(
                    new Point3d(
                        wallDetails.EndCoordinate.X,
                        wallDetails.EndCoordinate.Y,
                        wallDetails.ZCoordinate));
                heights.Add(wallDetails.Height);
                arcAngles.Add(wallDetails.ArcAngle.Value);
                var arcs = new List<Arc>();
                if (wallDetails.ArcAngle.HasValue &&
                    Math.Abs(wallDetails.ArcAngle.Value) >=
                    Utilities.Convert.Epsilon)
                {
                    arcs.Add(
                        new Arc
                        {
                            BegIndex = 0,
                            EndIndex = 1,
                            arcAngle = wallDetails.ArcAngle.Value
                        });
                }

                curves.Add(
                    Utilities.Convert.ToPolyCurve(
                        new List<Point2D>
                        {
                            wallDetails.BegCoordinate,
                            wallDetails.EndCoordinate
                        },
                        arcs,
                        wallDetails.ZCoordinate));
            }

            da.SetDataList(
                0,
                walls);
            da.SetDataList(
                1,
                begCoords);
            da.SetDataList(
                2,
                endCoords);
            da.SetDataList(
                3,
                heights);
            da.SetDataList(
                4,
                arcAngles);
            da.SetDataList(
                5,
                curves);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.WallDetails;

        public override Guid ComponentGuid =>
            new Guid("2b7b8e37-b293-475f-a333-d6afe4c5ffff");
    }

    public class GetDetailsOfBeamsComponent : AbsGetDetailsComponent
    {
        protected override string InputFieldName => "BeamGuids";

        public GetDetailsOfBeamsComponent()
            : base("BeamDetails")
        {
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "BeamGuids",
                "Elements Guids of the found beams.");


            OutPoints(
                "StartCoordinates",
                "Start coordinates.");

            OutPoints(
                "EndCoordinates",
                "End coordinates.");

            OutNumbers(
                "SlantAngles",
                "SlantAngles.");

            OutNumbers(
                "ArcAngles",
                "ArcAngles.");

            OutNumbers(
                "VerticalCurveHeights",
                "VerticalCurveHeights.");

            OutCurves(
                "Lines",
                "Lines or curves.");
        }

        protected override void ManageResponse(
            IGH_DataAccess da)
        {
            var beams = new List<ElementIdItemObj>();
            var begCoords = new List<Point3d>();
            var endCoords = new List<Point3d>();
            var slantAngles = new List<double>();
            var arcAngles = new List<double>();
            var verticalCurveHeights = new List<double>();
            var curves = new List<PolyCurve>();

            for (var i = 0; i < response.DetailsOfElements.Count; i++)
            {
                var detailsOfElement = response.DetailsOfElements[i];
                if (detailsOfElement.Type != "Beam")
                {
                    continue;
                }

                var beamDetails =
                    detailsOfElement.Details.ToObject<BeamDetails>();

                if (beamDetails == null)
                {
                    continue;
                }

                beams.Add(
                    new ElementIdItemObj
                    {
                        ElementId = inputs.Elements[i].ElementId
                    });

                var begPoint3D = new Point3d(
                    beamDetails.BegCoordinate.X,
                    beamDetails.BegCoordinate.Y,
                    beamDetails.ZCoordinate);

                var endPoint3D = new Point3d(
                    beamDetails.EndCoordinate.X,
                    beamDetails.EndCoordinate.Y,
                    beamDetails.ZCoordinate);

                begCoords.Add(begPoint3D);

                if (Math.Abs(beamDetails.SlantAngle) >=
                    Utilities.Convert.Epsilon)
                {
                    var dirV = endPoint3D - begPoint3D;
                    var delta = Math.Sin(beamDetails.SlantAngle) * dirV.Length;
                    endPoint3D.Z += delta;
                }

                endCoords.Add(endPoint3D);
                slantAngles.Add(beamDetails.SlantAngle);
                arcAngles.Add(beamDetails.ArcAngle);
                verticalCurveHeights.Add(beamDetails.VerticalCurveHeight);
                var arcs = new List<Arc>();
                if (Math.Abs(beamDetails.ArcAngle) >= Utilities.Convert.Epsilon)
                {
                    arcs.Add(
                        new Arc
                        {
                            BegIndex = 0,
                            EndIndex = 1,
                            arcAngle = beamDetails.ArcAngle
                        });
                }

                curves.Add(
                    Utilities.Convert.ToPolyCurve(
                        new List<Point2D>
                        {
                            beamDetails.BegCoordinate,
                            beamDetails.EndCoordinate
                        },
                        arcs,
                        beamDetails.ZCoordinate,
                        beamDetails.SlantAngle,
                        beamDetails.VerticalCurveHeight));
            }

            da.SetDataList(
                0,
                beams);
            da.SetDataList(
                1,
                begCoords);
            da.SetDataList(
                2,
                endCoords);
            da.SetDataList(
                3,
                slantAngles);
            da.SetDataList(
                4,
                arcAngles);
            da.SetDataList(
                5,
                verticalCurveHeights);
            da.SetDataList(
                6,
                curves);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.BeamDetails;

        public override Guid ComponentGuid =>
            new Guid("6e0deaaa-a9b0-4c30-9bc0-f2a1ef299c5d");
    }

    public class GetDetailsOfColumnsComponent : AbsGetDetailsComponent
    {
        protected override string InputFieldName => "ColumnGuids";

        public GetDetailsOfColumnsComponent()
            : base("ColumnDetails")
        {
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ColumnGuids",
                "Elements Guids of the found columns.");

            OutPoints(
                "OrigoCoordinates",
                "Origo coordinates.");
        }

        protected override void ManageResponse(
            IGH_DataAccess da)
        {
            var columns = new List<ElementIdItemObj>();
            var origoCoords = new List<Point3d>();

            for (var i = 0; i < response.DetailsOfElements.Count; i++)
            {
                var detailsOfElement = response.DetailsOfElements[i];
                if (detailsOfElement.Type != "Column")
                {
                    continue;
                }

                var columnDetails =
                    detailsOfElement.Details.ToObject<ColumnDetails>();

                if (columnDetails == null)
                {
                    continue;
                }

                columns.Add(
                    new ElementIdItemObj()
                    {
                        ElementId = inputs.Elements[i].ElementId
                    });

                origoCoords.Add(
                    new Point3d(
                        columnDetails.OrigoCoordinate.X,
                        columnDetails.OrigoCoordinate.Y,
                        columnDetails.ZCoordinate));
            }

            da.SetDataList(
                0,
                columns);
            da.SetDataList(
                1,
                origoCoords);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ColumnDetails;

        public override Guid ComponentGuid =>
            new Guid("ded49694-9869-4670-af85-645535a7be6a");
    }

    public class GetDetailsOfSlabsComponent : AbsGetDetailsComponent
    {
        protected override string InputFieldName => "SlabGuids";

        public GetDetailsOfSlabsComponent()
            : base("SlabDetails")
        {
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "SlabGuids",
                "Elements Guids of the found slabs.");

            OutCurves(
                "Polygons",
                "The outline polygons of the slabs.");

            OutCurveTree(
                "HolePolygons",
                "The outline polygons of the holes in the slabs.");
        }

        protected override void ManageResponse(
            IGH_DataAccess da)
        {
            var slabs = new List<ElementIdItemObj>();
            var polygons = new List<PolyCurve>();
            var holePolygonsTree = new DataTree<PolyCurve>();

            for (var i = 0; i < response.DetailsOfElements.Count; i++)
            {
                var detailsOfElement = response.DetailsOfElements[i];
                if (detailsOfElement.Type != "Slab")
                {
                    continue;
                }

                var slabDetails =
                    detailsOfElement.Details.ToObject<SlabDetails>();
                if (slabDetails == null)
                {
                    continue;
                }

                slabs.Add(
                    new ElementIdItemObj()
                    {
                        ElementId = inputs.Elements[i].ElementId
                    });
                polygons.Add(
                    Utilities.Convert.ToPolyCurve(
                        slabDetails.PolygonCoordinates,
                        slabDetails.PolygonArcs,
                        slabDetails.ZCoordinate));

                var holePolygons = new List<PolyCurve>();
                foreach (var holeDetail in slabDetails.Holes)
                {
                    holePolygons.Add(
                        Utilities.Convert.ToPolyCurve(
                            holeDetail.PolygonCoordinates,
                            holeDetail.PolygonArcs,
                            slabDetails.ZCoordinate));
                }

                holePolygonsTree.AddRange(
                    holePolygons,
                    new GH_Path(slabs.Count));
            }

            da.SetDataList(
                0,
                slabs);
            da.SetDataList(
                1,
                polygons);
            da.SetDataTree(
                2,
                holePolygonsTree);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SlabDetails;

        public override Guid ComponentGuid =>
            new Guid("f942eece-cc80-4945-a911-fe548dae4ae8");
    }

    public class GetDetailsOfPolylinesComponent : AbsGetDetailsComponent
    {
        protected override string InputFieldName => "PolylineGuids";

        public GetDetailsOfPolylinesComponent()
            : base("PolylineDetails")
        {
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "PolylineGuids",
                "Elements Guids of the found polylines.");

            OutCurves(
                "Coordinates",
                "The coordinates of the polylines.");
        }

        protected override void ManageResponse(
            IGH_DataAccess da)
        {
            var polylines = new List<ElementIdItemObj>();
            var rhinoPolylines = new List<PolyCurve>();

            for (var i = 0; i < response.DetailsOfElements.Count; i++)
            {
                var detailsOfElement = response.DetailsOfElements[i];
                if (detailsOfElement.Type != "PolyLine")
                {
                    continue;
                }

                var polylineDetails =
                    detailsOfElement.Details.ToObject<PolylineDetails>();
                if (polylineDetails == null)
                {
                    continue;
                }

                polylines.Add(
                    new ElementIdItemObj()
                    {
                        ElementId = inputs.Elements[i].ElementId
                    });
                rhinoPolylines.Add(
                    Utilities.Convert.ToPolyCurve(
                        polylineDetails.Coordinates,
                        polylineDetails.Arcs,
                        polylineDetails.ZCoordinate));
            }

            da.SetDataList(
                0,
                polylines);
            da.SetDataList(
                1,
                rhinoPolylines);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.PolylineDetails;

        public override Guid ComponentGuid =>
            new Guid("b96c3b7e-303d-44f2-af22-6fd07ade11fc");
    }

    public class GetDetailsOfZonesComponent : AbsGetDetailsComponent
    {
        protected override string InputFieldName => "ZoneGuids";

        public GetDetailsOfZonesComponent()
            : base("ZoneDetails")
        {
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ZoneGuids",
                "Elements Guids of the found zones.");

            OutTexts(
                "Names",
                "Names of zones.");

            OutTexts(
                "Numbers",
                "Numbers of zones.");

            OutGenerics(
                "CategoryAttributeIds",
                "Ids of zone category attributes.");

            OutPoints(
                "StampPositions",
                "Position of zone stamps.");

            OutBooleans(
                "IsManual",
                "Is the coordinates of the zone manually placed?");

            OutCurves(
                "Polygons",
                "The outline polygons of the zones.");

            OutCurveTree(
                "HolePolygons",
                "The outline polygons of the holes in the zones.");
        }

        protected override void ManageResponse(
            IGH_DataAccess da)
        {
            var zones = new List<ElementIdItemObj>();
            var names = new List<string>();
            var numberStrs = new List<string>();
            var categoryAttributeIds = new List<AttributeIdObj>();
            var stampPositions = new List<Point3d>();
            var isManuals = new List<bool>();
            var polygons = new List<PolyCurve>();
            var holePolygonsTree = new DataTree<PolyCurve>();

            for (var i = 0; i < response.DetailsOfElements.Count; i++)
            {
                var detailsOfElement = response.DetailsOfElements[i];
                if (detailsOfElement.Type != "Zone")
                {
                    continue;
                }

                var zoneDetails =
                    detailsOfElement.Details.ToObject<ZoneDetails>();
                if (zoneDetails == null)
                {
                    continue;
                }

                zones.Add(
                    new ElementIdItemObj()
                    {
                        ElementId = inputs.Elements[i].ElementId
                    });
                names.Add(zoneDetails.Name);
                numberStrs.Add(zoneDetails.NumberStr);
                categoryAttributeIds.Add(zoneDetails.CategoryAttributeId);
                stampPositions.Add(
                    new Point3d(
                        zoneDetails.StampPosition.X,
                        zoneDetails.StampPosition.Y,
                        zoneDetails.ZCoordinate));
                isManuals.Add(zoneDetails.IsManual);
                polygons.Add(
                    Utilities.Convert.ToPolyCurve(
                        zoneDetails.PolygonCoordinates,
                        zoneDetails.PolygonArcs,
                        zoneDetails.ZCoordinate));

                var holePolygons = new List<PolyCurve>();

                foreach (var holeDetail in zoneDetails.Holes)
                {
                    holePolygons.Add(
                        Utilities.Convert.ToPolyCurve(
                            holeDetail.PolygonCoordinates,
                            holeDetail.PolygonArcs,
                            zoneDetails.ZCoordinate));
                }

                holePolygonsTree.AddRange(
                    holePolygons,
                    new GH_Path(zones.Count));
            }

            da.SetDataList(
                0,
                zones);
            da.SetDataList(
                1,
                names);
            da.SetDataList(
                2,
                numberStrs);
            da.SetDataList(
                3,
                categoryAttributeIds);
            da.SetDataList(
                4,
                stampPositions);
            da.SetDataList(
                5,
                isManuals);
            da.SetDataList(
                6,
                polygons);
            da.SetDataTree(
                7,
                holePolygonsTree);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ZoneDetails;

        public override Guid ComponentGuid =>
            new Guid("65b5952f-fc7d-4d9e-9742-9be32ac3c5d1");
    }
}