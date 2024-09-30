using System;
using System.Collections.Generic;
using System.Linq;
using System.Security.Policy;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Linq;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using static System.Windows.Forms.VisualStyles.VisualStyleElement;

namespace TapirGrasshopperPlugin.Data
{
    public class ElementIdObj
    {
        public override string ToString ()
        {
            return Guid;
        }

        [JsonProperty ("guid")]
        public string Guid;
    }

    public class ElementIdItemObj
    {
        public override string ToString ()
        {
            return ElementId.ToString ();
        }

        [JsonProperty ("elementId")]
        public ElementIdObj ElementId;
    }

    public class ElementsObj
    {
        [JsonProperty ("elements")]
        public List<ElementIdItemObj> Elements;
    }

    public class ElementPropertyValueObj
    {
        [JsonProperty ("elementId")]
        public ElementIdObj ElementId;

        [JsonProperty ("propertyId")]
        public PropertyIdObj PropertyId;

        [JsonProperty ("propertyValue")]
        public PropertyValueObj PropertyValue;
    }

    public class ElementPropertyValuesObj
    {
        [JsonProperty ("elementPropertyValues")]
        public List<ElementPropertyValueObj> ElementPropertyValues;
    }

    public class DetailsOfElementObj
    {
        [JsonProperty ("type")]
        public string Type;

        [JsonProperty ("details")]
        public JObject Details;
    }

    public class DetailsOfElementsObj
    {
        [JsonProperty ("detailsOfElements")]
        public List<DetailsOfElementObj> DetailsOfElements;
    }

    public class GDLParameterDetailsObj
    {
        [JsonProperty ("name")]
        public string Name;

        [JsonProperty ("index")]
        public string Index;

        [JsonProperty ("type")]
        public string Type;

        [JsonProperty ("dimension1")]
        public int Dimension1;

        [JsonProperty ("dimension2")]
        public int Dimension2;

        [JsonProperty ("value")]
        public Object Value;
    }

    public class GDLParameterListObj
    {
        public override string ToString ()
        {
            List<string> paramNames = new List<string> ();
            foreach (GDLParameterDetailsObj param in Parameters) {
                paramNames.Add (param.Name);
            }
            return String.Join (", ", paramNames);
        }

        [JsonProperty ("parameters")]
        public List<GDLParameterDetailsObj> Parameters;
    }
}
