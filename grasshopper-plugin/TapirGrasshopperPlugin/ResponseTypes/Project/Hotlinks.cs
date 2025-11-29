using Grasshopper;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;

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

        public static DataTree<string> GetTree(
            this HotlinksResponse response)
        {
            var tree = new DataTree<string>();

            response.Hotlinks.AddToTree(
                tree,
                new GH_Path());

            return tree;
        }

        public static void AddToTree(
            this Hotlinks hotlinks,
            DataTree<string> tree,
            GH_Path path)
        {
            for (int i = 0; i < hotlinks.Count; i++)
            {
                var link = hotlinks[i];
                var currentPath = path.AppendElement(i);

                if (!string.IsNullOrEmpty(link.Location))
                {
                    tree.Add(
                        link.Location,
                        currentPath);
                }

                if (link.Children != null && link.Children.Any())
                {
                    link.Children.AddToTree(
                        tree,
                        currentPath);
                }
            }
        }
    }
}