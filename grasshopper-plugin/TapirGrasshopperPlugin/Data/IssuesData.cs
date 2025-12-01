using System.Collections.Generic;
using Newtonsoft.Json;
using TapirGrasshopperPlugin.ResponseTypes.IdObjects;

namespace TapirGrasshopperPlugin.Data
{
    public class IssueGuid : GuidObject<IssueGuid>
    {
    }

    public class IssueGuidItemObject
        : GuidItemObject<IssueGuid, IssueGuidItemObject>
    {
        [JsonProperty("issueId")]
        public IssueGuid IssueId;

        [JsonIgnore]
        public override IssueGuid Id
        {
            get => IssueId;
            set => IssueId = value;
        }
    }

    public class IssuesObj
        : GuidItemsObject<IssueGuid, IssueGuidItemObject, IssuesObj>
    {
        [JsonProperty("issues")]
        public List<IssueGuidItemObject> Issues;

        [JsonIgnore]
        public override List<IssueGuidItemObject> GuidItems
        {
            get => Issues;
            set => Issues = value;
        }
    }

    public class IssueDetailsObj
    {
        public override string ToString()
        {
            return IssueId + "; " + Name;
        }

        [JsonProperty("issueId")]
        public IssueGuid IssueId;

        [JsonProperty("name")]
        public string Name;
    }

    public class AllIssues
    {
        [JsonProperty("issues")]
        public List<IssueDetailsObj> Issues { get; set; }
    }

    public class IssueComment
    {
        public override string ToString()
        {
            return Author + ": " + Text;
        }

        [JsonProperty("author")]
        public string Author;

        [JsonProperty("text")]
        public string Text;
    }

    public class IssueComments
    {
        [JsonProperty("comments")]
        public List<IssueComment> Comments { get; set; }
    }
}