using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Data
{
    using ElementIdObj = IdObj;
    using ClassificationIdObj = IdObj;

    public class ClassificationSystemObj
    {
        public override string ToString ()
        {
            return ClassificationSystemId.ToString ();
        }

        [JsonProperty ("classificationSystemId")]
        public ClassificationIdObj ClassificationSystemId;
    }

    public class ClassificationObj
    {
        public override string ToString ()
        {
            return ClassificationSystemId.ToString () + "/" + ClassificationItemId.ToString ();
        }

        [JsonProperty ("classificationSystemId")]
        public ClassificationIdObj ClassificationSystemId;

        [JsonProperty ("classificationItemId")]
        public ClassificationIdObj ClassificationItemId;
    }

    public class ElementClassificationObj
    {
        [JsonProperty ("elementId")]
        public ElementIdObj ElementId;

        [JsonProperty ("classificationId")]
        public ClassificationObj Classification;
    }

    public class ElementClassificationsObj
    {
        [JsonProperty ("elementClassifications")]
        public List<ElementClassificationObj> ElementClassifications;
    }

    public class ClassificationSystemDetailsObj
    {
        [JsonProperty ("classificationSystemId")]
        public ClassificationIdObj ClassificationSystemId;

        [JsonProperty ("name")]
        public string Name;

        [JsonProperty ("description")]
        public string Description;

        [JsonProperty ("source")]
        public string Source;

        [JsonProperty ("version")]
        public string Version;

        [JsonProperty ("date")]
        public string Date;
    }

    public class ClassificationItemDetailsObj
    {
        [JsonProperty ("classificationItemId")]
        public ClassificationIdObj ClassificationItemId;

        [JsonProperty ("id")]
        public string Id;

        [JsonProperty ("name")]
        public string Name;

        [JsonProperty ("description")]
        public string Description;
        
        [JsonProperty ("children")]
        public List<ClassificationItemObj> Children;
    }

    public class ClassificationItemObj
    {
        [JsonProperty ("classificationItem")]
        public ClassificationItemDetailsObj ClassificationItem;
    }

    public class AllClassificationSystems
    {
        [JsonProperty ("classificationSystems")]
        public List<ClassificationSystemDetailsObj> ClassificationSystems;
    }

    public class AllClassificationItemsInSystem
    {
        [JsonProperty ("classificationItems")]
        public List<ClassificationItemObj> ClassificationItems;
    }
}
