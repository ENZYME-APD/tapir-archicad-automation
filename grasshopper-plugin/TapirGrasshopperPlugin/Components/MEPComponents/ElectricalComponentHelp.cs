using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    // Shared code of the electrical components (panels, circuits, devices and
    // the electrical project settings, available from Archicad 30).
    internal static class ElectricalComponentHelp
    {
        // A command that fails as a whole (for example on Archicad versions
        // before 30) answers {"error": {...}} as its complete response, which
        // the connection reports as a successful call. Reports that error on
        // the component, so it is not mistaken for an empty result.
        public static bool ReportResponseError(
            GH_ActiveObject component,
            JObject response)
        {
            var error = response?["error"];
            if (error == null)
            {
                return false;
            }

            component.AddError(
                error["message"]?.ToString() ?? "Unknown error.");
            return true;
        }

        // Reads a list of element identifiers; an unconnected optional input
        // yields an empty list. Returns false on an invalid identifier.
        public static bool TryGetElementIds(
            GH_Component component,
            IGH_DataAccess da,
            int index,
            out List<ElementGuid> ids)
        {
            ids = new List<ElementGuid>();
            da.TryGetList(index, out List<GH_ObjectWrapper> wrappers);
            foreach (var wrapper in wrappers ?? new List<GH_ObjectWrapper>())
            {
                var id = GuidObject<ElementGuid>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    component.AddError(
                        $"Invalid element identifier in the {component.Params.Input[index].Name} input.");
                    return false;
                }
                ids.Add(id);
            }
            return true;
        }

        // The Elements array of the commands: [{"elementId": {"guid": ...}}].
        public static JArray ElementsArray(
            IEnumerable<ElementGuid> ids)
        {
            var elements = new JArray();
            foreach (var id in ids)
            {
                elements.Add(
                    new JObject
                    {
                        ["elementId"] = new JObject { ["guid"] = id.Guid }
                    });
            }
            return elements;
        }

        public static JObject Coordinate(
            Point3d point)
        {
            return new JObject
            {
                ["x"] = point.X,
                ["y"] = point.Y,
                ["z"] = point.Z
            };
        }

        public static JObject Coordinate(
            Vector3d vector)
        {
            return new JObject
            {
                ["x"] = vector.X,
                ["y"] = vector.Y,
                ["z"] = vector.Z
            };
        }

        public static object PointOf(
            JToken coordinate)
        {
            if (coordinate == null || coordinate.Type == JTokenType.Null)
            {
                return null;
            }
            return new Point3d(
                coordinate.Value<double?>("x") ?? 0.0,
                coordinate.Value<double?>("y") ?? 0.0,
                coordinate.Value<double?>("z") ?? 0.0);
        }

        public static object VectorOf(
            JToken coordinate)
        {
            if (coordinate == null || coordinate.Type == JTokenType.Null)
            {
                return null;
            }
            return new Vector3d(
                coordinate.Value<double?>("x") ?? 0.0,
                coordinate.Value<double?>("y") ?? 0.0,
                coordinate.Value<double?>("z") ?? 0.0);
        }

        // An {"guid": ...} element id object to the wrapper the element
        // components hand out; null when it is missing.
        public static ElementGuidWrapper ElementIdOf(
            JToken idObject)
        {
            var guid = idObject?["guid"]?.ToString();
            if (guid == null)
            {
                return null;
            }
            return new ElementGuidWrapper
            {
                ElementId = new ElementGuid { Guid = guid }
            };
        }

        // Adds the {"elementId": ...} items of an Elements array to a branch.
        public static void AddElementIds(
            JToken elements,
            DataTree<object> tree,
            GH_Path path)
        {
            if (elements is JArray items)
            {
                foreach (var item in items)
                {
                    tree.Add(ElementIdOf(item?["elementId"]), path);
                }
            }
        }

        // An ElectricalLoads object: the total to a list, the loads of the
        // phases A, B and C to a branch of a tree.
        public static void AddLoads(
            JToken loads,
            List<object> totals,
            DataTree<object> perPhase,
            GH_Path path)
        {
            totals.Add(JsonOutputHelp.Scalar(loads, "total"));
            perPhase.EnsurePath(path);
            if (loads == null)
            {
                return;
            }
            perPhase.Add(JsonOutputHelp.Scalar(loads, "phaseA"), path);
            perPhase.Add(JsonOutputHelp.Scalar(loads, "phaseB"), path);
            perPhase.Add(JsonOutputHelp.Scalar(loads, "phaseC"), path);
        }
    }

    // The optional inputs of a batch command whose items are built from
    // several list inputs: each input takes no value (the field is left out),
    // one value (used for every item) or one value per item.
    internal sealed class ElectricalItemFields
    {
        private readonly GH_Component component;
        private readonly string countInputName;
        private readonly int itemCount;
        private readonly List<(string JsonKey, List<JToken> Tokens)> fields =
            new List<(string JsonKey, List<JToken> Tokens)>();

        public ElectricalItemFields(
            GH_Component component,
            int countInputIndex,
            int itemCount)
        {
            this.component = component;
            countInputName = component.Params.Input[countInputIndex].Name;
            this.itemCount = itemCount;
        }

        private string InputName(
            int index)
        {
            return component.Params.Input[index].Name;
        }

        private bool CheckSize(
            string inputName,
            int count)
        {
            if (count > 1 && count != itemCount)
            {
                component.AddError(
                    $"The size of the input {inputName} must be 0, 1 or equal to the size of the input {countInputName}.");
                return false;
            }
            return true;
        }

        private bool Add<T>(
            IGH_DataAccess da,
            int index,
            string jsonKey,
            Func<T, JToken> convert)
        {
            da.TryGetList(index, out List<T> values);
            values = values ?? new List<T>();
            if (!CheckSize(InputName(index), values.Count))
            {
                return false;
            }
            if (values.Count == 0)
            {
                return true;
            }

            var tokens = new List<JToken>();
            foreach (var value in values)
            {
                var token = convert(value);
                if (token == null)
                {
                    component.AddError(
                        $"Invalid value in the {InputName(index)} input.");
                    return false;
                }
                tokens.Add(token);
            }
            fields.Add((jsonKey, tokens));
            return true;
        }

        public bool AddNumbers(
            IGH_DataAccess da,
            int index,
            string jsonKey)
        {
            return Add<double>(da, index, jsonKey, v => v);
        }

        public bool AddIntegers(
            IGH_DataAccess da,
            int index,
            string jsonKey)
        {
            return Add<int>(da, index, jsonKey, v => v);
        }

        // The integers that may not be negative (e.g. the apparent power).
        public bool AddNonNegativeIntegers(
            IGH_DataAccess da,
            int index,
            string jsonKey)
        {
            return Add<int>(da, index, jsonKey, v => v < 0 ? null : (JToken)v);
        }

        public bool AddBooleans(
            IGH_DataAccess da,
            int index,
            string jsonKey)
        {
            return Add<bool>(da, index, jsonKey, v => v);
        }

        public bool AddTexts(
            IGH_DataAccess da,
            int index,
            string jsonKey)
        {
            // An empty text is a valid value (e.g. "no wiring type").
            return Add<string>(da, index, jsonKey, v => v ?? "");
        }

        public bool AddPoints(
            IGH_DataAccess da,
            int index,
            string jsonKey)
        {
            return Add<Point3d>(da, index, jsonKey, v => ElectricalComponentHelp.Coordinate(v));
        }

        // Element identifiers, written as {"guid": ...}.
        public bool AddElementIds(
            IGH_DataAccess da,
            int index,
            string jsonKey)
        {
            return Add<GH_ObjectWrapper>(da, index, jsonKey, v =>
            {
                var id = GuidObject<ElementGuid>.CreateFromWrapper(v);
                return id == null ? null : new JObject { ["guid"] = id.Guid };
            });
        }

        // Library part guids as texts (as GetAvailableLibraryParts lists them),
        // written as {"guid": ...}.
        public bool AddLibraryPartIds(
            IGH_DataAccess da,
            int index,
            string jsonKey)
        {
            return Add<string>(da, index, jsonKey, v =>
                Guid.TryParse(v, out var guid)
                    ? new JObject { ["guid"] = guid.ToString("D") }
                    : null);
        }

        // The MEP orientation needs both of its vectors, so the two inputs
        // have to be given together.
        public bool AddOrientations(
            IGH_DataAccess da,
            int directionsIndex,
            int rotationsIndex,
            string jsonKey)
        {
            da.TryGetList(directionsIndex, out List<Vector3d> directions);
            directions = directions ?? new List<Vector3d>();
            da.TryGetList(rotationsIndex, out List<Vector3d> rotations);
            rotations = rotations ?? new List<Vector3d>();

            if (!CheckSize(InputName(directionsIndex), directions.Count) ||
                !CheckSize(InputName(rotationsIndex), rotations.Count))
            {
                return false;
            }
            if (directions.Count == 0 && rotations.Count == 0)
            {
                return true;
            }
            if (directions.Count == 0 || rotations.Count == 0)
            {
                component.AddError(
                    $"The inputs {InputName(directionsIndex)} and {InputName(rotationsIndex)} must be given together.");
                return false;
            }

            var tokens = new List<JToken>();
            var count = Math.Max(directions.Count, rotations.Count);
            for (var i = 0; i < count; i++)
            {
                tokens.Add(
                    new JObject
                    {
                        ["direction"] = ElectricalComponentHelp.Coordinate(
                            directions[directions.Count == 1 ? 0 : i]),
                        ["rotation"] = ElectricalComponentHelp.Coordinate(
                            rotations[rotations.Count == 1 ? 0 : i])
                    });
            }
            fields.Add((jsonKey, tokens));
            return true;
        }

        // Writes the field values of the item with the given index.
        public void ApplyTo(
            JObject item,
            int index)
        {
            foreach (var (jsonKey, tokens) in fields)
            {
                item[jsonKey] = tokens.Count == 1
                    ? tokens[0].DeepClone()
                    : tokens[index].DeepClone();
            }
        }
    }

    // The placement fields shared by the electrical panels and devices:
    // orientation, base rotation, mirroring, library part and size. The
    // anchor point is not among them, the commands name it differently.
    internal static class ElectricalPlacementInputs
    {
        public const int Count = 7;

        public static void Add(
            Component component,
            int firstIndex)
        {
            component.InVectors(
                "OrientationDirections",
                "Direction vector of the orientation of each element; give OrientationRotations too " +
                "(input only 1 to use the same direction for all). Optional.");

            component.InVectors(
                "OrientationRotations",
                "Rotation vector of the orientation of each element; give OrientationDirections too " +
                "(input only 1 to use the same rotation for all). Optional.");

            component.InNumbers(
                "Rotations",
                "Base rotation of each element in radians (input only 1 to use the same rotation for all). Optional.");

            component.InBooleans(
                "IsReflected",
                "Mirror each element (input only 1 to use the same value for all). Optional.");

            component.InTexts(
                "LibraryPartGuids",
                "Guid of the library part of each element, as listed by GetAvailableLibraryParts " +
                "(input only 1 to use the same library part for all). Optional.");

            component.InNumbers(
                "XSizes",
                "Size of each element along its local x axis; fails for library parts that calculate their own size " +
                "(input only 1 to use the same size for all). Optional.");

            component.InNumbers(
                "YSizes",
                "Size of each element along its local y axis; fails for library parts that calculate their own size " +
                "(input only 1 to use the same size for all). Optional.");

            var indices = new int[Count];
            for (var i = 0; i < Count; i++)
            {
                indices[i] = firstIndex + i;
            }
            component.SetOptionality(indices);
        }

        public static bool TryRead(
            ElectricalItemFields fields,
            IGH_DataAccess da,
            int firstIndex)
        {
            return fields.AddOrientations(da, firstIndex, firstIndex + 1, "orientation") &&
                   fields.AddNumbers(da, firstIndex + 2, "rotation") &&
                   fields.AddBooleans(da, firstIndex + 3, "isReflected") &&
                   fields.AddLibraryPartIds(da, firstIndex + 4, "libraryPartId") &&
                   fields.AddNumbers(da, firstIndex + 5, "xSize") &&
                   fields.AddNumbers(da, firstIndex + 6, "ySize");
        }
    }

    // The placement outputs of the electrical panel and device getters, one
    // value per queried element (null for the failed ones).
    internal sealed class ElectricalPlacementOutputs
    {
        public const int Count = 8;

        private readonly List<object> anchorPoints = new List<object>();
        private readonly List<object> directions = new List<object>();
        private readonly List<object> rotationVectors = new List<object>();
        private readonly List<object> rotations = new List<object>();
        private readonly List<object> isReflected = new List<object>();
        private readonly List<object> libraryPartGuids = new List<object>();
        private readonly List<object> xSizes = new List<object>();
        private readonly List<object> ySizes = new List<object>();

        public static void AddOutputs(
            Component component)
        {
            component.OutPoints(
                "AnchorPoints",
                "Anchor point of each element.");

            component.OutVectors(
                "OrientationDirections",
                "Direction vector of the orientation of each element.");

            component.OutVectors(
                "OrientationRotations",
                "Rotation vector of the orientation of each element.");

            component.OutNumberList(
                "Rotations",
                "Base rotation of each element in radians.");

            component.OutBooleans(
                "IsReflected",
                "True if the element is mirrored.");

            component.OutTexts(
                "LibraryPartGuids",
                "Guid of the library part of each element.");

            component.OutNumberList(
                "XSizes",
                "Size of each element along its local x axis.");

            component.OutNumberList(
                "YSizes",
                "Size of each element along its local y axis.");
        }

        // Adds the placement of an element; a null item adds nulls.
        public void Add(
            JToken item)
        {
            anchorPoints.Add(ElectricalComponentHelp.PointOf(item?["anchorPoint"]));
            directions.Add(ElectricalComponentHelp.VectorOf(item?["orientation"]?["direction"]));
            rotationVectors.Add(ElectricalComponentHelp.VectorOf(item?["orientation"]?["rotation"]));
            rotations.Add(JsonOutputHelp.Scalar(item, "rotation"));
            isReflected.Add(JsonOutputHelp.Scalar(item, "isReflected"));
            libraryPartGuids.Add(JsonOutputHelp.GuidOf(item?["libraryPartId"]));
            xSizes.Add(JsonOutputHelp.Scalar(item, "xSize"));
            ySizes.Add(JsonOutputHelp.Scalar(item, "ySize"));
        }

        public void SetData(
            IGH_DataAccess da,
            int firstIndex)
        {
            da.SetDataList(firstIndex, anchorPoints);
            da.SetDataList(firstIndex + 1, directions);
            da.SetDataList(firstIndex + 2, rotationVectors);
            da.SetDataList(firstIndex + 3, rotations);
            da.SetDataList(firstIndex + 4, isReflected);
            da.SetDataList(firstIndex + 5, libraryPartGuids);
            da.SetDataList(firstIndex + 6, xSizes);
            da.SetDataList(firstIndex + 7, ySizes);
        }
    }
}
