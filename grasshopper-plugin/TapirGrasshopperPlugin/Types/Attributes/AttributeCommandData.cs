using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Types.Attributes
{
    public class AttributeToDelete
    {
        [JsonProperty("attributeType")]
        public string AttributeType;

        [JsonProperty("attributeId")]
        public AttributeGuidWrapper AttributeId;
    }

    public class DeleteAttributesParameters
    {
        [JsonProperty("attributesToDelete")]
        public List<AttributeToDelete> AttributesToDelete;
    }

    public class BuildingMaterialPhysicalProperties
    {
        [JsonProperty("thermalConductivity")]
        public double? ThermalConductivity;

        [JsonProperty("density")]
        public double? Density;

        [JsonProperty("heatCapacity")]
        public double? HeatCapacity;

        [JsonProperty("embodiedEnergy")]
        public double? EmbodiedEnergy;

        [JsonProperty("embodiedCarbon")]
        public double? EmbodiedCarbon;
    }

    public class BuildingMaterialPhysicalPropertiesItem
    {
        [JsonProperty("properties")]
        public BuildingMaterialPhysicalProperties Properties;
    }

    public class GetBuildingMaterialPhysicalPropertiesResponse
    {
        [JsonProperty("properties")]
        public List<BuildingMaterialPhysicalPropertiesItem> Properties;
    }
}
