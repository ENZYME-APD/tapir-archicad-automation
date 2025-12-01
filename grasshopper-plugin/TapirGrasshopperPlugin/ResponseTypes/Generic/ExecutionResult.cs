using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.ResponseTypes.Generic
{
    public class ExecutionResultsResponse
    {
        [JsonProperty("executionResults")]
        public List<ExecutionResult> ExecutionResults { get; set; }

        public ExecutionResultsResponse()
        {
            ExecutionResults = new List<ExecutionResult>();
        }

        public static ExecutionResultsResponse Deserialize(
            JObject jObject)
        {
            var response = new ExecutionResultsResponse();

            var results = jObject["executionResults"] as JArray;

            if (results == null)
            {
                return response;
            }

            foreach (var token in results)
            {
                var resultItem = token as JObject;
                if (resultItem == null)
                {
                    continue;
                }

                response.ExecutionResults.Add(
                    ExecutionResult.Deserialize(resultItem));
            }

            return response;
        }
    }

    public class ExecutionResultResponse
    {
        [JsonProperty("executionResult")]
        public ExecutionResult ExecutionResult { get; set; }

        [JsonProperty("conflicts")]
        public List<ConflictResponse> ConflictResponses { get; set; }

        public static ExecutionResultResponse Deserialize(
            JObject jObject)
        {
            var response = new ExecutionResultResponse();

            var resultProperty = jObject["executionResult"] as JObject;

            if (resultProperty != null)
            {
                response.ExecutionResult =
                    ExecutionResult.Deserialize(resultProperty);
            }

            var conflicts = jObject["conflicts"];

            if (conflicts != null && conflicts.Type != JTokenType.Null)
            {
                response.ConflictResponses =
                    conflicts.ToObject<List<ConflictResponse>>();
            }
            else
            {
                response.ConflictResponses = new List<ConflictResponse>();
            }

            return response;
        }

        public string Message()
        {
            var builder = new StringBuilder();

            builder.AppendLine(
                $"{ExecutionResult?.Message() ?? "No execution result."}");

            if (ConflictResponses == null || ConflictResponses == null ||
                !ConflictResponses.Any())
            {
                return builder.ToString();
            }

            builder.AppendLine("Conflicts found:");

            foreach (var conflict in ConflictResponses)
            {
                builder.AppendLine(conflict.ToString());
            }

            return builder.ToString();
        }
    }

    public class ConflictResponse
    {
        [JsonProperty("elementId")]
        public ElementGuid ElementId { get; set; }

        [JsonProperty("user")]
        public User User { get; set; }

        public override string ToString() =>
            $"Element {ElementId.Guid} is reserved by user {User.Name} (ID: {User.Id}).";
    }

    public class User
    {
        [JsonProperty("userId")]
        public int Id { get; set; }

        [JsonProperty("userName")]
        public string Name { get; set; }
    }

    public class ExecutionResult
    {
        public static string Doc =>
            "Result of the node's logic's execution. " +
            "Success on successful execution, a list of error messages on failed execution.";

        [JsonProperty("success")]
        public bool Success { get; set; }

        public static ExecutionResult Deserialize(
            JObject jObject)
        {
            var baseResult = jObject.ToObject<ExecutionResult>();

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

    public class SuccessfulExecutionResult : ExecutionResult
    {
    }

    public class FailedExecutionResult : ExecutionResult
    {
        [JsonProperty("error")]
        public Error Error { get; set; }

        public override string Message() => Error.ToString();
    }
}