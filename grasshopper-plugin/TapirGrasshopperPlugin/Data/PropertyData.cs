using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Linq;
using Newtonsoft.Json;
using static System.Windows.Forms.VisualStyles.VisualStyleElement;

namespace TapirGrasshopperPlugin.Data
{
    public class PropertyIdObj
    {
        public override string ToString ()
        {
            return Guid;
        }

        [JsonProperty ("guid")]
        public string Guid;
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
}
