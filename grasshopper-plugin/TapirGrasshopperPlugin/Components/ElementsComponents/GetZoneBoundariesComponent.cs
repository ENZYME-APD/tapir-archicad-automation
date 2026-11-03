using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
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
                "Gets the boundaries of the given Zones (connected elements, neighbour zones, etc.). Give " +
                "either a single ZoneElementGuid or a list of ZoneElementGuids. Prefer the list: the " +
                "expensive boundary recalculation runs once per call, so querying many Zones at once is much " +
                "faster than one Zone per call.",
                GroupNames.ElementDetails)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "ZoneElementGuid",
                "The identifier of a single zone. Give either this or ZoneElementGuids.");

            InGenerics(
                "ZoneElementGuids",
                "The identifiers of several zones; the outputs then have one branch per zone. Give either " +
                "this or ZoneElementGuid.");

            SetOptionality(
                new[]
                {
                    0,
                    1
                });
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

            OutErrorMessages(
                "Error message of each zone given in ZoneElementGuids (empty when successful).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var zoneWrapper = da.GetOptional<GH_ObjectWrapper>(
                0,
                null);
            var zoneWrappers = da.GetOptionals<GH_ObjectWrapper>(1);

            if ((zoneWrapper != null) == (zoneWrappers.Count > 0))
            {
                this.AddError("Give exactly one of the inputs ZoneElementGuid and ZoneElementGuids.");
                return;
            }

            if (zoneWrappers.Count > 0)
            {
                SolveZones(
                    da,
                    zoneWrappers);
                return;
            }

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

        // The zones input: one call for every zone, the outputs get one
        // branch per zone.
        private void SolveZones(
            IGH_DataAccess da,
            List<GH_ObjectWrapper> zoneWrappers)
        {
            if (!zoneWrappers.TryBuildObject(out ElementsObject zones))
            {
                this.AddError("Invalid zone identifier in the ZoneElementGuids input.");
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["zones"] = JArray.FromObject(zones.Elements) },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var connectedElements = new DataTree<object>();
            var isExternal = new DataTree<object>();
            var neighbouringZones = new DataTree<object>();
            var areas = new DataTree<object>();
            var outlines = new DataTree<object>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "zoneBoundariesOfZones");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                connectedElements.EnsurePath(path);
                isExternal.EnsurePath(path);
                neighbouringZones.EnsurePath(path);
                areas.EnsurePath(path);
                outlines.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");
                if (!(item["zoneBoundaries"] is JArray boundaries))
                {
                    continue;
                }

                foreach (var boundary in boundaries)
                {
                    connectedElements.Add(
                        ElementsStructuredGetterComponent.ElementIdOf(
                            boundary,
                            "connectedElementId"),
                        path);
                    isExternal.Add(
                        JsonOutputHelp.Scalar(
                            boundary,
                            "isExternal"),
                        path);
                    neighbouringZones.Add(
                        ElementsStructuredGetterComponent.ElementIdOf(
                            boundary,
                            "neighbouringZoneElementId"),
                        path);
                    areas.Add(
                        JsonOutputHelp.Scalar(
                            boundary,
                            "area"),
                        path);
                    outlines.Add(
                        ElementGeometryJsonHelp.Polyline3D(
                            boundary["polygonOutline"],
                            false),
                        path);
                }
            }

            da.SetDataTree(
                0,
                connectedElements);
            da.SetDataTree(
                1,
                isExternal);
            da.SetDataTree(
                2,
                neighbouringZones);
            da.SetDataTree(
                3,
                areas);
            da.SetDataTree(
                4,
                outlines);
            da.SetDataList(
                5,
                errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ZoneBoundaries;

        public override Guid ComponentGuid =>
            new Guid("50a6793f-e9ba-4a6f-a20d-bcaccaa3e6d9");
    }
}
