using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetPublisherSetsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetPublisherSets";

        public GetPublisherSetsComponent()
            : base(
                "GetPublisherSets",
                "Returns the publisher sets of the project with their saved output: the local or LAN path, the " +
                "Teamwork file server path, or the Internet host and directory name.",
                GroupNames.Navigator)
        {
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Names",
                "The name of each publisher set, in the order of the Publisher Sets map.");

            OutGenerics(
                "RootNavigatorItemIds",
                "The navigator item of the root of each publisher set.");

            OutTexts(
                "Paths",
                "The saved local or LAN output path of each set published to files (empty otherwise). When the set " +
                "is also uploaded to a Teamwork file server, this is the temporary local path.");

            OutTexts(
                "TwFileServerPaths",
                "The saved output path on the Teamwork file server of each set published onto a file server " +
                "(empty otherwise).");

            OutTexts(
                "Hosts",
                "The saved host name of each set published to the Internet (empty otherwise).");

            OutTexts(
                "DirNames",
                "The saved directory name on the host of each set published to the Internet (empty otherwise).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var names = new List<string>();
            var rootIds = new List<NavigatorGuidWrapper>();
            var paths = new List<string>();
            var twFileServerPaths = new List<string>();
            var hosts = new List<string>();
            var dirNames = new List<string>();

            foreach (var item in JsonOutputHelp.Items(response, "publisherSets"))
            {
                names.Add(item["name"]?.ToString());
                var rootId = item["rootNavigatorItemId"]?.ToObject<NavigatorGuid>();
                rootIds.Add(rootId == null ? null : new NavigatorGuidWrapper { Id = rootId });
                paths.Add(item["path"]?.ToString());
                twFileServerPaths.Add(item["twFileServerPath"]?.ToString());
                hosts.Add(item["host"]?.ToString());
                dirNames.Add(item["dirName"]?.ToString());
            }

            da.SetDataList(0, names);
            da.SetDataList(1, rootIds);
            da.SetDataList(2, paths);
            da.SetDataList(3, twFileServerPaths);
            da.SetDataList(4, hosts);
            da.SetDataList(5, dirNames);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.PublisherSetNames;

        public override Guid ComponentGuid =>
            new Guid("a44ceb2d-7a65-471c-a045-ab31ac784730");
    }
}
