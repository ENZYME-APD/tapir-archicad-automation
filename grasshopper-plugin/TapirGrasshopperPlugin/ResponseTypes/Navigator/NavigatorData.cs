using System.Collections.Generic;
using Newtonsoft.Json;
using TapirGrasshopperPlugin.ResponseTypes.IdObjects;

namespace TapirGrasshopperPlugin.ResponseTypes.Navigator
{
    public class NavigatorGuid : GuidObject<NavigatorGuid>
    {
    }

    public class NavigatorGuidWrapper
        : GuidWrapper<NavigatorGuid, NavigatorGuidWrapper>
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
        : GuidItemsObject<NavigatorGuid, NavigatorGuidWrapper,
            NavigatorItemsObject>
    {
        [JsonProperty("navigatorItemIds")]
        public List<NavigatorGuidWrapper> NavigatorItemIds;

        [JsonIgnore]
        public override List<NavigatorGuidWrapper> GuidItems
        {
            get => NavigatorItemIds;
            set => NavigatorItemIds = value;
        }
    }

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

    public class DatabasesObj
        : GuidItemsObject<DatabaseGuidObject, DatabaseGuidWrapper, DatabasesObj>
    {
        [JsonProperty("databases")]
        public List<DatabaseGuidWrapper> Databases;

        [JsonIgnore]
        public override List<DatabaseGuidWrapper> GuidItems
        {
            get => Databases;
            set => Databases = value;
        }
    }
}