using System.Collections.Generic;
using Newtonsoft.Json;

namespace TapirGrasshopperPlugin.Data
{
    public class ErrorObj
    {
        [JsonProperty ("code")]
        public int Code;

        [JsonProperty ("message")]
        public string Message;
    }

    public class ExecutionResultObj
    {
        [JsonProperty ("success")]
        public bool Success;

        [JsonProperty ("error")]
        public ErrorObj Error;
    }

    public class ExecutionResultsObj
    {
        [JsonProperty ("executionResults")]
        public List<ExecutionResultObj> ExecutionResults;
    }
}
