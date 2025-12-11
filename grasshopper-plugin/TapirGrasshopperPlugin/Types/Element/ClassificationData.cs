using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Types.Element
{
    public class ClassificationGuid : GuidObject<ClassificationGuid>
    {
    }

    public class ClassificationObj
    {
        [JsonProperty("classificationSystemId")]
        public ClassificationGuid ClassificationSystemId;

        [JsonProperty("classificationItemId")]
        public ClassificationGuid ClassificationItemId;

        public override string ToString()
        {
            return ClassificationSystemId + "/" + ClassificationItemId;
        }
    }

    public class ElementClassificationObj
    {
        [JsonProperty("elementId")]
        public ElementGuid ElementId;

        [JsonProperty(
            "classificationId",
            NullValueHandling = NullValueHandling.Ignore)]
        public ClassificationObj Classification;
    }

    public class ElementClassificationsObject
    {
        [JsonProperty("elementClassifications")]
        public List<ElementClassificationObj> ElementClassifications;
    }

    public class ClassificationSystemDetailsObj
    {
        [JsonProperty("classificationSystemId")]
        public ClassificationGuid ClassificationSystemId;

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
        public ClassificationGuid ClassificationItemId;

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