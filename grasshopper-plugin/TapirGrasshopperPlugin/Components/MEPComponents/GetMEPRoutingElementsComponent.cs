using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class GetMEPRoutingElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetMEPRoutingElements";

        public GetMEPRoutingElementsComponent()
            : base(
                "GetMEPRoutingElements",
                "Get the details of the given MEP routing elements. " +
                "Tree outputs have one branch per routing element. " +
                "Available from Archicad 28.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the MEP routing elements.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Domains",
                "MEP domain of each routing element.");

            OutGenerics(
                "MEPSystemGuids",
                "MEP system attribute of each routing element.");

            outManager.AddPointParameter(
                "Polylines",
                "Polylines",
                "Route polyline points of each routing element (one branch per element).",
                GH_ParamAccess.tree);

            OutGenericTree(
                "SegmentGuids",
                "Identifier of each routing segment (one branch per element).");

            OutNumbers(
                "SegmentWidths",
                "Cross section width of each routing segment (one branch per element).");

            OutNumbers(
                "SegmentHeights",
                "Cross section height of each routing segment (one branch per element).");

            OutTextTree(
                "SegmentShapes",
                "Cross section shape of each routing segment (one branch per element).");

            OutGenericTree(
                "NodeGuids",
                "Identifier of each routing node (one branch per element).");

            outManager.AddPointParameter(
                "NodePositions",
                "NodePositions",
                "Position of each routing node (one branch per element).",
                GH_ParamAccess.tree);

            OutTexts(
                "ErrorMessages",
                "Error message for each queried element (empty when successful).");
        }

        private static ElementGuidWrapper ElementIdOf(
            string guid)
        {
            if (guid == null)
            {
                return null;
            }
            return new ElementGuidWrapper
            {
                ElementId = new ElementGuid { Guid = guid }
            };
        }

        private static Point3d PointOf(
            JToken coordinate)
        {
            return new Point3d(
                coordinate?.Value<double?>("x") ?? 0.0,
                coordinate?.Value<double?>("y") ?? 0.0,
                coordinate?.Value<double?>("z") ?? 0.0);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(elements),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var domains = new List<object>();
            var mepSystemGuids = new List<object>();
            var polylines = new DataTree<object>();
            var segmentGuids = new DataTree<object>();
            var segmentWidths = new DataTree<object>();
            var segmentHeights = new DataTree<object>();
            var segmentShapes = new DataTree<object>();
            var nodeGuids = new DataTree<object>();
            var nodePositions = new DataTree<object>();
            var errors = new List<string>();

            var items = response["routingElements"] as JArray ?? new JArray();
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                polylines.EnsurePath(path);
                segmentGuids.EnsurePath(path);
                segmentWidths.EnsurePath(path);
                segmentHeights.EnsurePath(path);
                segmentShapes.EnsurePath(path);
                nodeGuids.EnsurePath(path);
                nodePositions.EnsurePath(path);

                if (item?["error"] != null)
                {
                    errors.Add(item["error"]?["message"]?.ToString() ?? "");
                    domains.Add(null);
                    mepSystemGuids.Add(null);
                    continue;
                }

                errors.Add("");
                domains.Add(item["domain"]?.ToString());
                var mepSystemGuid = item["mepSystemId"]?["guid"]?.ToString();
                mepSystemGuids.Add(
                    mepSystemGuid == null
                        ? null
                        : new AttributeGuidWrapper
                        {
                            AttributeId = new AttributeGuidObject { Guid = mepSystemGuid }
                        });

                if (item["polyline"] is JArray polyline)
                {
                    foreach (var coordinate in polyline)
                    {
                        polylines.Add(PointOf(coordinate), path);
                    }
                }

                if (item["segments"] is JArray segments)
                {
                    foreach (var segment in segments)
                    {
                        segmentGuids.Add(ElementIdOf(segment["elementId"]?["guid"]?.ToString()), path);
                        segmentWidths.Add(segment["crossSectionWidth"]?.Value<double>(), path);
                        segmentHeights.Add(segment["crossSectionHeight"]?.Value<double>(), path);
                        segmentShapes.Add(segment["crossSectionShape"]?.ToString(), path);
                    }
                }

                if (item["nodes"] is JArray nodes)
                {
                    foreach (var node in nodes)
                    {
                        nodeGuids.Add(ElementIdOf(node["elementId"]?["guid"]?.ToString()), path);
                        nodePositions.Add(PointOf(node["position"]), path);
                    }
                }
            }

            da.SetDataList(0, domains);
            da.SetDataList(1, mepSystemGuids);
            da.SetDataTree(2, polylines);
            da.SetDataTree(3, segmentGuids);
            da.SetDataTree(4, segmentWidths);
            da.SetDataTree(5, segmentHeights);
            da.SetDataTree(6, segmentShapes);
            da.SetDataTree(7, nodeGuids);
            da.SetDataTree(8, nodePositions);
            da.SetDataList(9, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetMEPRoutingElements;

        public override Guid ComponentGuid =>
            new Guid("63de6be1-d219-4a61-89db-b86b6af204e8");
    }
}
