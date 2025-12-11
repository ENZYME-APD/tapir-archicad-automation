using Newtonsoft.Json;
using System;

namespace TapirGrasshopperPlugin.Types.Generic
{
    public class Error
    {
        [JsonProperty("code")]
        public int Code { get; set; }

        [JsonProperty("message")]
        public string Message { get; set; }

        public override string ToString()
        {
            return
                $"Failure.{Environment.NewLine}Code: {Code}{Environment.NewLine}Message: {Message}";
        }
    }
}