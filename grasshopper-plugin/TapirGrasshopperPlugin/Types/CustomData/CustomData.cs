using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Types.CustomData
{
    // A key-value pair as the element user data commands store it
    // (SetElementUserData) and answer it (GetElementUserData).
    public class KeyValuePairObj
    {
        [JsonProperty("key")]
        public string Key;

        [JsonProperty("value")]
        public string Value;
    }

    // One project data of SetProjectData: text saved in the project under a
    // name.
    public class ProjectDataValue
    {
        [JsonProperty("name")]
        public string Name;

        [JsonProperty("content")]
        public string Content;
    }

    public class SetProjectDataParameters
    {
        [JsonProperty("values")]
        public List<ProjectDataValue> Values;
    }

    // GetProjectData and DeleteProjectData.
    public class ProjectDataNamesParameters
    {
        [JsonProperty("names")]
        public List<string> Names;
    }

    // One element of SetElementUserData: the pairs to store on it and the keys
    // to remove from it. The command merges both into the pairs the element
    // already holds; a left-out list is omitted from the JSON, because the
    // command accepts no null there.
    public class ElementUserDataToSet
    {
        [JsonProperty("elementId")]
        public ElementGuid ElementId;

        [JsonProperty(
            "userData",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<KeyValuePairObj> UserData;

        [JsonProperty(
            "keysToDelete",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<string> KeysToDelete;
    }

    public class SetElementUserDataParameters
    {
        [JsonProperty("elementUserData")]
        public List<ElementUserDataToSet> ElementUserData;
    }

    public class GetElementUserDataParameters
    {
        [JsonProperty("elements")]
        public List<ElementGuidWrapper> Elements;

        [JsonProperty(
            "keys",
            NullValueHandling = NullValueHandling.Ignore)]
        public List<string> Keys;
    }
}
