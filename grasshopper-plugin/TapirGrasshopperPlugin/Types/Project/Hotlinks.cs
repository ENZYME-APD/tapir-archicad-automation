using Grasshopper;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Types.Project
{
    // The identifier of a hotlink node - the module source file that
    // instances are placed from. CreateHotlinkNodes hands these out and
    // CreateHotlinkInstances takes them.
    public class HotlinkNodeGuid : GuidObject<HotlinkNodeGuid>
    {
    }

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

        [JsonProperty("hotlinkNodeId")]
        public HotlinkNodeGuid HotlinkNodeId { get; set; }

        [JsonProperty("name")]
        public string Name { get; set; }

        [JsonProperty("type")]
        public string Type { get; set; }

        [JsonProperty("children")]
        public Hotlinks Children { get; set; }

        public Hotlink()
        {
            Children = new Hotlinks();
        }
    }


    public static class HotlinkExtensions
    {
        // Every node of the tree, parents before their children, in the order
        // of GetLocations.
        public static IEnumerable<Hotlink> Flatten(
            this Hotlinks hotlinks)
        {
            foreach (var link in hotlinks)
            {
                yield return link;

                if (link.Children != null && link.Children.Any())
                {
                    foreach (var child in link.Children.Flatten())
                    {
                        yield return child;
                    }
                }
            }
        }

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
            this Hotlinks links,
            DataTree<string> tree,
            GH_Path path)
        {
            for (int i = 0; i < links.Count; i++)
            {
                var link = links[i];
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