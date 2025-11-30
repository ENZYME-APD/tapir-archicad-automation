using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetPublisherSetNames : ArchicadAccessorComponent
    {
        public override string CommandName => "GetPublisherSetNames";

        public GetPublisherSetNames()
            : base(
                "PublisherSetNames",
                "Get names of the publisher sets' names.",
                GroupNames.Navigator)
        {
        }

        protected override void AddOutputs()
        {
            OutTexts("PublisherSetNames");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetConvertedValues(
                    CommandName,
                    null,
                    SendToArchicad,
                    JHelp.Deserialize<PublisherSetNamesObj>,
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