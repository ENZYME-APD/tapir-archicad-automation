using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
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

            SetOptionality(new[] { 2, 3, 4 });
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

            foreach (var pair in new (string Name, int Count)[]
                     {
                         ("Domains", domains.Count),
                         ("OrientationDirections", directions.Count),
                         ("OrientationRotations", rotations.Count)
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

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateMEPElements;

        public override Guid ComponentGuid =>
            new Guid("402457a8-3e3f-4717-93c3-b35affdd3d45");
    }
}
