using System.Collections.Generic;
using Newtonsoft.Json;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.Data
{
    public class NavItemId
    {
        [JsonProperty("guid")]
        public string Guid { get; set; }
    }

    public class NavItemIdArrayItem
    {
        [JsonProperty("navigatorItemId")]
        public NavItemId NavItemId { get; set; }
    }

    public class NavItemIds : List<NavItemIdArrayItem>
    {
    }

    public class NavItemInputStructure
    {
        [JsonProperty("navigatorItemIds")]
        public NavItemIds NavItemIds { get; set; }
    }


    public class NavigatorIdObj : IdObj<NavigatorIdObj>
    {
    }

    public class NavigatorIdItemObj
        : IdItemObj<NavigatorIdObj, NavigatorIdItemObj>
    {
        [JsonProperty("navigatorItemId")]
        public NavigatorIdObj NavigatorId;

        [JsonIgnore]
        public override NavigatorIdObj Id
        {
            get { return NavigatorId; }
            set { NavigatorId = value; }
        }
    }

    public class NavigatorItemIdsObj
        : IdsObj<NavigatorIdObj, NavigatorIdItemObj, NavigatorItemIdsObj>
    {
        [JsonProperty("navigatorItemIds")]
        public List<NavigatorIdItemObj> NavigatorItemIds;

        [JsonIgnore]
        public override List<NavigatorIdItemObj> Ids
        {
            get { return NavigatorItemIds; }
            set { NavigatorItemIds = value; }
        }
    }

    public class DatabaseIdObj : IdObj<DatabaseIdObj>
    {
    }

    public class DatabaseIdItemObj : IdItemObj<DatabaseIdObj, DatabaseIdItemObj>
    {
        [JsonProperty("databaseId")]
        public DatabaseIdObj DatabaseId;

        [JsonIgnore]
        public override DatabaseIdObj Id
        {
            get { return DatabaseId; }
            set { DatabaseId = value; }
        }
    }

    public class DatabasesObj
        : IdsObj<DatabaseIdObj, DatabaseIdItemObj, DatabasesObj>
    {
        [JsonProperty("databases")]
        public List<DatabaseIdItemObj> Databases;

        [JsonIgnore]
        public override List<DatabaseIdItemObj> Ids
        {
            get { return Databases; }
            set { Databases = value; }
        }
    }
}