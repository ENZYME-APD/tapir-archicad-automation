using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class ChangeHotlinkInstancesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ChangeHotlinkInstances";

        public ChangeHotlinkInstancesComponent()
            : base(
                "ChangeHotlinkInstances",
                "Change placed hotlink instances. Every input but ElementGuids is optional, " +
                "and an input left empty keeps the instances' current value.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the placed hotlink instances to change.");

            InPoints(
                "Origins",
                "Where each instance's origin lands, in the project's coordinates " +
                "(input only 1 to use the same value for all). Optional.");

            InNumbers(
                "RotationAngles",
                "Rotation about the origin, counter-clockwise, in radians " +
                "(input only 1 to use the same value for all). Optional.");

            InBooleans(
                "Mirrored",
                "Reflect the module's local X axis before the rotation " +
                "(input only 1 to use the same value for all). Optional.");

            InIntegers(
                "FloorDifferences",
                "Story offset applied to the module's stories " +
                "(input only 1 to use the same value for all). Optional.");

            InIntegers(
                "LayerIndices",
                "Move each instance to this layer " +
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

            SetOptionality(new[] { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each instance (empty when it was changed successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> elementWrappers))
            {
                return;
            }

            var elementIds = new List<ElementGuid>();
            foreach (var wrapper in elementWrappers)
            {
                var elementId = GuidObject<ElementGuid>.CreateFromWrapper(wrapper);
                if (elementId == null)
                {
                    this.AddError(
                        "Invalid element identifier in the ElementGuids input.");
                    return;
                }
                elementIds.Add(elementId);
            }

            da.TryGetList(1, out List<Point3d> origins);
            da.TryGetList(2, out List<double> rotationAngles);
            da.TryGetList(3, out List<bool> mirrored);
            da.TryGetList(4, out List<int> floorDifferences);
            da.TryGetList(5, out List<int> layerIndices);
            da.TryGetList(6, out List<bool> skipNested);
            da.TryGetList(7, out List<bool> suspendFixAngle);
            da.TryGetList(8, out List<bool> ignoreTopFloorLinks);
            da.TryGetList(9, out List<bool> relinkWallOpenings);
            da.TryGetList(10, out List<bool> adjustLevelDiffs);

            var numbers = new (string Name, string Field, List<double> Values)[]
            {
                ("RotationAngles", "rotationAngle", rotationAngles)
            };

            var integers = new (string Name, string Field, List<int> Values)[]
            {
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

            var counts = new List<(string Name, int Count)>
            {
                ("Origins", origins.Count)
            };
            foreach (var number in numbers)
            {
                counts.Add((number.Name, number.Values.Count));
            }
            foreach (var integer in integers)
            {
                counts.Add((integer.Name, integer.Values.Count));
            }
            foreach (var flag in flags)
            {
                counts.Add((flag.Name, flag.Values.Count));
            }

            foreach (var count in counts)
            {
                if (count.Count > 1 &&
                    count.Count != elementIds.Count)
                {
                    this.AddError(
                        $"The size of the input {count.Name} must be 0, 1 or equal to the size of the input ElementGuids.");
                    return;
                }
            }

            var instances = new JArray();
            for (var i = 0; i < elementIds.Count; i++)
            {
                var instance = new JObject
                {
                    ["elementId"] = new JObject { ["guid"] = elementIds[i].Guid }
                };

                if (origins.Count > 0)
                {
                    var origin = origins[origins.Count == 1 ? 0 : i];
                    instance["origin"] = new JObject
                    {
                        ["x"] = origin.X,
                        ["y"] = origin.Y,
                        ["z"] = origin.Z
                    };
                }

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

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["hotlinkInstances"] = instances },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ChangeHotlinkInstances;

        public override Guid ComponentGuid =>
            new Guid("c2b33962-4bd8-79d1-5965-d325d87d694d");
    }
}
