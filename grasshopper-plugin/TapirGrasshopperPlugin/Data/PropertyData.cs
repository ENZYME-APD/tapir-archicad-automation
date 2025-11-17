using System.Collections.Generic;
using System.ComponentModel;
using Newtonsoft.Json;

namespace TapirGrasshopperPlugin.Data
{
    public class PropertyIdObj : IdObj<PropertyIdObj> { }

    public class PropertyIdItemObj : IdItemObj<PropertyIdObj, PropertyIdItemObj>
    {
        [JsonProperty ("propertyId")]
        public PropertyIdObj PropertyId;

        [JsonIgnore]
        public override PropertyIdObj Id
        {
            get { return PropertyId; }
            set { PropertyId = value; }
        }
    }

    public class PropertiesObj : IdsObj<PropertyIdObj, PropertyIdItemObj, PropertiesObj>
    {
        [JsonProperty ("properties")]
        public List<PropertyIdItemObj> Properties;

        [JsonIgnore]
        public override List<PropertyIdItemObj> Ids
        {
            get { return Properties; }
            set { Properties = value; }
        }
    }

    public class PropertyValueObj
    {
        [JsonProperty ("value")]
        public string Value;
    }

    public class PropertyDetailsObj
    {
        public override string ToString ()
        {
            return PropertyId.ToString () + "; " + PropertyGroupName + "; " + PropertyName;
        }

        [JsonProperty ("propertyId")]
        public PropertyIdObj PropertyId;

        [JsonProperty ("propertyGroupName")]
        public string PropertyGroupName;

        [JsonProperty ("propertyName")]
        public string PropertyName;
    }

    public class AllProperties
    {
        [JsonProperty ("properties")]
        public List<PropertyDetailsObj> Properties { get; set; }
    }

    public class PropertyValue
    {
        [JsonProperty ("value")]
        public string Value;
    }

    public class PropertyValueOrError
    {
        [JsonProperty ("propertyValue", DefaultValueHandling = DefaultValueHandling.Populate)]
        [DefaultValue (null)]
        public PropertyValue PropertyValue;
    }

    public class PropertyValues
    {
        [JsonProperty ("propertyValues", DefaultValueHandling = DefaultValueHandling.Populate)]
        [DefaultValue (null)]
        public List<PropertyValueOrError> PropertyValuesOrErrors;
    }

    public class PropertyValuesForElements
    {
        [JsonProperty ("propertyValuesForElements")]
        public List<PropertyValues> PropertyValuesOrErrors { get; set; }
    }
}
