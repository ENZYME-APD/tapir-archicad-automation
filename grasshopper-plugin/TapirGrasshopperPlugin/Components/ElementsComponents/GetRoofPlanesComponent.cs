using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetRoofPlanesComponent : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetRoofPlanes";

        public GetRoofPlanesComponent()
            : base(
                "GetRoofPlanes",
                "Gets the single planes of roofs, without changing the roofs. A multi-plane roof is " +
                "decomposed by Archicad into its planes, each returned with its part identifier and its " +
                "details as a single-plane roof (slope, pivot line, level, outline with arcs and holes); a " +
                "single-plane roof is returned as its only plane. The outputs are aligned with the input " +
                "list of elements, one branch per roof.",
                GroupNames.ElementDetails)
        {
        }

        protected override string ResponseArrayKey => "roofPlanesOfElements";

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the Roof elements.");
        }

        protected override void AddOutputs()
        {
            OutCurveTree(
                "Outlines",
                "The outline of each plane on the XY plane, with its arcs (one branch per roof).");

            OutGenericTree(
                "Slopes",
                "The slope of each plane in radians (one branch per roof).");

            OutGenericTree(
                "Levels",
                "The height of the pivot line of each plane above the floor level (one branch per roof).");

            OutIntegerTree(
                "PartIdMains",
                "The main part of Archicad's identifier of each plane within the multi-plane roof; null for " +
                "a single-plane roof (one branch per roof).");

            OutIntegerTree(
                "PartIdSubs",
                "The sub part of Archicad's identifier of each plane within the multi-plane roof; null for " +
                "a single-plane roof (one branch per roof).");

            OutTextTree(
                "Details",
                "The details of each plane as a single-plane roof, as a JSON object in the format of the " +
                "roof details of GetDetailsOfElements (one branch per roof).");

            OutErrorMessages(
                "Error message of each element (empty when successful), e.g. when it is not a roof.");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var outlines = new DataTree<object>();
            var slopes = new DataTree<object>();
            var levels = new DataTree<object>();
            var partIdMains = new DataTree<object>();
            var partIdSubs = new DataTree<object>();
            var details = new DataTree<object>();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                outlines.EnsurePath(path);
                slopes.EnsurePath(path);
                levels.EnsurePath(path);
                partIdMains.EnsurePath(path);
                partIdSubs.EnsurePath(path);
                details.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");
                if (!(item["planes"] is JArray planes))
                {
                    continue;
                }

                foreach (var plane in planes)
                {
                    var planeDetails = plane["details"];
                    outlines.Add(
                        PolygonHelp.ToCurve(
                            planeDetails?["polygonOutline"],
                            planeDetails?["polygonArcs"],
                            true),
                        path);
                    slopes.Add(
                        JsonOutputHelp.Scalar(
                            planeDetails,
                            "angle"),
                        path);
                    levels.Add(
                        JsonOutputHelp.Scalar(
                            planeDetails,
                            "level"),
                        path);
                    partIdMains.Add(
                        plane["partId"]?.Value<int?>("main"),
                        path);
                    partIdSubs.Add(
                        plane["partId"]?.Value<int?>("sub"),
                        path);
                    details.Add(
                        planeDetails?.ToString(Formatting.None),
                        path);
                }
            }

            da.SetDataTree(
                0,
                outlines);
            da.SetDataTree(
                1,
                slopes);
            da.SetDataTree(
                2,
                levels);
            da.SetDataTree(
                3,
                partIdMains);
            da.SetDataTree(
                4,
                partIdSubs);
            da.SetDataTree(
                5,
                details);
            da.SetDataList(
                6,
                errors);
        }

        // Shares the icon of RoofDetails until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RoofDetails;

        public override Guid ComponentGuid =>
            new Guid("bde08bc4-da52-4929-9cf2-d23e0f449f5a");
    }
}
