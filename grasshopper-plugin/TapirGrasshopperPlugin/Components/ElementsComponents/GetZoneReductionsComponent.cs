using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetZoneReductionsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetZoneReductions";

        public GetZoneReductionsComponent()
            : base(
                "ZoneReductions",
                "Gets the area reductions of Zones: the polygons the Zone Stamp's area calculation subtracts " +
                "(Walls, Columns, reducing Fills, sloping Roofs, low ceilings), with their percentage and area.",
                GroupNames.ElementDetails)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ZoneGuids",
                "Identifiers of the Zones to get the area reductions of.");
        }

        protected override void AddOutputs()
        {
            OutTextTree(
                "Types",
                "What reduces the area in each polygon (Wall, Column, Hatch, Gable, LowArea or Rest), " +
                "one branch per Zone.");

            OutNumbers(
                "Percents",
                "The percentage each polygon reduces the Zone area by, one branch per Zone, aligned with Types.");

            OutNumbers(
                "Areas",
                "The area of each polygon in square meters, one branch per Zone, aligned with Types.");

            OutCurveTree(
                "Polygons",
                "The polygons on the XY plane, one branch per reduction with the path {zone index; reduction index}: " +
                "the outline first, then its holes.");

            OutErrorMessages(
                "Error message of each Zone (empty when its reductions were read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject zones))
            {
                return;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    zones,
                    ToAddOn,
                    JHelp.Deserialize<ReductionsOfZonesObject>,
                    out ReductionsOfZonesObject response))
            {
                return;
            }

            var types = new DataTree<string>();
            var percents = new DataTree<double>();
            var areas = new DataTree<double>();
            var polygons = new DataTree<Curve>();
            var errorMessages = new List<string>();

            for (var i = 0; i < response.ReductionsOfZones.Count; i++)
            {
                var item = response.ReductionsOfZones[i];
                var zonePath = new GH_Path(i);
                types.EnsurePath(zonePath);
                percents.EnsurePath(zonePath);
                areas.EnsurePath(zonePath);

                if (item?.Error != null)
                {
                    errorMessages.Add(item.Error.Message ?? "Unknown error.");
                    continue;
                }

                errorMessages.Add("");
                if (item?.Reductions == null)
                {
                    continue;
                }

                for (var j = 0; j < item.Reductions.Count; j++)
                {
                    var reduction = item.Reductions[j];
                    if (reduction == null)
                    {
                        continue;
                    }

                    types.Add(reduction.Type ?? "Unknown", zonePath);
                    percents.Add(reduction.Percent, zonePath);
                    areas.Add(reduction.Area, zonePath);

                    var polygonPath = new GH_Path(i, j);
                    polygons.EnsurePath(polygonPath);
                    if (reduction.PolygonOutline != null && reduction.PolygonOutline.Count > 1)
                    {
                        polygons.Add(
                            Helps.Convert.ToPolyCurve(
                                reduction.PolygonOutline,
                                reduction.PolygonArcs,
                                0.0),
                            polygonPath);
                    }

                    if (reduction.Holes == null)
                    {
                        continue;
                    }

                    foreach (var hole in reduction.Holes)
                    {
                        if (hole?.PolygonOutline == null || hole.PolygonOutline.Count < 2)
                        {
                            continue;
                        }

                        polygons.Add(
                            Helps.Convert.ToPolyCurve(
                                hole.PolygonOutline,
                                hole.PolygonArcs,
                                0.0),
                            polygonPath);
                    }
                }
            }

            da.SetDataTree(0, types);
            da.SetDataTree(1, percents);
            da.SetDataTree(2, areas);
            da.SetDataTree(3, polygons);
            da.SetDataList(4, errorMessages);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ZoneBoundaries;

        public override Guid ComponentGuid =>
            new Guid("7d2e4b1a-9c63-4f58-a1b7-2e8d5c0f9a46");
    }
}
