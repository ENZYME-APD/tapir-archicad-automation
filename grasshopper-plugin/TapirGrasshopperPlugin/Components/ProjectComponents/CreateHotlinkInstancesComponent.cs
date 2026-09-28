using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class CreateHotlinkInstancesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateHotlinkInstances";

        public CreateHotlinkInstancesComponent()
            : base(
                "CreateHotlinkInstances",
                "Place instances of hotlink module nodes. The nodes come from Hotlinks or " +
                "CreateHotlinkNodes. Every input but HotlinkNodeIds and Origins keeps the " +
                "hotlink tool's current default when it is left empty.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "HotlinkNodeIds",
                "Identifier of the node to place for each instance.");

            InPoints(
                "Origins",
                "Where each instance's origin lands, in the project's coordinates.");

            InNumbers(
                "RotationAngles",
                "Rotation about the origin, counter-clockwise, in radians, defaulting to 0 " +
                "(input only 1 to use the same value for all). Optional.");

            InBooleans(
                "Mirrored",
                "Reflect the module's local X axis before the rotation " +
                "(input only 1 to use the same value for all). Optional.");

            InIntegers(
                "FloorIndices",
                "Story each instance is placed on, defaulting to the current story " +
                "(input only 1 to use the same value for all). Optional.");

            InIntegers(
                "FloorDifferences",
                "Story offset applied to the module's stories " +
                "(input only 1 to use the same value for all). Optional.");

            InIntegers(
                "LayerIndices",
                "Layer of each instance " +
                "(input only 1 to use the same value for all). Optional.");

            InBooleans(
                "SkipNested",
                "Do not place hotlinks nested inside the module " +
                "(input only 1 to use the same value for all). Optional.");

            InBooleans(
                "SuspendFixAngle",
                "Rotate fixed-angle elements with the module " +
                "(input only 1 to use the same value for all). Optional.");

            InBooleans(
                "IgnoreTopFloorLinks",
                "Top-linked elements keep their height rather than their top story link " +
                "(input only 1 to use the same value for all). Optional.");

            InBooleans(
                "RelinkWallOpenings",
                "Relink the openings of the module's walls " +
                "(input only 1 to use the same value for all). Optional.");

            InBooleans(
                "AdjustLevelDiffs",
                "Adjust the level differences of the module's stories " +
                "(input only 1 to use the same value for all). Optional.");

            SetOptionality(new[] { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifiers of the placed instances (null for failed items).");

            OutErrorMessages(
                "Error message of each instance (empty when it was placed successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> nodeIdWrappers))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<Point3d> origins))
            {
                return;
            }

            if (origins.Count != nodeIdWrappers.Count)
            {
                this.AddError(
                    "The size of the inputs HotlinkNodeIds and Origins must be equal.");
                return;
            }

            var nodeIds = new List<HotlinkNodeGuid>();
            foreach (var wrapper in nodeIdWrappers)
            {
                var nodeId = GuidObject<HotlinkNodeGuid>.CreateFromWrapper(wrapper);
                if (nodeId == null)
                {
                    this.AddError(
                        "Invalid hotlink node identifier in the HotlinkNodeIds input.");
                    return;
                }
                nodeIds.Add(nodeId);
            }

            da.TryGetList(2, out List<double> rotationAngles);
            da.TryGetList(3, out List<bool> mirrored);
            da.TryGetList(4, out List<int> floorIndices);
            da.TryGetList(5, out List<int> floorDifferences);
            da.TryGetList(6, out List<int> layerIndices);
            da.TryGetList(7, out List<bool> skipNested);
            da.TryGetList(8, out List<bool> suspendFixAngle);
            da.TryGetList(9, out List<bool> ignoreTopFloorLinks);
            da.TryGetList(10, out List<bool> relinkWallOpenings);
            da.TryGetList(11, out List<bool> adjustLevelDiffs);

            var numbers = new (string Name, string Field, List<double> Values)[]
            {
                ("RotationAngles", "rotationAngle", rotationAngles)
            };

            var integers = new (string Name, string Field, List<int> Values)[]
            {
                ("FloorIndices", "floorIndex", floorIndices),
                ("FloorDifferences", "floorDifference", floorDifferences),
                ("LayerIndices", "layerIndex", layerIndices)
            };

            var flags = new (string Name, string Field, List<bool> Values)[]
            {
                ("Mirrored", "mirrored", mirrored),
                ("SkipNested", "skipNested", skipNested),
                ("SuspendFixAngle", "suspendFixAngle", suspendFixAngle),
                ("IgnoreTopFloorLinks", "ignoreTopFloorLinks", ignoreTopFloorLinks),
                ("RelinkWallOpenings", "relinkWallOpenings", relinkWallOpenings),
                ("AdjustLevelDiffs", "adjustLevelDiffs", adjustLevelDiffs)
            };

            foreach (var count in new (string Name, int Count)[]
                     {
                         (numbers[0].Name, numbers[0].Values.Count),
                         (integers[0].Name, integers[0].Values.Count),
                         (integers[1].Name, integers[1].Values.Count),
                         (integers[2].Name, integers[2].Values.Count),
                         (flags[0].Name, flags[0].Values.Count),
                         (flags[1].Name, flags[1].Values.Count),
                         (flags[2].Name, flags[2].Values.Count),
                         (flags[3].Name, flags[3].Values.Count),
                         (flags[4].Name, flags[4].Values.Count),
                         (flags[5].Name, flags[5].Values.Count)
                     })
            {
                if (count.Count > 1 &&
                    count.Count != nodeIds.Count)
                {
                    this.AddError(
                        $"The size of the input {count.Name} must be 0, 1 or equal to the size of the input HotlinkNodeIds.");
                    return;
                }
            }

            var instances = new JArray();
            for (var i = 0; i < nodeIds.Count; i++)
            {
                var instance = new JObject
                {
                    ["hotlinkNodeId"] = new JObject { ["guid"] = nodeIds[i].Guid },
                    ["origin"] = new JObject
                    {
                        ["x"] = origins[i].X,
                        ["y"] = origins[i].Y,
                        ["z"] = origins[i].Z
                    }
                };

                foreach (var number in numbers)
                {
                    if (number.Values.Count > 0)
                    {
                        instance[number.Field] =
                            number.Values[number.Values.Count == 1 ? 0 : i];
                    }
                }

                foreach (var integer in integers)
                {
                    if (integer.Values.Count > 0)
                    {
                        instance[integer.Field] =
                            integer.Values[integer.Values.Count == 1 ? 0 : i];
                    }
                }

                foreach (var flag in flags)
                {
                    if (flag.Values.Count > 0)
                    {
                        instance[flag.Field] =
                            flag.Values[flag.Values.Count == 1 ? 0 : i];
                    }
                }

                instances.Add(instance);
            }

            SetCadValuesWithCreatedIds<ElementGuid>(
                CommandName,
                new JObject { ["hotlinkInstances"] = instances },
                ToAddOn,
                da,
                "elements",
                "elementId");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateHotlinkInstances;

        public override Guid ComponentGuid =>
            new Guid("d86802c2-a8b3-a307-068c-7f5bcfc0e6cc");
    }
}
