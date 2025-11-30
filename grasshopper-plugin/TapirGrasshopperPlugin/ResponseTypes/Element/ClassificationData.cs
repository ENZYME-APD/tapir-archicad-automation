using Newtonsoft.Json;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.ResponseTypes.Element
{
    public class ClassificationIdObj : IdObj<ClassificationIdObj>
    {
    }

    public class ClassificationSystemObj
    {
        public override string ToString()
        {
            return ClassificationSystemId.ToString();
        }

        [JsonProperty("classificationSystemId")]
        public ClassificationIdObj ClassificationSystemId;
    }

    public class ClassificationObj
    {
        [JsonProperty("classificationSystemId")]
        public ClassificationIdObj ClassificationSystemId;

        [JsonProperty("classificationItemId")]
        public ClassificationIdObj ClassificationItemId;

        public override string ToString()
        {
            return ClassificationSystemId + "/" + ClassificationItemId;
        }
    }

    public class ElementClassificationObj
    {
        [JsonProperty("elementId")]
        public ElementIdObj ElementId;

        [JsonProperty(
            "classificationId",
            NullValueHandling = NullValueHandling.Ignore)]
        public ClassificationObj Classification;
    }

    public class ElementClassificationsObj
    {
        [JsonProperty("elementClassifications")]
        public List<ElementClassificationObj> ElementClassifications;
    }

    public class ClassificationSystemDetailsObj
    {
        [JsonProperty("classificationSystemId")]
        public ClassificationIdObj ClassificationSystemId;

        [JsonProperty("name")]
        public string Name;

        [JsonProperty("description")]
        public string Description;

        [JsonProperty("source")]
        public string Source;

        [JsonProperty("version")]
        public string Version;

        [JsonProperty("date")]
        public string Date;

        public override string ToString()
        {
            return Name + " " + Version;
        }
    }

    public class ClassificationItemDetailsObj
    {
        [JsonProperty("classificationItemId")]
        public ClassificationIdObj ClassificationItemId;

        [JsonProperty("id")]
        public string Id;

        [JsonProperty("name")]
        public string Name;

        [JsonProperty("description")]
        public string Description;

        [JsonProperty("children")]
        public List<ClassificationItemObj> Children;

        public override string ToString()
        {
            if (Name.Length == 0)
            {
                return Id;
            }

            return Id + " " + Name;
        }
    }

    public class ClassificationItemObj
    {
        [JsonProperty("classificationItem")]
        public ClassificationItemDetailsObj ClassificationItem;

        public static ClassificationItemDetailsObj FindClassificationItemInTree(
            List<ClassificationItemObj> branch,
            string ClassificationItemName)
        {
            foreach (var item in branch)
            {
                if (item.ClassificationItem.Id.ToLower() ==
                    ClassificationItemName)
                {
                    return item.ClassificationItem;
                }

                if (item.ClassificationItem.Children != null)
                {
                    var foundInChildren = FindClassificationItemInTree(
                        item.ClassificationItem.Children,
                        ClassificationItemName);
                    if (foundInChildren != null)
                    {
                        return foundInChildren;
                    }
                }
            }

            return null;
        }
    }

    public class AllClassificationSystems
    {
        [JsonProperty("classificationSystems")]
        public List<ClassificationSystemDetailsObj> ClassificationSystems;
    }

    public class AllClassificationItemsInSystem
    {
        [JsonProperty("classificationItems")]
        public List<ClassificationItemObj> ClassificationItems;
    }
}