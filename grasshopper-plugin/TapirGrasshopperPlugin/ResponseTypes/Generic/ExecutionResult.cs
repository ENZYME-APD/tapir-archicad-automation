using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace TapirGrasshopperPlugin.ResponseTypes.Generic
{
    public class ExecutionResultBase
    {
        [JsonProperty ("success")]
        public bool Success { get; set; }

        public static ExecutionResultBase Deserialize (JObject jObject)
        {
            var baseResult = jObject.ToObject<ExecutionResultBase> ();

            if (baseResult.Success)
            {
                return jObject.ToObject<SuccessfulExecutionResult> ();
            }
            else
            {
                return jObject.ToObject<FailedExecutionResult> ();
            }
        }

        public virtual string ResultMessage () => "Success.";
    }

    public class SuccessfulExecutionResult : ExecutionResultBase
    {
    }

    public class FailedExecutionResult : ExecutionResultBase
    {
        [JsonProperty ("error")]
        public Error Error { get; set; }

        public override string ResultMessage () => Error.ToString();
    }
}
