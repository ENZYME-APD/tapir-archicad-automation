using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

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
            if (!TryGetConvertedCadValues(
                    CommandName,
                    null,
                    ToArchicad,
                    JHelp.Deserialize<PublisherSetNamesObject>,
                    out PublisherSetNamesObject response))
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