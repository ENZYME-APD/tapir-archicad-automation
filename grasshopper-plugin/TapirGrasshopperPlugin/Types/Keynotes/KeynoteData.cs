using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Types.Keynotes
{
    public class KeynoteFolderGuid : GuidObject<KeynoteFolderGuid>
    {
    }

    public class KeynoteFolderGuidWrapper
        : GuidWrapper<KeynoteFolderGuid, KeynoteFolderGuidWrapper>
    {
        [JsonProperty("keynoteFolderId")]
        public KeynoteFolderGuid KeynoteFolderId;

        [JsonIgnore]
        public override KeynoteFolderGuid Id
        {
            get => KeynoteFolderId;
            set => KeynoteFolderId = value;
        }
    }

    public class KeynoteFolderIdsObject
        : GuidItemsObject<KeynoteFolderGuid, KeynoteFolderGuidWrapper, KeynoteFolderIdsObject>
    {
        [JsonProperty("keynoteFolderIds")]
        public List<KeynoteFolderGuidWrapper> KeynoteFolderIds;

        [JsonIgnore]
        public override List<KeynoteFolderGuidWrapper> GuidWrappers
        {
            get => KeynoteFolderIds;
            set => KeynoteFolderIds = value;
        }
    }

    public class KeynoteItemGuid : GuidObject<KeynoteItemGuid>
    {
    }

    public class KeynoteItemGuidWrapper
        : GuidWrapper<KeynoteItemGuid, KeynoteItemGuidWrapper>
    {
        [JsonProperty("keynoteItemId")]
        public KeynoteItemGuid KeynoteItemId;

        [JsonIgnore]
        public override KeynoteItemGuid Id
        {
            get => KeynoteItemId;
            set => KeynoteItemId = value;
        }
    }

    public class KeynoteItemIdsObject
        : GuidItemsObject<KeynoteItemGuid, KeynoteItemGuidWrapper, KeynoteItemIdsObject>
    {
        [JsonProperty("keynoteItemIds")]
        public List<KeynoteItemGuidWrapper> KeynoteItemIds;

        [JsonIgnore]
        public override List<KeynoteItemGuidWrapper> GuidWrappers
        {
            get => KeynoteItemIds;
            set => KeynoteItemIds = value;
        }
    }

    public class KeynoteItemsObject
        : GuidItemsObject<KeynoteItemGuid, KeynoteItemGuidWrapper, KeynoteItemsObject>
    {
        [JsonProperty("keynoteItems")]
        public List<KeynoteItemGuidWrapper> KeynoteItems;

        [JsonIgnore]
        public override List<KeynoteItemGuidWrapper> GuidWrappers
        {
            get => KeynoteItems;
            set => KeynoteItems = value;
        }
    }
}
