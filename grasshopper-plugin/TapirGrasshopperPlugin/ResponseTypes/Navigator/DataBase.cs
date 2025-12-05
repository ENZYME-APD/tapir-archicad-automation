using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.ResponseTypes.GuidObjects;

namespace TapirGrasshopperPlugin.ResponseTypes.Navigator
{
    public class DatabaseGuidObject : GuidObject<DatabaseGuidObject>
    {
    }

    public class DatabaseGuidWrapper
        : GuidWrapper<DatabaseGuidObject, DatabaseGuidWrapper>
    {
        [JsonProperty("databaseId")]
        public DatabaseGuidObject DatabaseId;

        [JsonIgnore]
        public override DatabaseGuidObject Id
        {
            get => DatabaseId;
            set => DatabaseId = value;
        }
    }

    public class DatabasesObject
        : GuidItemsObject<DatabaseGuidObject, DatabaseGuidWrapper,
            DatabasesObject>
    {
        [JsonProperty("databases")]
        public List<DatabaseGuidWrapper> Databases;

        [JsonIgnore]
        public override List<DatabaseGuidWrapper> GuidWrappers
        {
            get => Databases;
            set => Databases = value;
        }
    }
}