using Newtonsoft.Json;
using System.Collections.Generic;
using System.Linq;

namespace TapirGrasshopperPlugin.ResponseTypes.Project
{
    public class HotlinksResponse
    {
        [JsonProperty("hotlinks")]
        public Hotlinks Hotlinks { get; set; }
    }

    public class Hotlinks : List<Hotlink>
    {
    }

    public class Hotlink
    {
        [JsonProperty("location")]
        public string Location { get; set; }

        [JsonProperty("children")]
        public Hotlinks Children { get; set; }

        public Hotlink()
        {
            Children = new Hotlinks();
        }
    }


    public static class HotlinkExtensions
    {
        public static IEnumerable<string> GetLocations(
            this Hotlinks hotlinks)
        {
            return hotlinks.SelectMany(link =>
            {
                var locations = new List<string>();

                if (!string.IsNullOrEmpty(link.Location))
                {
                    locations.Add(link.Location);
                }

                if (link.Children != null && link.Children.Any())
                {
                    locations.AddRange(link.Children.GetLocations());
                }

                return locations;
            });
        }
    }
}