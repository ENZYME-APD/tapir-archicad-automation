using System.Collections.Generic;
using Newtonsoft.Json;

namespace TapirGrasshopperPlugin.Data
{
    public class IssueIdObj : IdObj<IssueIdObj> { }

    public class IssueIdItemObj : IdItemObj<IssueIdObj, IssueIdItemObj>
    {
        [JsonProperty ("issueId")]
        public IssueIdObj IssueId;

        [JsonIgnore]
        public override IssueIdObj Id
        {
            get { return IssueId; }
            set { IssueId = value; }
        }
    }

    public class IssuesObj : IdsObj<IssueIdObj, IssueIdItemObj, IssuesObj>
    {
        [JsonProperty ("issues")]
        public List<IssueIdItemObj> Issues;

        [JsonIgnore]
        public override List<IssueIdItemObj> Ids
        {
            get { return Issues; }
            set { Issues = value; }
        }
    }

    public class IssueDetailsObj
    {
        public override string ToString ()
        {
            return IssueId.ToString () + "; " + Name;
        }

        [JsonProperty ("issueId")]
        public IssueIdObj IssueId;

        [JsonProperty ("name")]
        public string Name;
    }

    public class AllIssues
    {
        [JsonProperty ("issues")]
        public List<IssueDetailsObj> Issues { get; set; }
    }

    public class IssueComment
    {
        public override string ToString ()
        {
            return Author + ": " + Text;
        }

        [JsonProperty ("author")]
        public string Author;

        [JsonProperty ("text")]
        public string Text;
    }

    public class IssueComments
    {
        [JsonProperty ("comments")]
        public List<IssueComment> Comments { get; set; }
    }
}
