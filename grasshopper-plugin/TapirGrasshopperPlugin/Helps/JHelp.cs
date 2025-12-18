using Newtonsoft.Json.Linq;

namespace TapirGrasshopperPlugin.Helps
{
    public static class JHelp
    {
        public static T Deserialize<T>(
            this JObject jObject)
            where T : class
        {
            return jObject.ToObject<T>();
        }
    }
}