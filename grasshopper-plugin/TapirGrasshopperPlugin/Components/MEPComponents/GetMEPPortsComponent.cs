using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class GetMEPPortsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetMEPPorts";

        public GetMEPPortsComponent()
            : base(
                "GetMEPPorts",
                "Get the ports of the given MEP elements including position, shape, size and connection status. " +
                "Tree outputs have one branch per queried element. " +
                "Available from Archicad 28.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the MEP elements.");
        }

        protected override void AddOutputs()
        {
            OutTextTree(
                "PortIds",
                "Identifier of each port (one branch per element).");

            OutTextTree(
                "PortNames",
                "Name of each port (one branch per element).");

            outManager.AddPointParameter(
                "Positions",
                "Positions",
                "Position of each port (one branch per element).",
                GH_ParamAccess.tree);

            outManager.AddVectorParameter(
                "Directions",
                "Directions",
                "Direction of each port (one branch per element).",
                GH_ParamAccess.tree);

            OutTextTree(
                "Shapes",
                "Connector shape of each port (one branch per element).");

            OutNumbers(
                "Widths",
                "Width of each port (one branch per element).");

            OutNumbers(
                "Heights",
                "Height of each port (one branch per element).");

            OutTextTree(
                "Domains",
                "MEP domain of each port (one branch per element).");

            OutBooleanTree(
                "IsConnected",
                "True if the port is physically connected (one branch per element).");

            OutGenericTree(
                "ConnectedElementGuids",
                "Identifier of the MEP element connected to each port (null when unconnected; one branch per element).");

            OutTexts(
                "ErrorMessages",
                "Error message for each queried element (empty when successful).");
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

            var portIds = new DataTree<object>();
            var portNames = new DataTree<object>();
            var positions = new DataTree<object>();
            var directions = new DataTree<object>();
            var shapes = new DataTree<object>();
            var widths = new DataTree<object>();
            var heights = new DataTree<object>();
            var domains = new DataTree<object>();
            var isConnected = new DataTree<object>();
            var connectedElementGuids = new DataTree<object>();
            var errors = new List<string>();

            var items = response["elementPorts"] as JArray ?? new JArray();
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                portIds.EnsurePath(path);
                portNames.EnsurePath(path);
                positions.EnsurePath(path);
                directions.EnsurePath(path);
                shapes.EnsurePath(path);
                widths.EnsurePath(path);
                heights.EnsurePath(path);
                domains.EnsurePath(path);
                isConnected.EnsurePath(path);
                connectedElementGuids.EnsurePath(path);

                if (item?["error"] != null)
                {
                    errors.Add(item["error"]?["message"]?.ToString() ?? "");
                    continue;
                }

                errors.Add("");
                if (item["ports"] is JArray ports)
                {
                    foreach (var port in ports)
                    {
                        portIds.Add(port["portId"]?.ToString(), path);
                        portNames.Add(port["name"]?.ToString(), path);
                        var position = port["position"];
                        positions.Add(
                            new Point3d(
                                position?.Value<double?>("x") ?? 0.0,
                                position?.Value<double?>("y") ?? 0.0,
                                position?.Value<double?>("z") ?? 0.0),
                            path);
                        var direction = port["direction"];
                        directions.Add(
                            new Vector3d(
                                direction?.Value<double?>("x") ?? 0.0,
                                direction?.Value<double?>("y") ?? 0.0,
                                direction?.Value<double?>("z") ?? 0.0),
                            path);
                        shapes.Add(port["shape"]?.ToString(), path);
                        widths.Add(port["width"]?.Value<double>(), path);
                        heights.Add(port["height"]?.Value<double>(), path);
                        domains.Add(port["domain"]?.ToString(), path);
                        isConnected.Add(port["isPhysicallyConnected"]?.Value<bool>(), path);
                        var connectedGuid = port["connectedElementId"]?["guid"]?.ToString();
                        connectedElementGuids.Add(
                            connectedGuid == null
                                ? null
                                : new ElementGuidWrapper
                                {
                                    ElementId = new ElementGuid { Guid = connectedGuid }
                                },
                            path);
                    }
                }
            }

            da.SetDataTree(0, portIds);
            da.SetDataTree(1, portNames);
            da.SetDataTree(2, positions);
            da.SetDataTree(3, directions);
            da.SetDataTree(4, shapes);
            da.SetDataTree(5, widths);
            da.SetDataTree(6, heights);
            da.SetDataTree(7, domains);
            da.SetDataTree(8, isConnected);
            da.SetDataTree(9, connectedElementGuids);
            da.SetDataList(10, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetMEPPorts;

        public override Guid ComponentGuid =>
            new Guid("2ebd2344-35f9-4891-a9b0-2ac51be51199");
    }
}
