using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetDimensionDataComponent : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetDimensionData";

        public GetDimensionDataComponent()
            : base(
                "GetDimensionData",
                "Get witness point data (coordinates, measured values) from existing dimension chains. " +
                "Witness point outputs have one branch per queried dimension element.",
                GroupNames.ElementDetails)
        {
        }

        protected override string ResponseArrayKey => "dimensionsData";

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifier of each queried dimension element.");

            OutVectors(
                "Directions",
                "Direction of each dimension line.");

            OutPoints(
                "DimensionLinePositions",
                "Position of each dimension line.");

            OutPointTree(
                "WitnessPoints",
                "2D coordinate of each witness point (one branch per dimension).");

            OutPointTree(
                "WitnessPoints3D",
                "3D coordinate of each witness point (one branch per dimension).");

            OutPointTree(
                "WitnessDimensionPositions",
                "Position of each witness point on the dimension line (one branch per dimension).");

            OutNumbers(
                "WitnessDimensionValues",
                "Measured value at each witness point (one branch per dimension).");

            OutTextTree(
                "WitnessForms",
                "Witness line form at each witness point: None, Small, Large, Fix or Unknown (one branch per dimension).");

            OutNumbers(
                "WitnessVals",
                "Witness line value at each witness point (one branch per dimension).");

            OutGenericTree(
                "WitnessBaseElementGuids",
                "Base element of each witness point (one branch per dimension).");

            OutTexts(
                "ErrorMessages",
                "Error message for each queried element (empty when successful).");

            OutBooleanTree(
                "WitnessLines",
                "True when the witness point lies on an edge of its base element rather than at a node (one branch per dimension). " +
                "Together with the next outputs these are the values the AdditionalSettings input of CreateAssociativeDimensions takes.");

            OutIntegerTree(
                "WitnessInIndices",
                "Subindex of the base element's node at each witness point (one branch per dimension).");

            OutIntegerTree(
                "WitnessSpecials",
                "Special reference marker at each witness point, e.g. a wall plane or a window hole (one branch per dimension).");

            OutIntegerTree(
                "WitnessNodeTypes",
                "Node type at each witness point, reserved by Archicad for section dimensions (one branch per dimension).");

            OutIntegerTree(
                "WitnessNodeStatuses",
                "Node status at each witness point, reserved by Archicad for section dimensions (one branch per dimension).");

            OutIntegerTree(
                "WitnessNodeIds",
                "Polygon vertex id of the base element at each witness point (one branch per dimension).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var elementGuids = new List<object>();
            var directions = new List<object>();
            var dimensionLinePositions = new List<object>();
            var witnessPoints = new DataTree<object>();
            var witnessPoints3D = new DataTree<object>();
            var witnessDimensionPositions = new DataTree<object>();
            var witnessDimensionValues = new DataTree<object>();
            var witnessForms = new DataTree<object>();
            var witnessVals = new DataTree<object>();
            var witnessBaseElementGuids = new DataTree<object>();
            var witnessLines = new DataTree<object>();
            var witnessInIndices = new DataTree<object>();
            var witnessSpecials = new DataTree<object>();
            var witnessNodeTypes = new DataTree<object>();
            var witnessNodeStatuses = new DataTree<object>();
            var witnessNodeIds = new DataTree<object>();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                witnessPoints.EnsurePath(path);
                witnessPoints3D.EnsurePath(path);
                witnessDimensionPositions.EnsurePath(path);
                witnessDimensionValues.EnsurePath(path);
                witnessForms.EnsurePath(path);
                witnessVals.EnsurePath(path);
                witnessBaseElementGuids.EnsurePath(path);
                witnessLines.EnsurePath(path);
                witnessInIndices.EnsurePath(path);
                witnessSpecials.EnsurePath(path);
                witnessNodeTypes.EnsurePath(path);
                witnessNodeStatuses.EnsurePath(path);
                witnessNodeIds.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    elementGuids.Add(null);
                    directions.Add(null);
                    dimensionLinePositions.Add(null);
                    continue;
                }

                errors.Add("");
                elementGuids.Add(ElementIdOf(item));

                var direction = item["direction"];
                if (direction == null)
                {
                    directions.Add(null);
                }
                else
                {
                    directions.Add(
                        new Vector3d(
                            direction.Value<double?>("x") ?? 0.0,
                            direction.Value<double?>("y") ?? 0.0,
                            0.0));
                }

                dimensionLinePositions.Add(
                    JsonOutputHelp.Point(item["dimensionLinePosition"]));

                if (item["witnessPoints"] is JArray points)
                {
                    foreach (var point in points)
                    {
                        witnessPoints.Add(
                            JsonOutputHelp.Point(point["coordinate"]),
                            path);
                        witnessPoints3D.Add(
                            JsonOutputHelp.Point(point["coordinate3D"]),
                            path);
                        witnessDimensionPositions.Add(
                            JsonOutputHelp.Point(point["dimensionPosition"]),
                            path);
                        witnessDimensionValues.Add(
                            JsonOutputHelp.Scalar(point, "dimensionValue"),
                            path);
                        witnessForms.Add(
                            JsonOutputHelp.Scalar(point, "witnessForm"),
                            path);
                        witnessVals.Add(
                            JsonOutputHelp.Scalar(point, "witnessVal"),
                            path);
                        witnessBaseElementGuids.Add(
                            ElementIdOf(point, "baseElementId"),
                            path);
                        witnessLines.Add(
                            JsonOutputHelp.Scalar(point, "line"),
                            path);
                        witnessInIndices.Add(
                            JsonOutputHelp.Scalar(point, "inIndex"),
                            path);
                        witnessSpecials.Add(
                            JsonOutputHelp.Scalar(point, "special"),
                            path);
                        witnessNodeTypes.Add(
                            JsonOutputHelp.Scalar(point, "nodeType"),
                            path);
                        witnessNodeStatuses.Add(
                            JsonOutputHelp.Scalar(point, "nodeStatus"),
                            path);
                        witnessNodeIds.Add(
                            JsonOutputHelp.Scalar(point, "nodeId"),
                            path);
                    }
                }
            }

            da.SetDataList(0, elementGuids);
            da.SetDataList(1, directions);
            da.SetDataList(2, dimensionLinePositions);
            da.SetDataTree(3, witnessPoints);
            da.SetDataTree(4, witnessPoints3D);
            da.SetDataTree(5, witnessDimensionPositions);
            da.SetDataTree(6, witnessDimensionValues);
            da.SetDataTree(7, witnessForms);
            da.SetDataTree(8, witnessVals);
            da.SetDataTree(9, witnessBaseElementGuids);
            da.SetDataList(10, errors);
            da.SetDataTree(11, witnessLines);
            da.SetDataTree(12, witnessInIndices);
            da.SetDataTree(13, witnessSpecials);
            da.SetDataTree(14, witnessNodeTypes);
            da.SetDataTree(15, witnessNodeStatuses);
            da.SetDataTree(16, witnessNodeIds);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetDimensionData;

        public override Guid ComponentGuid =>
            new Guid("2190e829-344e-4dd1-a748-7aafa8b695d9");
    }
}
