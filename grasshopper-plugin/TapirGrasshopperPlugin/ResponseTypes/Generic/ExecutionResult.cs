using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.ResponseTypes.Generic
{
    public class ExecutionResultsResponse
    {
        public static string Doc => "A list of execution results.";

        [JsonProperty("executionResults")]
        public List<ExecutionResultBase> ExecutionResults { get; set; }
    }

    public class ExecutionResultBase
    {
        public static string Doc =>
            "Result of the node's logic's execution. " +
            "Success on successful execution, a list of error messages on failed execution.";

        [JsonProperty("success")]
        public bool Success { get; set; }

        public static ExecutionResultBase Deserialize(
            JObject jObject)
        {
            var baseResult = jObject.ToObject<ExecutionResultBase>();

            if (baseResult.Success)
            {
                return jObject.ToObject<SuccessfulExecutionResult>();
            }
            else
            {
                return jObject.ToObject<FailedExecutionResult>();
            }
        }

        public virtual string Message() => "Success.";
    }

    public class SuccessfulExecutionResult : ExecutionResultBase
    {
    }

    public class FailedExecutionResult : ExecutionResultBase
    {
        [JsonProperty("error")]
        public Error Error { get; set; }

        public override string Message() => Error.ToString();
    }
}