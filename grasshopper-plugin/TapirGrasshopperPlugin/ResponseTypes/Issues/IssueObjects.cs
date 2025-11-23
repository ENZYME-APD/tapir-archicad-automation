using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.ResponseTypes.Issues
{
    public class ParametersOfAttachElements
    {
        [JsonProperty("issueId")]
        public IssueIdObj IssueId;

        [JsonProperty("elements")]
        public List<ElementIdItemObj> Elements;

        [JsonProperty("type")]
        public string Type;
    }

    public class ParametersOfGetAttachedElements
    {
        [JsonProperty("issueId")]
        public IssueIdObj IssueId;

        [JsonProperty("type")]
        public string Type;
    }

    public class ParametersOfNewComment
    {
        [JsonProperty("issueId")]
        public IssueIdObj IssueId;

        [JsonProperty("author")]
        public string Author;

        [JsonProperty("text")]
        public string Text;
    }

    public class ParametersOfDelete
    {
        [JsonProperty("issueId")]
        public IssueIdObj IssueId;

        [JsonProperty("acceptAllElements")]
        public bool AcceptAllElements;
    }

    public class ParametersOfDetachElements
    {
        [JsonProperty("issueId")]
        public IssueIdObj IssueId;

        [JsonProperty("elements")]
        public List<ElementIdItemObj> Elements;
    }

    public class ParametersOfExport
    {
        [JsonProperty("issues")]
        public List<IssueIdItemObj> Issues;

        [JsonProperty("exportPath")]
        public string ExportPath;

        [JsonProperty("useExternalId")]
        public bool UseExternalId;

        [JsonProperty("alignBySurveyPoint")]
        public bool AlignBySurveyPoint;
    }

    public class ParametersOfImport
    {
        [JsonProperty("importPath")]
        public string ImportPath;

        [JsonProperty("alignBySurveyPoint")]
        public bool AlignBySurveyPoint;
    }

    public class ParametersOfNewIssue
    {
        [JsonProperty("name")]
        public string Name;
    }
}