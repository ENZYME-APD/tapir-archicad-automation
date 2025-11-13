using Newtonsoft.Json;
using System.Collections.Generic;
using System.Linq;

namespace TapirGrasshopperPlugin.ResponseTypes.Properties
{
    public class GetHotlinksResponse
    {
        [JsonProperty ("hotlinks")]
        public Hotlinks Hotlinks { get; set; }
    }

    public class Hotlinks : List<Hotlink> { }

    public class Hotlink
    {
        [JsonProperty ("location")]
        public string Location { get; set; }

        [JsonProperty ("children")]
        public Hotlinks Children { get; set; }

        public Hotlink ()
        {
            Children = new Hotlinks ();
        }
    }


    public static class HotlinkExtensions
    {
        public static IEnumerable<string> GetLocationsRecursively(this Hotlinks hotlinks)
        {
            return hotlinks.SelectMany(hotlink =>
            {
                var locations = new List<string> ();

                if (!string.IsNullOrEmpty(hotlink.Location))
                {
                    locations.Add (hotlink.Location);
                }

                if (hotlink.Children != null && hotlink.Children.Any ())
                {
                    locations.AddRange(hotlink.Children.GetLocationsRecursively ());
                }

                return locations;
            });
        }
    }
}
