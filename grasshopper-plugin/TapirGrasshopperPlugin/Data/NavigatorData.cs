using System.Collections.Generic;
using Newtonsoft.Json;
using TapirGrasshopperPlugin.ResponseTypes.IdObjects;

namespace TapirGrasshopperPlugin.Data
{
    public class NavigatorGuid : GuidObject<NavigatorGuid>
    {
    }

    public class NavigatorGuidItemObject
        : GuidItemObject<NavigatorGuid, NavigatorGuidItemObject>
    {
        [JsonProperty("navigatorItemId")]
        public NavigatorGuid NavigatorId;

        [JsonIgnore]
        public override NavigatorGuid Id
        {
            get => NavigatorId;
            set => NavigatorId = value;
        }
    }

    public class NavigatorItemsObject
        : GuidItemsObject<NavigatorGuid, NavigatorGuidItemObject,
            NavigatorItemsObject>
    {
        [JsonProperty("navigatorItemIds")]
        public List<NavigatorGuidItemObject> NavigatorItemIds;

        [JsonIgnore]
        public override List<NavigatorGuidItemObject> GuidItems
        {
            get => NavigatorItemIds;
            set => NavigatorItemIds = value;
        }
    }

    public class DatabaseGuidObject : GuidObject<DatabaseGuidObject>
    {
    }

    public class DatabaseGuidItemObject
        : GuidItemObject<DatabaseGuidObject, DatabaseGuidItemObject>
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

    public class DatabasesObj
        : GuidItemsObject<DatabaseGuidObject, DatabaseGuidItemObject,
            DatabasesObj>
    {
        [JsonProperty("databases")]
        public List<DatabaseGuidItemObject> Databases;

        [JsonIgnore]
        public override List<DatabaseGuidItemObject> GuidItems
        {
            get => Databases;
            set => Databases = value;
        }
    }
}