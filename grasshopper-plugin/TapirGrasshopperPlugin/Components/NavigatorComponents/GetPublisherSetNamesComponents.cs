using Grasshopper.Kernel;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class PublisherSetNamesObj
    {
        [JsonProperty("publisherSetNames")]
        public List<string> PublisherSetNames;
    }

    public class GetPublisherSetNames : ArchicadAccessorComponent
    {
        public static string Doc => "Get names of the publisher sets.";

        public override string CommandName => "GetPublisherSetNames";

        public GetPublisherSetNames()
            : base(
                "PublisherSetNames",
                "PublisherSetNames",
                Doc,
                GroupNames.Navigator)
        {
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "PublisherSetNames",
                "List of names of the publisher sets.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!GetConvertedResponse(
                    CommandName,
                    out PublisherSetNamesObj response))
            {
                return;
            }

            da.SetDataList(
                0,
                response.PublisherSetNames);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.PublisherSetNames;

        public override Guid ComponentGuid =>
            new Guid("446d3bca-f817-4c90-b163-44da5197e707");
    }
}