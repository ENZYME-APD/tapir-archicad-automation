using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class CreateMEPElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateMEPElements";

        public CreateMEPElementsComponent()
            : base(
                "CreateMEPElements",
                "Create MEP elements (Terminal, Accessory, Equipment or Fitting) at the given positions. " +
                "Available from Archicad 28.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InPoints(
                "Positions",
                "Position of each MEP element.");

            InTexts(
                "Types",
                "Type of each MEP element: Terminal, Accessory, Equipment or Fitting " +
                "(input only 1 to use the same type for all).");

            InTexts(
                "Domains",
                "MEP domain of each element: Ventilation, Piping or CableCarrier " +
                "(input only 1 to use the same domain for all). Required for all types except Equipment.");

            InVectors(
                "OrientationDirections",
                "Direction vector of each element (input only 1 to use the same direction for all). Optional.");

            InVectors(
                "OrientationRotations",
                "Rotation vector of each element (input only 1 to use the same rotation for all). Optional.");

            InTexts(
                "LibraryPartGuids",
                "Library part to place for each element instead of the one in the tool's settings, as listed by " +
                "GetAvailableLibraryParts (input only 1 to use the same library part for all). Its ports replace " +
                "the default ones and keep the MEP system of the library part, so give MEPSystemGuids with it to " +
                "connect it. Optional.");

            InNumbers(
                "XSizes",
                "Size of each element along its local x axis in meters (input only 1 to use the same size for all). " +
                "Optional; available from Archicad 30.");

            InNumbers(
                "YSizes",
                "Size of each element along its local y axis in meters (input only 1 to use the same size for all). " +
                "Optional; available from Archicad 30.");

            InGenerics(
                "MEPSystemGuids",
                "MEP system attribute of the ports of each element's domain (of every port when an Equipment " +
                "item has no domain; input only 1 to use the same system for all). Optional.");

            InNumbers(
                "InsulationThicknesses",
                "Insulation thickness of each element in meters, changing the tool default only for this element " +
                "(input only 1 to use the same thickness for all). Optional.");

            InBooleans(
                "RemoveInsulation",
                "If true, the element is placed without insulation; cannot be combined with " +
                "InsulationThicknesses (input only 1 to use the same value for all). Optional.");

            InTexts(
                "Ports",
                "Changes to single ports of each element before it is placed, as a JSON array (input only 1 to " +
                "use the same changes for all), for example [{\"portIndex\": 0, \"diameter\": 0.1}]. " +
                "Each entry has portIndex (zero-based index of the port in the library part) and optionally " +
                "mepSystemId ({\"guid\": ...}), shape (Rectangular, Circular, Oval or UShape), width, height " +
                "(ventilation and cable carrier) and diameter (piping), in meters. Optional.");

            InIntegers(
                "FloorIndices",
                "Home story index of each element. The position stays absolute; a story that does not exist " +
                "gives an error for the element (input only 1 to use the same story for all). Optional.");

            SetOptionality(new[] { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifier of each created MEP element.");

            OutErrorMessages(
                "Error message of each MEP element (empty when it was created successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<Point3d> positions))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<string> types))
            {
                return;
            }
            if (types.Count != 1 &&
                types.Count != positions.Count)
            {
                this.AddError(
                    "The size of the input Types must be 1 or equal to the size of the input Positions.");
                return;
            }

            da.TryGetList(2, out List<string> domains);
            domains = domains ?? new List<string>();
            da.TryGetList(3, out List<Vector3d> directions);
            directions = directions ?? new List<Vector3d>();
            da.TryGetList(4, out List<Vector3d> rotations);
            rotations = rotations ?? new List<Vector3d>();
            da.TryGetList(12, out List<int> floorIndices);
            floorIndices = floorIndices ?? new List<int>();

            foreach (var pair in new (string Name, int Count)[]
                     {
                         ("Domains", domains.Count),
                         ("OrientationDirections", directions.Count),
                         ("OrientationRotations", rotations.Count),
                         ("FloorIndices", floorIndices.Count)
                     })
            {
                if (pair.Count > 1 &&
                    pair.Count != positions.Count)
                {
                    this.AddError(
                        $"The size of the input {pair.Name} must be 0, 1 or equal to the size of the input Positions.");
                    return;
                }
            }

            var fields = new ElectricalItemFields(this, 0, positions.Count);
            var defaultsFields = new ElectricalItemFields(this, 0, positions.Count);
            if (!fields.AddLibraryPartIds(da, 5, "libraryPartId") ||
                !fields.AddNumbers(da, 6, "xSize") ||
                !fields.AddNumbers(da, 7, "ySize") ||
                !fields.AddIds<AttributeGuidObject>(da, 8, "mepSystemId") ||
                !defaultsFields.AddNumbers(da, 9, "insulationThickness") ||
                !defaultsFields.AddBooleans(da, 10, "removeInsulation") ||
                !TryGetPorts(da, 11, positions.Count, out List<JArray> ports))
            {
                return;
            }

            var items = new JArray();
            for (var i = 0; i < positions.Count; i++)
            {
                var item = new JObject
                {
                    ["type"] = types[types.Count == 1 ? 0 : i],
                    ["position"] = new JObject
                    {
                        ["x"] = positions[i].X,
                        ["y"] = positions[i].Y,
                        ["z"] = positions[i].Z
                    }
                };
                if (domains.Count > 0)
                {
                    item["domain"] = domains[domains.Count == 1 ? 0 : i];
                }
                if (directions.Count > 0)
                {
                    var direction = directions[directions.Count == 1 ? 0 : i];
                    item["orientationDirection"] = new JObject
                    {
                        ["x"] = direction.X,
                        ["y"] = direction.Y,
                        ["z"] = direction.Z
                    };
                }
                if (rotations.Count > 0)
                {
                    var rotation = rotations[rotations.Count == 1 ? 0 : i];
                    item["orientationRotation"] = new JObject
                    {
                        ["x"] = rotation.X,
                        ["y"] = rotation.Y,
                        ["z"] = rotation.Z
                    };
                }
                fields.ApplyTo(item, i);
                var defaults = new JObject();
                defaultsFields.ApplyTo(defaults, i);
                if (defaults.Count > 0)
                {
                    item["defaults"] = defaults;
                }
                if (ports.Count > 0)
                {
                    item["ports"] = ports[ports.Count == 1 ? 0 : i].DeepClone();
                }
                if (floorIndices.Count > 0)
                {
                    item["floorIndex"] = floorIndices[floorIndices.Count == 1 ? 0 : i];
                }
                items.Add(item);
            }

            var parameters = new JObject { ["elementsData"] = items };

            SetCadValuesWithCreatedIds<ElementGuid>(
                CommandName,
                parameters,
                ToAddOn,
                da,
                "elements",
                "elementId");
        }

        // The keys an entry of the Ports input may have.
        private static readonly HashSet<string> PortKeys = new HashSet<string>
        {
            "portIndex",
            "mepSystemId",
            "shape",
            "width",
            "height",
            "diameter"
        };

        // Reads the optional Ports input: one JSON array of port changes for
        // every element, or one per element.
        private bool TryGetPorts(
            IGH_DataAccess da,
            int index,
            int count,
            out List<JArray> ports)
        {
            ports = new List<JArray>();
            da.TryGetList(index, out List<string> texts);
            texts = texts ?? new List<string>();
            if (texts.Count > 1 &&
                texts.Count != count)
            {
                this.AddError(
                    "The size of the input Ports must be 0, 1 or equal to the size of the input Positions.");
                return false;
            }

            foreach (var text in texts)
            {
                JArray array;
                try
                {
                    array = JArray.Parse(text ?? "");
                }
                catch (Newtonsoft.Json.JsonException exception)
                {
                    this.AddError($"The input Ports must hold JSON arrays: {exception.Message}");
                    return false;
                }
                foreach (var entry in array)
                {
                    if (!(entry is JObject port) ||
                        port["portIndex"] == null)
                    {
                        this.AddError("Each entry of the input Ports must be a JSON object with a portIndex.");
                        return false;
                    }
                    foreach (var property in port.Properties())
                    {
                        if (!PortKeys.Contains(property.Name))
                        {
                            this.AddError($"Unknown key '{property.Name}' in the input Ports.");
                            return false;
                        }
                    }
                }
                ports.Add(array);
            }
            return true;
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateMEPElements;

        public override Guid ComponentGuid =>
            new Guid("402457a8-3e3f-4717-93c3-b35affdd3d45");
    }
}
