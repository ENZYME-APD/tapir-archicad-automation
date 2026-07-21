using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetBuildingMaterialsComponent : AttributeStructuredGetterComponent
    {
        public override string CommandName => "GetBuildingMaterials";

        public GetBuildingMaterialsComponent()
            : base(
                "GetBuildingMaterials",
                "Get the details of the given Building Material attributes.")
        {
        }

        protected override string ResponseArrayKey => "buildingMaterials";

        protected override void AddOutputs()
        {
            OutGenerics(
                "AttributeGuids",
                "Identifier of each building material attribute.");

            OutIntegers(
                "Indices",
                "Index of each building material attribute.");

            OutTexts(
                "Names",
                "Name of each building material attribute.");

            OutTexts(
                "Ids",
                "Id string of each building material.");

            OutTexts(
                "Manufacturers",
                "Manufacturer of each building material.");

            OutTexts(
                "Descriptions",
                "Description of each building material.");

            OutIntegers(
                "ConnPriorities",
                "Intersection (connection) priority of each building material.");

            OutIntegers(
                "CutFillIndices",
                "Cut fill index of each building material.");

            OutIntegers(
                "CutFillPens",
                "Cut fill pen of each building material.");

            OutIntegers(
                "CutFillBackgroundPens",
                "Cut fill background pen of each building material.");

            OutIntegers(
                "CutSurfaceIndices",
                "Cut surface index of each building material.");

            OutTexts(
                "CutFillOrientations",
                "Cut fill orientation: ProjectOrigin, ElementOrigin or FitToSkin.");

            OutNumberList(
                "ThermalConductivities",
                "Thermal conductivity of each building material.");

            OutNumberList(
                "Densities",
                "Density of each building material.");

            OutNumberList(
                "HeatCapacities",
                "Heat capacity of each building material.");

            OutNumberList(
                "EmbodiedEnergies",
                "Embodied energy of each building material.");

            OutNumberList(
                "EmbodiedCarbons",
                "Embodied carbon of each building material.");

            OutBooleans(
                "ShowUncutLines",
                "True if uncut lines are shown.");

            OutBooleans(
                "CollisionDetections",
                "True if collision detection is enabled.");

            OutTexts(
                "ErrorMessages",
                "Error message for each queried attribute (empty when successful).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var fields = new[]
            {
                "index", "name", "id", "manufacturer", "description",
                "connPriority", "cutFillIndex", "cutFillPen",
                "cutFillBackgroundPen", "cutSurfaceIndex", "cutFillOrientation",
                "thermalConductivity", "density", "heatCapacity",
                "embodiedEnergy", "embodiedCarbon", "showUncutLines",
                "collisionDetection"
            };

            var guids = new List<object>();
            var values = new List<object>[fields.Length];
            for (var f = 0; f < fields.Length; f++)
            {
                values[f] = new List<object>();
            }
            var errors = new List<string>();

            foreach (var item in items)
            {
                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    guids.Add(null);
                    for (var f = 0; f < fields.Length; f++)
                    {
                        values[f].Add(null);
                    }
                    continue;
                }

                errors.Add("");
                guids.Add(AttributeIdOf(item));
                for (var f = 0; f < fields.Length; f++)
                {
                    values[f].Add(JsonOutputHelp.Scalar(item, fields[f]));
                }
            }

            da.SetDataList(0, guids);
            for (var f = 0; f < fields.Length; f++)
            {
                da.SetDataList(f + 1, values[f]);
            }
            da.SetDataList(fields.Length + 1, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetBuildingMaterials;

        public override Guid ComponentGuid =>
            new Guid("b5601564-506b-4f93-becf-74dd99502976");
    }
}
