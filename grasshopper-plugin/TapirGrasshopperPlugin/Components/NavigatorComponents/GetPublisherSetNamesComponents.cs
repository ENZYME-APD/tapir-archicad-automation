using Grasshopper.Kernel;
using System;
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
            OutTexts(
                "PublisherSetNames",
                "List of names of the publisher sets.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var response = SendArchicadCommand(
                "GetPublisherSetNames",
                null);

            if (!response.Succeeded)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    response.GetErrorMessage());

                return;
            }

            var obj = response.Result.ToObject<PublisherSetNamesObj>();
            da.SetDataList(
                0,
                obj.PublisherSetNames);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.PublisherSetNames;

        public override Guid ComponentGuid =>
            new Guid("446d3bca-f817-4c90-b163-44da5197e707");
    }
}